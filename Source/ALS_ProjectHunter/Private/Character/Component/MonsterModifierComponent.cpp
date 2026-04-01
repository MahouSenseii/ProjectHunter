// Character/Component/MonsterModifierComponent.cpp
#include "Character/Component/MonsterModifierComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY(LogMonsterModifier);



UMonsterModifierComponent::UMonsterModifierComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false); // State is authority-only
}

void UMonsterModifierComponent::BeginPlay()
{
	Super::BeginPlay();

	// Auto-roll on BeginPlay if we have authority
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		RollAndApplyMods();
	}
}

// ─────────────────────────────────────────────────────────────────────────────

void UMonsterModifierComponent::RollAndApplyMods()
{
	if (bModsApplied)
	{
		return;
	}
	bModsApplied = true;

	// OPT: Cache the resolved pointer to avoid repeated LoadSynchronous() calls.
	// Each LoadSynchronous() hits the asset system even if the asset is warm,
	// which adds up during bulk spawns (e.g. pool recycling a wave of mobs).
	if (!CachedSpawnConfig)
	{
		CachedSpawnConfig = SpawnConfig.LoadSynchronous();
	}
	UMonsterSpawnConfig* Config = CachedSpawnConfig;
	if (!Config)
	{
		UE_LOG(LogMonsterModifier, Warning,
			TEXT("RollAndApplyMods: No SpawnConfig set on %s — monster will be Normal with no mods"),
			*GetOwner()->GetName());
		return;
	}

	// Determine tier
	if (ForcedTier != EMonsterTier::MT_Normal || AssignedTier != EMonsterTier::MT_Normal)
	{
		AssignedTier = (ForcedTier != EMonsterTier::MT_Normal) ? ForcedTier : AssignedTier;
	}
	else
	{
		AssignedTier = Config->RollMonsterTier(AreaLevel, NearbyPlayerMagicFind);
	}

	if (AssignedTier == EMonsterTier::MT_Normal)
	{
		// Normal monsters get no mods
		FullDisplayName = GetOwner()->GetClass()->GetDisplayNameText();
		OnMonsterModsApplied.Broadcast(AssignedTier, FullDisplayName);
		return;
	}

	// Load modifier table
	UDataTable* ModTable = Config->ModifierTable.LoadSynchronous();
	if (!ModTable)
	{
		UE_LOG(LogMonsterModifier, Warning,
			TEXT("RollAndApplyMods: SpawnConfig has no ModifierTable assigned"));
		return;
	}

	// Determine mod count
	int32 NumMods = 0;
	switch (AssignedTier)
	{
	case EMonsterTier::MT_Magic:
		NumMods = FMath::RandRange(Config->MagicModMin, Config->MagicModMax);
		break;
	case EMonsterTier::MT_Rare:
		NumMods = FMath::RandRange(Config->RareModMin, Config->RareModMax);
		break;
	case EMonsterTier::MT_Unique:
		// Unique monsters have fixed mods — set externally via AppliedMods before RollAndApplyMods
		NumMods = 0;
		break;
	default:
		break;
	}

	// Roll mods
	TArray<FMonsterModRow> RolledMods = RollMods(NumMods, AssignedTier, ModTable);
	AppliedMods.Append(RolledMods);

	// Apply mods to ASC
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;

	for (const FMonsterModRow& Mod : AppliedMods)
	{
		ApplyMod(Mod, ASC);
	}

	FullDisplayName = BuildDisplayName(AppliedMods);

	UE_LOG(LogMonsterModifier, Log,
		TEXT("RollAndApplyMods: %s is now Tier=%d with %d mods: %s"),
		*GetOwner()->GetName(),
		static_cast<int32>(AssignedTier),
		AppliedMods.Num(),
		*FullDisplayName.ToString());

	OnMonsterModsApplied.Broadcast(AssignedTier, FullDisplayName);
}

void UMonsterModifierComponent::SetTier(EMonsterTier NewTier)
{
	AssignedTier = NewTier;
}

void UMonsterModifierComponent::RerollMods()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) { return; }

	// ── Clean up previously applied GEs so they don't stack ──────────────
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;

	if (ASC)
	{
		for (const FActiveGameplayEffectHandle& Handle : AppliedGEHandles)
		{
			if (Handle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(Handle);
			}
		}
	}
	AppliedGEHandles.Empty();
	AppliedMods.Empty();

	// Reset the idempotency guard so RollAndApplyMods runs fresh
	bModsApplied = false;
	AssignedTier = (ForcedTier != EMonsterTier::MT_Normal) ? ForcedTier : EMonsterTier::MT_Normal;

	RollAndApplyMods();
}

// ─────────────────────────────────────────────────────────────────────────────
// Mod rolling — weighted random selection matching FAffixGenerator pattern
// ─────────────────────────────────────────────────────────────────────────────

TArray<FMonsterModRow> UMonsterModifierComponent::RollMods(int32 NumMods,
	EMonsterTier Tier, const UDataTable* Table) const
{
	TArray<FMonsterModRow> Result;
	if (!Table || NumMods <= 0)
	{
		return Result;
	}

	// Gather eligible rows — store copies to avoid const_cast UB on DataTable memory.
	TArray<FMonsterModRow> EligibleRows;
	TArray<int32> Weights;
	int32 TotalWeight = 0;

	Table->ForeachRow<FMonsterModRow>(TEXT("RollMods"),
		[&](const FName& Key, const FMonsterModRow& Row)
		{
			if (Row.MinAreaLevel <= AreaLevel
				&& static_cast<uint8>(Row.MinTier) <= static_cast<uint8>(Tier))
			{
				EligibleRows.Add(Row);
				Weights.Add(Row.Weight);
				TotalWeight += Row.Weight;
			}
		});

	if (EligibleRows.Num() == 0 || TotalWeight <= 0)
	{
		return Result;
	}

	// Ensure we never try to roll more mods than available unique rows
	NumMods = FMath::Min(NumMods, EligibleRows.Num());

	// Track selected indices to avoid duplicates
	TSet<int32> SelectedIndices;

	for (int32 i = 0; i < NumMods; ++i)
	{
		// Rebuild pool without already-selected entries
		int32 RemainingWeight = 0;
		TArray<TPair<int32, int32>> Pool; // index, weight
		for (int32 j = 0; j < EligibleRows.Num(); ++j)
		{
			if (!SelectedIndices.Contains(j))
			{
				Pool.Add({ j, Weights[j] });
				RemainingWeight += Weights[j];
			}
		}

		if (Pool.Num() == 0 || RemainingWeight <= 0)
		{
			break;
		}

		int32 Roll = FMath::RandRange(0, RemainingWeight - 1);
		int32 Cumulative = 0;
		for (const auto& Pair : Pool)
		{
			Cumulative += Pair.Value;
			if (Roll < Cumulative)
			{
				SelectedIndices.Add(Pair.Key);
				Result.Add(EligibleRows[Pair.Key]);
				break;
			}
		}
	}

	return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mod application
// ─────────────────────────────────────────────────────────────────────────────

void UMonsterModifierComponent::ApplyMod(const FMonsterModRow& Mod, UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	// Apply the spawn GE (infinite persistent buff)
	if (Mod.OnSpawnGE)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Mod.OnSpawnGE, 1, Context);
		if (Spec.IsValid())
		{
			FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			if (Handle.IsValid())
			{
				AppliedGEHandles.Add(Handle);
			}
		}
	}

	// Grant ability (e.g. Teleporting, Flamebearer)
	if (Mod.GrantedAbilityClass)
	{
		FGameplayAbilitySpec AbilitySpec(Mod.GrantedAbilityClass, 1);
		ASC->GiveAbility(AbilitySpec);
	}

	// Apply gameplay tags for AI/behaviour branching
	if (Mod.GrantedTags.Num() > 0)
	{
		ASC->AddLooseGameplayTags(Mod.GrantedTags);
	}

	// Apply movement speed multiplier (was previously a dead data field)
	if (!FMath::IsNearlyEqual(Mod.MoveSpeedMultiplier, 1.0f))
	{
		if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				MoveComp->MaxWalkSpeed *= Mod.MoveSpeedMultiplier;
			}
		}
	}

	UE_LOG(LogMonsterModifier, Verbose,
		TEXT("ApplyMod: Applied '%s' to %s (MoveSpd=x%.2f, Aggro=x%.2f)"),
		*Mod.ModID.ToString(), *GetOwner()->GetName(),
		Mod.MoveSpeedMultiplier, Mod.AggroRangeMultiplier);
}

// ─────────────────────────────────────────────────────────────────────────────
// Display name assembly
// ─────────────────────────────────────────────────────────────────────────────

FText UMonsterModifierComponent::BuildDisplayName(const TArray<FMonsterModRow>& Mods) const
{
	FString BaseName = GetOwner()->GetClass()->GetDisplayNameText().ToString();

	FString PrefixStr;
	FString SuffixStr;

	for (const FMonsterModRow& Mod : Mods)
	{
		if (Mod.ModType == EMonsterModType::MMT_Prefix)
		{
			if (!PrefixStr.IsEmpty()) PrefixStr += TEXT(" ");
			PrefixStr += Mod.DisplayLabel.ToString();
		}
		else
		{
			if (!SuffixStr.IsEmpty()) SuffixStr += TEXT(", ");
			SuffixStr += Mod.DisplayLabel.ToString();
		}
	}

	FString Full = BaseName;
	if (!PrefixStr.IsEmpty())
	{
		Full = PrefixStr + TEXT(" ") + Full;
	}
	if (!SuffixStr.IsEmpty())
	{
		Full = Full + TEXT(" ") + SuffixStr;
	}

	return FText::FromString(Full);
}

// ─────────────────────────────────────────────────────────────────────────────
// Combined multipliers
// ─────────────────────────────────────────────────────────────────────────────

float UMonsterModifierComponent::GetCombinedHPMultiplier() const
{
	float Combined = 1.0f;
	for (const FMonsterModRow& Mod : AppliedMods)
	{
		Combined *= Mod.HPMultiplier;
	}
	return Combined;
}

float UMonsterModifierComponent::GetCombinedDamageMultiplier() const
{
	float Combined = 1.0f;
	for (const FMonsterModRow& Mod : AppliedMods)
	{
		Combined *= Mod.DamageMultiplier;
	}
	return Combined;
}

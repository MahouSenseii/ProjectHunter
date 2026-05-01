// Character/Component/MonsterModifierComponent.cpp
#include "AI/Components/MonsterModifierComponent.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
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

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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
		PH_LOG_WARNING(LogMonsterModifier,
			"RollAndApplyMods fallback: No SpawnConfig was set on Owner=%s, so the monster will remain normal.",
			*GetOwner()->GetName());
		return;
	}

	// Roll per-instance base-stat variation BEFORE the Normal-tier early return.
	// This is what makes two Normal Goblins mechanically different — every mob
	// gets a small randomised HP/damage/resist/movespeed spread regardless of tier.
	RollBaseStatVariation();

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
		// Normal monsters get no mod rows, but still benefit from the base-stat
		// variation rolled above — so they're never identical clones.
		FullDisplayName = GetOwner()->GetClass()->GetDisplayNameText();
		OnMonsterModsApplied.Broadcast(AssignedTier, FullDisplayName);
		return;
	}

	// Load modifier table
	UDataTable* ModTable = Config->ModifierTable.LoadSynchronous();
	if (!ModTable)
	{
		PH_LOG_WARNING(LogMonsterModifier, "RollAndApplyMods failed: SpawnConfig had no ModifierTable assigned.");
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
		// Unique monsters have fixed mods â€” set externally via AppliedMods before RollAndApplyMods
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

	// â”€â”€ Clean up previously applied GEs so they don't stack â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;

	ClearAppliedRuntimeMods(ASC);
	AppliedMods.Empty();

	// Reset the per-instance variation so the reroll also re-rolls base stats.
	// Without this, a pooled mob keeps its previous life's variation.
	BaseStatVariation = FMonsterStatVariation();

	// Reset the idempotency guard so RollAndApplyMods runs fresh
	bModsApplied = false;
	AssignedTier = (ForcedTier != EMonsterTier::MT_Normal) ? ForcedTier : EMonsterTier::MT_Normal;

	RollAndApplyMods();
}

// ─────────────────────────────────────────────────────────────────────────────
// Per-instance base-stat variation
// ─────────────────────────────────────────────────────────────────────────────

void UMonsterModifierComponent::RollBaseStatVariation()
{
	if (BaseStatVariation.bRolled)
	{
		return; // Idempotent — already rolled this life
	}

	if (!bEnableBaseStatVariation)
	{
		// Mark as rolled so we don't re-enter; leave multipliers at 1.0.
		BaseStatVariation.bRolled = true;
		return;
	}

	auto RollSymmetric = [](const float VariancePct) -> float
	{
		if (VariancePct <= 0.0f) { return 1.0f; }
		return 1.0f + FMath::FRandRange(-VariancePct, VariancePct);
	};

	BaseStatVariation.HPMultiplier        = RollSymmetric(HPVariancePct);
	BaseStatVariation.DamageMultiplier    = RollSymmetric(DamageVariancePct);
	BaseStatVariation.MoveSpeedMultiplier = RollSymmetric(MoveSpeedVariancePct);
	BaseStatVariation.ResistBonusPct      = (ResistVariancePct > 0.0f)
		? FMath::FRandRange(-ResistVariancePct, ResistVariancePct)
		: 0.0f;
	BaseStatVariation.bRolled = true;

	// Apply movespeed variation directly to CharacterMovement — mirrors how
	// ApplyMod handles the per-mod MoveSpeedMultiplier.
	if (!FMath::IsNearlyEqual(BaseStatVariation.MoveSpeedMultiplier, 1.0f))
	{
		if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				MoveComp->MaxWalkSpeed *= BaseStatVariation.MoveSpeedMultiplier;
				AppliedMoveSpeedMultiplier *= BaseStatVariation.MoveSpeedMultiplier;
			}
		}
	}

	UE_LOG(LogMonsterModifier, Verbose,
		TEXT("RollBaseStatVariation: %s HP=x%.3f Dmg=x%.3f Spd=x%.3f Resist=%+.2f%%"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("<null>"),
		BaseStatVariation.HPMultiplier,
		BaseStatVariation.DamageMultiplier,
		BaseStatVariation.MoveSpeedMultiplier,
		BaseStatVariation.ResistBonusPct);

	OnBaseStatVariationRolled.Broadcast(BaseStatVariation);
}

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Mod rolling â€” weighted random selection matching FAffixGenerator pattern
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TArray<FMonsterModRow> UMonsterModifierComponent::RollMods(int32 NumMods,
	EMonsterTier Tier, const UDataTable* Table) const
{
	TArray<FMonsterModRow> Result;
	if (!Table || NumMods <= 0)
	{
		return Result;
	}

	// Gather eligible rows â€” store copies to avoid const_cast UB on DataTable memory.
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

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Mod application
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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
		const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(AbilitySpec);
		if (Handle.IsValid())
		{
			GrantedAbilityHandles.Add(Handle);
		}
	}

	// Apply gameplay tags for AI/behaviour branching
	if (Mod.GrantedTags.Num() > 0)
	{
		ASC->AddLooseGameplayTags(Mod.GrantedTags);
		GrantedLooseTags.AppendTags(Mod.GrantedTags);
		GrantedLooseTagGrants.Add(Mod.GrantedTags);
	}

	// Apply movement speed multiplier (was previously a dead data field)
	if (!FMath::IsNearlyEqual(Mod.MoveSpeedMultiplier, 1.0f))
	{
		if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				MoveComp->MaxWalkSpeed *= Mod.MoveSpeedMultiplier;
				AppliedMoveSpeedMultiplier *= Mod.MoveSpeedMultiplier;
			}
		}
	}

	UE_LOG(LogMonsterModifier, Verbose,
		TEXT("ApplyMod: Applied '%s' to %s (MoveSpd=x%.2f, Aggro=x%.2f)"),
		*Mod.ModID.ToString(), *GetOwner()->GetName(),
		Mod.MoveSpeedMultiplier, Mod.AggroRangeMultiplier);
}

void UMonsterModifierComponent::ClearAppliedRuntimeMods(UAbilitySystemComponent* ASC)
{
	if (ASC)
	{
		for (const FActiveGameplayEffectHandle& Handle : AppliedGEHandles)
		{
			if (Handle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(Handle);
			}
		}

		for (const FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
		{
			if (Handle.IsValid())
			{
				ASC->ClearAbility(Handle);
			}
		}

		for (const FGameplayTagContainer& GrantedTags : GrantedLooseTagGrants)
		{
			if (GrantedTags.Num() > 0)
			{
				ASC->RemoveLooseGameplayTags(GrantedTags);
			}
		}
	}

	if (!FMath::IsNearlyEqual(AppliedMoveSpeedMultiplier, 1.0f)
		&& !FMath::IsNearlyZero(AppliedMoveSpeedMultiplier))
	{
		if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				MoveComp->MaxWalkSpeed /= AppliedMoveSpeedMultiplier;
			}
		}
	}

	AppliedGEHandles.Empty();
	GrantedAbilityHandles.Empty();
	GrantedLooseTags.Reset();
	GrantedLooseTagGrants.Empty();
	AppliedMoveSpeedMultiplier = 1.0f;
}

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Display name assembly
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Combined multipliers
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

float UMonsterModifierComponent::GetCombinedHPMultiplier() const
{
	// Per-instance variation is folded in so Normal-tier mobs (which have no
	// AppliedMods) still get a non-1.0 HP multiplier.
	float Combined = BaseStatVariation.HPMultiplier;
	for (const FMonsterModRow& Mod : AppliedMods)
	{
		Combined *= Mod.HPMultiplier;
	}
	return Combined;
}

float UMonsterModifierComponent::GetCombinedDamageMultiplier() const
{
	float Combined = BaseStatVariation.DamageMultiplier;
	for (const FMonsterModRow& Mod : AppliedMods)
	{
		Combined *= Mod.DamageMultiplier;
	}
	return Combined;
}


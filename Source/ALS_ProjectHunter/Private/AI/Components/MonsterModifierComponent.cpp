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
	SetIsReplicatedByDefault(false);
}

void UMonsterModifierComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		RollAndApplyMods();
	}
}

void UMonsterModifierComponent::RollAndApplyMods()
{
	if (bModsApplied)
	{
		return;
	}
	bModsApplied = true;


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


	RollBaseStatVariation();

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

		FullDisplayName = GetOwner()->GetClass()->GetDisplayNameText();
		OnMonsterModsApplied.Broadcast(AssignedTier, FullDisplayName);
		return;
	}

	UDataTable* ModTable = Config->ModifierTable.LoadSynchronous();
	if (!ModTable)
	{
		PH_LOG_WARNING(LogMonsterModifier, "RollAndApplyMods failed: SpawnConfig had no ModifierTable assigned.");
		return;
	}

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
		NumMods = 0;
		break;
	default:
		break;
	}

	TArray<FMonsterModRow> RolledMods = RollMods(NumMods, AssignedTier, ModTable);
	AppliedMods.Append(RolledMods);

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

	
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;

	// Always revert runtime effects (GEs, abilities, tags, movespeed) — they get
	// re-applied below from whatever ends up in AppliedMods.
	ClearAppliedRuntimeMods(ASC);

	const EMonsterTier ResolvedTier =
		(ForcedTier != EMonsterTier::MT_Normal) ? ForcedTier : EMonsterTier::MT_Normal;

	// Unique monsters carry FIXED mods that are populated externally before
	// RollAndApplyMods (which rolls 0 new mods for Unique and just re-applies
	// AppliedMods). MobPoolSubsystem::ResetMobState deliberately preserves
	// AppliedMods for Unique on recycle — emptying it here unconditionally was
	// wiping those fixed mods, so a recycled Unique respawned with no effects
	// and an empty display name. Mirror the pool's preservation rule.
	if (ResolvedTier != EMonsterTier::MT_Unique)
	{
		AppliedMods.Empty();
	}

	BaseStatVariation = FMonsterStatVariation();

	bModsApplied = false;
	AssignedTier = ResolvedTier;

	RollAndApplyMods();
}

void UMonsterModifierComponent::RollBaseStatVariation()
{
	if (BaseStatVariation.bRolled)
	{
		return;
	}

	if (!bEnableBaseStatVariation)
	{
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

TArray<FMonsterModRow> UMonsterModifierComponent::RollMods(int32 NumMods,
	EMonsterTier Tier, const UDataTable* Table) const
{
	TArray<FMonsterModRow> Result;
	if (!Table || NumMods <= 0)
	{
		return Result;
	}

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

	NumMods = FMath::Min(NumMods, EligibleRows.Num());
	TSet<int32> SelectedIndices;

	for (int32 i = 0; i < NumMods; ++i)
	{
		int32 RemainingWeight = 0;
		TArray<TPair<int32, int32>> Pool;
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


void UMonsterModifierComponent::ApplyMod(const FMonsterModRow& Mod, UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

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

	if (Mod.GrantedAbilityClass)
	{
		FGameplayAbilitySpec AbilitySpec(Mod.GrantedAbilityClass, 1);
		const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(AbilitySpec);
		if (Handle.IsValid())
		{
			GrantedAbilityHandles.Add(Handle);
		}
	}

	if (Mod.GrantedTags.Num() > 0)
	{
		ASC->AddLooseGameplayTags(Mod.GrantedTags);
		GrantedLooseTags.AppendTags(Mod.GrantedTags);
		GrantedLooseTagGrants.Add(Mod.GrantedTags);
	}

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

float UMonsterModifierComponent::GetCombinedHPMultiplier() const
{
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


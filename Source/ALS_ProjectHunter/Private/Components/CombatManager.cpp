// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/CombatManager.h"
#include "EngineUtils.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Character/PHBaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Item/WeaponItem.h"
#include "Library/PHCombatStructLibrary.h"

/* =========================== */
/* === Constructor & Setup === */
/* =========================== */

// Sets default values for this component's properties
UCombatManager::UCombatManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void UCombatManager::BeginPlay()
{
	Super::BeginPlay();
}

/* ==================================== */
/* === Damage Calculation Functions === */
/* ==================================== */

FDamageByType UCombatManager::GetAllDamageByType(const APHBaseCharacter* Attacker, const UEquippableItem* Weapon)
{
	FDamageByType Output;

	if (!Attacker) return Output;

	// Weapon base damage
	if (Weapon && Weapon->GetWeaponData().DamageStats.Num() > 0)
	{
		for (const auto& Pair : Weapon->GetWeaponData().DamageStats)
		{
			const EDamageTypes Type = Pair.Key;
			const FMinMax& Range = Pair.Value;
			const float Rolled = FMath::RandRange(Range.Min, Range.Max);

			switch (Type)
			{
			case EDamageTypes::DT_Physical:
				Output.PhysicalDamage.Min += Rolled;
				Output.PhysicalDamage.Max += Rolled;
				break;
			case EDamageTypes::DT_Fire:
				Output.FireDamage.Min += Rolled;
				Output.FireDamage.Max += Rolled;
				break;
			case EDamageTypes::DT_Ice:
				Output.IceDamage.Min += Rolled;
				Output.IceDamage.Max += Rolled;
				break;
			case EDamageTypes::DT_Lightning:
				Output.LightningDamage.Min += Rolled;
				Output.LightningDamage.Max += Rolled;
				break;
			case EDamageTypes::DT_Light:
				Output.LightDamage.Min += Rolled;
				Output.LightDamage.Max += Rolled;
				break;
			case EDamageTypes::DT_Corruption:
				Output.CorruptionDamage.Min += Rolled;
				Output.CorruptionDamage.Max += Rolled;
				break;
			default:
				break;
			}
		}
	}

	// Optional: you could add passives here directly if they're additive (e.g., item-less fireball)

	return Output;
}

float UCombatManager::CalculateBaseDamage(const APHBaseCharacter* Attacker, const EAttackType AttackType)
{
	const UAbilitySystemComponent* ASC = Attacker->GetAbilitySystemComponent();
	const UPHAttributeSet* Attributes = Cast<UPHAttributeSet>(Attacker->GetAttributeSet());

	if (!ASC || !Attributes) return 0.f;

	switch (AttackType)
	{
	case EAttackType::AT_Melee:
		return Attributes->GetMeleeDamage();
	case EAttackType::AT_Ranged:
		return Attributes->GetRangedDamage();
	case EAttackType::AT_Spell:
		return Attributes->GetSpellDamage();
	default:
		return 0.f;
	}
}


float UCombatManager::CalculateDamageAfterDefenses(float RawDamage, const APHBaseCharacter* Defender, EDamageTypes DamageType)
{
    const FDefenseStats Defense = GetAllDefenses(Defender);

    float ResistanceFlat = 0.f;
    float ResistancePercent = 0.f;

    switch (DamageType)
    {
    case EDamageTypes::DT_Fire:
        ResistanceFlat = Defense.FireResistanceFlat;
        ResistancePercent = Defense.FireResistancePercent;
        break;
        // Add other types here
    default:
        break;
    }

    // Flat resistance first
    RawDamage = FMath::Max(0.f, RawDamage - ResistanceFlat);

    // Then apply % reduction
    const float FinalDamage = RawDamage * (1.f - ResistancePercent);
    return FMath::Max(0.f, FinalDamage);
}

bool UCombatManager::RollForCriticalHit(const APHBaseCharacter* Attacker, const EAttackType AttackType)
{
	const UPHAttributeSet* Attributes = Cast<UPHAttributeSet>(Attacker->GetAttributeSet());

	if (!Attributes) return false;

	float CritChance = 0.f;

	switch (AttackType)
	{
	case EAttackType::AT_Melee:
	case EAttackType::AT_Ranged:
		CritChance = Attributes->GetCritChance();
		break;
	case EAttackType::AT_Spell:
		CritChance = Attributes->GetSpellsCritChance();
		break;
	default:
		break;
	}

	return FMath::FRand() <= CritChance;
}



float UCombatManager::GetCriticalMultiplier(const APHBaseCharacter* Attacker, const EAttackType AttackType)
{
	const UPHAttributeSet* Attributes = Cast<UPHAttributeSet>(Attacker->GetAttributeSet());
	if (!Attributes) return 1.0f; // default: no multiplier

	switch (AttackType)
	{
	case EAttackType::AT_Melee:
	case EAttackType::AT_Ranged:
		return Attributes->GetCritMultiplier();

	case EAttackType::AT_Spell:
		return Attributes->GetSpellsCritMultiplier();

	default:
		return 1.0f;
	}
}

FCombatHitResult UCombatManager::CalculateFinalDamage(const APHBaseCharacter* Attacker,
	const APHBaseCharacter* Defender, const UEquippableItem* Weapon, const EAttackType AttackType,
	const EDamageTypes PrimaryDamageType)
{
	FCombatHitResult Result;
	Result.PrimaryDamageType = PrimaryDamageType;

	if (!Attacker || !Defender) return Result;

	const UPHAttributeSet* Attributes = Cast<UPHAttributeSet>(Attacker->GetAttributeSet());

	// === 1. Roll Weapon + Attribute Damage ===
	const float WeaponBase = GetAllDamageByType(Attacker, Weapon).GetTotalDamageByType(PrimaryDamageType);
	const float StatBonus = CalculateBaseDamage(Attacker, AttackType);
	float TotalDamage = WeaponBase + StatBonus;

	// === 2. Critical Hit? ===
	const bool bIsCrit = RollForCriticalHit(Attacker, AttackType);
	if (bIsCrit)
	{
		const float CritMultiplier = GetCriticalMultiplier(Attacker, AttackType);
		TotalDamage *= CritMultiplier;
		Result.bCrit = true;
	}

	// === 3. Apply Defenses ===
	const float Final = CalculateDamageAfterDefenses(TotalDamage, Defender, PrimaryDamageType);
	Result.FinalDamage = Final;

	// === 4. Status Effect Application ===
	const bool bStatus = RollForStatusEffect(Attacker, PrimaryDamageType);
	if (bStatus)
	{
		Result.bAppliedStatus = true;

		switch (PrimaryDamageType)
		{
		case EDamageTypes::DT_Fire:
			Result.StatusEffectName = FName("Burn");
			Result.StatusTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Status.Burn")));
			break;
		case EDamageTypes::DT_Ice:
			Result.StatusEffectName = FName("Freeze");
			Result.StatusTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Status.Freeze")));
			break;
		case EDamageTypes::DT_Corruption:
			Result.StatusEffectName = FName("Poison");
			Result.StatusTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Status.Poison")));
			break;
			// Add more as needed
		default:
			break;
		}
	}

	return Result;
}


FDamageHitResultByType UCombatManager::CalculateAllFinalDamageByType(
	const APHBaseCharacter* Attacker,
	const APHBaseCharacter* Defender,
	const FDamageByType& RawDamage,
	const EAttackType AttackType
)
{
	FDamageHitResultByType Result;

	if (!Attacker || !Defender) return Result;

	const bool bCrit = RollForCriticalHit(Attacker, AttackType);
	const float CritMultiplier = GetCriticalMultiplier(Attacker, AttackType);

	Result.bCrit = bCrit;

	for (int32 i = 0; i <= static_cast<int32>(EDamageTypes::DT_Corruption); ++i)
	{
		const EDamageTypes DamageType = static_cast<EDamageTypes>(i);
		float Damage = RawDamage.RollDamageByType(DamageType);

		if (bCrit)
		{
			Damage *= CritMultiplier;
		}

		// Apply resistance
		Damage = CalculateDamageAfterDefenses(Damage, Defender, DamageType);
		if (Damage <= 0.f) continue;

		// Store final result
		Result.FinalDamageByType.Add(DamageType, Damage);

		// Check for status effect
		if (RollForStatusEffect(Attacker, DamageType))
		{
			Result.bAppliedStatusEffectPerType.Add(DamageType, true);

			switch (DamageType)
			{
			case EDamageTypes::DT_Fire:
				Result.StatusEffectNamePerType.Add(DamageType, FName("Burn"));
				break;
			case EDamageTypes::DT_Ice:
				Result.StatusEffectNamePerType.Add(DamageType, FName("Freeze"));
				break;
			case EDamageTypes::DT_Corruption:
				Result.StatusEffectNamePerType.Add(DamageType, FName("Poison"));
				break;
			default:
				break;
			}
		}
	}

	return Result;
}


bool UCombatManager::RollForStatusEffect(const APHBaseCharacter* Attacker, const EDamageTypes DamageType)
{
    const UPHAttributeSet* Attributes = Cast<UPHAttributeSet>(Attacker->GetAttributeSet());

    float StatusChance = 0.f;

    switch (DamageType)
    {
    case EDamageTypes::DT_Fire:
        StatusChance = Attributes->GetChanceToIgnite();
        break;
    case EDamageTypes::DT_Ice:
        StatusChance = Attributes->GetChanceToFreeze();
        break;
        // Add more here
    default:
        break;
    }

    return FMath::FRand() <= StatusChance;
}


FDamageByType UCombatManager::GetAllDamageByType(const APHBaseCharacter* Attacker, const UEquippableItem* Weapon, bool bIsSpell)
{
    FDamageByType Output;

    // Base power
    const UPHAttributeSet* Attributes = Cast<UPHAttributeSet>(Attacker->GetAttributeSet());

    // From weapon
    if (Weapon && Weapon->GetWeaponData().DamageStats.Num() > 0)
    {
        for (const auto& Pair : Weapon->GetWeaponData().DamageStats)
        {
            const EDamageTypes Type = Pair.Key;
            const FMinMax& Range = Pair.Value;

            const float Rolled = FMath::RandRange(Range.Min, Range.Max);
            switch (Type)
            {
            case EDamageTypes::DT_Fire:
                Output.FireDamage.Min += Rolled;
                Output.FireDamage.Max += Rolled;
                break;
                // ... other types
            default: break;
            }
        }
    }

    // Add passive bonuses if needed

    return Output;
}

/* ============================= */
/* ========== Defenses ========= */
/* ============================= */


FDefenseStats UCombatManager::GetAllDefenses(const APHBaseCharacter* Defender)
{
	FDefenseStats Defense;

	if (!Defender) return Defense;

	const UAbilitySystemComponent* ASC = Defender->GetAbilitySystemComponent();
	const UPHAttributeSet* Attributes = Cast<UPHAttributeSet>(Defender->GetAttributeSet());
	if (!ASC || !Attributes) return Defense;

	// === Base AttributeSet Stats ===
	Defense.ArmorFlat                   = Attributes->GetArmourFlatBonus();
	Defense.FireResistanceFlat         = Attributes->GetFireResistanceFlatBonus();
	Defense.IceResistanceFlat          = Attributes->GetIceResistanceFlatBonus();
	Defense.LightningResistanceFlat    = Attributes->GetLightningResistanceFlatBonus();
	Defense.LightResistanceFlat        = Attributes->GetLightResistanceFlatBonus();
	Defense.CorruptionResistanceFlat   = Attributes->GetCorruptionResistanceFlatBonus();

	Defense.ArmorPercent               = Attributes->GetArmourPercentBonus();
	Defense.FireResistancePercent      = Attributes->GetFireResistancePercentBonus();
	Defense.IceResistancePercent       = Attributes->GetIceResistancePercentBonus();
	Defense.LightningResistancePercent = Attributes->GetLightningResistancePercentBonus();
	Defense.LightResistancePercent     = Attributes->GetLightResistancePercentBonus();
	Defense.CorruptionResistancePercent= Attributes->GetCorruptionResistancePercentBonus();

	Defense.GlobalDefenses             = Attributes->GetGlobalDefenses();
	Defense.BlockStrength              = Attributes->GetBlockStrength();

	// === Optional: Add Gear Bonuses ===
	if (const UEquipmentManager* EquipmentManager = Defender->GetEquipmentManager())
	{
		for (UBaseItem* Item : EquipmentManager->EquipmentCheck())
		{
			if (const UEquippableItem* Equip = Cast<UEquippableItem>(Item))
			{
				const TArray<FPHAttributeData>& AttributesToCheck = Equip->GetEquippableData().ArmorAttributes;

				for (const FPHAttributeData& Attr : AttributesToCheck)
				{
					if (!Attr.StatChanged.IsValid()) continue;

					const float Rolled = FMath::RandRange(Attr.MinStatChanged, Attr.MaxStatChanged);

					if (Attr.StatChanged == Attributes->GetArmourFlatBonusAttribute())                      Defense.ArmorFlat += Rolled;
					else if (Attr.StatChanged == Attributes->GetFireResistanceFlatBonusAttribute())    Defense.FireResistanceFlat += Rolled;
					else if (Attr.StatChanged == Attributes->GetIceResistanceFlatBonusAttribute())     Defense.IceResistanceFlat += Rolled;
					else if (Attr.StatChanged == Attributes->GetLightningResistanceFlatBonusAttribute()) Defense.LightningResistanceFlat += Rolled;
					else if (Attr.StatChanged == Attributes->GetLightResistanceFlatBonusAttribute())   Defense.LightResistanceFlat += Rolled;
					else if (Attr.StatChanged == Attributes->GetCorruptionResistanceFlatBonusAttribute()) Defense.CorruptionResistanceFlat += Rolled;

					else if (Attr.StatChanged == Attributes->GetArmourPercentBonusAttribute())           Defense.ArmorPercent += Rolled;
					else if (Attr.StatChanged == Attributes->GetFireResistancePercentBonusAttribute())  Defense.FireResistancePercent += Rolled;
					else if (Attr.StatChanged == Attributes->GetIceResistancePercentBonusAttribute())   Defense.IceResistancePercent += Rolled;
					else if (Attr.StatChanged == Attributes->GetLightningResistancePercentBonusAttribute()) Defense.LightningResistancePercent += Rolled;
					else if (Attr.StatChanged == Attributes->GetLightResistancePercentBonusAttribute()) Defense.LightResistancePercent += Rolled;
					else if (Attr.StatChanged == Attributes->GetCorruptionResistancePercentBonusAttribute()) Defense.CorruptionResistancePercent += Rolled;

					else if (Attr.StatChanged == Attributes->GetGlobalDefensesAttribute())          Defense.GlobalDefenses += Rolled;
					else if (Attr.StatChanged == Attributes->GetBlockStrengthAttribute())          Defense.BlockStrength += Rolled;
				}
			}
		}
	}

	return Defense;
}


/* ============================= */
/* === Poise & Stagger System === */
/* ============================= */

void UCombatManager::ApplyPoiseDamage(APHBaseCharacter* Defender, float DamageTaken)
{
    if (!Defender || !Defender->HasAuthority()) return;

    const UPHAttributeSet* Attributes = Cast<UPHAttributeSet>(Defender->GetAttributeSet());
    if (!Attributes) return;

    const float PoiseThreshold = Attributes->GetPoise();
    const float PoiseResistance = Attributes->GetPoiseResistance();

    // Calculate Poise Damage
    const float PoiseDamage = DamageTaken * (1.0f - FMath::Clamp(PoiseResistance / 100.0f, 0.0f, 1.0f));

    Defender->ApplyPoiseDamage(PoiseDamage);

    // Check for Poise Break
    if (Defender->GetPoisePercentage() <= 0.0f)
    {
        StaggerCharacter(Defender);
        Defender->ApplyPoiseDamage(-Defender->GetCurrentPoiseDamage()); // Reset poise
    }

    Defender->ForceNetUpdate();
}

void UCombatManager::StaggerCharacter(APHBaseCharacter* Defender)
{
    if (!Defender || !Defender->HasAuthority()) return;

    if (UAnimMontage* StaggerMontage = Defender->GetStaggerAnimation())
    {
        Defender->PlayAnimMontage(StaggerMontage);
    }

    if (UCharacterMovementComponent* Movement = Defender->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
        Movement->DisableMovement();

        FTimerHandle StaggerTimerHandle;
        Defender->GetWorldTimerManager().SetTimer(
            StaggerTimerHandle,
            FTimerDelegate::CreateWeakLambda(Defender, [Movement]() {
                if (Movement) Movement->SetMovementMode(MOVE_Walking);
            }),
            1.0f, 
            false
        );
    }

    UE_LOG(LogTemp, Warning, TEXT("%s is staggered!"), *Defender->GetName());
}

/* ============================= */
/* === Utility & Other Logic === */
/* ============================= */

void UCombatManager::RegeneratePoise(float DeltaTime) const
{
    for (TActorIterator<APHBaseCharacter> It(GetWorld()); It; ++It)
    {
        APHBaseCharacter* Character = *It;
        if (!Character || !Character->HasAuthority()) continue;

        const float TimeSinceLastHit = Character->GetTimeSinceLastPoiseHit();
        const float CurrentPoiseDamage = Character->GetCurrentPoiseDamage();

        if (CurrentPoiseDamage <= 0.0f) continue;

        if (TimeSinceLastHit >= 2.0f)
        {
            const float RegenRate = 10.0f * DeltaTime;
            Character->ApplyPoiseDamage(-RegenRate);
        }

        Character->SetTimeSinceLastPoiseHit(TimeSinceLastHit + DeltaTime);
    }
}




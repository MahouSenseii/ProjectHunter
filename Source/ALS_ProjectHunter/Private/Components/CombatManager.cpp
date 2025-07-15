// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/CombatManager.h"
#include "EngineUtils.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Character/PHBaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Library/PHCombatStructLibrary.h"
#include "Library/PHDamageTypeUtils.h"

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

/* ============================= */
/* ========== Defenses ========= */
/* ============================= */

FDamageHitResultByType UCombatManager::CalculateDamage(const APHBaseCharacter* Attacker,
                                                      const APHBaseCharacter* Defender) const
{
    FDamageHitResultByType Result;
    if (!Attacker || !Defender) return Result;

    const UPHAttributeSet* AttAtt = Cast<UPHAttributeSet>(Attacker->GetAttributeSet());
    const UPHAttributeSet* DefAtt = Cast<UPHAttributeSet>(Defender->GetAttributeSet());
    if (!AttAtt || !DefAtt) return Result;

    FDamageByType BaseDamage;
    BaseDamage.PhysicalDamage   = {AttAtt->GetMinPhysicalDamage(),   AttAtt->GetMaxPhysicalDamage()};
    BaseDamage.FireDamage       = {AttAtt->GetMinFireDamage(),       AttAtt->GetMaxFireDamage()};
    BaseDamage.IceDamage        = {AttAtt->GetMinIceDamage(),        AttAtt->GetMaxIceDamage()};
    BaseDamage.LightningDamage  = {AttAtt->GetMinLightningDamage(),  AttAtt->GetMaxLightningDamage()};
    BaseDamage.LightDamage      = {AttAtt->GetMinLightDamage(),      AttAtt->GetMaxLightDamage()};
    BaseDamage.CorruptionDamage = {AttAtt->GetMinCorruptionDamage(), AttAtt->GetMaxCorruptionDamage()};

    const float GlobalDamage  = 1.f + AttAtt->GetGlobalDamages();
    const float GlobalDefense = 1.f - DefAtt->GetGlobalDefenses();

    const bool bCrit = FMath::FRand() <= AttAtt->GetCritChance();
    const float CritMult = 1.f + AttAtt->GetCritMultiplier();

    auto GetFlatBonus = [&](EDamageTypes Type) -> float
    {
        switch (Type)
        {
        case EDamageTypes::DT_Physical:   return AttAtt->GetPhysicalFlatBonus();
        case EDamageTypes::DT_Fire:       return AttAtt->GetFireFlatBonus();
        case EDamageTypes::DT_Ice:        return AttAtt->GetIceFlatBonus();
        case EDamageTypes::DT_Lightning:  return AttAtt->GetLightningFlatBonus();
        case EDamageTypes::DT_Light:      return AttAtt->GetLightFlatBonus();
        case EDamageTypes::DT_Corruption: return AttAtt->GetCorruptionFlatBonus();
        default:                          return 0.f;
        }
    };

    auto GetPercentBonus = [&](EDamageTypes Type) -> float
    {
        switch (Type)
        {
        case EDamageTypes::DT_Physical:   return AttAtt->GetPhysicalPercentBonus();
        case EDamageTypes::DT_Fire:       return AttAtt->GetFirePercentBonus();
        case EDamageTypes::DT_Ice:        return AttAtt->GetIcePercentBonus();
        case EDamageTypes::DT_Lightning:  return AttAtt->GetLightningPercentBonus();
        case EDamageTypes::DT_Light:      return AttAtt->GetLightPercentBonus();
        case EDamageTypes::DT_Corruption: return AttAtt->GetCorruptionPercentBonus();
        default:                          return 0.f;
        }
    };

    auto GetResistanceFlat = [&](EDamageTypes Type) -> float
    {
        switch (Type)
        {
        case EDamageTypes::DT_Physical:   return DefAtt->GetArmour() + DefAtt->GetArmourFlatBonus();
        case EDamageTypes::DT_Fire:       return DefAtt->GetFireResistanceFlatBonus();
        case EDamageTypes::DT_Ice:        return DefAtt->GetIceResistanceFlatBonus();
        case EDamageTypes::DT_Lightning:  return DefAtt->GetLightningResistanceFlatBonus();
        case EDamageTypes::DT_Light:      return DefAtt->GetLightResistanceFlatBonus();
        case EDamageTypes::DT_Corruption: return DefAtt->GetCorruptionResistanceFlatBonus();
        default:                          return 0.f;
        }
    };

    auto GetResistancePercent = [&](EDamageTypes Type) -> float
    {
        switch (Type)
        {
        case EDamageTypes::DT_Physical:   return DefAtt->GetArmourPercentBonus();
        case EDamageTypes::DT_Fire:       return DefAtt->GetFireResistancePercentBonus();
        case EDamageTypes::DT_Ice:        return DefAtt->GetIceResistancePercentBonus();
        case EDamageTypes::DT_Lightning:  return DefAtt->GetLightningResistancePercentBonus();
        case EDamageTypes::DT_Light:      return DefAtt->GetLightResistancePercentBonus();
        case EDamageTypes::DT_Corruption: return DefAtt->GetCorruptionResistancePercentBonus();
        default:                          return 0.f;
        }
    };

    auto GetPiercing = [&](EDamageTypes Type) -> float
    {
        switch (Type)
        {
        case EDamageTypes::DT_Physical:   return AttAtt->GetArmourPiercing();
        case EDamageTypes::DT_Fire:       return AttAtt->GetFirePiercing();
        case EDamageTypes::DT_Ice:        return AttAtt->GetIcePiercing();
        case EDamageTypes::DT_Lightning:  return AttAtt->GetLightningPiercing();
        case EDamageTypes::DT_Light:      return AttAtt->GetLightPiercing();
        case EDamageTypes::DT_Corruption: return AttAtt->GetCorruptionPiercing();
        default:                          return 0.f;
        }
    };

    for (int32 Index = (int32)EDamageTypes::DT_Fire; Index <= (int32)EDamageTypes::DT_Corruption; ++Index)
    {
        EDamageTypes Type = static_cast<EDamageTypes>(Index);
        float Rolled = BaseDamage.RollDamageByType(Type);
        if (Rolled <= 0.f) continue;

        float Damage = (Rolled + GetFlatBonus(Type)) * (1.f + GetPercentBonus(Type));
        Damage *= GlobalDamage;

        float ResistFlat = GetResistanceFlat(Type);
        float ResistPercent = FMath::Max(0.f, GetResistancePercent(Type) - GetPiercing(Type));
        Damage = FMath::Max(0.f, (Damage - ResistFlat) * (1.f - ResistPercent));
        Damage *= GlobalDefense;

        if (bCrit) Damage *= CritMult;

        Result.FinalDamageByType.Add(Type, Damage);
        Result.bAppliedStatusEffectPerType.Add(Type, RollForStatusEffect(Attacker, Type));
    }

    Result.bCrit = bCrit;
    return Result;
}

void UCombatManager::ApplyDamage(const APHBaseCharacter* Attacker,
                                 APHBaseCharacter* Defender)
{
    if (!Defender) return;

    FDamageHitResultByType HitResult = CalculateDamage(Attacker, Defender);
    float TotalDamage = HitResult.GetTotalDamage();

    // Determine the damage type that dealt the most damage
    const EDamageTypes HighestType = GetHighestDamageType(HitResult);
    const FString TypeName = DamageTypeToString(HighestType);

    UE_LOG(LogTemp, Log, TEXT("ApplyDamage: TotalDamage=%.0f, HighestType=%s"), TotalDamage, *TypeName);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange,
            FString::Printf(TEXT("Damage %.0f (%s)"), TotalDamage, *TypeName));
    }

    UPHAttributeSet* DefAtt = Cast<UPHAttributeSet>(Defender->GetAttributeSet());
    if (!DefAtt) return;

    UAbilitySystemComponent* ASC = Defender->GetAbilitySystemComponent();
    if (!ASC) return;

    // Handle blocking logic
    if (Defender->GetCombatManager() && Defender->GetCombatManager()->IsBlocking())
    {
        const float BlockPercent = FMath::Clamp(DefAtt->GetBlockStrength(), 0.f, 1.f);
        TotalDamage *= (1.f - BlockPercent);

        const float NewStamina = DefAtt->GetStamina() - TotalDamage;
        ASC->SetNumericAttributeBase(UPHAttributeSet::GetStaminaAttribute(), NewStamina);

        if (NewStamina <= 0.f && StaggerMontage)
        {
            Defender->PlayAnimMontage(StaggerMontage);
        }
    }

    const float NewHealth = DefAtt->GetHealth() - TotalDamage;
    ASC->SetNumericAttributeBase(UPHAttributeSet::GetHealthAttribute(), NewHealth);

    float FinalPoiseDamage = TotalDamage * 0.5f;
    const float NewPoise = DefAtt->GetPoise() - FinalPoiseDamage;
    ASC->SetNumericAttributeBase(UPHAttributeSet::GetPoiseAttribute(), NewPoise);

    if (NewPoise <= 0.f && StaggerMontage)
    {
        Defender->PlayAnimMontage(StaggerMontage);
    }

    // Broadcast damage info for UI or other listeners
    OnDamageApplied.Broadcast(TotalDamage, HighestType);
}

EDamageTypes UCombatManager::GetHighestDamageType(const FDamageHitResultByType& HitResult)
{
    EDamageTypes MaxType = EDamageTypes::DT_None;
    float MaxValue = 0.f;

    for (const auto& Pair : HitResult.FinalDamageByType)
    {
        if (Pair.Value > MaxValue)
        {
            MaxType = Pair.Key;
            MaxValue = Pair.Value;
        }
    }

    return MaxType;
}

void UCombatManager::IncreaseComboCounter(float Amount)
{
    APHBaseCharacter* OwnerCharacter = Cast<APHBaseCharacter>(GetOwner());
    if (!OwnerCharacter) return;

    UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
    UPHAttributeSet* Attributes = Cast<UPHAttributeSet>(OwnerCharacter->GetAttributeSet());
    if (!ASC || !Attributes) return;

    const float CurrentValue = Attributes->GetComboCounter();
    ASC->SetNumericAttributeBase(UPHAttributeSet::GetComboCounterAttribute(), CurrentValue + Amount);

    GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(ComboResetTimerHandle, this, &UCombatManager::ResetComboCounter, ComboResetTime, false);
}

void UCombatManager::ResetComboCounter()
{
    APHBaseCharacter* OwnerCharacter = Cast<APHBaseCharacter>(GetOwner());
    if (!OwnerCharacter) return;

    UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
    if (ASC)
    {
        ASC->SetNumericAttributeBase(UPHAttributeSet::GetComboCounterAttribute(), 0.f);
    }
}

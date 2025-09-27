// CombatManager.cpp - Complete Path of Exile 2 Style Combat System
#include "Components/CombatManager.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Character/PHBaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Library/PHCombatStructLibrary.h"
#include "Library/PHDamageTypeUtils.h"
#include "Library/PHGameplayTagLibrary.h"
#include "UI/Widgets/DamagePopup.h"

/* =========================== */
/* === Constructor & Setup === */
/* =========================== */

UCombatManager::UCombatManager(): OwnerCharacter(nullptr), StaggerMontage(nullptr)
{
    PrimaryComponentTick.bCanEverTick = true; // Enable tick for combat status updates
    PrimaryComponentTick.TickInterval = 0.1f; // Check every 100 ms
}

void UCombatManager::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<APHBaseCharacter>(GetOwner());
	OnDamageApplied.AddDynamic(this, &UCombatManager::InitPopup);
	
	// Initialize combat status
	SetCombatStatus(ECombatStatus::OutOfCombat);
}

void UCombatManager::TickComponent(float DeltaTime, ELevelTick TickType, 
                                  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// Update combat status regeneration
	if (CurrentCombatStatus == ECombatStatus::OutOfCombat && OwnerCharacter)
	{
		HandleOutOfCombatRegeneration(DeltaTime);
	}
}

/* ========================================== */
/* =========   DAMAGE CALCULATION ============ */
/* ========================================== */

FDamageHitResultByType UCombatManager::CalculateDamage(const APHBaseCharacter* Attacker,
                                                      const APHBaseCharacter* Defender) const
{
    FDamageHitResultByType Result;
    if (!Attacker || !Defender) return Result;

    const UPHAttributeSet* AttAtt = Cast<UPHAttributeSet>(Attacker->GetAttributeSet());
    const UPHAttributeSet* DefAtt = Cast<UPHAttributeSet>(Defender->GetAttributeSet());
    if (!AttAtt || !DefAtt) return Result;

    // STEP 1: Get Base Damage
    FDamageByType BaseDamage = GetBaseDamage(AttAtt);
    
    // STEP 2: Apply Damage Conversions (Order matters in PoE2!)
    BaseDamage = ApplyDamageConversions(BaseDamage, AttAtt);
    
    // STEP 3: Roll damage ranges
    TMap<EDamageTypes, float> RolledDamage = RollDamageValues(BaseDamage);
    
    // STEP 4: Apply damage increases and more multipliers
    RolledDamage = ApplyDamageModifiers(RolledDamage, AttAtt);
    
    // STEP 5: Check for critical strike
    const bool bCrit = RollCriticalStrike(AttAtt);
    if (bCrit)
    {
        const float CritMulti = 1.5f + AttAtt->GetCritMultiplier(); // Base 150% + bonus
        for (auto& DmgPair : RolledDamage)
        {
            DmgPair.Value *= CritMulti;
        }
    }
    
    // STEP 6: Apply enemy defenses
    RolledDamage = ApplyDefenses(RolledDamage, AttAtt, DefAtt);
    
    // STEP 7: Roll for status effects
    TMap<EDamageTypes, bool> StatusEffects = RollStatusEffects(AttAtt, RolledDamage);
    
    // Package results
    Result.FinalDamageByType = RolledDamage;
    Result.bCrit = bCrit;
    Result.bAppliedStatusEffectPerType = StatusEffects;
    
    return Result;
}

FDamageByType UCombatManager::GetBaseDamage(const UPHAttributeSet* AttAtt)
{
    FDamageByType BaseDamage;
    BaseDamage.PhysicalDamage   = {AttAtt->GetMinPhysicalDamage(),   AttAtt->GetMaxPhysicalDamage()};
    BaseDamage.FireDamage       = {AttAtt->GetMinFireDamage(),       AttAtt->GetMaxFireDamage()};
    BaseDamage.IceDamage        = {AttAtt->GetMinIceDamage(),        AttAtt->GetMaxIceDamage()};
    BaseDamage.LightningDamage  = {AttAtt->GetMinLightningDamage(),  AttAtt->GetMaxLightningDamage()};
    BaseDamage.LightDamage      = {AttAtt->GetMinLightDamage(),      AttAtt->GetMaxLightDamage()};
    BaseDamage.CorruptionDamage = {AttAtt->GetMinCorruptionDamage(), AttAtt->GetMaxCorruptionDamage()};
    
    return BaseDamage;
}

FDamageByType UCombatManager::ApplyDamageConversions(const FDamageByType& InDamage, 
                                                    const UPHAttributeSet* AttAtt)
{
    FDamageByType ConvertedDamage = InDamage;
    
    // Track how much of each damage type has been converted
    TMap<EDamageTypes, float> ConversionTracking;
    ConversionTracking.Add(EDamageTypes::DT_Physical, 0.f);
    ConversionTracking.Add(EDamageTypes::DT_Fire, 0.f);
    ConversionTracking.Add(EDamageTypes::DT_Ice, 0.f);
    ConversionTracking.Add(EDamageTypes::DT_Lightning, 0.f);
    ConversionTracking.Add(EDamageTypes::DT_Light, 0.f);
    ConversionTracking.Add(EDamageTypes::DT_Corruption, 0.f);
    
    // Helper to convert damage from one type to another
    auto ConvertDamage = [&](const EDamageTypes From, const EDamageTypes To, const float ConversionPercent)
    {
        if (ConversionPercent <= 0.f) return;
        
        // Cap total conversion at 100%
        float& AlreadyConverted = ConversionTracking[From];
        float ActualConversion = FMath::Min(ConversionPercent, 1.f - AlreadyConverted);
        if (ActualConversion <= 0.f) return;
        
        FDamageRange* FromDamage = nullptr;
        FDamageRange* ToDamage = nullptr;
        
        // Get source damage
        switch (From)
        {
            case EDamageTypes::DT_Physical:   FromDamage = &ConvertedDamage.PhysicalDamage; break;
            case EDamageTypes::DT_Fire:       FromDamage = &ConvertedDamage.FireDamage; break;
            case EDamageTypes::DT_Ice:        FromDamage = &ConvertedDamage.IceDamage; break;
            case EDamageTypes::DT_Lightning:  FromDamage = &ConvertedDamage.LightningDamage; break;
            case EDamageTypes::DT_Light:      FromDamage = &ConvertedDamage.LightDamage; break;
            case EDamageTypes::DT_Corruption: FromDamage = &ConvertedDamage.CorruptionDamage; break;
        default: ;
        }
        
        // Get target damage
        switch (To)
        {
            case EDamageTypes::DT_Physical:   ToDamage = &ConvertedDamage.PhysicalDamage; break;
            case EDamageTypes::DT_Fire:       ToDamage = &ConvertedDamage.FireDamage; break;
            case EDamageTypes::DT_Ice:        ToDamage = &ConvertedDamage.IceDamage; break;
            case EDamageTypes::DT_Lightning:  ToDamage = &ConvertedDamage.LightningDamage; break;
            case EDamageTypes::DT_Light:      ToDamage = &ConvertedDamage.LightDamage; break;
            case EDamageTypes::DT_Corruption: ToDamage = &ConvertedDamage.CorruptionDamage; break;
        default: ;
        }
        
        if (FromDamage && ToDamage && From != To)
        {
            float ConvertMin = FromDamage->Min * ActualConversion;
            float ConvertMax = FromDamage->Max * ActualConversion;
            
            // Add to target
            ToDamage->Min += ConvertMin;
            ToDamage->Max += ConvertMax;
            
            // Remove from source
            FromDamage->Min -= ConvertMin;
            FromDamage->Max -= ConvertMax;
            
            // Track conversion
            AlreadyConverted += ActualConversion;
        }
    };
    
    // Apply conversions in PoE2 order (Physical -> Elemental -> Chaos)
    // Physical conversions first
    ConvertDamage(EDamageTypes::DT_Physical, EDamageTypes::DT_Fire,       AttAtt->GetPhysicalToFire());
    ConvertDamage(EDamageTypes::DT_Physical, EDamageTypes::DT_Ice,        AttAtt->GetPhysicalToIce());
    ConvertDamage(EDamageTypes::DT_Physical, EDamageTypes::DT_Lightning,  AttAtt->GetPhysicalToLightning());
    ConvertDamage(EDamageTypes::DT_Physical, EDamageTypes::DT_Light,      AttAtt->GetPhysicalToLight());
    ConvertDamage(EDamageTypes::DT_Physical, EDamageTypes::DT_Corruption, AttAtt->GetPhysicalToCorruption());
    
    // Then elemental conversions (Fire, Ice, Lightning, Light to others)
    ConvertDamage(EDamageTypes::DT_Fire, EDamageTypes::DT_Ice,        AttAtt->GetFireToIce());
    ConvertDamage(EDamageTypes::DT_Fire, EDamageTypes::DT_Lightning,  AttAtt->GetFireToLightning());
    ConvertDamage(EDamageTypes::DT_Fire, EDamageTypes::DT_Corruption, AttAtt->GetFireToCorruption());
    
    ConvertDamage(EDamageTypes::DT_Ice, EDamageTypes::DT_Fire,        AttAtt->GetIceToFire());
    ConvertDamage(EDamageTypes::DT_Ice, EDamageTypes::DT_Lightning,   AttAtt->GetIceToLightning());
    ConvertDamage(EDamageTypes::DT_Ice, EDamageTypes::DT_Corruption,  AttAtt->GetIceToCorruption());
    
    ConvertDamage(EDamageTypes::DT_Lightning, EDamageTypes::DT_Fire,       AttAtt->GetLightningToFire());
    ConvertDamage(EDamageTypes::DT_Lightning, EDamageTypes::DT_Ice,        AttAtt->GetLightningToIce());
    ConvertDamage(EDamageTypes::DT_Lightning, EDamageTypes::DT_Corruption, AttAtt->GetLightningToCorruption());
    
    return ConvertedDamage;
}

TMap<EDamageTypes, float> UCombatManager::RollDamageValues(const FDamageByType& DamageRanges)
{
    TMap<EDamageTypes, float> RolledValues;
    
    // Roll each damage type
    if (DamageRanges.PhysicalDamage.Max > 0)
        RolledValues.Add(EDamageTypes::DT_Physical, FMath::FRandRange(DamageRanges.PhysicalDamage.Min, DamageRanges.PhysicalDamage.Max));
    
    if (DamageRanges.FireDamage.Max > 0)
        RolledValues.Add(EDamageTypes::DT_Fire, FMath::FRandRange(DamageRanges.FireDamage.Min, DamageRanges.FireDamage.Max));
    
    if (DamageRanges.IceDamage.Max > 0)
        RolledValues.Add(EDamageTypes::DT_Ice, FMath::FRandRange(DamageRanges.IceDamage.Min, DamageRanges.IceDamage.Max));
    
    if (DamageRanges.LightningDamage.Max > 0)
        RolledValues.Add(EDamageTypes::DT_Lightning, FMath::FRandRange(DamageRanges.LightningDamage.Min, DamageRanges.LightningDamage.Max));
    
    if (DamageRanges.LightDamage.Max > 0)
        RolledValues.Add(EDamageTypes::DT_Light, FMath::FRandRange(DamageRanges.LightDamage.Min, DamageRanges.LightDamage.Max));
    
    if (DamageRanges.CorruptionDamage.Max > 0)
        RolledValues.Add(EDamageTypes::DT_Corruption, FMath::FRandRange(DamageRanges.CorruptionDamage.Min, DamageRanges.CorruptionDamage.Max));
    
    return RolledValues;
}

TMap<EDamageTypes, float> UCombatManager::ApplyDamageModifiers(const TMap<EDamageTypes, float>& RolledDamage,
                                                              const UPHAttributeSet* AttAtt)
{
    TMap<EDamageTypes, float> ModifiedDamage = RolledDamage;
    
    // Global damage modifier (applies to all)
    const float GlobalDamageMulti = 1.f + AttAtt->GetGlobalDamages();
    
    for (auto& DmgPair : ModifiedDamage)
    {
        float& Damage = DmgPair.Value;
        EDamageTypes Type = DmgPair.Key;
        
        // Apply flat bonuses first
        switch (Type)
        {
            case EDamageTypes::DT_Physical:   Damage += AttAtt->GetPhysicalFlatBonus(); break;
            case EDamageTypes::DT_Fire:       Damage += AttAtt->GetFireFlatBonus(); break;
            case EDamageTypes::DT_Ice:        Damage += AttAtt->GetIceFlatBonus(); break;
            case EDamageTypes::DT_Lightning:  Damage += AttAtt->GetLightningFlatBonus(); break;
            case EDamageTypes::DT_Light:      Damage += AttAtt->GetLightFlatBonus(); break;
            case EDamageTypes::DT_Corruption: Damage += AttAtt->GetCorruptionFlatBonus(); break;
        default: ;
        }
        
        // Apply percentage increases
        float PercentMulti = 1.f;
        switch (Type)
        {
            case EDamageTypes::DT_Physical:   PercentMulti += AttAtt->GetPhysicalPercentBonus(); break;
            case EDamageTypes::DT_Fire:       PercentMulti += AttAtt->GetFirePercentBonus(); break;
            case EDamageTypes::DT_Ice:        PercentMulti += AttAtt->GetIcePercentBonus(); break;
            case EDamageTypes::DT_Lightning:  PercentMulti += AttAtt->GetLightningPercentBonus(); break;
            case EDamageTypes::DT_Light:      PercentMulti += AttAtt->GetLightPercentBonus(); break;
            case EDamageTypes::DT_Corruption: PercentMulti += AttAtt->GetCorruptionPercentBonus(); break;
        default: ;
        }
        
        // Apply all multipliers
        Damage *= PercentMulti * GlobalDamageMulti;
        
        // Special conditional modifiers
        if (const float CurrentHealthPercent = AttAtt->GetHealth() / AttAtt->GetMaxHealth(); CurrentHealthPercent >= 0.95f) // At full HP
        {
            Damage *= (1.f + AttAtt->GetDamageBonusWhileAtFullHP());
        }
        else if (CurrentHealthPercent <= 0.35f) // Low HP
        {
            Damage *= (1.f + AttAtt->GetDamageBonusWhileAtLowHP());
        }
    }
    
    return ModifiedDamage;
}

TMap<EDamageTypes, float> UCombatManager::ApplyDefenses(const TMap<EDamageTypes, float>& IncomingDamage,
                                                        const UPHAttributeSet* AttAtt,
                                                        const UPHAttributeSet* DefAtt)
{
    TMap<EDamageTypes, float> MitigatedDamage = IncomingDamage;
    
    // Global defense modifier
    const float GlobalDefenseMulti = FMath::Max(0.f, 1.f - DefAtt->GetGlobalDefenses());
    
    for (auto& DmgPair : MitigatedDamage)
    {
        float& Damage = DmgPair.Value;
        EDamageTypes Type = DmgPair.Key;
        
        // Get flat reduction
        float FlatReduction = 0.f;
        switch (Type)
        {
            case EDamageTypes::DT_Physical:
                FlatReduction = DefAtt->GetArmour() + DefAtt->GetArmourFlatBonus();
                break;
            case EDamageTypes::DT_Fire:
                FlatReduction = DefAtt->GetFireResistanceFlatBonus();
                break;
            case EDamageTypes::DT_Ice:
                FlatReduction = DefAtt->GetIceResistanceFlatBonus();
                break;
            case EDamageTypes::DT_Lightning:
                FlatReduction = DefAtt->GetLightningResistanceFlatBonus();
                break;
            case EDamageTypes::DT_Light:
                FlatReduction = DefAtt->GetLightResistanceFlatBonus();
                break;
            case EDamageTypes::DT_Corruption:
                FlatReduction = DefAtt->GetCorruptionResistanceFlatBonus();
                break;
            default: ;
        }
        
        // Apply flat reduction
        Damage = FMath::Max(0.f, Damage - FlatReduction);
        
        // Get percentage resistance and piercing
        float PercentResist = 0.f;
        float Piercing = 0.f;
        
        switch (Type)
        {
            case EDamageTypes::DT_Physical:
                PercentResist = DefAtt->GetArmourPercentBonus();
                Piercing = AttAtt->GetArmourPiercing();
                break;
            case EDamageTypes::DT_Fire:
                PercentResist = DefAtt->GetFireResistancePercentBonus();
                Piercing = AttAtt->GetFirePiercing();
                break;
            case EDamageTypes::DT_Ice:
                PercentResist = DefAtt->GetIceResistancePercentBonus();
                Piercing = AttAtt->GetIcePiercing();
                break;
            case EDamageTypes::DT_Lightning:
                PercentResist = DefAtt->GetLightningResistancePercentBonus();
                Piercing = AttAtt->GetLightningPiercing();
                break;
            case EDamageTypes::DT_Light:
                PercentResist = DefAtt->GetLightResistancePercentBonus();
                Piercing = AttAtt->GetLightPiercing();
                break;
            case EDamageTypes::DT_Corruption:
                PercentResist = DefAtt->GetCorruptionResistancePercentBonus();
                Piercing = AttAtt->GetCorruptionPiercing();
                break;
            default: ;
        }
        
        // Apply piercing (reduces effective resistance)
        float EffectiveResist = FMath::Max(0.f, PercentResist - Piercing);
        
        // Cap resistance at maximum values
        switch (Type)
        {
            case EDamageTypes::DT_Fire:
                EffectiveResist = FMath::Min(EffectiveResist, DefAtt->GetMaxFireResistance());
                break;
            case EDamageTypes::DT_Ice:
                EffectiveResist = FMath::Min(EffectiveResist, DefAtt->GetMaxIceResistance());
                break;
            case EDamageTypes::DT_Lightning:
                EffectiveResist = FMath::Min(EffectiveResist, DefAtt->GetMaxLightningResistance());
                break;
            case EDamageTypes::DT_Light:
                EffectiveResist = FMath::Min(EffectiveResist, DefAtt->GetMaxLightResistance());
                break;
            case EDamageTypes::DT_Corruption:
                EffectiveResist = FMath::Min(EffectiveResist, DefAtt->GetMaxCorruptionResistance());
                break;
            default: ;
        }
        
        // Apply percentage reduction
        Damage *= FMath::Max(0.f, 1.f - EffectiveResist);
        
        // Apply global defense
        Damage *= GlobalDefenseMulti;
    }
    
    return MitigatedDamage;
}

bool UCombatManager::RollCriticalStrike(const UPHAttributeSet* AttAtt)
{
    return FMath::FRand() <= AttAtt->GetCritChance();
}

TMap<EDamageTypes, bool> UCombatManager::RollStatusEffects(const UPHAttributeSet* AttAtt,
                                                          const TMap<EDamageTypes, float>& FinalDamage)
{
    TMap<EDamageTypes, bool> StatusEffects;
    
    for (const auto& DmgPair : FinalDamage)
    {
        if (DmgPair.Value <= 0.f) continue;
        
        float StatusChance = 0.f;
        switch (DmgPair.Key)
        {
            case EDamageTypes::DT_Physical:
                StatusChance = AttAtt->GetChanceToBleed();
                break;
            case EDamageTypes::DT_Fire:
                StatusChance = AttAtt->GetChanceToIgnite();
                break;
            case EDamageTypes::DT_Ice:
                StatusChance = AttAtt->GetChanceToFreeze();
                break;
            case EDamageTypes::DT_Lightning:
                StatusChance = AttAtt->GetChanceToShock();
                break;
            case EDamageTypes::DT_Light:
                StatusChance = AttAtt->GetChanceToPurify();
                break;
            case EDamageTypes::DT_Corruption:
                StatusChance = AttAtt->GetChanceToCorrupt();
                break;
            default: ;
        }
        
        bool bApplied = FMath::FRand() <= StatusChance;
        StatusEffects.Add(DmgPair.Key, bApplied);
    }
    
    return StatusEffects;
}

/* =============================== */
/* === DAMAGE APPLICATION ======== */
/* =============================== */

void UCombatManager::ApplyDamage(const APHBaseCharacter* Attacker, APHBaseCharacter* Defender)
{
    static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Condition.State.Dead"));
    if (!Defender || UPHGameplayTagLibrary::CheckTag(Defender, DeadTag))
    {
        return;
    }


    // Calculate damage
    const FDamageHitResultByType HitResult = CalculateDamage(Attacker, Defender);
    const float TotalDamage = HitResult.GetTotalDamage();
    
    if (TotalDamage <= 0.f) return;

    UPHAttributeSet* DefAtt = Cast<UPHAttributeSet>(Defender->GetAttributeSet());
    UAbilitySystemComponent* DefASC = Defender->GetAbilitySystemComponent();
    if (!DefAtt || !DefASC) return;
    
    // Enter combat for both attacker and defender
    if (Attacker && Attacker->GetCombatManager())
    {
        Attacker->GetCombatManager()->EnterCombat(Defender);
    }
    if (Defender->GetCombatManager())
    {
        Defender->GetCombatManager()->EnterCombat(const_cast<APHBaseCharacter*>(Attacker));
    }

    // Handle blocking
    float ActualDamage = TotalDamage;
    if (Defender->GetCombatManager() && Defender->GetCombatManager()->IsBlocking())
    {
        const float BlockPercent = FMath::Clamp(DefAtt->GetBlockStrength(), 0.f, 1.f);
        float BlockedDamage = TotalDamage * BlockPercent;
        ActualDamage = TotalDamage * (1.f - BlockPercent);
        
        // Drain stamina for blocked damage
        const float NewStamina = DefAtt->GetStamina() - BlockedDamage;
        DefASC->SetNumericAttributeBase(UPHAttributeSet::GetStaminaAttribute(), NewStamina);
        
        if (NewStamina <= 0.f && StaggerMontage)
        {
            Defender->PlayAnimMontage(StaggerMontage);
        }
    }
    
    // Apply damage to health
    const float NewHealth = DefAtt->GetHealth() - ActualDamage;
    DefASC->SetNumericAttributeBase(UPHAttributeSet::GetHealthAttribute(), NewHealth);
    
    // Handle poise damage
    float FinalPoiseDamage = ActualDamage * 0.5f;
    const float NewPoise = DefAtt->GetPoise() - FinalPoiseDamage;
    DefASC->SetNumericAttributeBase(UPHAttributeSet::GetPoiseAttribute(), NewPoise);
    
    if (NewPoise <= 0.f && StaggerMontage)
    {
        Defender->PlayAnimMontage(StaggerMontage);
    }
    
    // Apply life/mana leech to attacker
    if (Attacker)
    {
        ApplyLeech(Attacker, ActualDamage);
    }
    
    // Process reflection/thorns
    ProcessReflectionAndThorns(Attacker, Defender, HitResult);
    
    // Apply status effects
    ApplyStatusEffects(Defender, HitResult);
    
    // Get the highest damage type for popup
    EDamageTypes HighestType = GetHighestDamageType(HitResult);
    
    // Broadcast damage event
    OnDamageApplied.Broadcast(ActualDamage, HighestType, HitResult.bCrit);
    
    // Check if defender died
    CheckAliveStatus();
    
    // Debug logging
    UE_LOG(LogTemp, Log, TEXT("Applied %.0f damage (%s%s)"), 
        ActualDamage, 
        *DamageTypeToString(HighestType),
        HitResult.bCrit ? TEXT(" CRIT") : TEXT(""));
}

/* =============================== */
/* === LEECH SYSTEM ============== */
/* =============================== */

void UCombatManager::ApplyLeech(const APHBaseCharacter* Attacker, float DamageDealt)
{
    if (!Attacker) return;
    
    UPHAttributeSet* AttAtt = Cast<UPHAttributeSet>(Attacker->GetAttributeSet());
    UAbilitySystemComponent* AttASC = Attacker->GetAbilitySystemComponent();
    if (!AttAtt || !AttASC) return;
    
    // Life Leech
    float LifeLeechPercent = AttAtt->GetLifeLeech();
    if (LifeLeechPercent > 0.f)
    {
        float LeechedLife = DamageDealt * (LifeLeechPercent / 100.f);
        float NewHealth = FMath::Min(AttAtt->GetHealth() + LeechedLife, AttAtt->GetMaxHealth());
        AttASC->SetNumericAttributeBase(UPHAttributeSet::GetHealthAttribute(), NewHealth);
    }
    
    // Mana Leech
    float ManaLeechPercent = AttAtt->GetManaLeech();
    if (ManaLeechPercent > 0.f)
    {
        float LeechedMana = DamageDealt * (ManaLeechPercent / 100.f);
        float NewMana = FMath::Min(AttAtt->GetMana() + LeechedMana, AttAtt->GetMaxMana());
        AttASC->SetNumericAttributeBase(UPHAttributeSet::GetManaAttribute(), NewMana);
    }
}

/* =============================== */
/* === REFLECTION & THORNS ======= */
/* =============================== */

void UCombatManager::ProcessReflectionAndThorns(const APHBaseCharacter* Attacker,
                                               const APHBaseCharacter* Defender,
                                               const FDamageHitResultByType& IncomingHit)
{
    if (!Attacker || !Defender) return;
    
    const UPHAttributeSet* DefAtt = Cast<UPHAttributeSet>(Defender->GetAttributeSet());
    const UPHAttributeSet* AttAtt = Cast<UPHAttributeSet>(Attacker->GetAttributeSet());
    UAbilitySystemComponent* AttASC = Attacker->GetAbilitySystemComponent();
    
    if (!DefAtt || !AttAtt || !AttASC) return;
    
    // Process reflection for each damage type
    for (const auto& DmgPair : IncomingHit.FinalDamageByType)
    {
        if (DmgPair.Value <= 0.f) continue;
        
        float ReflectChance = 0.f;
        float ReflectPercent = 0.f;
        
        // Determine reflection stats based on a damage type
        if (DmgPair.Key == EDamageTypes::DT_Physical)
        {
            ReflectChance = DefAtt->GetReflectChancePhysical();
            ReflectPercent = DefAtt->GetReflectPhysical();
        }
        else // All elemental types
        {
            ReflectChance = DefAtt->GetReflectChanceElemental();
            ReflectPercent = DefAtt->GetReflectElemental();
        }
        
        // Roll for reflection
        if (FMath::FRand() <= (ReflectChance / 100.f))
        {
            float ReflectedDamage = DmgPair.Value * (ReflectPercent / 100.f);
            
            // Apply reflected damage to the attacker (ignoring their defenses)
            float NewHealth = AttAtt->GetHealth() - ReflectedDamage;
            AttASC->SetNumericAttributeBase(UPHAttributeSet::GetHealthAttribute(), NewHealth);
            
            UE_LOG(LogTemp, Log, TEXT("Reflected %.0f %s damage back to attacker"),
                ReflectedDamage, *DamageTypeToString(DmgPair.Key));
        }
    }
}

/* =============================== */
/* === STATUS EFFECTS ============ */
/* =============================== */

void UCombatManager::ApplyStatusEffects(const APHBaseCharacter* Target, 
                                       const FDamageHitResultByType& HitResult)
{
    if (!Target) return;
    
    UAbilitySystemComponent* ASC = Target->GetAbilitySystemComponent();
    if (!ASC) return;
    
    for (const auto& StatusPair : HitResult.bAppliedStatusEffectPerType)
    {
        if (!StatusPair.Value) continue; // Status isn't applied
        
        FGameplayTag StatusTag;
        switch (StatusPair.Key)
        {
            case EDamageTypes::DT_Physical:
                StatusTag = FGameplayTag::RequestGameplayTag(TEXT("Condition.Status.Bleeding"));
                break;
            case EDamageTypes::DT_Fire:
                StatusTag = FGameplayTag::RequestGameplayTag(TEXT("Condition.Status.Burning"));
                break;
            case EDamageTypes::DT_Ice:
                StatusTag = FGameplayTag::RequestGameplayTag(TEXT("Condition.Status.Frozen"));
                break;
            case EDamageTypes::DT_Lightning:
                StatusTag = FGameplayTag::RequestGameplayTag(TEXT("Condition.Status.Shocked"));
                break;
            case EDamageTypes::DT_Light:
                StatusTag = FGameplayTag::RequestGameplayTag(TEXT("Condition.Status.Purified"));
                break;
            case EDamageTypes::DT_Corruption:
                StatusTag = FGameplayTag::RequestGameplayTag(TEXT("Condition.Status.Corrupted"));
                break;
            default: ;
        }
        
        if (StatusTag.IsValid())
        {
            ASC->AddLooseGameplayTag(StatusTag);
            UE_LOG(LogTemp, Log, TEXT("Applied status effect: %s"), *StatusTag.ToString());
        }
    }
}

/* =============================== */
/* === HELPER FUNCTIONS ========== */
/* =============================== */

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

void UCombatManager::CheckAliveStatus() const
{
    if (!OwnerCharacter) return;

    UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
    if (!ASC) return;

    const float CurrentHealth = ASC->GetNumericAttributeBase(UPHAttributeSet::GetHealthAttribute());

    if (CurrentHealth <= 0.f)
    {
        const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Condition.State.Dead"));
        if (!ASC->HasMatchingGameplayTag(DeadTag))
        {
            ASC->AddLooseGameplayTag(DeadTag);
            
            if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
            {
                Mesh->SetCollisionProfileName(FName("Ragdoll"));
                Mesh->SetSimulatePhysics(true);
                Mesh->WakeAllRigidBodies();
                OwnerCharacter->GetCharacterMovement()->DisableMovement();
                OwnerCharacter->RagdollStart();
            }
        }
    }
}

/* =============================== */
/* === UI FUNCTIONS ============== */
/* =============================== */

void UCombatManager::InitPopup(const float DamageAmount, const EDamageTypes DamageType, const bool bIsCrit)
{
    if (!DamagePopupClass || !OwnerCharacter) return;

    FVector BaseLocation = OwnerCharacter->GetActorLocation() + FVector(0.f, 0.f, 120.f);
    float RandX = FMath::FRandRange(-50.f, 40.f);
    float RandY = FMath::FRandRange(-50.f, 40.f);
    FVector PopupLocation = BaseLocation + FVector(RandX, RandY, 0.f);

    UWidgetComponent* PopupComp = NewObject<UWidgetComponent>(OwnerCharacter);
    if (!PopupComp) return;

    PopupComp->RegisterComponent();
    PopupComp->AttachToComponent(OwnerCharacter->GetRootComponent(), 
        FAttachmentTransformRules::KeepWorldTransform);
    PopupComp->SetWorldLocation(PopupLocation);
    PopupComp->SetWidgetSpace(EWidgetSpace::World);
    PopupComp->SetDrawSize(FVector2D(50.f, 25.f));
    PopupComp->SetWidgetClass(DamagePopupClass);
    PopupComp->SetTwoSided(true);

    if (APlayerCameraManager* CamManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager)
    {
        FVector CamLocation = CamManager->GetCameraLocation();
        FVector ToCamera = (CamLocation - PopupComp->GetComponentLocation()).GetSafeNormal();
        FRotator LookAtRotation = ToCamera.Rotation();
        PopupComp->SetWorldRotation(LookAtRotation);
    }

    if (UDamagePopup* Popup = Cast<UDamagePopup>(PopupComp->GetUserWidgetObject()))
    {
        Popup->SetDamageData(static_cast<int32>(DamageAmount), DamageType, bIsCrit);
    }

    FTimerHandle TimerHandle;
    OwnerCharacter->GetWorldTimerManager().SetTimer(TimerHandle, [PopupComp]()
    {
        if (PopupComp) PopupComp->DestroyComponent();
    }, 1.5f, false);
}

/* =============================== */
/* === COMBO SYSTEM ============== */
/* =============================== */

void UCombatManager::IncreaseComboCounter(float Amount)
{
    OwnerCharacter = Cast<APHBaseCharacter>(GetOwner());
    if (!OwnerCharacter) return;

    UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
    UPHAttributeSet* Attributes = Cast<UPHAttributeSet>(OwnerCharacter->GetAttributeSet());
    if (!ASC || !Attributes) return;

    const float CurrentValue = Attributes->GetComboCounter();
    ASC->SetNumericAttributeBase(UPHAttributeSet::GetComboCounterAttribute(), CurrentValue + Amount);

    GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(ComboResetTimerHandle, this, 
        &UCombatManager::ResetComboCounter, ComboResetTime, false);
}

void UCombatManager::ResetComboCounter() const
{
    if (!OwnerCharacter) return;

    if (UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent())
    {
        ASC->SetNumericAttributeBase(UPHAttributeSet::GetComboCounterAttribute(), 0.f);
    }
}

/* ================================ */
/* === COMBAT STATUS MANAGEMENT === */
/* ================================ */

void UCombatManager::SetCombatStatus(ECombatStatus NewStatus)
{
    if (CurrentCombatStatus == NewStatus) return;
    
    ECombatStatus OldStatus = CurrentCombatStatus;
    CurrentCombatStatus = NewStatus;
    
    // Update attribute
    if (OwnerCharacter)
    {
        if (UPHAttributeSet* AttSet = Cast<UPHAttributeSet>(OwnerCharacter->GetAttributeSet()))
        {
            if (UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent())
            {
                ASC->SetNumericAttributeBase(UPHAttributeSet::GetCombatStatusAttribute(), 
                    static_cast<float>(NewStatus));
            }
        }
    }
    
    // Handle status transitions
    switch (NewStatus)
    {
        case ECombatStatus::InCombat:
            OnEnterCombat();
            break;
            
        case ECombatStatus::OutOfCombat:
            OnExitCombat();
            break;
            
        case ECombatStatus::EnteringCombat:
            // Optional: Play entering combat animation or effects
            GetWorld()->GetTimerManager().SetTimer(CombatTransitionTimer, [this]()
            {
                SetCombatStatus(ECombatStatus::InCombat);
            }, CombatEnterTransitionTime, false);
            break;
            
        case ECombatStatus::LeavingCombat:
            // Optional: Play leaving combat animation or effects
            GetWorld()->GetTimerManager().SetTimer(CombatTransitionTimer, [this]()
            {
                SetCombatStatus(ECombatStatus::OutOfCombat);
            }, CombatExitTransitionTime, false);
            break;
    }
    
    // Broadcast the change
    OnCombatStatusChanged.Broadcast(OldStatus, NewStatus);
    
    UE_LOG(LogTemp, Log, TEXT("%s: Combat status changed from %s to %s"),
        *OwnerCharacter->GetName(),
        *UEnum::GetValueAsString(OldStatus),
        *UEnum::GetValueAsString(NewStatus));
}

void UCombatManager::EnterCombat(APHBaseCharacter* Enemy)
{
    // Add to combat targets
    if (Enemy && !CombatTargets.Contains(Enemy))
    {
        CombatTargets.Add(Enemy);
    }
    
    // Enter combat if not already
    if (CurrentCombatStatus == ECombatStatus::OutOfCombat)
    {
        SetCombatStatus(bUseTransitions ? ECombatStatus::EnteringCombat : ECombatStatus::InCombat);
    }
    
    // Reset combat timer
    RefreshCombatTimer();
}

void UCombatManager::ExitCombat()
{
    if (CurrentCombatStatus == ECombatStatus::InCombat)
    {
        // Check if we should immediately exit or transition
        if (CombatTargets.Num() > 0)
        {
            // Still have enemies, don't exit
            RefreshCombatTimer();
            return;
        }
        
        SetCombatStatus(bUseTransitions ? ECombatStatus::LeavingCombat : ECombatStatus::OutOfCombat);
    }
}

void UCombatManager::RefreshCombatTimer()
{
    LastCombatActivityTime = GetWorld()->GetTimeSeconds();
    
    // Clear existing timer
    GetWorld()->GetTimerManager().ClearTimer(CombatExitTimer);
    
    // Set new timer
    GetWorld()->GetTimerManager().SetTimer(CombatExitTimer, this, 
        &UCombatManager::CheckCombatTimeout, 1.0f, true);
}

void UCombatManager::CheckCombatTimeout()
{
    if (CurrentCombatStatus != ECombatStatus::InCombat) return;
    
    float TimeSinceLastActivity = GetWorld()->GetTimeSeconds() - LastCombatActivityTime;
    
    if (TimeSinceLastActivity >= CombatTimeout)
    {
        // Clean up dead or distant targets
        CombatTargets.RemoveAll([this](const APHBaseCharacter* Target)
        {
            if (!IsValid(Target)) return true;
            
            // Remove if dead
            if (const UAbilitySystemComponent* ASC = Target->GetAbilitySystemComponent())
            {
                if (const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Condition.State.Dead")); ASC->HasMatchingGameplayTag(DeadTag)) return true;
            }
            
            // Remove if too far away
            float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), 
                                          Target->GetActorLocation());
            if (Distance > MaxCombatDistance) return true;
            
            return false;
        });
        
        // Exit combat if no valid targets
        if (CombatTargets.Num() == 0)
        {
            GetWorld()->GetTimerManager().ClearTimer(CombatExitTimer);
            ExitCombat();
        }
    }
}

void UCombatManager::ForceCombatExit()
{
    CombatTargets.Empty();
    GetWorld()->GetTimerManager().ClearTimer(CombatExitTimer);
    GetWorld()->GetTimerManager().ClearTimer(CombatTransitionTimer);
    SetCombatStatus(ECombatStatus::OutOfCombat);
}

void UCombatManager::OnEnterCombat()
{
    // Stop any out-of-combat regeneration
    bCanRegenerate = false;
    
    // Apply combat-related gameplay tags
    if (UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent())
    {
        ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Condition.State.InCombat")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Condition.State.OutOfCombat")));
    }
    
    // Broadcast for UI updates
    OnCombatEntered.Broadcast();
}

void UCombatManager::OnExitCombat()
{
    // Clear combat targets
    CombatTargets.Empty();
    
    // Enable regeneration after delay
    GetWorld()->GetTimerManager().SetTimer(RegenerationDelayTimer, [this]()
    {
        bCanRegenerate = true;
    }, OutOfCombatRegenDelay, false);
    
    // Update gameplay tags
    if (UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent())
    {
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Condition.State.InCombat")));
        ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Condition.State.OutOfCombat")));
    }
    
    // Reset combo counter
    ResetComboCounter();
    
    // Broadcast for UI updates
    OnCombatExited.Broadcast();
}

void UCombatManager::HandleOutOfCombatRegeneration(const float DeltaTime) const
{
    if (!bCanRegenerate || !OwnerCharacter) return;
    
    UPHAttributeSet* AttSet = Cast<UPHAttributeSet>(OwnerCharacter->GetAttributeSet());
    UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
    if (!AttSet || !ASC) return;
    
    // Health regeneration
    if (OutOfCombatHealthRegen > 0.f)
    {
        const float CurrentHealth = AttSet->GetHealth();
        if (const float MaxHealth = AttSet->GetMaxHealth(); CurrentHealth < MaxHealth)
        {
            float NewHealth = FMath::Min(CurrentHealth + (OutOfCombatHealthRegen * DeltaTime), MaxHealth);
            ASC->SetNumericAttributeBase(UPHAttributeSet::GetHealthAttribute(), NewHealth);
        }
    }
    
    // Mana regeneration
    if (OutOfCombatManaRegen > 0.f)
    {
        const float CurrentMana = AttSet->GetMana();
        if (const float MaxMana = AttSet->GetMaxMana(); CurrentMana < MaxMana)
        {
            float NewMana = FMath::Min(CurrentMana + (OutOfCombatManaRegen * DeltaTime), MaxMana);
            ASC->SetNumericAttributeBase(UPHAttributeSet::GetManaAttribute(), NewMana);
        }
    }
    
    // Stamina regeneration
    if (OutOfCombatStaminaRegen > 0.f)
    {
        float CurrentStamina = AttSet->GetStamina();
        float MaxStamina = AttSet->GetMaxStamina();
        if (CurrentStamina < MaxStamina)
        {
            float NewStamina = FMath::Min(CurrentStamina + (OutOfCombatStaminaRegen * DeltaTime), MaxStamina);
            ASC->SetNumericAttributeBase(UPHAttributeSet::GetStaminaAttribute(), NewStamina);
        }
    }
}

bool UCombatManager::IsInCombatWith(const APHBaseCharacter* Target) const
{
    return CombatTargets.Contains(Target);
}

TArray<APHBaseCharacter*> UCombatManager::GetActiveCombatTargets() const
{
    TArray<APHBaseCharacter*> ValidTargets;
    for (APHBaseCharacter* Target : CombatTargets)
    {
        if (IsValid(Target))
        {
            ValidTargets.Add(Target);
        }
    }
    return ValidTargets;
}
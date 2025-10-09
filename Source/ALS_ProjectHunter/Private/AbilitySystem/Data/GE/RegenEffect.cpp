// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/GE/RegenEffect.h"

#include "PHGameplayTags.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "AbilitySystem/ModMagCalc/MMC_Regen.h"

URegenEffect::URegenEffect()
{
    DurationPolicy = EGameplayEffectDurationType::Infinite;
    bExecutePeriodicEffectOnApplication = false;
    RegenType = EVitalRegenType::Health;
}

void URegenEffect::PostInitProperties()
{
    Super::PostInitProperties();

    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        switch (RegenType)
        {
            case EVitalRegenType::Health:
                SetupHealthRegen();
                break;
            case EVitalRegenType::Mana:
                SetupManaRegen();
                break;
            case EVitalRegenType::Stamina:
                SetupStaminaRegen();
                break;
            case EVitalRegenType::ArcaneShield:
                SetupArcaneShieldRegen();
                break;
        }
    }
}

void URegenEffect::SetupHealthRegen()
{
    const FPHGameplayTags& Tags = FPHGameplayTags::Get();
    
    // Set Period to 1.0 second - MMC will calculate amount per second
    Period.SetValue(1.0f);
    
    int32 Idx = Modifiers.Num();
    Modifiers.SetNum(Idx + 1);
    
    FGameplayModifierInfo& ModifierInfo = Modifiers[Idx];
    ModifierInfo.Attribute = UPHAttributeSet::GetHealthAttribute();
    ModifierInfo.ModifierOp = EGameplayModOp::Additive;
    
    FCustomCalculationBasedFloat CustomCalc;
    CustomCalc.CalculationClassMagnitude = UMMC_Regen::StaticClass();
    ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);
    
    UTargetTagsGameplayEffectComponent& TargetTagsComponent = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
    TargetTagsComponent.SetAndApplyTargetTagChanges(
        FInheritedTagContainer(FGameplayTagContainer(Tags.Effect_Health_RegenActive))
    );
}

void URegenEffect::SetupManaRegen()
{
    const FPHGameplayTags& Tags = FPHGameplayTags::Get();
    
    // Set Period to 1.0 second - MMC will calculate amount per second
    Period.SetValue(1.0f);
    
    int32 Idx = Modifiers.Num();
    Modifiers.SetNum(Idx + 1);
    
    FGameplayModifierInfo& ModifierInfo = Modifiers[Idx];
    // FIX: Change from GetHealthAttribute() to GetManaAttribute()
    ModifierInfo.Attribute = UPHAttributeSet::GetManaAttribute(); 
    ModifierInfo.ModifierOp = EGameplayModOp::Additive;
    
    FCustomCalculationBasedFloat CustomCalc;
    CustomCalc.CalculationClassMagnitude = UMMC_Regen::StaticClass();
    ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);
    
    UTargetTagsGameplayEffectComponent& TargetTagsComponent = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
    TargetTagsComponent.SetAndApplyTargetTagChanges(
        FInheritedTagContainer(FGameplayTagContainer(Tags.Effect_Mana_RegenActive))
    );
}

void URegenEffect::SetupStaminaRegen()
{
    const FPHGameplayTags& Tags = FPHGameplayTags::Get();
    
    // Set Period to 1.0 second - MMC will calculate amount per second
    Period.SetValue(1.0f);
    
    int32 Idx = Modifiers.Num();
    Modifiers.SetNum(Idx + 1);
    
    FGameplayModifierInfo& ModifierInfo = Modifiers[Idx];
    // FIX: Change from GetHealthAttribute() to GetStaminaAttribute()
    ModifierInfo.Attribute = UPHAttributeSet::GetStaminaAttribute();  // ← FIXED!
    ModifierInfo.ModifierOp = EGameplayModOp::Additive;
    
    FCustomCalculationBasedFloat CustomCalc;
    CustomCalc.CalculationClassMagnitude = UMMC_Regen::StaticClass();
    ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);
    
    UTargetTagsGameplayEffectComponent& TargetTagsComponent = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
    TargetTagsComponent.SetAndApplyTargetTagChanges(
        FInheritedTagContainer(FGameplayTagContainer(Tags.Effect_Stamina_RegenActive))
    );
}

void URegenEffect::SetupArcaneShieldRegen()
{
    const FPHGameplayTags& Tags = FPHGameplayTags::Get();
    
    Period.SetValue(1.0f);
    
    int32 Idx = Modifiers.Num();
    Modifiers.SetNum(Idx + 1);
    
    FGameplayModifierInfo& ModifierInfo = Modifiers[Idx];
    ModifierInfo.Attribute = UPHAttributeSet::GetArcaneShieldAttribute();
    ModifierInfo.ModifierOp = EGameplayModOp::Additive;
    
    FCustomCalculationBasedFloat CustomCalc;
    CustomCalc.CalculationClassMagnitude = UMMC_Regen::StaticClass();
    ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);

    UTargetTagsGameplayEffectComponent& TargetTagsComponent = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
    TargetTagsComponent.SetAndApplyTargetTagChanges(
        FInheritedTagContainer(FGameplayTagContainer(Tags.Effect_ArcaneShield_RegenActive))
    );
}
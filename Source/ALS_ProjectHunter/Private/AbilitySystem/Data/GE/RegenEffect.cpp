// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/GE/RegenEffect.h"

#include "PHGameplayTags.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "AbilitySystem/ModMagCalc/MMC_Regen.h"
#include "Fonts/UnicodeBlockRange.h"

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
    
    // Period defaults to 1.0 second (you can adjust in Blueprint)
    Period.SetValue(1.0f);
    
    // Add modifier
    int32 Idx = Modifiers.Num();
    Modifiers.SetNum(Idx + 1);
    
    FGameplayModifierInfo& ModifierInfo = Modifiers[Idx];
    ModifierInfo.Attribute = UPHAttributeSet::GetHealthAttribute();
    ModifierInfo.ModifierOp = EGameplayModOp::Additive;
    
    // Create custom calculation
    FCustomCalculationBasedFloat CustomCalc;
    CustomCalc.CalculationClassMagnitude = UMMC_Regen::StaticClass();
    ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);
    
    // Add tags using the new API
    UTargetTagsGameplayEffectComponent& TargetTagsComponent = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
    FInheritedTagContainer GrantedTags = TargetTagsComponent.GetConfiguredTargetTagChanges();
    GrantedTags.Added.AddTag(Tags.Effect_Health_RegenActive);
}

void URegenEffect::SetupManaRegen()
{
    const FPHGameplayTags& Tags = FPHGameplayTags::Get();
    
    Period.SetValue(1.0f);
    
    int32 Idx = Modifiers.Num();
    Modifiers.SetNum(Idx + 1);
    
    FGameplayModifierInfo& ModifierInfo = Modifiers[Idx];
    ModifierInfo.Attribute = UPHAttributeSet::GetManaAttribute();
    ModifierInfo.ModifierOp = EGameplayModOp::Additive;
    
    FCustomCalculationBasedFloat CustomCalc;
    CustomCalc.CalculationClassMagnitude = UMMC_Regen::StaticClass();
    ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);
    
    UTargetTagsGameplayEffectComponent& TargetTagsComponent = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
    FInheritedTagContainer GrantedTags = TargetTagsComponent.GetConfiguredTargetTagChanges();
    GrantedTags.Added.AddTag(Tags.Effect_Mana_RegenActive);
}

void URegenEffect::SetupStaminaRegen()
{
    const FPHGameplayTags& Tags = FPHGameplayTags::Get();
    
    Period.SetValue(1.0f);
    
    int32 Idx = Modifiers.Num();
    Modifiers.SetNum(Idx + 1);
    
    FGameplayModifierInfo& ModifierInfo = Modifiers[Idx];
    ModifierInfo.Attribute = UPHAttributeSet::GetStaminaAttribute();
    ModifierInfo.ModifierOp = EGameplayModOp::Additive;
    
    FCustomCalculationBasedFloat CustomCalc;
    CustomCalc.CalculationClassMagnitude = UMMC_Regen::StaticClass();
    ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalc);
    
    UTargetTagsGameplayEffectComponent& TargetTagsComponent = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
    FInheritedTagContainer GrantedTags = TargetTagsComponent.GetConfiguredTargetTagChanges();
    GrantedTags.Added.AddTag(Tags.Effect_Stamina_RegenActive);
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
    FInheritedTagContainer GrantedTags = TargetTagsComponent.GetConfiguredTargetTagChanges();
    GrantedTags.Added.AddTag(Tags.Effect_ArcaneShield_RegenActive);
}
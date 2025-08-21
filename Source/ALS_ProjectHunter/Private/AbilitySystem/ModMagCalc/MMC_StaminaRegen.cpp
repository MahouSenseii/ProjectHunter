// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_StaminaRegen.h"

#include "AbilitySystemComponent.h"
#include "PHGameplayTags.h"
#include "GameplayTagContainer.h"
#include "Logging/LogMacros.h"
// MMC_StaminaRegen.cpp


float UMMC_StaminaRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    // Always prefer your native tag singletons to avoid string typos
    const auto& Tags = FPHGameplayTags::Get();

    const FGameplayTag RegenAmountTag = Tags.Attributes_Secondary_Vital_StaminaRegenAmount; // "Attribute.Secondary.Vital.StaminaRegenAmount"
    const FGameplayTag DegenTag       = Tags.Attributes_Secondary_Vital_StaminaDegen;       // "Attribute.Secondary.Vital.StaminaDegen"

    float RegenAmount = 0.0f;   // +stamina / tick
    float DegenAmount = 0.0f;   // -stamina / tick

    // Pull the SetByCaller magnitudes that your StatsManager rows supply
    // Row A should set RegenAmountTag; Row B should set DegenTag (the value)
    if (!Spec.GetSetByCallerMagnitude(RegenAmountTag, /*warnIfNotFound*/ false, RegenAmount))
    {
        UE_LOG(LogTemp, Verbose, TEXT("[MMC_StaminaRegen] Missing SetByCaller %s (using 0)."),
            *RegenAmountTag.ToString());
    }

    if (!Spec.GetSetByCallerMagnitude(DegenTag, /*warnIfNotFound*/ false, DegenAmount))
    {
        UE_LOG(LogTemp, Verbose, TEXT("[MMC_StaminaRegen] Missing SetByCaller %s (using 0)."),
            *DegenTag.ToString());
    }

    // Decide whether we are in "degen mode" by checking the TARGET's tags.
    // (StatsManager re-applies the effect when this trigger tag toggles.)
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
    const bool bShouldDegen = (TargetTags && TargetTags->HasTag(DegenTag));

    // If degen is active, return a negative amount based on DegenAmount.
    // Otherwise, return the (positive) regen amount.
    if (bShouldDegen)
    {
        return -FMath::Abs(DegenAmount);
    }

    return RegenAmount;
}

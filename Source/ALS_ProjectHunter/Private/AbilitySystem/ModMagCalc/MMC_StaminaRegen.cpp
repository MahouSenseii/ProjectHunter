// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_StaminaRegen.h"

#include "AbilitySystemComponent.h"
#include "PHGameplayTags.h"

float UMMC_StaminaRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTag StaminaDegenTag = FPHGameplayTags::Get().Attributes_Secondary_Vital_StaminaDegen;

	const UAbilitySystemComponent* TargetASC = Spec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
	if (!TargetASC)
		return 0.f;

	float Amount = Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attribute.Secondary.Vital.StaminaRegenAmount")), false, 0.0f);
	float AmountDegen = Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attribute.Secondary.Vital.StaminaDegen")), false, 0.0f);
	// Check if sprinting/degen tag is active
	if (TargetASC->HasMatchingGameplayTag(StaminaDegenTag))
	{
		Amount = -FMath::Abs(AmountDegen ); // Make it drain instead
	}

	return Amount;
}
// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_StaminaRegen.h"

#include "AbilitySystemComponent.h"
#include "PHGameplayTags.h"

float UMMC_StaminaRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTag StaminaRegenTag = FGameplayTag::RequestGameplayTag(FName("Attribute.Secondary.Vital.StaminaRegenAmount"));
	const FGameplayTag StaminaDegenTag = FGameplayTag::RequestGameplayTag(FName("Attributes_Secondary_Vital_StaminaDegen"));
	const FGameplayTag StaminaDegenValueTag = FGameplayTag::RequestGameplayTag(FName("Attribute.Secondary.Vital.StaminaDegen"));

	const UAbilitySystemComponent* TargetASC = Spec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
	if (!TargetASC)
		return 0.f;

	float Amount = 0.0f;
	float AmountDegen = 0.0f;

	if (!Spec.GetSetByCallerMagnitude(StaminaRegenTag, false, Amount))
	{
		UE_LOG(LogTemp, Warning, TEXT("Missing SetByCaller tag: %s"), *StaminaRegenTag.ToString());
	}

	if (!Spec.GetSetByCallerMagnitude(StaminaDegenValueTag, false, AmountDegen))
	{
		UE_LOG(LogTemp, Warning, TEXT("Missing SetByCaller tag: %s"), *StaminaDegenValueTag.ToString());
	}

	if (TargetASC->HasMatchingGameplayTag(StaminaDegenTag))
	{
		Amount = -FMath::Abs(AmountDegen); // Convert regen to drain
	}

	return Amount;
}
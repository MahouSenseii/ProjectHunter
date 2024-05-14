// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"


/**
 * Gameplay tags
 * Singleton containing native gameplay tags 
 */

struct FPHGameplayTags
{
public:

	static  const FPHGameplayTags& Get() {return GameplayTags;}
	static void InitializeNativeGameplayTags();

	// primary attributes 
	FGameplayTag Attributes_Primary_Strength;
	FGameplayTag Attributes_Primary_Intelligence;
	FGameplayTag Attributes_Primary_Dexterity;
	FGameplayTag Attributes_Primary_Endurance;
	FGameplayTag Attributes_Primary_Affliction;
	FGameplayTag Attributes_Primary_Luck;
	FGameplayTag Attributes_Primary_Covenant;

	//secondary attributes
	FGameplayTag Attributes_Secondary_Armor;
	FGameplayTag Attributes_Secondary_HealthRegenRate;
	FGameplayTag Attributes_Secondary_HealthRegenAmount;
	FGameplayTag Attributes_Secondary_MaxHealthRegenRate;
	FGameplayTag Attributes_Secondary_MaxHealthRegenAmount;
	FGameplayTag Attributes_Secondary_HealthReservedAmount;
	FGameplayTag Attributes_Secondary_MaxHealthReservedAmount;
	FGameplayTag Attributes_Secondary_HealthFlatReservedAmount;
	FGameplayTag Attributes_Secondary_HealthPercentageReserved;

	FGameplayTag Attributes_Secondary_ManaRegenRate;
	FGameplayTag Attributes_Secondary_ManaRegenAmount;
	FGameplayTag Attributes_Secondary_MaxManaRegenRate;
	FGameplayTag Attributes_Secondary_MaxManaRegenAmount;
	FGameplayTag Attributes_Secondary_ManaReservedAmount;
	FGameplayTag Attributes_Secondary_MaxManaReservedAmount;
	FGameplayTag Attributes_Secondary_ManaFlatReservedAmount;
	FGameplayTag Attributes_Secondary_ManaPercentageReserved;

	FGameplayTag Attributes_Secondary_StaminaRegenRate;
	FGameplayTag Attributes_Secondary_StaminaRegenAmount;
	FGameplayTag Attributes_Secondary_MaxStaminaRegenRate;
	FGameplayTag Attributes_Secondary_MaxStaminaRegenAmount;
	FGameplayTag Attributes_Secondary_StaminaReservedAmount;
	FGameplayTag Attributes_Secondary_MaxStaminaReservedAmount;
	FGameplayTag Attributes_Secondary_StaminaFlatReservedAmount;
	FGameplayTag Attributes_Secondary_StaminaPercentageReserved;
	
	
	FGameplayTag Attributes_Secondary_MaxHealth;
	FGameplayTag Attributes_Secondary_MaxEffectiveHealth;
	FGameplayTag Attributes_Secondary_MaxMana;
	FGameplayTag Attributes_Secondary_MaxEffectiveMana;
	FGameplayTag Attributes_Secondary_MaxStamina;
	FGameplayTag Attributes_Secondary_MaxEffectiveStamina;


	
	//Vitals
	FGameplayTag Attributes_Vital_Health;
	FGameplayTag Attributes_Vital_Mana;
	FGameplayTag Attributes_Vital_Stamina;
	
protected:
 

private:
	static  FPHGameplayTags GameplayTags;
};

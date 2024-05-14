// Fill out your copyright notice in the Description page of Project Settings.


#include "PHGameplayTags.h"
#include "GameplayTagsManager.h"

FPHGameplayTags  FPHGameplayTags::GameplayTags;

//ToDo You can add a new tag by copying this GameplayTags.Attributes_"Primary/Secondary"_"Name" = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Name"), FString("description"));
void FPHGameplayTags::InitializeNativeGameplayTags()
{

	// Primary gameplay tags
	GameplayTags.Attributes_Primary_Strength = UGameplayTagsManager::Get().AddNativeGameplayTag(
	FName("Attribute.Primary.Strength"),
	FString("Increases physical damage  and slightly increase health."));

	GameplayTags.Attributes_Primary_Intelligence = UGameplayTagsManager::Get().AddNativeGameplayTag(
	FName("Attribute.Primary.Intelligence"),
	FString("Increases Mana and slightly increase elemental damage."));

	GameplayTags.Attributes_Primary_Dexterity = UGameplayTagsManager::Get().AddNativeGameplayTag(
	FName("Attribute.Primary.Dexterity"),
	FString("Increases critical multiplier  and slightly attack and cast speed."));

	GameplayTags.Attributes_Primary_Endurance = UGameplayTagsManager::Get().AddNativeGameplayTag(
	FName("Attribute.Primary.Endurance"),
	FString(" Increases Stamina and slightly increases resistances."));

	GameplayTags.Attributes_Primary_Affliction = UGameplayTagsManager::Get().AddNativeGameplayTag(
	FName("Attribute.Primary.Affliction"),
	FString("Increases DOT and slightly duration."));

	GameplayTags.Attributes_Primary_Luck = UGameplayTagsManager::Get().AddNativeGameplayTag(
	FName("Attribute.Primary.Luck"),
	FString("Increases chance to apply and drop chance."));

	GameplayTags.Attributes_Primary_Covenant = UGameplayTagsManager::Get().AddNativeGameplayTag(
	FName("Attribute.Primary.Covenant"),
	FString("Increases minions health and damage."));

	//Secondary gameplay tags

	/** Regen Start */
	GameplayTags.Attributes_Secondary_HealthRegenRate = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.HealthRegenRate"),
	FString("How fast health is regen."));

	GameplayTags.Attributes_Secondary_HealthRegenAmount = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.HealthRegenAmount"),
	FString("How much health is regen."));

	GameplayTags.Attributes_Secondary_MaxHealthRegenAmount = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.MaxHealthRegenAmount"),
	FString("Max amount health that  can be regen"));

	GameplayTags.Attributes_Secondary_HealthReservedAmount = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.HealthReservedAmount"),
	FString("Reserved Amount of Health"));

	GameplayTags.Attributes_Secondary_MaxHealthReservedAmount = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.MaxHealthReservedAmount"),
	FString("MaxReserved Amount of Health"));

	GameplayTags.Attributes_Secondary_MaxHealthRegenRate = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.MaxHealthRegenRate"),
	FString("Max speed health can regen."));

	GameplayTags.Attributes_Secondary_ManaRegenAmount = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.ManaRegenAmount"),
	FString("How much mana is regen"));

	GameplayTags.Attributes_Secondary_MaxManaRegenAmount = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.MaxManaRegenAmount"),
	FString("Max amount mana that can be regen "));

	GameplayTags.Attributes_Secondary_ManaReservedAmount = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.ManaReservedAmount"),
	FString("Reserved Amount of Mana"));

	GameplayTags.Attributes_Secondary_MaxManaReservedAmount = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.MaxManaReservedAmount"),
	FString("Max Reserved Amount of Mana"));

	GameplayTags.Attributes_Secondary_ManaRegenRate= UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.ManaRegenRate"),
	FString("How fast mana can be regen."));

	GameplayTags.Attributes_Secondary_MaxManaRegenRate = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.MaxManaRegenRate"),
	FString("Max speed mana can be regen."));

	GameplayTags.Attributes_Secondary_StaminaRegenAmount = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.StaminaRegenAmount"),
	FString("How much stamina is regen."));

	GameplayTags.Attributes_Secondary_MaxStaminaRegenAmount = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.MaxStaminaRegenAmount"),
	FString("Max amount stamina that can be regen "));
	
	GameplayTags.Attributes_Secondary_StaminaReservedAmount = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.StaminaReservedAmount"),
	FString("Reserved Amount of Stamina"));

	GameplayTags.Attributes_Secondary_MaxStaminaReservedAmount = UGameplayTagsManager::Get().AddNativeGameplayTag
    (FName("Attribute.Secondary.MaxStaminaReservedAmount"),
    FString("Max Reserved Amount of Stamina"));

	GameplayTags.Attributes_Secondary_StaminaRegenRate = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.StaminaRegenRate"),
	FString("How fast stamina can be regen."));

	GameplayTags.Attributes_Secondary_MaxStaminaRegenRate = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.MaxStaminaRegenRate"),
	FString("Max speed mana can be regen."));
	/** Regen End */
	/** Defenses Start*/
	GameplayTags.Attributes_Secondary_Armor = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.Armor"),
	FString("Reduces damage taken from physical damage."));
	/** Defenses End*/

	/** Max Vital Start*/
	GameplayTags.Attributes_Secondary_MaxHealth = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.MaxHealth"),
	FString("Max amount of health."));

	GameplayTags.Attributes_Secondary_MaxEffectiveHealth = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.MaxEffectiveHealth"),
	FString("Max amount of health after reserved health is calculated."));
	
	GameplayTags.Attributes_Secondary_MaxMana = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.MaxMana"),
	FString("Max amount of Mana."));

	GameplayTags.Attributes_Secondary_MaxEffectiveMana = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.MaxEffectiveMana"),
	FString("Max amount of mana after reserved mana is calculated."));
	
	GameplayTags.Attributes_Secondary_MaxStamina  = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.MaxStamina"),
	FString("Max amount of stamina"));


	GameplayTags.Attributes_Secondary_MaxEffectiveStamina = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Secondary.MaxEffectiveHealth"),
	FString("Max amount of stamina after reserved stamina is calculated."));

	/** Max Vital End*/

	/** Vital Start*/
	GameplayTags.Attributes_Vital_Health = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Vital.MaxHealth"),
	FString("Amount of health."));
	GameplayTags.Attributes_Vital_Mana = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Vital.Mana"),
	FString("Amount of Mana."));
	GameplayTags.Attributes_Vital_Stamina  = UGameplayTagsManager::Get().AddNativeGameplayTag
	(FName("Attribute.Vital.Stamina"),
	FString("Amount of stamina"));
	/** Vital End*/
	
}

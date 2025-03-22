// Copyright@2024 Quentin Davis 

#include "PHGameplayTags.h"
#include "GameplayTagsManager.h"

// Initialize the static singleton instance
FPHGameplayTags FPHGameplayTags::GameplayTags;

/* ============================= */
/* === Initialize Gameplay Tags === */
/* ============================= */
void FPHGameplayTags::InitializeNativeGameplayTags()
{
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	/* ========================== */
	/* === Primary Attributes === */
	/* ========================== */
	GameplayTags.Attributes_Primary_Strength = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Primary.Strength"), 
		FString("Increases physical damage and slightly increases health."));

	GameplayTags.Attributes_Primary_Intelligence = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Primary.Intelligence"), 
		FString("Increases mana and slightly increases elemental damage."));

	GameplayTags.Attributes_Primary_Dexterity = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Primary.Dexterity"), 
		FString("Increases critical multiplier and slightly increases attack and cast speed."));

	GameplayTags.Attributes_Primary_Endurance = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Primary.Endurance"), 
		FString("Increases stamina and slightly increases resistances."));

	GameplayTags.Attributes_Primary_Affliction = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Primary.Affliction"), 
		FString("Increases DOT (Damage Over Time) and slightly increases duration."));

	GameplayTags.Attributes_Primary_Luck = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Primary.Luck"), 
		FString("Increases chance to apply effects and item drop rate."));

	GameplayTags.Attributes_Primary_Covenant = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Primary.Covenant"), 
		FString("Increases minion health and damage."));

	/* =========================== */
	/* === Secondary Attributes === */
	/* =========================== */

	/** === Defenses === */
	GameplayTags.Attributes_Secondary_Defenses_Armor = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Armor"), 
		FString("Reduces physical damage taken."));

	/** === Health & Regen === */
	GameplayTags.Attributes_Secondary_Vital_MaxHealth = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.MaxHealth"), 
		FString("Maximum amount of health."));

	GameplayTags.Attributes_Secondary_Vital_HealthRegenRate = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.HealthRegenRate"), 
		FString("Determines how fast health regenerates."));

	GameplayTags.Attributes_Secondary_Vital_HealthRegenAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.HealthRegenAmount"), 
		FString("Determines how much health is regenerated per tick."));

	GameplayTags.Attributes_Secondary_Vital_MaxHealthRegenAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.MaxHealthRegenAmount"), 
		FString("Max amount of health that can be regenerated."));

	GameplayTags.Attributes_Secondary_Vital_MaxEffectiveHealth = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.MaxEffectiveHealth"), 
		FString("Max health after reserved health is calculated."));

	/** === Mana & Regen === */
	GameplayTags.Attributes_Secondary_Vital_MaxMana = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.MaxMana"), 
		FString("Maximum amount of mana."));

	GameplayTags.Attributes_Secondary_Vital_ManaRegenRate = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.ManaRegenRate"), 
		FString("Determines how fast mana regenerates."));

	GameplayTags.Attributes_Secondary_Vital_ManaRegenAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.ManaRegenAmount"), 
		FString("Determines how much mana is regenerated per tick."));

	GameplayTags.Attributes_Secondary_Vital_MaxManaRegenAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.MaxManaRegenAmount"), 
		FString("Max amount of mana that can be regenerated."));

	GameplayTags.Attributes_Secondary_Vital_MaxEffectiveMana = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.MaxEffectiveMana"), 
		FString("Max mana after reserved mana is calculated."));

	/** === Stamina & Regen === */
	GameplayTags.Attributes_Secondary_Vital_MaxStamina = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.MaxStamina"), 
		FString("Maximum amount of stamina."));

	GameplayTags.Attributes_Secondary_Vital_MaxEffectiveStamina = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.MaxEffectiveStamina"), 
		FString("Max stamina after reserved stamina is calculated."));

	GameplayTags.Attributes_Secondary_Vital_StaminaRegenRate = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.StaminaRegenRate"), 
		FString("Determines how fast stamina regenerates."));

	GameplayTags.Attributes_Secondary_Vital_StaminaRegenAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.StaminaRegenAmount"), 
		FString("Determines how much stamina is regenerated per tick."));

	GameplayTags.Attributes_Secondary_Vital_MaxStaminaRegenAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.MaxStaminaRegenAmount"), 
		FString("Max amount of stamina that can be regenerated."));

	/* =================== */
	/* === Damage Tags === */
	/* =================== */
	GameplayTags.Attributes_Secondary_Damages_MinPhysicalDamage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.MinPhysicalDamage"), 
		FString("Minimum base physical damage dealt."));

	GameplayTags.Attributes_Secondary_Damages_MaxPhysicalDamage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.MaxPhysicalDamage"), 
		FString("Maximum base physical damage dealt."));

	/* ======================= */
	/* === Resistance Tags === */
	/* ======================= */
	GameplayTags.Attributes_Secondary_Resistances_Armour = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Armour"), 
		FString("Reduces physical damage taken."));

	GameplayTags.Attributes_Secondary_Resistances_GlobalDefenses = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.GlobalDefenses"), 
		FString("Provides general damage mitigation."));

	GameplayTags.Attributes_Secondary_Resistances_BlockStrength = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.BlockStrength"), 
		FString("Determines the effectiveness of blocking attacks."));

	/* ============================= */
	/* === Miscellaneous Gameplay Tags === */
	/* ============================= */
	GameplayTags.Attributes_Secondary_Misc_Poise = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Poise"), 
		FString("Prevents staggering when taking damage until it is depleted."));

	GameplayTags.Attributes_Secondary_Misc_StunRecovery = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.StunRecovery"), 
		FString("Reduces the duration of stuns."));

	GameplayTags.Attributes_Secondary_Misc_MovementSpeed = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.MovementSpeed"), 
		FString("Affects how fast the character moves."));
}

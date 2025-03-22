// Copyright@2024 Quentin Davis 

#include "PHGameplayTags.h"
#include "GameplayTagsManager.h"
#include "AbilitySystem/PHAttributeSet.h"



// === Static Map Definitions ===
TMap<FGameplayTag, FGameplayAttribute> FPHGameplayTags::StatusEffectTagToAttributeMap;
TMap<FGameplayTag, FGameplayTag> FPHGameplayTags::TagsMinMax;
TMap<FString, FGameplayAttribute> FPHGameplayTags::FlatDamageToAttributesMap;
TMap<FString, FGameplayAttribute> FPHGameplayTags::PercentDamageToAttributesMap;
TMap<FString, FGameplayAttribute> FPHGameplayTags::BaseDamageToAttributesMap;
TMap<FString, FGameplayAttribute> FPHGameplayTags::AllAttributesMap;



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



		/* ============================= */
	/* === Ailment / Status Effects === */
	/* ============================= */

	// Status Effect Chances (Connected to Luck / Affliction)
	GameplayTags.Attributes_Secondary_ChanceToApply_ChanceToBleed = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.Bleed"),
		FString("Chance to apply Bleed based on physical damage and Luck."));

	GameplayTags.Attributes_Secondary_ChanceToApply_ChanceToIgnite = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.Ignite"),
		FString("Chance to apply Ignite (Burn) based on fire damage and Luck."));

	GameplayTags.Attributes_Secondary_ChanceToApply_ChanceToFreeze = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.Freeze"),
		FString("Chance to apply Freeze based on ice damage and Luck."));

	GameplayTags.Attributes_Secondary_ChanceToApply_ChanceToShock = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.Shock"),
		FString("Chance to apply Shock based on lightning damage and Luck."));

	GameplayTags.Attributes_Secondary_ChanceToApply_ChanceToStun = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.Stun"),
		FString("Chance to apply Stun based on Dexterity and Affliction."));

	GameplayTags.Attributes_Secondary_ChanceToApply_ChanceToKnockBack = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.KnockBack"),
		FString("Chance to Knock Back the enemy."));

	GameplayTags.Attributes_Secondary_ChanceToApply_ChanceToPetrify = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.Petrify"),
		FString("Chance to apply Petrify effect."));

	GameplayTags.Attributes_Secondary_ChanceToApply_ChanceToPurify = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.Purify"),
		FString("Chance to apply Purification effect (e.g., cleanse debuffs)."));

	GameplayTags.Attributes_Secondary_ChanceToApply_ChanceToCorrupt = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.Corrupt"),
		FString("Chance to apply Corruption-based DOT."));

	// Status Effect Durations (Connected to Affliction / Attribute Duration)
	GameplayTags.Attributes_Secondary_Duration_BleedDuration = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.Duration.Bleed"),
		FString("Duration of Bleed (physical DOT) effects."));

	GameplayTags.Attributes_Secondary_Duration_BurnDuration = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.Duration.Burn"),
		FString("Duration of Burn (fire DOT) effects."));

	GameplayTags.Attributes_Secondary_Duration_FreezeDuration = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.Duration.Freeze"),
		FString("Duration of Freeze (immobilization) effects."));

	GameplayTags.Attributes_Secondary_Duration_ShockDuration = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.Duration.Shock"),
		FString("Duration of Shock (stun/lightning) effects."));

	GameplayTags.Attributes_Secondary_Duration_CorruptionDuration = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.Duration.Corruption"),
		FString("Duration of Corruption (DOT and debuffs)."));

	GameplayTags.Attributes_Secondary_Duration_PetrifyBuildUpDuration = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.Duration.PetrifyBuildUp"),
		FString("Time required to trigger full Petrify."));

	GameplayTags.Attributes_Secondary_Duration_PurifyDuration = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.Duration.Purify"),
		FString("Duration of Purification buff effect."));
}

void FPHGameplayTags::RegisterStatusEffectAttributes()
{
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToBleed, UPHAttributeSet::GetChanceToBleedAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToIgnite, UPHAttributeSet::GetChanceToIgniteAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToFreeze, UPHAttributeSet::GetChanceToFreezeAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToShock, UPHAttributeSet::GetChanceToShockAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToStun, UPHAttributeSet::GetChanceToStunAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToKnockBack, UPHAttributeSet::GetChanceToKnockBackAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToPetrify, UPHAttributeSet::GetChanceToPetrifyAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToPurify, UPHAttributeSet::GetChanceToPurifyAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToCorrupt, UPHAttributeSet::GetChanceToCorruptAttribute());

		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_BleedDuration, UPHAttributeSet::GetBleedDurationAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_BurnDuration, UPHAttributeSet::GetBurnDurationAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_FreezeDuration, UPHAttributeSet::GetFreezeDurationAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_ShockDuration, UPHAttributeSet::GetShockDurationAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_CorruptionDuration, UPHAttributeSet::GetCorruptionDurationAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_PetrifyBuildUpDuration, UPHAttributeSet::GetPetrifyBuildUpDurationAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_PurifyDuration, UPHAttributeSet::GetPurifyDurationAttribute());
}

void FPHGameplayTags::RegisterMinMaxTagMap()
{
	// Adding primary attribute tags
    TagsMinMax.Add(GameplayTags.Attributes_Primary_Strength, GameplayTags.Attributes_Primary_Strength);
    TagsMinMax.Add(GameplayTags.Attributes_Primary_Intelligence, GameplayTags.Attributes_Primary_Intelligence);
    TagsMinMax.Add(GameplayTags.Attributes_Primary_Endurance, GameplayTags.Attributes_Primary_Endurance);
    TagsMinMax.Add(GameplayTags.Attributes_Primary_Affliction, GameplayTags.Attributes_Primary_Affliction);
    TagsMinMax.Add(GameplayTags.Attributes_Primary_Dexterity, GameplayTags.Attributes_Primary_Dexterity);
    TagsMinMax.Add(GameplayTags.Attributes_Primary_Luck, GameplayTags.Attributes_Primary_Luck);
    TagsMinMax.Add(GameplayTags.Attributes_Primary_Covenant, GameplayTags.Attributes_Primary_Covenant);

    // Adding secondary attribute tags
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_HealthReservedAmount, GameplayTags.Attributes_Secondary_Vital_MaxHealthReservedAmount);
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_HealthRegenRate, GameplayTags.Attributes_Secondary_Vital_MaxHealthRegenRate);
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_HealthRegenAmount, GameplayTags.Attributes_Secondary_Vital_MaxHealthRegenAmount);
    
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_ManaRegenRate, GameplayTags.Attributes_Secondary_Vital_MaxManaRegenRate);
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_ManaRegenAmount, GameplayTags.Attributes_Secondary_Vital_MaxManaRegenAmount);
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_ManaReservedAmount, GameplayTags.Attributes_Secondary_Vital_MaxManaReservedAmount);
    
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_StaminaRegenRate, GameplayTags.Attributes_Secondary_Vital_MaxStaminaRegenRate);
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_StaminaRegenAmount, GameplayTags.Attributes_Secondary_Vital_MaxStaminaRegenAmount);
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_StaminaReservedAmount, GameplayTags.Attributes_Secondary_Vital_MaxStaminaReservedAmount);
    
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_ArcaneShieldRegenRate, GameplayTags.Attributes_Secondary_Vital_MaxArcaneShieldRegenRate);
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_ArcaneShieldRegenAmount, GameplayTags.Attributes_Secondary_Vital_MaxArcaneShieldRegenAmount);
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_ArcaneShieldReservedAmount, GameplayTags.Attributes_Secondary_Vital_MaxArcaneShieldReservedAmount);

}


void FPHGameplayTags::RegisterFlatDamageAttributes()
{
	FlatDamageToAttributesMap.Add("Physical", UPHAttributeSet::GetPhysicalFlatBonusAttribute());
	FlatDamageToAttributesMap.Add("Fire", UPHAttributeSet::GetFireFlatBonusAttribute());
	FlatDamageToAttributesMap.Add("Ice", UPHAttributeSet::GetIceFlatBonusAttribute());
	FlatDamageToAttributesMap.Add("Lightning", UPHAttributeSet::GetLightningFlatBonusAttribute());
	FlatDamageToAttributesMap.Add("Light", UPHAttributeSet::GetLightFlatBonusAttribute());
	FlatDamageToAttributesMap.Add("Corruption", UPHAttributeSet::GetCorruptionFlatBonusAttribute());
}


void FPHGameplayTags::RegisterPercentDamageAttributes()
{
	PercentDamageToAttributesMap.Add("Physical", UPHAttributeSet::GetPhysicalPercentBonusAttribute());
	PercentDamageToAttributesMap.Add("Fire", UPHAttributeSet::GetFirePercentBonusAttribute());
	PercentDamageToAttributesMap.Add("Ice", UPHAttributeSet::GetIcePercentBonusAttribute());
	PercentDamageToAttributesMap.Add("Lightning", UPHAttributeSet::GetLightningPercentBonusAttribute());
	PercentDamageToAttributesMap.Add("Light", UPHAttributeSet::GetLightPercentBonusAttribute());
	PercentDamageToAttributesMap.Add("Corruption", UPHAttributeSet::GetCorruptionPercentBonusAttribute());
}

void FPHGameplayTags::RegisterBaseDamageAttributes()
{
	BaseDamageToAttributesMap.Add("Min Physical", UPHAttributeSet::GetMinPhysicalDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Fire", UPHAttributeSet::GetMinFireDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Ice", UPHAttributeSet::GetMinIceDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Lightning", UPHAttributeSet::GetMinLightningDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Light", UPHAttributeSet::GetMinLightDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Corruption", UPHAttributeSet::GetMinCorruptionDamageAttribute());

	BaseDamageToAttributesMap.Add("Max Physical", UPHAttributeSet::GetMaxPhysicalDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Fire", UPHAttributeSet::GetMaxFireDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Ice", UPHAttributeSet::GetMaxIceDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Lightning", UPHAttributeSet::GetMaxLightningDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Light", UPHAttributeSet::GetMaxLightDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Corruption", UPHAttributeSet::GetMaxCorruptionDamageAttribute());
}

void FPHGameplayTags::RegisterAllAttributes()
{
	AllAttributesMap.Append(FlatDamageToAttributesMap);
	AllAttributesMap.Append(PercentDamageToAttributesMap);
	AllAttributesMap.Append(BaseDamageToAttributesMap);

	AllAttributesMap.Add("Strength", UPHAttributeSet::GetStrengthAttribute());
	AllAttributesMap.Add("Intelligence", UPHAttributeSet::GetIntelligenceAttribute());
	AllAttributesMap.Add("Dexterity", UPHAttributeSet::GetDexterityAttribute());
	AllAttributesMap.Add("Endurance", UPHAttributeSet::GetEnduranceAttribute());
	AllAttributesMap.Add("Affliction", UPHAttributeSet::GetAfflictionAttribute());
	AllAttributesMap.Add("Luck", UPHAttributeSet::GetLuckAttribute());
	AllAttributesMap.Add("Covenant", UPHAttributeSet::GetCovenantAttribute());

	// You can add other global or rare attributes here too
}


void FPHGameplayTags::RegisterAllAttribute()
{
	RegisterStatusEffectAttributes();
	RegisterMinMaxTagMap();
	RegisterFlatDamageAttributes();
	RegisterPercentDamageAttributes();
	RegisterBaseDamageAttributes();
	RegisterAllAttributes();
}


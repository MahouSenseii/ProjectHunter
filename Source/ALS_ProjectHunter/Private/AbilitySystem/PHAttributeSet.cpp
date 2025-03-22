// Copyright@2024 Quentin Davis 


#include "AbilitySystem/PHAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Library/PHCharacterEnumLibrary.h"
#include "PHGameplayTags.h"
#include "Net/UnrealNetwork.h"

UPHAttributeSet::UPHAttributeSet()
{
	const FPHGameplayTags& GameplayTags = FPHGameplayTags::Get();
	if(TagsToAttributes.IsEmpty())
	{
		// Combat Alignment
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Misc_CombatAlignment, GetCombatAlignmentAttribute); 

		// Primary Attributes
		TagsToAttributes.Add(GameplayTags.Attributes_Primary_Strength, GetStrengthAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Primary_Intelligence, GetIntelligenceAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Primary_Endurance, GetEnduranceAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Primary_Affliction, GetAfflictionAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Primary_Dexterity, GetDexterityAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Primary_Luck, GetLuckAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Primary_Covenant, GetCovenantAttribute);

		// Secondary Vital Attributes
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_MaxHealth, GetMaxHealthAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_MaxEffectiveHealth, GetMaxEffectiveHealthAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_MaxStamina, GetMaxStaminaAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_MaxEffectiveStamina, GetMaxEffectiveStaminaAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_MaxMana, GetMaxManaAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_MaxEffectiveMana, GetMaxEffectiveManaAttribute);

		TagsToAttributes.Add(GameplayTags.Attributes_Vital_Health, GetHealthAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Vital_Stamina, GetStaminaAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Vital_Mana, GetManaAttribute);

		// Regeneration Attributes
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_HealthRegenRate, GetHealthRegenRateAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_HealthRegenAmount, GetHealthRegenAmountAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_MaxHealthRegenRate, GetMaxHealthRegenRateAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_MaxHealthRegenAmount, GetMaxHealthRegenAmountAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_HealthReservedAmount, GetReservedHealthAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_MaxHealthReservedAmount, GetMaxReservedHealthAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_HealthFlatReservedAmount, GetFlatReservedHealthAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_HealthPercentageReserved, GetPercentageReservedHealthAttribute);

		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_ManaRegenRate, GetManaRegenRateAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_ManaRegenAmount, GetManaRegenAmountAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_MaxManaRegenRate, GetMaxManaRegenRateAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_MaxManaRegenAmount, GetMaxManaRegenAmountAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_ManaReservedAmount, GetReservedManaAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_MaxManaReservedAmount, GetMaxReservedManaAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_ManaFlatReservedAmount, GetFlatReservedManaAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_ManaPercentageReserved, GetPercentageReservedManaAttribute);

		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_StaminaRegenRate, GetStaminaRegenRateAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_StaminaRegenAmount, GetStaminaRegenAmountAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_MaxStaminaRegenRate, GetMaxStaminaRegenRateAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_MaxStaminaRegenAmount, GetMaxStaminaRegenAmountAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_StaminaReservedAmount, GetReservedStaminaAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_MaxStaminaReservedAmount, GetMaxReservedStaminaAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_StaminaFlatReservedAmount, GetFlatReservedStaminaAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Vital_StaminaPercentageReserved, GetPercentageReservedStaminaAttribute);

		// Damage Attributes

		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Damages_MinPhysicalDamage, GetMinPhysicalDamageAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Damages_MinFireDamage, GetMinFireDamageAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Damages_MinIceDamage, GetMinIceDamageAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Damages_MinLightningDamage, GetMinLightningDamageAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Damages_MinLightDamage, GetMinLightDamageAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Damages_MinCorruptionDamage, GetMinCorruptionDamageAttribute);

		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Damages_MaxPhysicalDamage, GetMaxPhysicalDamageAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Damages_MaxFireDamage, GetMaxFireDamageAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Damages_MaxIceDamage, GetMaxIceDamageAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Damages_MaxLightningDamage, GetMaxLightningDamageAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Damages_MaxLightDamage, GetMaxLightDamageAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Damages_MaxCorruptionDamage, GetMaxCorruptionDamageAttribute);

		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_BonusDamage_GlobalDamages, GetGlobalDamagesAttribute);

		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_BonusDamage_PhysicalFlatBonus, GetPhysicalFlatBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_BonusDamage_FireFlatBonus, GetFireFlatBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_BonusDamage_IceFlatBonus, GetIceFlatBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_BonusDamage_LightningFlatBonus, GetLightningFlatBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_BonusDamage_LightFlatBonus, GetLightFlatBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_BonusDamage_CorruptionFlatBonus, GetCorruptionFlatBonusAttribute);

		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_BonusDamage_PhysicalPercentBonus, GetPhysicalPercentBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_BonusDamage_FirePercentBonus, GetFirePercentBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_BonusDamage_IcePercentBonus, GetIcePercentBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_BonusDamage_LightningPercentBonus, GetLightningPercentBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_BonusDamage_LightPercentBonus, GetLightPercentBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_BonusDamage_CorruptionPercentBonus, GetCorruptionPercentBonusAttribute);

		// Misc Attributes
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Misc_Poise, GetPoiseAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Misc_StunRecovery, GetStunRecoveryAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Misc_ManaCostChanges, GetManaCostChangesAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Misc_LifeOnHit, GetLifeOnHitAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Misc_ManaOnHit, GetManaOnHitAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Misc_StaminaOnHit, GetStaminaOnHitAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Misc_StaminaCostChanges, GetStaminaCostChangesAttribute);

		// Resistances
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Resistances_GlobalDefenses, GetGlobalDefensesAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Resistances_Armour, GetArmourAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Resistances_FireResistanceFlat, GetFireResistanceFlatBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Resistances_LightResistanceFlat, GetLightResistanceFlatBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Resistances_LightningResistanceFlat, GetLightningResistanceFlatBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Resistances_CorruptionResistanceFlat, GetCorruptionResistanceFlatBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Resistances_IceResistanceFlat, GetIceResistanceFlatBonusAttribute);


		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Resistances_FireResistancePercentage, GetFireResistancePercentBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Resistances_LightResistancePercentage, GetLightResistancePercentBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Resistances_LightningResistancePercentage, GetLightningResistancePercentBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Resistances_CorruptionResistancePercentage, GetCorruptionResistancePercentBonusAttribute);
		TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Resistances_IceResistancePercentage, GetIceResistancePercentBonusAttribute);
	}



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

	BaseDamageAttributesMap.Add("Min Physical", GetMinPhysicalDamageAttribute());
	BaseDamageAttributesMap.Add("Min Fire", GetMinFireDamageAttribute());
	BaseDamageAttributesMap.Add("Min Ice", GetMinIceDamageAttribute());
	BaseDamageAttributesMap.Add("Min Lightning", GetMinLightningDamageAttribute());
	BaseDamageAttributesMap.Add("Min Light", GetMinLightDamageAttribute());
	BaseDamageAttributesMap.Add("Min Corruption", GetMinCorruptionDamageAttribute());

	BaseDamageAttributesMap.Add("Max Physical", GetMaxPhysicalDamageAttribute());
	BaseDamageAttributesMap.Add("Max Fire", GetMaxFireDamageAttribute());
	BaseDamageAttributesMap.Add("Max Ice", GetMaxIceDamageAttribute());
	BaseDamageAttributesMap.Add("Max Lightning", GetMaxLightningDamageAttribute());
	BaseDamageAttributesMap.Add("Max Light", GetMaxLightDamageAttribute());
	BaseDamageAttributesMap.Add("Max Corruption", GetMaxCorruptionDamageAttribute());

	FlatDamageAttributesMap.Add("Physical Flat Damage ", GetPhysicalFlatBonusAttribute());
	FlatDamageAttributesMap.Add("Fire Flat Damage", GetFireFlatBonusAttribute());
	FlatDamageAttributesMap.Add("Ice Flat Damage", GetIceFlatBonusAttribute());
	FlatDamageAttributesMap.Add("Lightning Flat Damage", GetLightningFlatBonusAttribute());
	FlatDamageAttributesMap.Add("Light Flat Damage", GetLightFlatBonusAttribute());
	FlatDamageAttributesMap.Add("Corruption Flat Damage", GetCorruptionFlatBonusAttribute());

	PercentDamageAttributesMap.Add("Physical Percent Damage", GetPhysicalPercentBonusAttribute());
	PercentDamageAttributesMap.Add("Fire Percent Damage", GetFirePercentBonusAttribute());
	PercentDamageAttributesMap.Add("Ice Percent Damage", GetIcePercentBonusAttribute());
	PercentDamageAttributesMap.Add("Lightning Percent Damage", GetLightningPercentBonusAttribute());
	PercentDamageAttributesMap.Add("Light Percent Damage", GetLightPercentBonusAttribute());
	PercentDamageAttributesMap.Add("Corruption Percent Damage", GetCorruptionPercentBonusAttribute());
{
    // Primary Attributes
    AllAttributesMap.Add("Strength", GetStrengthAttribute());
    AllAttributesMap.Add("Intelligence", GetIntelligenceAttribute());
    AllAttributesMap.Add("Dexterity", GetDexterityAttribute());
    AllAttributesMap.Add("Endurance", GetEnduranceAttribute());
    AllAttributesMap.Add("Affliction", GetAfflictionAttribute());
    AllAttributesMap.Add("Luck", GetLuckAttribute());
    AllAttributesMap.Add("Covenant", GetCovenantAttribute());

    // Min Damage Attributes
    AllAttributesMap.Add("Min Physical Damage", GetMinPhysicalDamageAttribute());
    AllAttributesMap.Add("Min Fire Damage", GetMinFireDamageAttribute());
    AllAttributesMap.Add("Min Ice Damage", GetMinIceDamageAttribute());
    AllAttributesMap.Add("Min Lightning Damage", GetMinLightningDamageAttribute());
    AllAttributesMap.Add("Min Light Damage", GetMinLightDamageAttribute());


	// Max Damage Attributes
    AllAttributesMap.Add("Min Physical Damage", GetMinPhysicalDamageAttribute());
    AllAttributesMap.Add("Min Fire Damage", GetMinFireDamageAttribute());
    AllAttributesMap.Add("Min Ice Damage", GetMinIceDamageAttribute());
    AllAttributesMap.Add("Min Lightning Damage", GetMinLightningDamageAttribute());
    AllAttributesMap.Add("Min Light Damage", GetMinLightDamageAttribute());

    // Flat Bonuses
    AllAttributesMap.Add("Physical Flat Bonus", GetPhysicalFlatBonusAttribute());
    AllAttributesMap.Add("Fire Flat Bonus", GetFireFlatBonusAttribute());
    AllAttributesMap.Add("Ice Flat Bonus", GetIceFlatBonusAttribute());
    AllAttributesMap.Add("Lightning Flat Bonus", GetLightningFlatBonusAttribute());
    AllAttributesMap.Add("Light Flat Bonus", GetLightFlatBonusAttribute());
    AllAttributesMap.Add("Corruption Flat Bonus", GetCorruptionFlatBonusAttribute());

    // Percent Bonuses
    AllAttributesMap.Add("Physical Percent Bonus", GetPhysicalPercentBonusAttribute());
    AllAttributesMap.Add("Fire Percent Bonus", GetFirePercentBonusAttribute());
    AllAttributesMap.Add("Ice Percent Bonus", GetIcePercentBonusAttribute());
    AllAttributesMap.Add("Lightning Percent Bonus", GetLightningPercentBonusAttribute());
    AllAttributesMap.Add("Light Percent Bonus", GetLightPercentBonusAttribute());
    AllAttributesMap.Add("Corruption Percent Bonus", GetCorruptionPercentBonusAttribute());

    // Resistances
    AllAttributesMap.Add("Flat Fire Resistance", GetFireResistanceFlatBonusAttribute());
    AllAttributesMap.Add("Flat Ice Resistance", GetIceResistanceFlatBonusAttribute());
    AllAttributesMap.Add("Flat Lightning Resistance", GetLightningResistanceFlatBonusAttribute());
    AllAttributesMap.Add("Flat Corruption Resistance", GetCorruptionResistanceFlatBonusAttribute());

    // Misc Attributes
    AllAttributesMap.Add("Poise", GetPoiseAttribute());
    AllAttributesMap.Add("Stun Recovery", GetStunRecoveryAttribute());
    AllAttributesMap.Add("Mana Cost Changes", GetManaCostChangesAttribute());
    AllAttributesMap.Add("Life On Hit", GetLifeOnHitAttribute());
    AllAttributesMap.Add("Mana On Hit", GetManaOnHitAttribute());
    AllAttributesMap.Add("Stamina On Hit", GetStaminaOnHitAttribute());
}

	
}

void UPHAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Indicators
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CombatAlignment, COND_None, REPNOTIFY_Always);

	// Primary Attribute
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Intelligence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Dexterity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Endurance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Affliction, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Luck, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Covenant, COND_None, REPNOTIFY_Always);

	//Secondary  Max Attribute

	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxEffectiveHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxEffectiveStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxEffectiveMana, COND_None, REPNOTIFY_Always);

	// Regeneration 
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, HealthRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, HealthRegenAmount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, HealthRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ReservedHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxReservedHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FlatReservedHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PercentageReservedHealth, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaRegenAmount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ReservedMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxReservedMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FlatReservedMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PercentageReservedMana, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaRegenAmount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ReservedStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxReservedStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FlatReservedStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PercentageReservedStamina, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ArcaneShieldRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet,ArcaneShieldRegenAmount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ArcaneShieldRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ReservedArcaneShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxReservedArcaneShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FlatReservedArcaneShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PercentageReservedArcaneShield, COND_None, REPNOTIFY_Always);

	//Damages
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, GlobalDamages, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinPhysicalDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinFireDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinLightDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinLightningDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinCorruptionDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinIceDamage, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxPhysicalDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxFireDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxLightDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxLightningDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxCorruptionDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxIceDamage, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PhysicalFlatBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FireFlatBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightFlatBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningFlatBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionFlatBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, IceFlatBonus, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PhysicalPercentBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FirePercentBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightPercentBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningPercentBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionPercentBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, IcePercentBonus, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, DamageBonusWhileAtFullHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, DamageBonusWhileAtLowHP, COND_None, REPNOTIFY_Always);

	// Other Offensive Stats
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, AreaDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, AreaOfEffect, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, AttackRange, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CastSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CritMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, DamageOverTime, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ElementalDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, SpellsCritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, SpellsCritMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MeleeDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ProjectileCount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ProjectileSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet,  RangedDamage, COND_None, REPNOTIFY_Always);


	//Duration
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, BurnDuration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, BleedDuration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FreezeDuration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionDuration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ShockDuration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet,PetrifyBuildUpDuration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet,  PurifyDuration, COND_None, REPNOTIFY_Always);

	

	//Resistances
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, GlobalDefenses, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Armour, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ArmourFlatBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ArmourPercentBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FireResistanceFlatBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightResistanceFlatBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningResistanceFlatBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionResistanceFlatBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, IceResistanceFlatBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet,  BlockStrength, COND_None, REPNOTIFY_Always);

	
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxFireResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxLightResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxLightningResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxCorruptionResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet,  MaxIceResistance, COND_None, REPNOTIFY_Always);



	//Piercing
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ArmourPiercing, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FirePiercing, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningPiercing, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningPiercing, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionPiercing, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet,  IcePiercing, COND_None, REPNOTIFY_Always);

	//Chance to apply
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToBleed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToCorrupt, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToFreeze, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToIgnite, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToPetrify, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToPurify, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToShock, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToStun, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToKnockBack, COND_None, REPNOTIFY_Always);

	//Misc
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ComboCounter, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CoolDown, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LifeLeech, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaLeech, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MovementSpeed, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Poise, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StunRecovery, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaCostChanges, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LifeOnHit, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaOnHit, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaOnHit, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaCostChanges, COND_None, REPNOTIFY_Always);

	//Secondary Current Attribute
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Stamina, COND_None, REPNOTIFY_Always);


	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Gems, COND_None, REPNOTIFY_Always);
	
}

float UPHAttributeSet::GetAttributeValue(const FGameplayAttribute& Attribute) const 
{
	if (const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
	{
		return ASC->GetNumericAttribute(Attribute);
	}
	return 0.0f;
}

float UPHAttributeSet::GetAttributeValue(FGameplayAttribute& Attribute) const
{
	// Get the Ability System Component that owns this Attribute Set
	if (const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
	{
		return ASC->GetNumericAttribute(Attribute);
	}
    
	return 0.0f; // Default return value
}

// for use in BP will show enum not float values 
ECombatAlignment UPHAttributeSet::GetCombatAlignmentBP() const
{
	// Convert the float value stored in CombatAlignment to the ECombatAlignment enum
	return static_cast<ECombatAlignment>(CombatAlignment.GetCurrentValue());
}

// for use in BP will show enum not float values 
void UPHAttributeSet::SetCombatAlignmentBP(ECombatAlignment NewAlignment)
{
	// Set the CombatAlignment value using the float representation of the enum
	SetCombatAlignment(static_cast<float>(NewAlignment));

	// Broadcast the change
	OnCombatAlignmentChange.Broadcast(static_cast<float>(NewAlignment));
}

void UPHAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Handle health attribute changes.
	if(Attribute == GetHealthAttribute())
	{
		{
			NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxEffectiveHealth());
		}
	}
	else if(Attribute == GetManaAttribute())
	{
		
		NewValue = FMath::Clamp(NewValue, 0.0f,GetMaxMana());
	}
	else if(Attribute == GetStaminaAttribute())
	{
		
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
}

void UPHAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	FEffectProperties Props;
	SetEffectProperties(Data, Props);

	const FGameplayAttribute Attribute = Data.EvaluatedData.Attribute;
	
	if(Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxEffectiveHealth()));
		SetMaxEffectiveHealth(GetMaxEffectiveHealth());
		SetMaxHealth(GetMaxHealth());
		SetReservedHealth(GetReservedHealth());
	}
	if(Attribute == GetManaAttribute())
	{
		SetMana(  FMath::Clamp(GetMana(), 0.0f, GetMaxMana()));
		SetMaxEffectiveMana(GetMaxEffectiveMana());
		SetMaxMana(GetMaxMana());
		SetReservedMana(GetReservedMana());
	}
	if(Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
		SetMaxEffectiveStamina(GetMaxEffectiveStamina());
		SetMaxStamina(GetMaxStamina());
		SetReservedStamina(GetReservedStamina());
	}

}




void UPHAttributeSet::SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props)
{
	Props.EffectContextHandle = Data.EffectSpec.GetContext();
	Props.SourceAsc = Props.EffectContextHandle.GetOriginalInstigatorAbilitySystemComponent();

	if(IsValid(Props.SourceAsc) && Props.SourceAsc->AbilityActorInfo.IsValid() && Props.SourceAsc->AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.SourceAvatarActor = Props.SourceAsc->AbilityActorInfo->AvatarActor.Get();
		Props.SourceController = Props.SourceAsc->AbilityActorInfo->PlayerController.Get();
		if(Props.SourceController == nullptr && Props.SourceAvatarActor == nullptr)
		{
			if(const APawn* Pawn = Cast<APawn>(Props.SourceAvatarActor))
			{
				Props.SourceAvatarActor = Pawn->GetController();
			}
		}
		if(Props.SourceController)
		{
			Props.SourceCharacter = Cast<ACharacter>(Props.SourceController->GetPawn());
		}
	}

	if(Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.TargetAvatarActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		Props.TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		Props.TargetCharacter = Cast<ACharacter>(Props.TargetAvatarActor);
		Props.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Props.TargetAvatarActor);
	}
}

//
void UPHAttributeSet::OnRep_CombatAlignment(const FGameplayAttributeData& OldCombatAlignment) const
{
	// Notify listeners of the attribute change
	OnCombatAlignmentChange.Broadcast(GetCombatAlignment());
	
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet, CombatAlignment, OldCombatAlignment);
}

//Health
void UPHAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Health, OldHealth)
}

void UPHAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxHealth, OldMaxHealth)
}

void UPHAttributeSet::OnRep_MaxEffectiveHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxEffectiveHealth, OldAmount);
}

void UPHAttributeSet::OnRep_HealthRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,HealthRegenRate, OldAmount);
}

void UPHAttributeSet::OnRep_MaxHealthRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxHealthRegenRate, OldAmount);
}

void UPHAttributeSet::OnRep_HealthRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,HealthRegenAmount, OldAmount);
}


void UPHAttributeSet::OnRep_MaxHealthRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxHealthRegenAmount, OldAmount);
}

void UPHAttributeSet::OnRep_ReservedHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ReservedHealth, OldAmount);
}

void UPHAttributeSet::OnRep_MaxReservedHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxReservedHealth, OldAmount);
}

void UPHAttributeSet::OnRep_FlatReservedHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , FlatReservedHealth, OldAmount);
}

void UPHAttributeSet::OnRep_PercentageReservedHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , PercentageReservedHealth, OldAmount);
}

//Health End

//Stamina 
void UPHAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , Stamina,OldStamina)
}

void UPHAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxStamina, OldMaxStamina)
}

void UPHAttributeSet::OnRep_MaxEffectiveStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxEffectiveStamina, OldAmount)

}

void UPHAttributeSet::OnRep_StaminaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaRegenRate, OldAmount)
}

void UPHAttributeSet::OnRep_MaxStaminaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxStaminaRegenRate, OldAmount)
}

void UPHAttributeSet::OnRep_StaminaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaRegenAmount, OldAmount)
}

void UPHAttributeSet::OnRep_MaxStaminaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxStaminaRegenAmount,OldAmount)
}

void UPHAttributeSet::OnRep_ReservedStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ReservedStamina, OldAmount)
}

void UPHAttributeSet::OnRep_MaxReservedStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxReservedStamina, OldAmount)
}


void UPHAttributeSet::OnRep_FlatReservedStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FlatReservedStamina, OldAmount)
}

void UPHAttributeSet::OnRep_PercentageReservedStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PercentageReservedStamina, OldAmount)
}

//Stamina End

//Mana
void UPHAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Mana, OldMana)
}

void UPHAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxMana, OldMaxMana)

}

void UPHAttributeSet::OnRep_MaxEffectiveMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxEffectiveMana, OldAmount)
}

void UPHAttributeSet::OnRep_ManaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ManaRegenRate,OldAmount)
}

void UPHAttributeSet::OnRep_MaxManaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxManaRegenRate,OldAmount)
}

void UPHAttributeSet::OnRep_ManaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ManaRegenAmount,OldAmount)
}

void UPHAttributeSet::OnRep_MaxManaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxManaRegenAmount,OldAmount)
}

void UPHAttributeSet::OnRep_ReservedMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ReservedMana,OldAmount)
}

void UPHAttributeSet::OnRep_MaxReservedMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxReservedMana,OldAmount)
}

void UPHAttributeSet::OnRep_FlatReservedMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , FlatReservedMana,OldAmount)
}

void UPHAttributeSet::OnRep_PercentageReservedMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , PercentageReservedMana,OldAmount)
}

void UPHAttributeSet::OnRep_ArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , ArcaneShield ,OldAmount)
}

void UPHAttributeSet::OnRep_MaxArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , MaxArcaneShield ,OldAmount)
}

void UPHAttributeSet::OnRep_MaxEffectiveArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , MaxEffectiveArcaneShield ,OldAmount)
}

void UPHAttributeSet::OnRep_ArcaneShieldRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , ArcaneShieldRegenRate ,OldAmount)
}

void UPHAttributeSet::OnRep_MaxArcaneShieldRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , MaxArcaneShieldRegenRate ,OldAmount)
}

void UPHAttributeSet::OnRep_ArcaneShieldRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , ArcaneShieldRegenAmount ,OldAmount)
}

void UPHAttributeSet::OnRep_MaxArcaneShieldRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , MaxArcaneShieldRegenAmount ,OldAmount)
}

void UPHAttributeSet::OnRep_ReservedArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , ReservedArcaneShield ,OldAmount)
}

void UPHAttributeSet::OnRep_MaxReservedArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , MaxReservedArcaneShield ,OldAmount)
}

void UPHAttributeSet::OnRep_FlatReservedArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , FlatReservedArcaneShield ,OldAmount)
}

//Mana End

void UPHAttributeSet::OnRep_PercentageReservedArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , PercentageReservedArcaneShield ,OldAmount)
}

//Gems
void UPHAttributeSet::OnRep_Gems(const FGameplayAttributeData& OldGems) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Gems, OldGems)
}

void UPHAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Strength, OldAmount)
}

void UPHAttributeSet::OnRep_Intelligence(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Intelligence, OldAmount)
}

void UPHAttributeSet::OnRep_Dexterity(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Dexterity, OldAmount)
}

void UPHAttributeSet::OnRep_Endurance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Endurance, OldAmount)
}

void UPHAttributeSet::OnRep_Affliction(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Affliction, OldAmount)
}

void UPHAttributeSet::OnRep_Luck(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Luck, OldAmount)
}

void UPHAttributeSet::OnRep_Covenant(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Covenant, OldAmount)
}

void UPHAttributeSet::OnRep_GlobalDamages(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,GlobalDamages, OldAmount)
}

void UPHAttributeSet::OnRep_MinCorruptionDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinCorruptionDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxCorruptionDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxCorruptionDamage, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionPercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionPercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_MinFireDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinFireDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxFireDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxFireDamage, OldAmount)
}

void UPHAttributeSet::OnRep_FireFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FireFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_FirePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FirePercentBonus, OldAmount)
}


void UPHAttributeSet::OnRep_MinIceDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinIceDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxIceDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxIceDamage, OldAmount)
}

void UPHAttributeSet::OnRep_IceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IceFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_IcePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IcePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_MinLightDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinLightDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxLightDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxLightDamage, OldAmount)
}

void UPHAttributeSet::OnRep_LightFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_LightPercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightPercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_MinLightningDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinLightningDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxLightningDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxLightningDamage, OldAmount)
}

void UPHAttributeSet::OnRep_LightningFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_LightningPercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningPercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_MinPhysicalDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinPhysicalDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxPhysicalDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxPhysicalDamage, OldAmount)
}

void UPHAttributeSet::OnRep_PhysicalFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PhysicalFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_PhysicalPercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PhysicalPercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_AreaDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,AreaDamage, OldAmount)
}

void UPHAttributeSet::OnRep_AreaOfEffect(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,AreaOfEffect, OldAmount)
}

void UPHAttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,AttackRange, OldAmount)
}

void UPHAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,AttackSpeed, OldAmount)
}

void UPHAttributeSet::OnRep_CastSpeed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CastSpeed, OldAmount)
}

void UPHAttributeSet::OnRep_CritChance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CritChance, OldAmount)
}

void UPHAttributeSet::OnRep_CritMultiplier(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CritMultiplier, OldAmount)
}

void UPHAttributeSet::OnRep_DamageOverTime(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,DamageOverTime, OldAmount)
}

void UPHAttributeSet::OnRep_ElementalDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ElementalDamage, OldAmount)
}

void UPHAttributeSet::OnRep_SpellsCritChance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,SpellsCritChance, OldAmount)
}

void UPHAttributeSet::OnRep_SpellsCritMultiplier(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,SpellsCritMultiplier, OldAmount)
}

void UPHAttributeSet::OnRep_DamageBonusWhileAtFullHP(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,DamageBonusWhileAtFullHP, OldAmount)
}

void UPHAttributeSet::OnRep_MaxCorruptionResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxCorruptionResistance, OldAmount)
}

void UPHAttributeSet::OnRep_ArmourPiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ArmourPiercing, OldAmount)
}

void UPHAttributeSet::OnRep_FirePiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , FirePiercing, OldAmount)
}

void UPHAttributeSet::OnRep_LightPiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightPiercing, OldAmount)
}

void UPHAttributeSet::OnRep_LightningPiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningPiercing, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionPiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionPiercing, OldAmount)
}

void UPHAttributeSet::OnRep_IcePiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IcePiercing, OldAmount)
}

void UPHAttributeSet::OnRep_MaxFireResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxFireResistance, OldAmount)
}

void UPHAttributeSet::OnRep_MaxIceResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxIceResistance, OldAmount)
}

void UPHAttributeSet::OnRep_MaxLightResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxLightResistance, OldAmount)
}

void UPHAttributeSet::OnRep_MaxLightningResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxLightningResistance, OldAmount)
}

void UPHAttributeSet::OnRep_DamageBonusWhileAtLowHP(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,DamageBonusWhileAtLowHP, OldAmount)
}

void UPHAttributeSet::OnRep_MeleeDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MeleeDamage, OldAmount)
}

void UPHAttributeSet::OnRep_SpellDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , SpellDamage, OldAmount);
}

void UPHAttributeSet::OnRep_ProjectileCount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ProjectileCount, OldAmount)
}

void UPHAttributeSet::OnRep_ProjectileSpeed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ProjectileSpeed, OldAmount)
}

void UPHAttributeSet::OnRep_RangedDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,RangedDamage, OldAmount)
}

void UPHAttributeSet::OnRep_GlobalDefenses(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,GlobalDefenses, OldAmount)
}

void UPHAttributeSet::OnRep_Armour(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Armour, OldAmount)
}

void UPHAttributeSet::OnRep_ArmourFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ArmourFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_ArmourPercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ArmourPercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionResistanceFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_FireResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FireResistanceFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_IceResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IceResistanceFlatBonus, OldAmount)
}


void UPHAttributeSet::OnRep_LightResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightResistanceFlatBonus, OldAmount)
}



void UPHAttributeSet::OnRep_LightningResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningResistanceFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionResistancePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_FireResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FireResistancePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_IceResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IceResistancePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_LightResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightResistancePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_LightningResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningResistancePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_BlockStrength(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,BlockStrength, OldAmount)
}

void UPHAttributeSet::OnRep_ComboCounter(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ComboCounter, OldAmount)
}

void UPHAttributeSet::OnRep_CoolDown(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CoolDown, OldAmount)
}

void UPHAttributeSet::OnRep_LifeLeech(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LifeLeech, OldAmount)
}

void UPHAttributeSet::OnRep_ManaLeech(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ManaLeech, OldAmount)
}

void UPHAttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MovementSpeed, OldAmount)
}

void UPHAttributeSet::OnRep_Poise(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Poise, OldAmount)
}

void UPHAttributeSet::OnRep_PoiseResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PoiseResistance, OldAmount)
}

void UPHAttributeSet::OnRep_StunRecovery(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StunRecovery, OldAmount)
}

void UPHAttributeSet::OnRep_ManaCostChanges(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ManaCostChanges, OldAmount)
}

void UPHAttributeSet::OnRep_LifeOnHit(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LifeOnHit, OldAmount)
}

void UPHAttributeSet::OnRep_ManaOnHit(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ManaOnHit, OldAmount)
}

void UPHAttributeSet::OnRep_StaminaOnHit(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaOnHit, OldAmount)
}

void UPHAttributeSet::OnRep_StaminaCostChanges(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaCostChanges, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToBleed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToBleed, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToCorrupt(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToCorrupt, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToFreeze(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToFreeze, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToIgnite(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToIgnite, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToKnockBack(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToKnockBack, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToPetrify(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToPetrify, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToShock(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToShock, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToStun(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToStun, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToPurify(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToPurify, OldAmount)
}

void UPHAttributeSet::OnRep_BurnDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,BurnDuration, OldAmount)
}

void UPHAttributeSet::OnRep_BleedDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,BleedDuration, OldAmount)
}

void UPHAttributeSet::OnRep_FreezeDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FreezeDuration, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionDuration, OldAmount)
}

void UPHAttributeSet::OnRep_ShockDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ShockDuration, OldAmount)
}

void UPHAttributeSet::OnRep_PetrifyBuildUpDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PetrifyBuildUpDuration, OldAmount)
}

void UPHAttributeSet::OnRep_PurifyDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PurifyDuration, OldAmount)
}


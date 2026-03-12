// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/BaseStatsData.h"
#include "AbilitySystem/HunterAttributeSet.h"

/* ═══════════════════════════════════════════════════════════════════════ */
/* ENUM TO ATTRIBUTE NAME CONVERTER */
/* ═══════════════════════════════════════════════════════════════════════ */

FName FHunterAttributeHelper::GetAttributeName(EHunterAttribute Attribute)
{
	switch (Attribute)
	{
		// Primary Attributes
		case EHunterAttribute::Strength:            return "Strength";
		case EHunterAttribute::Intelligence:        return "Intelligence";
		case EHunterAttribute::Dexterity:           return "Dexterity";
		case EHunterAttribute::Endurance:           return "Endurance";
		case EHunterAttribute::Affliction:          return "Affliction";
		case EHunterAttribute::Luck:                return "Luck";
		case EHunterAttribute::Covenant:            return "Covenant";

		// Vitals - Max
		case EHunterAttribute::MaxHealth:                return "MaxHealth";
		case EHunterAttribute::MaxEffectiveHealth:       return "MaxEffectiveHealth";
		case EHunterAttribute::MaxMana:                  return "MaxMana";
		case EHunterAttribute::MaxEffectiveMana:         return "MaxEffectiveMana";
		case EHunterAttribute::MaxStamina:               return "MaxStamina";
		case EHunterAttribute::MaxEffectiveStamina:      return "MaxEffectiveStamina";
		case EHunterAttribute::MaxArcaneShield:          return "MaxArcaneShield";
		case EHunterAttribute::MaxEffectiveArcaneShield: return "MaxEffectiveArcaneShield";

		// Vitals - Current
		case EHunterAttribute::Health:        return "Health";
		case EHunterAttribute::Mana:          return "Mana";
		case EHunterAttribute::Stamina:       return "Stamina";
		case EHunterAttribute::ArcaneShield:  return "ArcaneShield";
		case EHunterAttribute::Gems:          return "Gems";

		// Damage - Min/Max
		case EHunterAttribute::MinPhysicalDamage:   return "MinPhysicalDamage";
		case EHunterAttribute::MaxPhysicalDamage:   return "MaxPhysicalDamage";
		case EHunterAttribute::MinFireDamage:       return "MinFireDamage";
		case EHunterAttribute::MaxFireDamage:       return "MaxFireDamage";
		case EHunterAttribute::MinIceDamage:        return "MinIceDamage";
		case EHunterAttribute::MaxIceDamage:        return "MaxIceDamage";
		case EHunterAttribute::MinLightningDamage:  return "MinLightningDamage";
		case EHunterAttribute::MaxLightningDamage:  return "MaxLightningDamage";
		case EHunterAttribute::MinLightDamage:      return "MinLightDamage";
		case EHunterAttribute::MaxLightDamage:      return "MaxLightDamage";
		case EHunterAttribute::MinCorruptionDamage: return "MinCorruptionDamage";
		case EHunterAttribute::MaxCorruptionDamage: return "MaxCorruptionDamage";

		// Damage - Bonuses
		case EHunterAttribute::PhysicalFlatDamage:      return "PhysicalFlatDamage";
		case EHunterAttribute::FireFlatDamage:          return "FireFlatDamage";
		case EHunterAttribute::IceFlatDamage:           return "IceFlatDamage";
		case EHunterAttribute::LightningFlatDamage:     return "LightningFlatDamage";
		case EHunterAttribute::LightFlatDamage:         return "LightFlatDamage";
		case EHunterAttribute::CorruptionFlatDamage:    return "CorruptionFlatDamage";
		case EHunterAttribute::PhysicalPercentDamage:   return "PhysicalPercentDamage";
		case EHunterAttribute::FirePercentDamage:       return "FirePercentDamage";
		case EHunterAttribute::IcePercentDamage:        return "IcePercentDamage";
		case EHunterAttribute::LightningPercentDamage:  return "LightningPercentDamage";
		case EHunterAttribute::LightPercentDamage:      return "LightPercentDamage";
		case EHunterAttribute::CorruptionPercentDamage: return "CorruptionPercentDamage";

		// Combat Stats
		case EHunterAttribute::CritChance:            return "CritChance";
		case EHunterAttribute::CritMultiplier:        return "CritMultiplier";
		case EHunterAttribute::SpellsCritChance:      return "SpellsCritChance";
		case EHunterAttribute::SpellsCritMultiplier:  return "SpellsCritMultiplier";
		case EHunterAttribute::AttackSpeed:           return "AttackSpeed";
		case EHunterAttribute::CastSpeed:             return "CastSpeed";
		case EHunterAttribute::AttackRange:           return "AttackRange";
		case EHunterAttribute::AreaOfEffect:          return "AreaOfEffect";
		case EHunterAttribute::AreaDamage:            return "AreaDamage";
		case EHunterAttribute::ProjectileCount:       return "ProjectileCount";
		case EHunterAttribute::ProjectileSpeed:       return "ProjectileSpeed";

		// Defense - Armour
		case EHunterAttribute::Armour:             return "Armour";
		case EHunterAttribute::ArmourFlatBonus:    return "ArmourFlatBonus";
		case EHunterAttribute::ArmourPercentBonus: return "ArmourPercentBonus";
		case EHunterAttribute::BlockStrength:      return "BlockStrength";

		// Defense - Resistances
		case EHunterAttribute::FireResistanceFlatBonus:          return "FireResistanceFlatBonus";
		case EHunterAttribute::FireResistancePercentBonus:       return "FireResistancePercentBonus";
		case EHunterAttribute::MaxFireResistance:                return "MaxFireResistance";
		case EHunterAttribute::IceResistanceFlatBonus:           return "IceResistanceFlatBonus";
		case EHunterAttribute::IceResistancePercentBonus:        return "IceResistancePercentBonus";
		case EHunterAttribute::MaxIceResistance:                 return "MaxIceResistance";
		case EHunterAttribute::LightningResistanceFlatBonus:     return "LightningResistanceFlatBonus";
		case EHunterAttribute::LightningResistancePercentBonus:  return "LightningResistancePercentBonus";
		case EHunterAttribute::MaxLightningResistance:           return "MaxLightningResistance";
		case EHunterAttribute::LightResistanceFlatBonus:         return "LightResistanceFlatBonus";
		case EHunterAttribute::LightResistancePercentBonus:      return "LightResistancePercentBonus";
		case EHunterAttribute::MaxLightResistance:               return "MaxLightResistance";
		case EHunterAttribute::CorruptionResistanceFlatBonus:    return "CorruptionResistanceFlatBonus";
		case EHunterAttribute::CorruptionResistancePercentBonus: return "CorruptionResistancePercentBonus";
		case EHunterAttribute::MaxCorruptionResistance:          return "MaxCorruptionResistance";

		// Piercing
		case EHunterAttribute::ArmourPiercing:     return "ArmourPiercing";
		case EHunterAttribute::FirePiercing:       return "FirePiercing";
		case EHunterAttribute::IcePiercing:        return "IcePiercing";
		case EHunterAttribute::LightningPiercing:  return "LightningPiercing";
		case EHunterAttribute::LightPiercing:      return "LightPiercing";
		case EHunterAttribute::CorruptionPiercing: return "CorruptionPiercing";

		// Ailment Chances
		case EHunterAttribute::ChanceToBleed:     return "ChanceToBleed";
		case EHunterAttribute::ChanceToIgnite:    return "ChanceToIgnite";
		case EHunterAttribute::ChanceToFreeze:    return "ChanceToFreeze";
		case EHunterAttribute::ChanceToShock:     return "ChanceToShock";
		case EHunterAttribute::ChanceToCorrupt:   return "ChanceToCorrupt";
		case EHunterAttribute::ChanceToPetrify:   return "ChanceToPetrify";
		case EHunterAttribute::ChanceToPurify:    return "ChanceToPurify";
		case EHunterAttribute::ChanceToStun:      return "ChanceToStun";
		case EHunterAttribute::ChanceToKnockBack: return "ChanceToKnockBack";

		// Ailment Durations
		case EHunterAttribute::BleedDuration:             return "BleedDuration";
		case EHunterAttribute::BurnDuration:              return "BurnDuration";
		case EHunterAttribute::FreezeDuration:            return "FreezeDuration";
		case EHunterAttribute::ShockDuration:             return "ShockDuration";
		case EHunterAttribute::CorruptionDuration:        return "CorruptionDuration";
		case EHunterAttribute::PetrifyBuildUpDuration:    return "PetrifyBuildUpDuration";
		case EHunterAttribute::PurifyDuration:            return "PurifyDuration";

		// Regen/Reserve - Health
		case EHunterAttribute::HealthRegenRate:          return "HealthRegenRate";
		case EHunterAttribute::HealthRegenAmount:        return "HealthRegenAmount";
		case EHunterAttribute::MaxHealthRegenRate:       return "MaxHealthRegenRate";
		case EHunterAttribute::MaxHealthRegenAmount:     return "MaxHealthRegenAmount";
		case EHunterAttribute::ReservedHealth:           return "ReservedHealth";
		case EHunterAttribute::MaxReservedHealth:        return "MaxReservedHealth";
		case EHunterAttribute::FlatReservedHealth:       return "FlatReservedHealth";
		case EHunterAttribute::PercentageReservedHealth: return "PercentageReservedHealth";

		// Regen/Reserve - Mana
		case EHunterAttribute::ManaRegenRate:          return "ManaRegenRate";
		case EHunterAttribute::ManaRegenAmount:        return "ManaRegenAmount";
		case EHunterAttribute::MaxManaRegenRate:       return "MaxManaRegenRate";
		case EHunterAttribute::MaxManaRegenAmount:     return "MaxManaRegenAmount";
		case EHunterAttribute::ReservedMana:           return "ReservedMana";
		case EHunterAttribute::MaxReservedMana:        return "MaxReservedMana";
		case EHunterAttribute::FlatReservedMana:       return "FlatReservedMana";
		case EHunterAttribute::PercentageReservedMana: return "PercentageReservedMana";

		// Regen/Reserve - Stamina
		case EHunterAttribute::StaminaRegenRate:           return "StaminaRegenRate";
		case EHunterAttribute::StaminaRegenAmount:         return "StaminaRegenAmount";
		case EHunterAttribute::StaminaDegenRate:           return "StaminaDegenRate";
		case EHunterAttribute::StaminaDegenAmount:         return "StaminaDegenAmount";
		case EHunterAttribute::MaxStaminaRegenRate:        return "MaxStaminaRegenRate";
		case EHunterAttribute::MaxStaminaRegenAmount:      return "MaxStaminaRegenAmount";
		case EHunterAttribute::ReservedStamina:            return "ReservedStamina";
		case EHunterAttribute::MaxReservedStamina:         return "MaxReservedStamina";
		case EHunterAttribute::FlatReservedStamina:        return "FlatReservedStamina";
		case EHunterAttribute::PercentageReservedStamina:  return "PercentageReservedStamina";

		// Regen/Reserve - Arcane Shield
		case EHunterAttribute::ArcaneShieldRegenRate:           return "ArcaneShieldRegenRate";
		case EHunterAttribute::ArcaneShieldRegenAmount:         return "ArcaneShieldRegenAmount";
		case EHunterAttribute::ReservedArcaneShield:            return "ReservedArcaneShield";
		case EHunterAttribute::MaxReservedArcaneShield:         return "MaxReservedArcaneShield";
		case EHunterAttribute::FlatReservedArcaneShield:        return "FlatReservedArcaneShield";
		case EHunterAttribute::PercentageReservedArcaneShield:  return "PercentageReservedArcaneShield";

		// Utility
		case EHunterAttribute::MovementSpeed:        return "MovementSpeed";
		case EHunterAttribute::LifeLeech:            return "LifeLeech";
		case EHunterAttribute::ManaLeech:            return "ManaLeech";
		case EHunterAttribute::LifeOnHit:            return "LifeOnHit";
		case EHunterAttribute::ManaOnHit:            return "ManaOnHit";
		case EHunterAttribute::StaminaOnHit:         return "StaminaOnHit";
		case EHunterAttribute::CooldownReduction:    return "CooldownReduction";
		case EHunterAttribute::ManaCostChanges:      return "ManaCostChanges";
		case EHunterAttribute::HealthCostChanges:    return "HealthCostChanges";
		case EHunterAttribute::StaminaCostChanges:   return "StaminaCostChanges";
		case EHunterAttribute::Poise:                return "Poise";
		case EHunterAttribute::PoiseResistance:      return "PoiseResistance";
		case EHunterAttribute::Weight:               return "Weight";
		case EHunterAttribute::StunRecovery:         return "StunRecovery";
		case EHunterAttribute::ComboCounter:         return "ComboCounter";

		// Special
		case EHunterAttribute::GlobalDamages:               return "GlobalDamages";
		case EHunterAttribute::GlobalDefenses:              return "GlobalDefenses";
		case EHunterAttribute::ElementalDamage:             return "ElementalDamage";
		case EHunterAttribute::DamageOverTime:              return "DamageOverTime";
		case EHunterAttribute::MeleeDamage:                 return "MeleeDamage";
		case EHunterAttribute::SpellDamage:                 return "SpellDamage";
		case EHunterAttribute::RangedDamage:                return "RangedDamage";
		case EHunterAttribute::DamageBonusWhileAtFullHP:    return "DamageBonusWhileAtFullHP";
		case EHunterAttribute::DamageBonusWhileAtLowHP:     return "DamageBonusWhileAtLowHP";

		// XP Bonuses
		case EHunterAttribute::GlobalXPGain:      return "GlobalXPGain";
		case EHunterAttribute::LocalXPGain:       return "LocalXPGain";
		case EHunterAttribute::XPGainMultiplier:  return "XPGainMultiplier";
		case EHunterAttribute::XPPenalty:         return "XPPenalty";

		// Indicators
		case EHunterAttribute::CombatAlignment: return "CombatAlignment";
		case EHunterAttribute::CombatStatus:    return "CombatStatus";

		default:
			return NAME_None;
	}
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* GET ALL STATS AS MAP */
/* ═══════════════════════════════════════════════════════════════════════ */

TMap<FName, float> UBaseStatsData::GetAllStatsAsMap() const
{
	TMap<FName, float> StatsMap;

	
	if (Strength > 0.0f)        StatsMap.Add("Strength", Strength);
	if (Intelligence > 0.0f)    StatsMap.Add("Intelligence", Intelligence);
	if (Dexterity > 0.0f)       StatsMap.Add("Dexterity", Dexterity);
	if (Endurance > 0.0f)       StatsMap.Add("Endurance", Endurance);
	if (Affliction > 0.0f)      StatsMap.Add("Affliction", Affliction);
	if (Luck > 0.0f)            StatsMap.Add("Luck", Luck);
	if (Covenant > 0.0f)        StatsMap.Add("Covenant", Covenant);
	if (MaxHealth > 0.0f)       StatsMap.Add("MaxHealth", MaxHealth);
	if (MaxMana > 0.0f)         StatsMap.Add("MaxMana", MaxMana);
	if (MaxStamina > 0.0f)      StatsMap.Add("MaxStamina", MaxStamina);
	if (MaxArcaneShield > 0.0f) StatsMap.Add("MaxArcaneShield", MaxArcaneShield);
	if (MinPhysicalDamage > 0.0f) StatsMap.Add("MinPhysicalDamage", MinPhysicalDamage);
	if (MaxPhysicalDamage > 0.0f) StatsMap.Add("MaxPhysicalDamage", MaxPhysicalDamage);
	if (CritChance > 0.0f)        StatsMap.Add("CritChance", CritChance);
	if (CritMultiplier > 0.0f)    StatsMap.Add("CritMultiplier", CritMultiplier);
	if (AttackSpeed > 0.0f)       StatsMap.Add("AttackSpeed", AttackSpeed);
	if (CastSpeed > 0.0f)         StatsMap.Add("CastSpeed", CastSpeed);
	if (Armour > 0.0f)            StatsMap.Add("Armour", Armour);
	if (MovementSpeed > 0.0f)     StatsMap.Add("MovementSpeed", MovementSpeed);

	
	for (const FStatInitializationEntry& Entry : BaseAttributes)
	{
		FName AttributeName = Entry.GetAttributeName();
		if (AttributeName != NAME_None)
		{
			StatsMap.Add(AttributeName, Entry.BaseValue);
		}
	}

	return StatsMap;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* GET STAT VALUE */
/* ═══════════════════════════════════════════════════════════════════════ */

bool UBaseStatsData::GetStatValue(FName AttributeName, float& OutValue) const
{
	// Check Quick Setup
	if (AttributeName == "Strength")        { OutValue = Strength; return Strength > 0.0f; }
	if (AttributeName == "Intelligence")    { OutValue = Intelligence; return Intelligence > 0.0f; }
	if (AttributeName == "Dexterity")       { OutValue = Dexterity; return Dexterity > 0.0f; }
	if (AttributeName == "Endurance")       { OutValue = Endurance; return Endurance > 0.0f; }
	if (AttributeName == "Affliction")      { OutValue = Affliction; return Affliction > 0.0f; }
	if (AttributeName == "Luck")            { OutValue = Luck; return Luck > 0.0f; }
	if (AttributeName == "Covenant")        { OutValue = Covenant; return Covenant > 0.0f; }
	if (AttributeName == "MaxHealth")       { OutValue = MaxHealth; return MaxHealth > 0.0f; }
	if (AttributeName == "MaxMana")         { OutValue = MaxMana; return MaxMana > 0.0f; }
	if (AttributeName == "MaxStamina")      { OutValue = MaxStamina; return MaxStamina > 0.0f; }
	if (AttributeName == "MaxArcaneShield") { OutValue = MaxArcaneShield; return MaxArcaneShield > 0.0f; }
	if (AttributeName == "MinPhysicalDamage") { OutValue = MinPhysicalDamage; return MinPhysicalDamage > 0.0f; }
	if (AttributeName == "MaxPhysicalDamage") { OutValue = MaxPhysicalDamage; return MaxPhysicalDamage > 0.0f; }
	if (AttributeName == "CritChance")      { OutValue = CritChance; return CritChance > 0.0f; }
	if (AttributeName == "CritMultiplier")  { OutValue = CritMultiplier; return CritMultiplier > 0.0f; }
	if (AttributeName == "AttackSpeed")     { OutValue = AttackSpeed; return AttackSpeed > 0.0f; }
	if (AttributeName == "CastSpeed")       { OutValue = CastSpeed; return CastSpeed > 0.0f; }
	if (AttributeName == "Armour")          { OutValue = Armour; return Armour > 0.0f; }
	if (AttributeName == "MovementSpeed")   { OutValue = MovementSpeed; return MovementSpeed > 0.0f; }

	// Check BaseAttributes
	for (const FStatInitializationEntry& Entry : BaseAttributes)
	{
		if (Entry.GetAttributeName() == AttributeName)
		{
			OutValue = Entry.BaseValue;
			return true;
		}
	}

	OutValue = 0.0f;
	return false;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* HAS ATTRIBUTE */
/* ═══════════════════════════════════════════════════════════════════════ */

bool UBaseStatsData::HasAttribute(FName AttributeName) const
{
	float Value;
	return GetStatValue(AttributeName, Value);
}
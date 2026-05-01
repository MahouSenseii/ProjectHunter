#pragma once

#include "CoreMinimal.h"
#include "StatsEnumLibrary.generated.h"

/**
 * Enum-based attribute selector for UStatsManager::GetAttributeByType().
 *
 * Use this instead of FName to avoid typos, get BP autocomplete, and remove
 * the runtime string-lookup overhead. Every attribute on UHunterAttributeSet
 * has a corresponding entry here.
 *
 * Grouped by category to match the AttributeSet's UPROPERTY categories.
 */
UENUM(BlueprintType)
enum class EHunterAttribute : uint8
{
	// ── Primary Attributes ────────────────────────────────────────────────────
	Strength            UMETA(DisplayName = "Strength"),
	Intelligence        UMETA(DisplayName = "Intelligence"),
	Dexterity           UMETA(DisplayName = "Dexterity"),
	Endurance           UMETA(DisplayName = "Endurance"),
	Affliction          UMETA(DisplayName = "Affliction"),
	Luck                UMETA(DisplayName = "Luck"),
	Covenant            UMETA(DisplayName = "Covenant"),

	// ── Progression ───────────────────────────────────────────────────────────
	PlayerLevel         UMETA(DisplayName = "Player Level"),

	// ── Experience ────────────────────────────────────────────────────────────
	GlobalXPGain        UMETA(DisplayName = "Global XP Gain"),
	LocalXPGain         UMETA(DisplayName = "Local XP Gain"),
	XPGainMultiplier    UMETA(DisplayName = "XP Gain Multiplier"),
	XPPenalty           UMETA(DisplayName = "XP Penalty"),

	// ── Vitals — Health ───────────────────────────────────────────────────────
	Health                      UMETA(DisplayName = "Health"),
	MaxHealth                   UMETA(DisplayName = "Max Health"),
	MaxEffectiveHealth          UMETA(DisplayName = "Max Effective Health"),
	HealthRegenRate             UMETA(DisplayName = "Health Regen Rate"),
	MaxHealthRegenRate          UMETA(DisplayName = "Max Health Regen Rate"),
	HealthRegenAmount           UMETA(DisplayName = "Health Regen Amount"),
	MaxHealthRegenAmount        UMETA(DisplayName = "Max Health Regen Amount"),
	ReservedHealth              UMETA(DisplayName = "Reserved Health"),
	MaxReservedHealth           UMETA(DisplayName = "Max Reserved Health"),
	FlatReservedHealth          UMETA(DisplayName = "Flat Reserved Health"),
	PercentageReservedHealth    UMETA(DisplayName = "Percentage Reserved Health"),

	// ── Vitals — Stamina ──────────────────────────────────────────────────────
	Stamina                     UMETA(DisplayName = "Stamina"),
	MaxStamina                  UMETA(DisplayName = "Max Stamina"),
	MaxEffectiveStamina         UMETA(DisplayName = "Max Effective Stamina"),
	StaminaRegenRate            UMETA(DisplayName = "Stamina Regen Rate"),
	StaminaDegenRate            UMETA(DisplayName = "Stamina Degen Rate"),
	MaxStaminaRegenRate         UMETA(DisplayName = "Max Stamina Regen Rate"),
	StaminaRegenAmount          UMETA(DisplayName = "Stamina Regen Amount"),
	StaminaDegenAmount          UMETA(DisplayName = "Stamina Degen Amount"),
	MaxStaminaRegenAmount       UMETA(DisplayName = "Max Stamina Regen Amount"),
	ReservedStamina             UMETA(DisplayName = "Reserved Stamina"),
	MaxReservedStamina          UMETA(DisplayName = "Max Reserved Stamina"),
	FlatReservedStamina         UMETA(DisplayName = "Flat Reserved Stamina"),
	PercentageReservedStamina   UMETA(DisplayName = "Percentage Reserved Stamina"),

	// ── Vitals — Mana ─────────────────────────────────────────────────────────
	Mana                        UMETA(DisplayName = "Mana"),
	MaxMana                     UMETA(DisplayName = "Max Mana"),
	MaxEffectiveMana            UMETA(DisplayName = "Max Effective Mana"),
	ManaRegenRate               UMETA(DisplayName = "Mana Regen Rate"),
	MaxManaRegenRate            UMETA(DisplayName = "Max Mana Regen Rate"),
	ManaRegenAmount             UMETA(DisplayName = "Mana Regen Amount"),
	MaxManaRegenAmount          UMETA(DisplayName = "Max Mana Regen Amount"),
	ReservedMana                UMETA(DisplayName = "Reserved Mana"),
	MaxReservedMana             UMETA(DisplayName = "Max Reserved Mana"),
	FlatReservedMana            UMETA(DisplayName = "Flat Reserved Mana"),
	PercentageReservedMana      UMETA(DisplayName = "Percentage Reserved Mana"),

	// ── Vitals — Arcane Shield ────────────────────────────────────────────────
	ArcaneShield                    UMETA(DisplayName = "Arcane Shield"),
	MaxArcaneShield                 UMETA(DisplayName = "Max Arcane Shield"),
	MaxEffectiveArcaneShield        UMETA(DisplayName = "Max Effective Arcane Shield"),
	ArcaneShieldRegenRate           UMETA(DisplayName = "Arcane Shield Regen Rate"),
	MaxArcaneShieldRegenRate        UMETA(DisplayName = "Max Arcane Shield Regen Rate"),
	ArcaneShieldRegenAmount         UMETA(DisplayName = "Arcane Shield Regen Amount"),
	MaxArcaneShieldRegenAmount      UMETA(DisplayName = "Max Arcane Shield Regen Amount"),
	ReservedArcaneShield            UMETA(DisplayName = "Reserved Arcane Shield"),
	MaxReservedArcaneShield         UMETA(DisplayName = "Max Reserved Arcane Shield"),
	FlatReservedArcaneShield        UMETA(DisplayName = "Flat Reserved Arcane Shield"),
	PercentageReservedArcaneShield  UMETA(DisplayName = "Percentage Reserved Arcane Shield"),

	// ── Damage — Global ───────────────────────────────────────────────────────
	GlobalDamages           UMETA(DisplayName = "Global Damages (Increased%)"),
	GlobalMoreDamage        UMETA(DisplayName = "Global More Damage"),
	DamageBonusWhileAtFullHP UMETA(DisplayName = "Damage Bonus At Full HP"),
	DamageBonusWhileAtLowHP  UMETA(DisplayName = "Damage Bonus At Low HP"),

	// ── Damage — Physical ─────────────────────────────────────────────────────
	MinPhysicalDamage       UMETA(DisplayName = "Min Physical Damage"),
	MaxPhysicalDamage       UMETA(DisplayName = "Max Physical Damage"),
	PhysicalFlatDamage      UMETA(DisplayName = "Physical Flat Damage"),
	PhysicalPercentDamage   UMETA(DisplayName = "Physical Percent Damage"),
	PhysicalMoreDamage      UMETA(DisplayName = "Physical More Damage"),

	// ── Damage — Fire ─────────────────────────────────────────────────────────
	MinFireDamage           UMETA(DisplayName = "Min Fire Damage"),
	MaxFireDamage           UMETA(DisplayName = "Max Fire Damage"),
	FireFlatDamage          UMETA(DisplayName = "Fire Flat Damage"),
	FirePercentDamage       UMETA(DisplayName = "Fire Percent Damage"),
	FireMoreDamage          UMETA(DisplayName = "Fire More Damage"),

	// ── Damage — Ice ──────────────────────────────────────────────────────────
	MinIceDamage            UMETA(DisplayName = "Min Ice Damage"),
	MaxIceDamage            UMETA(DisplayName = "Max Ice Damage"),
	IceFlatDamage           UMETA(DisplayName = "Ice Flat Damage"),
	IcePercentDamage        UMETA(DisplayName = "Ice Percent Damage"),
	IceMoreDamage           UMETA(DisplayName = "Ice More Damage"),

	// ── Damage — Lightning ────────────────────────────────────────────────────
	MinLightningDamage      UMETA(DisplayName = "Min Lightning Damage"),
	MaxLightningDamage      UMETA(DisplayName = "Max Lightning Damage"),
	LightningFlatDamage     UMETA(DisplayName = "Lightning Flat Damage"),
	LightningPercentDamage  UMETA(DisplayName = "Lightning Percent Damage"),
	LightningMoreDamage     UMETA(DisplayName = "Lightning More Damage"),

	// ── Damage — Light ────────────────────────────────────────────────────────
	MinLightDamage          UMETA(DisplayName = "Min Light Damage"),
	MaxLightDamage          UMETA(DisplayName = "Max Light Damage"),
	LightFlatDamage         UMETA(DisplayName = "Light Flat Damage"),
	LightPercentDamage      UMETA(DisplayName = "Light Percent Damage"),
	LightMoreDamage         UMETA(DisplayName = "Light More Damage"),

	// ── Damage — Corruption ───────────────────────────────────────────────────
	MinCorruptionDamage     UMETA(DisplayName = "Min Corruption Damage"),
	MaxCorruptionDamage     UMETA(DisplayName = "Max Corruption Damage"),
	CorruptionFlatDamage    UMETA(DisplayName = "Corruption Flat Damage"),
	CorruptionPercentDamage UMETA(DisplayName = "Corruption Percent Damage"),
	CorruptionMoreDamage    UMETA(DisplayName = "Corruption More Damage"),

	// ── Damage — Elemental ────────────────────────────────────────────────────
	ElementalMoreDamage     UMETA(DisplayName = "Elemental More Damage"),
	ElementalDamage         UMETA(DisplayName = "Elemental Damage (Increased%)"),

	// ── Offensive Stats ───────────────────────────────────────────────────────
	AreaDamage              UMETA(DisplayName = "Area Damage"),
	AreaOfEffect            UMETA(DisplayName = "Area Of Effect"),
	AttackRange             UMETA(DisplayName = "Attack Range"),
	AttackSpeed             UMETA(DisplayName = "Attack Speed"),
	CastSpeed               UMETA(DisplayName = "Cast Speed"),
	CritChance              UMETA(DisplayName = "Crit Chance"),
	CritMultiplier          UMETA(DisplayName = "Crit Multiplier"),
	DamageOverTime          UMETA(DisplayName = "Damage Over Time"),
	MeleeDamage             UMETA(DisplayName = "Melee Damage"),
	SpellDamage             UMETA(DisplayName = "Spell Damage"),
	ProjectileCount         UMETA(DisplayName = "Projectile Count"),
	ProjectileSpeed         UMETA(DisplayName = "Projectile Speed"),
	RangedDamage            UMETA(DisplayName = "Ranged Damage"),
	SpellsCritChance        UMETA(DisplayName = "Spells Crit Chance"),
	SpellsCritMultiplier    UMETA(DisplayName = "Spells Crit Multiplier"),
	ChainCount              UMETA(DisplayName = "Chain Count"),
	ForkCount               UMETA(DisplayName = "Fork Count"),
	ChainDamage             UMETA(DisplayName = "Chain Damage"),

	// ── Damage Conversion — From Physical ─────────────────────────────────────
	PhysicalToFire          UMETA(DisplayName = "Physical → Fire %"),
	PhysicalToIce           UMETA(DisplayName = "Physical → Ice %"),
	PhysicalToLightning     UMETA(DisplayName = "Physical → Lightning %"),
	PhysicalToLight         UMETA(DisplayName = "Physical → Light %"),
	PhysicalToCorruption    UMETA(DisplayName = "Physical → Corruption %"),

	// ── Damage Conversion — From Fire ─────────────────────────────────────────
	FireToPhysical          UMETA(DisplayName = "Fire → Physical %"),
	FireToIce               UMETA(DisplayName = "Fire → Ice %"),
	FireToLightning         UMETA(DisplayName = "Fire → Lightning %"),
	FireToLight             UMETA(DisplayName = "Fire → Light %"),
	FireToCorruption        UMETA(DisplayName = "Fire → Corruption %"),

	// ── Damage Conversion — From Ice ──────────────────────────────────────────
	IceToPhysical           UMETA(DisplayName = "Ice → Physical %"),
	IceToFire               UMETA(DisplayName = "Ice → Fire %"),
	IceToLightning          UMETA(DisplayName = "Ice → Lightning %"),
	IceToLight              UMETA(DisplayName = "Ice → Light %"),
	IceToCorruption         UMETA(DisplayName = "Ice → Corruption %"),

	// ── Damage Conversion — From Lightning ────────────────────────────────────
	LightningToPhysical     UMETA(DisplayName = "Lightning → Physical %"),
	LightningToFire         UMETA(DisplayName = "Lightning → Fire %"),
	LightningToIce          UMETA(DisplayName = "Lightning → Ice %"),
	LightningToLight        UMETA(DisplayName = "Lightning → Light %"),
	LightningToCorruption   UMETA(DisplayName = "Lightning → Corruption %"),

	// ── Damage Conversion — From Light ────────────────────────────────────────
	LightToPhysical         UMETA(DisplayName = "Light → Physical %"),
	LightToFire             UMETA(DisplayName = "Light → Fire %"),
	LightToIce              UMETA(DisplayName = "Light → Ice %"),
	LightToLightning        UMETA(DisplayName = "Light → Lightning %"),
	LightToCorruption       UMETA(DisplayName = "Light → Corruption %"),

	// ── Damage Conversion — From Corruption ───────────────────────────────────
	CorruptionToPhysical    UMETA(DisplayName = "Corruption → Physical %"),
	CorruptionToFire        UMETA(DisplayName = "Corruption → Fire %"),
	CorruptionToIce         UMETA(DisplayName = "Corruption → Ice %"),
	CorruptionToLightning   UMETA(DisplayName = "Corruption → Lightning %"),
	CorruptionToLight       UMETA(DisplayName = "Corruption → Light %"),

	// ── Ailment Chances ───────────────────────────────────────────────────────
	ChanceToBleed           UMETA(DisplayName = "Chance To Bleed"),
	ChanceToCorrupt         UMETA(DisplayName = "Chance To Corrupt"),
	ChanceToFreeze          UMETA(DisplayName = "Chance To Freeze"),
	ChanceToPurify          UMETA(DisplayName = "Chance To Purify"),
	ChanceToIgnite          UMETA(DisplayName = "Chance To Ignite"),
	ChanceToKnockBack       UMETA(DisplayName = "Chance To Knock Back"),
	ChanceToPetrify         UMETA(DisplayName = "Chance To Petrify"),
	ChanceToShock           UMETA(DisplayName = "Chance To Shock"),
	ChanceToStun            UMETA(DisplayName = "Chance To Stun"),

	// ── DoT / Ailment Durations ───────────────────────────────────────────────
	BurnDuration            UMETA(DisplayName = "Burn Duration"),
	BleedDuration           UMETA(DisplayName = "Bleed Duration"),
	FreezeDuration          UMETA(DisplayName = "Freeze Duration"),
	CorruptionDuration      UMETA(DisplayName = "Corruption Duration"),
	ShockDuration           UMETA(DisplayName = "Shock Duration"),
	PetrifyBuildUpDuration  UMETA(DisplayName = "Petrify Build-Up Duration"),
	PurifyDuration          UMETA(DisplayName = "Purify Duration"),

	// ── Defense ───────────────────────────────────────────────────────────────
	GlobalDefenses              UMETA(DisplayName = "Global Defenses"),
	Armour                      UMETA(DisplayName = "Armour"),
	ArmourFlatBonus             UMETA(DisplayName = "Armour Flat Bonus"),
	ArmourPercentBonus          UMETA(DisplayName = "Armour Percent Bonus"),
	BlockStrength               UMETA(DisplayName = "Block Strength"),
	FlatBlockAmount             UMETA(DisplayName = "Flat Block Amount"),
	ChipDamageWhileBlocking     UMETA(DisplayName = "Chip Damage While Blocking"),
	BlockStaminaCostMultiplier  UMETA(DisplayName = "Block Stamina Cost Multiplier"),
	GuardBreakThreshold         UMETA(DisplayName = "Guard Break Threshold"),
	BlockAngle                  UMETA(DisplayName = "Block Angle"),
	BlockPhysicalMultiplier     UMETA(DisplayName = "Block Physical Multiplier"),
	BlockElementalMultiplier    UMETA(DisplayName = "Block Elemental Multiplier"),
	BlockCorruptionMultiplier   UMETA(DisplayName = "Block Corruption Multiplier"),

	// ── Resistances ───────────────────────────────────────────────────────────
	FireResistanceFlatBonus         UMETA(DisplayName = "Fire Resistance (Flat)"),
	FireResistancePercentBonus      UMETA(DisplayName = "Fire Resistance %"),
	MaxFireResistance               UMETA(DisplayName = "Max Fire Resistance"),
	IceResistanceFlatBonus          UMETA(DisplayName = "Ice Resistance (Flat)"),
	IceResistancePercentBonus       UMETA(DisplayName = "Ice Resistance %"),
	MaxIceResistance                UMETA(DisplayName = "Max Ice Resistance"),
	LightningResistanceFlatBonus    UMETA(DisplayName = "Lightning Resistance (Flat)"),
	LightningResistancePercentBonus UMETA(DisplayName = "Lightning Resistance %"),
	MaxLightningResistance          UMETA(DisplayName = "Max Lightning Resistance"),
	LightResistanceFlatBonus        UMETA(DisplayName = "Light Resistance (Flat)"),
	LightResistancePercentBonus     UMETA(DisplayName = "Light Resistance %"),
	MaxLightResistance              UMETA(DisplayName = "Max Light Resistance"),
	CorruptionResistanceFlatBonus   UMETA(DisplayName = "Corruption Resistance (Flat)"),
	CorruptionResistancePercentBonus UMETA(DisplayName = "Corruption Resistance %"),
	MaxCorruptionResistance         UMETA(DisplayName = "Max Corruption Resistance"),

	// ── Damage Taken Multipliers ──────────────────────────────────────────────
	GlobalDamageTakenMultiplier     UMETA(DisplayName = "Global Damage Taken Multiplier"),
	PhysicalDamageTakenMultiplier   UMETA(DisplayName = "Physical Damage Taken Multiplier"),
	ElementalDamageTakenMultiplier  UMETA(DisplayName = "Elemental Damage Taken Multiplier"),
	FireDamageTakenMultiplier       UMETA(DisplayName = "Fire Damage Taken Multiplier"),
	IceDamageTakenMultiplier        UMETA(DisplayName = "Ice Damage Taken Multiplier"),
	LightningDamageTakenMultiplier  UMETA(DisplayName = "Lightning Damage Taken Multiplier"),
	LightDamageTakenMultiplier      UMETA(DisplayName = "Light Damage Taken Multiplier"),
	CorruptionDamageTakenMultiplier UMETA(DisplayName = "Corruption Damage Taken Multiplier"),

	// ── Reflect ───────────────────────────────────────────────────────────────
	ReflectPhysical         UMETA(DisplayName = "Reflect Physical %"),
	ReflectElemental        UMETA(DisplayName = "Reflect Elemental %"),
	ReflectChancePhysical   UMETA(DisplayName = "Reflect Chance Physical"),
	ReflectChanceElemental  UMETA(DisplayName = "Reflect Chance Elemental"),

	// ── Piercing ──────────────────────────────────────────────────────────────
	ArmourPiercing          UMETA(DisplayName = "Armour Piercing"),
	FirePiercing            UMETA(DisplayName = "Fire Piercing"),
	IcePiercing             UMETA(DisplayName = "Ice Piercing"),
	LightningPiercing       UMETA(DisplayName = "Lightning Piercing"),
	LightPiercing           UMETA(DisplayName = "Light Piercing"),
	CorruptionPiercing      UMETA(DisplayName = "Corruption Piercing"),

	// ── Resource & Utility ────────────────────────────────────────────────────
	ComboCounter            UMETA(DisplayName = "Combo Counter"),
	Cooldown                UMETA(DisplayName = "Cooldown"),
	Gems                    UMETA(DisplayName = "Gems"),
	LifeLeech               UMETA(DisplayName = "Life Leech"),
	ManaLeech               UMETA(DisplayName = "Mana Leech"),
	StaminaLeechPercent     UMETA(DisplayName = "Stamina Leech %"),
	MovementSpeed           UMETA(DisplayName = "Movement Speed"),
	Poise                   UMETA(DisplayName = "Poise"),
	Weight                  UMETA(DisplayName = "Weight"),
	PoiseResistance         UMETA(DisplayName = "Poise Resistance"),
	StunRecovery            UMETA(DisplayName = "Stun Recovery"),
	ManaCostChanges         UMETA(DisplayName = "Mana Cost Changes"),
	HealthCostChanges       UMETA(DisplayName = "Health Cost Changes"),
	StaminaCostChanges      UMETA(DisplayName = "Stamina Cost Changes"),
	LifeOnHit               UMETA(DisplayName = "Life On Hit"),
	ManaOnHit               UMETA(DisplayName = "Mana On Hit"),
	StaminaOnHit            UMETA(DisplayName = "Stamina On Hit"),
	AuraEffect              UMETA(DisplayName = "Aura Effect"),
	AuraRadius              UMETA(DisplayName = "Aura Radius"),

	// ── Sentinel ──────────────────────────────────────────────────────────────
	// Keep last — used for bounds checking / iteration if needed.
	MAX                     UMETA(Hidden)
};

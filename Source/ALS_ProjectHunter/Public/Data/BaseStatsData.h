// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "BaseStatsData.generated.h"

// Forward declarations
class UGameplayEffect;
struct FHunterStatDefinition;
struct FPropertyChangedEvent;

/**
 * Enum for all 187 attributes
 * Provides dropdown selection in editor!
 */
UENUM(BlueprintType)
enum class EHunterAttribute : uint8
{
	// ═══════════════════════════════════════════════════════════════════════
	// PRIMARY ATTRIBUTES (7)
	// ═══════════════════════════════════════════════════════════════════════
	Strength            UMETA(DisplayName = "Strength"),
	Intelligence        UMETA(DisplayName = "Intelligence"),
	Dexterity          UMETA(DisplayName = "Dexterity"),
	Endurance          UMETA(DisplayName = "Endurance"),
	Affliction         UMETA(DisplayName = "Affliction"),
	Luck               UMETA(DisplayName = "Luck"),
	Covenant           UMETA(DisplayName = "Covenant"),

	// ═══════════════════════════════════════════════════════════════════════
	// VITAL POOLS - MAX (12)
	// ═══════════════════════════════════════════════════════════════════════
	MaxHealth                 UMETA(DisplayName = "Max Health"),
	MaxEffectiveHealth        UMETA(DisplayName = "Max Effective Health"),
	MaxMana                   UMETA(DisplayName = "Max Mana"),
	MaxEffectiveMana          UMETA(DisplayName = "Max Effective Mana"),
	MaxStamina                UMETA(DisplayName = "Max Stamina"),
	MaxEffectiveStamina       UMETA(DisplayName = "Max Effective Stamina"),
	MaxArcaneShield           UMETA(DisplayName = "Max Arcane Shield"),
	MaxEffectiveArcaneShield  UMETA(DisplayName = "Max Effective Arcane Shield"),

	// ═══════════════════════════════════════════════════════════════════════
	// VITAL POOLS - CURRENT (5)
	// ═══════════════════════════════════════════════════════════════════════
	Health         UMETA(DisplayName = "Health"),
	Mana           UMETA(DisplayName = "Mana"),
	Stamina        UMETA(DisplayName = "Stamina"),
	ArcaneShield   UMETA(DisplayName = "Arcane Shield"),
	Gems           UMETA(DisplayName = "Gems"),

	// ═══════════════════════════════════════════════════════════════════════
	// DAMAGE - MIN/MAX (12)
	// ═══════════════════════════════════════════════════════════════════════
	MinPhysicalDamage      UMETA(DisplayName = "Min Physical Damage"),
	MaxPhysicalDamage      UMETA(DisplayName = "Max Physical Damage"),
	MinFireDamage          UMETA(DisplayName = "Min Fire Damage"),
	MaxFireDamage          UMETA(DisplayName = "Max Fire Damage"),
	MinIceDamage           UMETA(DisplayName = "Min Ice Damage"),
	MaxIceDamage           UMETA(DisplayName = "Max Ice Damage"),
	MinLightningDamage     UMETA(DisplayName = "Min Lightning Damage"),
	MaxLightningDamage     UMETA(DisplayName = "Max Lightning Damage"),
	MinLightDamage         UMETA(DisplayName = "Min Light Damage"),
	MaxLightDamage         UMETA(DisplayName = "Max Light Damage"),
	MinCorruptionDamage    UMETA(DisplayName = "Min Corruption Damage"),
	MaxCorruptionDamage    UMETA(DisplayName = "Max Corruption Damage"),

	// ═══════════════════════════════════════════════════════════════════════
	// DAMAGE - FLAT/PERCENT BONUSES (12)
	// ═══════════════════════════════════════════════════════════════════════
	PhysicalFlatDamage       UMETA(DisplayName = "Physical Flat Damage"),
	FireFlatDamage           UMETA(DisplayName = "Fire Flat Damage"),
	IceFlatDamage            UMETA(DisplayName = "Ice Flat Damage"),
	LightningFlatDamage      UMETA(DisplayName = "Lightning Flat Damage"),
	LightFlatDamage          UMETA(DisplayName = "Light Flat Damage"),
	CorruptionFlatDamage     UMETA(DisplayName = "Corruption Flat Damage"),
	PhysicalPercentDamage    UMETA(DisplayName = "Physical % Damage"),
	FirePercentDamage        UMETA(DisplayName = "Fire % Damage"),
	IcePercentDamage         UMETA(DisplayName = "Ice % Damage"),
	LightningPercentDamage   UMETA(DisplayName = "Lightning % Damage"),
	LightPercentDamage       UMETA(DisplayName = "Light % Damage"),
	CorruptionPercentDamage  UMETA(DisplayName = "Corruption % Damage"),

	// ═══════════════════════════════════════════════════════════════════════
	// COMBAT STATS (10)
	// ═══════════════════════════════════════════════════════════════════════
	CritChance             UMETA(DisplayName = "Crit Chance"),
	CritMultiplier         UMETA(DisplayName = "Crit Multiplier"),
	SpellsCritChance       UMETA(DisplayName = "Spells Crit Chance"),
	SpellsCritMultiplier   UMETA(DisplayName = "Spells Crit Multiplier"),
	AttackSpeed            UMETA(DisplayName = "Attack Speed"),
	CastSpeed              UMETA(DisplayName = "Cast Speed"),
	AttackRange            UMETA(DisplayName = "Attack Range"),
	AreaOfEffect           UMETA(DisplayName = "Area of Effect"),
	AreaDamage             UMETA(DisplayName = "Area Damage"),
	ProjectileCount        UMETA(DisplayName = "Projectile Count"),
	ProjectileSpeed        UMETA(DisplayName = "Projectile Speed"),

	// ═══════════════════════════════════════════════════════════════════════
	// DEFENSE - ARMOUR (4)
	// ═══════════════════════════════════════════════════════════════════════
	Armour                  UMETA(DisplayName = "Armour"),
	ArmourFlatBonus         UMETA(DisplayName = "Armour Flat Bonus"),
	ArmourPercentBonus      UMETA(DisplayName = "Armour % Bonus"),
	BlockStrength           UMETA(DisplayName = "Block Strength"),

	// ═══════════════════════════════════════════════════════════════════════
	// DEFENSE - RESISTANCES (15)
	// ═══════════════════════════════════════════════════════════════════════
	FireResistanceFlatBonus           UMETA(DisplayName = "Fire Resistance Flat"),
	FireResistancePercentBonus        UMETA(DisplayName = "Fire Resistance %"),
	MaxFireResistance                 UMETA(DisplayName = "Max Fire Resistance"),
	IceResistanceFlatBonus            UMETA(DisplayName = "Ice Resistance Flat"),
	IceResistancePercentBonus         UMETA(DisplayName = "Ice Resistance %"),
	MaxIceResistance                  UMETA(DisplayName = "Max Ice Resistance"),
	LightningResistanceFlatBonus      UMETA(DisplayName = "Lightning Resistance Flat"),
	LightningResistancePercentBonus   UMETA(DisplayName = "Lightning Resistance %"),
	MaxLightningResistance            UMETA(DisplayName = "Max Lightning Resistance"),
	LightResistanceFlatBonus          UMETA(DisplayName = "Light Resistance Flat"),
	LightResistancePercentBonus       UMETA(DisplayName = "Light Resistance %"),
	MaxLightResistance                UMETA(DisplayName = "Max Light Resistance"),
	CorruptionResistanceFlatBonus     UMETA(DisplayName = "Corruption Resistance Flat"),
	CorruptionResistancePercentBonus  UMETA(DisplayName = "Corruption Resistance %"),
	MaxCorruptionResistance           UMETA(DisplayName = "Max Corruption Resistance"),

	// ═══════════════════════════════════════════════════════════════════════
	// PIERCING (6)
	// ═══════════════════════════════════════════════════════════════════════
	ArmourPiercing        UMETA(DisplayName = "Armour Piercing"),
	FirePiercing          UMETA(DisplayName = "Fire Piercing"),
	IcePiercing           UMETA(DisplayName = "Ice Piercing"),
	LightningPiercing     UMETA(DisplayName = "Lightning Piercing"),
	LightPiercing         UMETA(DisplayName = "Light Piercing"),
	CorruptionPiercing    UMETA(DisplayName = "Corruption Piercing"),

	// ═══════════════════════════════════════════════════════════════════════
	// AILMENT CHANCES (9)
	// ═══════════════════════════════════════════════════════════════════════
	ChanceToBleed      UMETA(DisplayName = "Chance to Bleed"),
	ChanceToIgnite     UMETA(DisplayName = "Chance to Ignite"),
	ChanceToFreeze     UMETA(DisplayName = "Chance to Freeze"),
	ChanceToShock      UMETA(DisplayName = "Chance to Shock"),
	ChanceToCorrupt    UMETA(DisplayName = "Chance to Corrupt"),
	ChanceToPetrify    UMETA(DisplayName = "Chance to Petrify"),
	ChanceToPurify     UMETA(DisplayName = "Chance to Purify"),
	ChanceToStun       UMETA(DisplayName = "Chance to Stun"),
	ChanceToKnockBack  UMETA(DisplayName = "Chance to Knock Back"),

	// ═══════════════════════════════════════════════════════════════════════
	// AILMENT DURATIONS (7)
	// ═══════════════════════════════════════════════════════════════════════
	BleedDuration              UMETA(DisplayName = "Bleed Duration"),
	BurnDuration               UMETA(DisplayName = "Burn Duration"),
	FreezeDuration             UMETA(DisplayName = "Freeze Duration"),
	ShockDuration              UMETA(DisplayName = "Shock Duration"),
	CorruptionDuration         UMETA(DisplayName = "Corruption Duration"),
	PetrifyBuildUpDuration     UMETA(DisplayName = "Petrify Build Up Duration"),
	PurifyDuration             UMETA(DisplayName = "Purify Duration"),

	// ═══════════════════════════════════════════════════════════════════════
	// REGEN/RESERVE - HEALTH (9)
	// ═══════════════════════════════════════════════════════════════════════
	HealthRegenRate             UMETA(DisplayName = "Health Regen Rate"),
	HealthRegenAmount           UMETA(DisplayName = "Health Regen Amount"),
	MaxHealthRegenRate          UMETA(DisplayName = "Max Health Regen Rate"),
	MaxHealthRegenAmount        UMETA(DisplayName = "Max Health Regen Amount"),
	ReservedHealth              UMETA(DisplayName = "Reserved Health"),
	MaxReservedHealth           UMETA(DisplayName = "Max Reserved Health"),
	FlatReservedHealth          UMETA(DisplayName = "Flat Reserved Health"),
	PercentageReservedHealth    UMETA(DisplayName = "% Reserved Health"),

	// ═══════════════════════════════════════════════════════════════════════
	// REGEN/RESERVE - MANA (9)
	// ═══════════════════════════════════════════════════════════════════════
	ManaRegenRate            UMETA(DisplayName = "Mana Regen Rate"),
	ManaRegenAmount          UMETA(DisplayName = "Mana Regen Amount"),
	MaxManaRegenRate         UMETA(DisplayName = "Max Mana Regen Rate"),
	MaxManaRegenAmount       UMETA(DisplayName = "Max Mana Regen Amount"),
	ReservedMana             UMETA(DisplayName = "Reserved Mana"),
	MaxReservedMana          UMETA(DisplayName = "Max Reserved Mana"),
	FlatReservedMana         UMETA(DisplayName = "Flat Reserved Mana"),
	PercentageReservedMana   UMETA(DisplayName = "% Reserved Mana"),

	// ═══════════════════════════════════════════════════════════════════════
	// REGEN/RESERVE - STAMINA (10)
	// ═══════════════════════════════════════════════════════════════════════
	StaminaRegenRate              UMETA(DisplayName = "Stamina Regen Rate"),
	StaminaRegenAmount            UMETA(DisplayName = "Stamina Regen Amount"),
	StaminaDegenRate              UMETA(DisplayName = "Stamina Degen Rate"),
	StaminaDegenAmount            UMETA(DisplayName = "Stamina Degen Amount"),
	MaxStaminaRegenRate           UMETA(DisplayName = "Max Stamina Regen Rate"),
	MaxStaminaRegenAmount         UMETA(DisplayName = "Max Stamina Regen Amount"),
	ReservedStamina               UMETA(DisplayName = "Reserved Stamina"),
	MaxReservedStamina            UMETA(DisplayName = "Max Reserved Stamina"),
	FlatReservedStamina           UMETA(DisplayName = "Flat Reserved Stamina"),
	PercentageReservedStamina     UMETA(DisplayName = "% Reserved Stamina"),

	// ═══════════════════════════════════════════════════════════════════════
	// REGEN/RESERVE - ARCANE SHIELD (6)
	// ═══════════════════════════════════════════════════════════════════════
	ArcaneShieldRegenRate              UMETA(DisplayName = "Arcane Shield Regen Rate"),
	ArcaneShieldRegenAmount            UMETA(DisplayName = "Arcane Shield Regen Amount"),
	ReservedArcaneShield               UMETA(DisplayName = "Reserved Arcane Shield"),
	MaxReservedArcaneShield            UMETA(DisplayName = "Max Reserved Arcane Shield"),
	FlatReservedArcaneShield           UMETA(DisplayName = "Flat Reserved Arcane Shield"),
	PercentageReservedArcaneShield     UMETA(DisplayName = "% Reserved Arcane Shield"),

	// ═══════════════════════════════════════════════════════════════════════
	// UTILITY (15)
	// ═══════════════════════════════════════════════════════════════════════
	MovementSpeed         UMETA(DisplayName = "Movement Speed"),
	LifeLeech             UMETA(DisplayName = "Life Leech"),
	ManaLeech             UMETA(DisplayName = "Mana Leech"),
	LifeOnHit             UMETA(DisplayName = "Life on Hit"),
	ManaOnHit             UMETA(DisplayName = "Mana on Hit"),
	StaminaOnHit          UMETA(DisplayName = "Stamina on Hit"),
	CooldownReduction     UMETA(DisplayName = "Cooldown Reduction"),
	ManaCostChanges       UMETA(DisplayName = "Mana Cost Changes"),
	HealthCostChanges     UMETA(DisplayName = "Health Cost Changes"),
	StaminaCostChanges    UMETA(DisplayName = "Stamina Cost Changes"),
	Poise                 UMETA(DisplayName = "Poise"),
	PoiseResistance       UMETA(DisplayName = "Poise Resistance"),
	Weight                UMETA(DisplayName = "Weight"),
	StunRecovery          UMETA(DisplayName = "Stun Recovery"),
	ComboCounter          UMETA(DisplayName = "Combo Counter"),

	// ═══════════════════════════════════════════════════════════════════════
	// SPECIAL (10)
	// ═══════════════════════════════════════════════════════════════════════
	GlobalDamages                UMETA(DisplayName = "Global Damages"),
	GlobalDefenses               UMETA(DisplayName = "Global Defenses"),
	ElementalDamage              UMETA(DisplayName = "Elemental Damage"),
	DamageOverTime               UMETA(DisplayName = "Damage Over Time"),
	MeleeDamage                  UMETA(DisplayName = "Melee Damage"),
	SpellDamage                  UMETA(DisplayName = "Spell Damage"),
	RangedDamage                 UMETA(DisplayName = "Ranged Damage"),
	DamageBonusWhileAtFullHP     UMETA(DisplayName = "Damage Bonus at Full HP"),
	DamageBonusWhileAtLowHP      UMETA(DisplayName = "Damage Bonus at Low HP"),

	// ═══════════════════════════════════════════════════════════════════════
	// XP BONUSES (4)
	// ═══════════════════════════════════════════════════════════════════════
	GlobalXPGain        UMETA(DisplayName = "Global XP Gain"),
	LocalXPGain         UMETA(DisplayName = "Local XP Gain"),
	XPGainMultiplier    UMETA(DisplayName = "XP Gain Multiplier"),
	XPPenalty           UMETA(DisplayName = "XP Penalty"),

	// ═══════════════════════════════════════════════════════════════════════
	// INDICATORS (2)
	// ═══════════════════════════════════════════════════════════════════════
	CombatAlignment     UMETA(DisplayName = "Combat Alignment"),
	CombatStatus        UMETA(DisplayName = "Combat Status"),

	// Add more as needed...
	// Note: Damage conversion attributes (25) omitted for brevity
	// You can add them if needed: PhysicalToFire, FireToIce, etc.
};

class FHunterAttributeHelper
{
public:
	static FName GetAttributeName(EHunterAttribute Attribute);
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FStatInitializationEntry
{
	GENERATED_BODY()

	FStatInitializationEntry();

	/** Legacy serialized dropdown value kept for backward compatibility with older assets. */
	UPROPERTY()
	EHunterAttribute Attribute = EHunterAttribute::Strength;

	/** Shared stat name resolved from the central definition registry. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FName StatName = NAME_None;

	/** Shared display name resolved from the central definition registry. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FText DisplayName;

	/** Shared category resolved from the central definition registry. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FName Category = NAME_None;

	/** Whether this entry should contribute a runtime override value. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	bool bOverrideValue = false;

	/** Asset-authored value for this stat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (EditCondition = "bOverrideValue", EditConditionHides))
	float BaseValue = 0.0f;

	FName GetAttributeName() const;
	bool HasRuntimeValue() const;
	void ApplyDefinition(const FHunterStatDefinition& Definition);
};

UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UBaseStatsData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UBaseStatsData();

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* IDENTITY */
	/* ═══════════════════════════════════════════════════════════════════════ */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText StatSetName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FGameplayTagContainer Tags;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* QUICK SETUP */
	/* ═══════════════════════════════════════════════════════════════════════ */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Primary Attributes", meta = (ClampMin = "0"))
	float Strength = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Primary Attributes", meta = (ClampMin = "0"))
	float Intelligence = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Primary Attributes", meta = (ClampMin = "0"))
	float Dexterity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Primary Attributes", meta = (ClampMin = "0"))
	float Endurance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Primary Attributes", meta = (ClampMin = "0"))
	float Affliction = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Primary Attributes", meta = (ClampMin = "0"))
	float Luck = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Primary Attributes", meta = (ClampMin = "0"))
	float Covenant = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Vitals", meta = (ClampMin = "0"))
	float MaxHealth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Vitals", meta = (ClampMin = "0"))
	float MaxMana = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Vitals", meta = (ClampMin = "0"))
	float MaxStamina = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Vitals", meta = (ClampMin = "0"))
	float MaxArcaneShield = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Combat", meta = (ClampMin = "0"))
	float MinPhysicalDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Combat", meta = (ClampMin = "0"))
	float MaxPhysicalDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Combat", meta = (ClampMin = "0", ClampMax = "100"))
	float CritChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Combat", meta = (ClampMin = "0"))
	float CritMultiplier = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Combat", meta = (ClampMin = "0"))
	float AttackSpeed = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Combat", meta = (ClampMin = "0"))
	float CastSpeed = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Defense", meta = (ClampMin = "0"))
	float Armour = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Setup|Movement", meta = (ClampMin = "0"))
	float MovementSpeed = 100.0f;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* ALL ATTRIBUTES */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/**
	 * Synchronized stat entries sourced from the shared UHunterAttributeSet registry.
	 * The array size is fixed by the registry; edit the per-entry override flag/value only.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "All Attributes", meta = (TitleProperty = "DisplayName", EditFixedSize))
	TArray<FStatInitializationEntry> BaseAttributes;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* GAMEPLAY EFFECTS */
	/* ═══════════════════════════════════════════════════════════════════════ */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Advanced")
	TArray<TSubclassOf<UGameplayEffect>> InitializationEffects;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* HELPER FUNCTIONS */
	/* ═══════════════════════════════════════════════════════════════════════ */

	UFUNCTION(BlueprintPure, Category = "Stats")
	TMap<FName, float> GetAllStatsAsMap() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	TMap<FName, float> GetStatsByCategory(FName CategoryName) const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	TArray<FName> GetSupportedCategories() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	TArray<FName> GetSupportedStatNames() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	TArray<FStatInitializationEntry> GetStatEntriesByCategory(FName CategoryName) const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	bool GetStatValue(FName AttributeName, float& OutValue) const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	bool HasAttribute(FName AttributeName) const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	bool HasCategory(FName CategoryName) const;

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void SortStatsByCategoryThenName();

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RefreshCategoriesFromDefinitions();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stats|Editor")
	void RefreshFromAttributeSetDefinition();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stats|Editor")
	void SortStats();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stats|Editor")
	void ValidateStats();

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("BaseStatsData", GetFName());
	}
};

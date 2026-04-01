// Data/MonsterModifierData.h
// Monster Modifier System — affix-style mods for enemy characters.
//
// Two assets are defined here:
//   UMonsterModifierTable  — DataTable row that defines one modifier
//   UMonsterSpawnConfig    — Global DA controlling spawn tier chances and pack sizes
//
// Designers create one DataTable of type FMonsterModRow, then configure
// UMonsterSpawnConfig in the game settings.  UMonsterModifierComponent on each
// enemy reads these assets at spawn time.
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "MonsterModifierData.generated.h"

class UGameplayEffect;
class UGameplayAbility;

// ─────────────────────────────────────────────────────────────────────────────
// Monster tier — mirrors item rarity naming convention
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EMonsterTier : uint8
{
	MT_Normal   UMETA(DisplayName = "Normal (White)"),
	MT_Magic    UMETA(DisplayName = "Magic (Blue)"),
	MT_Rare     UMETA(DisplayName = "Rare (Yellow)"),
	MT_Unique   UMETA(DisplayName = "Unique (Orange)"),
};

// ─────────────────────────────────────────────────────────────────────────────
// Mod type — controls how a mod row is formatted in display names
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EMonsterModType : uint8
{
	MMT_Prefix  UMETA(DisplayName = "Prefix"),
	MMT_Suffix  UMETA(DisplayName = "Suffix"),
};

// ─────────────────────────────────────────────────────────────────────────────
// One stat override entry within a modifier
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FMonsterStatOverride
{
	GENERATED_BODY()

	/** GAS attribute to modify on the monster (e.g. UHunterAttributeSet::GetStrengthAttribute()) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	FGameplayAttribute Attribute;

	/** Additive flat bonus */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float FlatBonus = 0.0f;

	/** Multiplicative percent bonus (0.25 = +25%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float PercentBonus = 0.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
// DataTable row — one named monster modifier
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FMonsterModRow : public FTableRowBase
{
	GENERATED_BODY()

	// ── Identity ─────────────────────────────────────────────────────────────

	/** Unique ID for this mod */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FName ModID;

	/** Prefix/Suffix label added to the monster's display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FText DisplayLabel;

	/** Is this a prefix ("Berserker X") or suffix ("X the Unyielding")? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	EMonsterModType ModType = EMonsterModType::MMT_Prefix;

	// ── Eligibility ───────────────────────────────────────────────────────────

	/** Minimum area level before this mod can roll */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eligibility",
		meta = (ClampMin = 1, ClampMax = 100))
	int32 MinAreaLevel = 1;

	/** Minimum monster tier required (Normal mods can only spawn on Magic/Rare) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eligibility")
	EMonsterTier MinTier = EMonsterTier::MT_Magic;

	/** Generation weight (higher = more common) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eligibility",
		meta = (ClampMin = 1))
	int32 Weight = 100;

	// ── Stat Modifications ────────────────────────────────────────────────────

	/** HP and damage multipliers (applied before stat overrides) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats",
		meta = (ClampMin = 0.1f))
	float HPMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats",
		meta = (ClampMin = 0.1f))
	float DamageMultiplier = 1.0f;

	/** Stat-level overrides applied as a Gameplay Effect at spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	TArray<FMonsterStatOverride> StatOverrides;

	// ── GAS Integration ───────────────────────────────────────────────────────

	/**
	 * Optional Gameplay Effect applied to the monster at spawn.
	 * Use for persistent buffs (e.g. fire aura, regen, thorns).
	 * The GE is applied as Infinite duration.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayEffect> OnSpawnGE;

	/**
	 * Optional Gameplay Effect applied as an AURA to players within radius.
	 * Applied/removed as players enter/exit the aura radius.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayEffect> AuraGE;

	/** Radius (in cm) for the aura effect — 0 = no aura */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS",
		meta = (ClampMin = 0.0f))
	float AuraRadius = 0.0f;

	/**
	 * Optional ability granted to the monster.
	 * Used for behaviour-modifying mods (e.g. Teleporting, Flamebearer).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayAbility> GrantedAbilityClass;

	// ── AI Tuning ─────────────────────────────────────────────────────────────

	/** Multiplier to aggro range (1.0 = normal, 1.5 = +50% range) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
		meta = (ClampMin = 0.1f))
	float AggroRangeMultiplier = 1.0f;

	/** Multiplier to movement speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
		meta = (ClampMin = 0.1f))
	float MoveSpeedMultiplier = 1.0f;

	/** Optional gameplay tags granted to the monster for AI/behaviour branching */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FGameplayTagContainer GrantedTags;
};

// ─────────────────────────────────────────────────────────────────────────────
// Global spawn config DA — one per game, referenced by GameMode or WorldSettings
// ─────────────────────────────────────────────────────────────────────────────
UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UMonsterSpawnConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// ── Tier spawn chances (base, before area-level scaling) ─────────────────

	/**
	 * Base probability [0-1] that any spawned monster will be Magic (Blue).
	 * Affected by player's MagicFind attribute at runtime.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tier Chances",
		meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float BaseMagicChance = 0.15f;

	/**
	 * Base probability [0-1] that any spawned monster will be Rare (Yellow).
	 * Checked after Magic — so actual rare rate = BaseRareChance * (1 - BaseMagicChance).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tier Chances",
		meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float BaseRareChance = 0.04f;

	/**
	 * Unique monsters are placed manually — this is NOT a spawn roll chance.
	 * Set via individual encounter data.
	 */

	/**
	 * Per-point-of-MagicFind how much the Magic and Rare chances increase.
	 * E.g. 0.001 = each MF point adds 0.1% to both chances.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tier Chances",
		meta = (ClampMin = 0.0f))
	float MagicFindChanceScalar = 0.001f;

	// ── Mod counts per tier ───────────────────────────────────────────────────

	/** Number of mods rolled for a Magic monster [min, max] */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod Counts",
		meta = (ClampMin = 0))
	int32 MagicModMin = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod Counts",
		meta = (ClampMin = 0))
	int32 MagicModMax = 2;

	/** Number of mods rolled for a Rare monster [min, max] */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod Counts",
		meta = (ClampMin = 0))
	int32 RareModMin = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod Counts",
		meta = (ClampMin = 0))
	int32 RareModMax = 6;

	// ── Pack sizes ────────────────────────────────────────────────────────────

	/**
	 * Minimum and maximum monsters per pack for a Normal-tier encounter.
	 * Magic/Rare packs use separate entries below.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack Sizes",
		meta = (ClampMin = 1))
	int32 NormalPackMin = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack Sizes",
		meta = (ClampMin = 1))
	int32 NormalPackMax = 8;

	/**
	 * Pack size when the pack leader is Magic.
	 * Followers are always Normal tier.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack Sizes",
		meta = (ClampMin = 1))
	int32 MagicPackMin = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack Sizes",
		meta = (ClampMin = 1))
	int32 MagicPackMax = 5;

	/**
	 * Pack size when the pack leader is Rare.
	 * One Rare leader, rest are Normal.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack Sizes",
		meta = (ClampMin = 1))
	int32 RarePackMin = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack Sizes",
		meta = (ClampMin = 1))
	int32 RarePackMax = 4;

	// ── Per-area-level scaling ────────────────────────────────────────────────

	/**
	 * For each area level above 1, add this value to BaseMagicChance.
	 * E.g. 0.001 = area 50 adds +4.9% to magic chance.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Area Scaling",
		meta = (ClampMin = 0.0f))
	float MagicChancePerAreaLevel = 0.001f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Area Scaling",
		meta = (ClampMin = 0.0f))
	float RareChancePerAreaLevel = 0.0005f;

	// ── DataTable reference ───────────────────────────────────────────────────

	/**
	 * The DataTable of FMonsterModRow entries used when rolling mods.
	 * Must be a DataTable with row type FMonsterModRow.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TSoftObjectPtr<UDataTable> ModifierTable;

	// ── Helpers ───────────────────────────────────────────────────────────────

	/** Calculate effective magic chance for a given area level and MagicFind value. */
	UFUNCTION(BlueprintPure, Category = "Spawn Config")
	float GetEffectiveMagicChance(int32 AreaLevel, float MagicFind) const;

	/** Calculate effective rare chance for a given area level and MagicFind value. */
	UFUNCTION(BlueprintPure, Category = "Spawn Config")
	float GetEffectiveRareChance(int32 AreaLevel, float MagicFind) const;

	/** Roll a tier for a new spawn based on area level and player MagicFind. */
	UFUNCTION(BlueprintPure, Category = "Spawn Config")
	EMonsterTier RollMonsterTier(int32 AreaLevel, float MagicFind) const;

	/** Roll the pack size for a given tier. */
	UFUNCTION(BlueprintPure, Category = "Spawn Config")
	int32 RollPackSize(EMonsterTier Tier) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("MonsterSpawnConfig"), GetFName());
	}
};

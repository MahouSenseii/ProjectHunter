// Data/MonsterModifierData.h
// Monster Modifier System — global spawn config data asset.
//
// UMonsterSpawnConfig is defined here — one per game, referenced by GameMode or
// WorldSettings. It controls tier spawn chances, mod counts, and pack sizes.
//
// All enum and struct types (EMonsterTier, EMonsterModType, FMonsterStatOverride,
// FMonsterModRow) have been moved to AI/Library/MobEnumLibrary.h and
// AI/Library/MobStructs.h to break the circular include that previously forced
// every system needing EMonsterTier to pull in the full data-asset header.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "AI/Library/MobEnumLibrary.h"
#include "AI/Library/MobStructs.h"
#include "MonsterModifierData.generated.h"

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
	 * Per-point-of-MagicFind how much the Magic and Rare chances increase.
	 * E.g. 0.001 = each MF point adds 0.1% to both chances.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tier Chances",
		meta = (ClampMin = 0.0f))
	float MagicFindChanceScalar = 0.001f;

	// ── Mod counts per tier ───────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod Counts",
		meta = (ClampMin = 0))
	int32 MagicModMin = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod Counts",
		meta = (ClampMin = 0))
	int32 MagicModMax = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod Counts",
		meta = (ClampMin = 0))
	int32 RareModMin = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod Counts",
		meta = (ClampMin = 0))
	int32 RareModMax = 6;

	// ── Pack sizes ────────────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack Sizes",
		meta = (ClampMin = 1))
	int32 NormalPackMin = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack Sizes",
		meta = (ClampMin = 1))
	int32 NormalPackMax = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack Sizes",
		meta = (ClampMin = 1))
	int32 MagicPackMin = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack Sizes",
		meta = (ClampMin = 1))
	int32 MagicPackMax = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack Sizes",
		meta = (ClampMin = 1))
	int32 RarePackMin = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack Sizes",
		meta = (ClampMin = 1))
	int32 RarePackMax = 4;

	// ── Per-area-level scaling ────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Area Scaling",
		meta = (ClampMin = 0.0f))
	float MagicChancePerAreaLevel = 0.001f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Area Scaling",
		meta = (ClampMin = 0.0f))
	float RareChancePerAreaLevel = 0.0005f;

	// ── DataTable reference ───────────────────────────────────────────────────

	/** The DataTable of FMonsterModRow entries used when rolling mods. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TSoftObjectPtr<UDataTable> ModifierTable;

	// ── Helpers ───────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "Spawn Config")
	float GetEffectiveMagicChance(int32 AreaLevel, float MagicFind) const;

	UFUNCTION(BlueprintPure, Category = "Spawn Config")
	float GetEffectiveRareChance(int32 AreaLevel, float MagicFind) const;

	UFUNCTION(BlueprintPure, Category = "Spawn Config")
	EMonsterTier RollMonsterTier(int32 AreaLevel, float MagicFind) const;

	UFUNCTION(BlueprintPure, Category = "Spawn Config")
	int32 RollPackSize(EMonsterTier Tier) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("MonsterSpawnConfig"), GetFName());
	}
};

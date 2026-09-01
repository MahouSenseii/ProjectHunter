// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RunSeedFunctionLibrary.generated.h"

/**
 * Pure seed math. Every function is deterministic, side-effect free, and safe
 * to call from Blueprint so Dungeon Architect floor configuration can pull the
 * same FloorSeed the C++ spawner uses.
 */
UCLASS()
class ALS_PROJECTHUNTER_API URunSeedFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Derives a stable seed from a parent, case-insensitive label, and index.
	 * Label text is hashed instead of process-local FName IDs so results survive
	 * restarts. Returns 1..MAX_int32; run/spawn callers reserve zero for unseeded use.
	 */
	UFUNCTION(BlueprintPure, Category = "Run|Seed")
	static int32 DeriveSeed(int32 ParentSeed, const FName Label, int32 Index = 0);

	/** Seed for one floor's layout, biome, and modifier roll. */
	UFUNCTION(BlueprintPure, Category = "Run|Seed")
	static int32 DeriveFloorSeed(int32 RunSeed, int32 FloorNumber);

	/** Seed for one floor's layout geometry, kept apart from encounter and loot draws. */
	UFUNCTION(BlueprintPure, Category = "Run|Seed")
	static int32 DeriveLayoutSeed(int32 FloorSeed);

	/** Seed for one encounter/spawn group on a floor. */
	UFUNCTION(BlueprintPure, Category = "Run|Seed")
	static int32 DeriveEncounterSeed(int32 FloorSeed, int32 EncounterIndex);

	/** Seed for one monster within an encounter. */
	UFUNCTION(BlueprintPure, Category = "Run|Seed")
	static int32 DeriveMonsterSeed(int32 EncounterSeed, int32 MonsterIndex);

	/** Seed for a monster's tier and modifier rolls. */
	UFUNCTION(BlueprintPure, Category = "Run|Seed")
	static int32 DeriveModifierSeed(int32 MonsterSeed);

	/** Seed for a floor's reward roll. */
	UFUNCTION(BlueprintPure, Category = "Run|Seed")
	static int32 DeriveRewardSeed(int32 FloorSeed);

	/** Seed for one loot drop produced by a reward. */
	UFUNCTION(BlueprintPure, Category = "Run|Seed")
	static int32 DeriveLootSeed(int32 RewardSeed, int32 DropIndex);

	/** Convenience: a ready-to-use stream for one floor's layout generation. */
	static FRandomStream MakeLayoutStream(int32 FloorSeed);

	/** Convenience: a ready-to-use stream for one floor. */
	static FRandomStream MakeFloorStream(int32 RunSeed, int32 FloorNumber);

	/** Convenience: a ready-to-use stream for one monster's modifier roll. */
	static FRandomStream MakeModifierStream(int32 MonsterSeed);
};

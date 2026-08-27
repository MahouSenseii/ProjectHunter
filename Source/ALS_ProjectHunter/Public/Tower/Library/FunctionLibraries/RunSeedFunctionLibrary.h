// Deterministic seed derivation for a tower run.
//
// One RunSeed must reproduce an entire run's procedural structure: floor
// layouts, encounter composition, monster tiers, monster modifiers, and reward
// or loot rolls. Every one of those decisions derives its stream from this
// chain rather than calling the global FMath RNG, so the same RunSeed plus the
// same inputs always produces the same run.
//
//   RunSeed
//     -> FloorSeed(Floor)
//          -> EncounterSeed(EncounterIndex)
//               -> MonsterSeed(MonsterIndex)
//                    -> ModifierSeed
//          -> RewardSeed
//               -> LootSeed(DropIndex)
//
// Moment-to-moment AI (pathing jitter, attack selection) deliberately does NOT
// use this - only decisions that define the shape of the run.
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
	 * Folds a label and an index into a parent seed.
	 *
	 * The label keeps sibling streams apart: without it, EncounterSeed(S, 1) and
	 * MonsterSeed(S, 1) would collide and two unrelated systems would draw the
	 * same numbers. Result is always non-zero, because FRandomStream treats 0 as
	 * "seed me from the global RNG" and would silently become non-deterministic.
	 */
	UFUNCTION(BlueprintPure, Category = "Run|Seed")
	static int32 DeriveSeed(int32 ParentSeed, const FName Label, int32 Index = 0);

	/** Seed for one floor's layout, biome, and modifier roll. */
	UFUNCTION(BlueprintPure, Category = "Run|Seed")
	static int32 DeriveFloorSeed(int32 RunSeed, int32 FloorNumber);

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

	/** Convenience: a ready-to-use stream for one floor. */
	static FRandomStream MakeFloorStream(int32 RunSeed, int32 FloorNumber);

	/** Convenience: a ready-to-use stream for one monster's modifier roll. */
	static FRandomStream MakeModifierStream(int32 MonsterSeed);
};

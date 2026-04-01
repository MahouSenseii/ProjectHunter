// AI/Mob/MobSpawnTypes.h
// All structs and enums used by the Mob Manager / Spawner system.
//
// Kept in a separate header so Blueprint function libraries and other systems
// can include just the types without pulling in the full AMobManagerActor.
#pragma once

#include "CoreMinimal.h"
#include "Character/PHBaseCharacter.h"
#include "MobSpawnTypes.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// Enums
// ─────────────────────────────────────────────────────────────────────────────

/** Why a spawn attempt failed — used for debug visualisation and analytics. */
UENUM(BlueprintType)
enum class EMobSpawnFailReason : uint8
{
	None            UMETA(DisplayName = "None"),
	NoMobClasses    UMETA(DisplayName = "No Mob Classes Configured"),
	AllOnCooldown   UMETA(DisplayName = "All Mob Types On Cooldown"),
	NoValidLocation UMETA(DisplayName = "No Valid Location Found"),
	NavMeshFailed   UMETA(DisplayName = "NavMesh Projection Failed"),
	CollisionFailed UMETA(DisplayName = "Collision Check Failed"),
	DistanceFailed  UMETA(DisplayName = "Too Close To Player"),
	GroundFailed    UMETA(DisplayName = "Ground Trace Failed"),
	SpawnFailed     UMETA(DisplayName = "Actor Spawn Failed"),
};

/** Runtime state of the manager. */
UENUM(BlueprintType)
enum class EMobManagerState : uint8
{
	Active      UMETA(DisplayName = "Active"),
	Paused      UMETA(DisplayName = "Paused"),
	Disabled    UMETA(DisplayName = "Disabled"),
};

// ─────────────────────────────────────────────────────────────────────────────
// FMobTypeEntry — one slot in the MobTypes array
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Defines one enemy type the manager can spawn, plus its weighting and cooldown.
 *
 * WEIGHT:
 *   Relative chance of being picked.  Higher = spawns more often.
 *   e.g. Goblin(weight=100), Wolf(weight=50), Boss(weight=10).
 *   Probability = ThisWeight / SumOfAllEligibleWeights.
 *
 * SPAWN COOLDOWN:
 *   Minimum seconds between successive spawns of this specific type.
 *   0.0 = no cooldown (type can be picked every interval).
 *   Useful to throttle rare or "boss-like" entries.
 */
USTRUCT(BlueprintType)
struct FMobTypeEntry
{
	GENERATED_BODY()

	/** Enemy class to spawn.  Must be a subclass of APHBaseCharacter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Type")
	TSubclassOf<APHBaseCharacter> MobClass;

	/**
	 * Relative spawn weight.  Minimum 1.
	 * Entries with weight 0 are skipped.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Type",
		meta = (ClampMin = 0, UIMin = 1))
	int32 SpawnWeight = 100;

	/**
	 * Per-type cooldown in seconds between successive spawns of this class.
	 * 0.0 = no cooldown.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Type",
		meta = (ClampMin = 0.0f))
	float SpawnCooldown = 0.0f;

	// ── Runtime (not exposed in Details, reset on manager restart) ────────────

	/** World time of the last successful spawn for this entry. -1 = never. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Mob Type|Runtime")
	float LastSpawnTime = -1.0f;

	/** True if this entry has a class and a non-zero weight. */
	bool IsEligible(float CurrentTime) const
	{
		if (!MobClass || SpawnWeight <= 0) { return false; }
		if (SpawnCooldown <= 0.0f)         { return true;  }
		return (LastSpawnTime < 0.0f) || ((CurrentTime - LastSpawnTime) >= SpawnCooldown);
	}
};

// ─────────────────────────────────────────────────────────────────────────────
// FSpawnAttemptDebug — one entry in the visual debug history
// ─────────────────────────────────────────────────────────────────────────────

/** Recorded result of a single spawn attempt, shown in the debug visualiser. */
USTRUCT(BlueprintType)
struct FSpawnAttemptDebug
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	EMobSpawnFailReason FailReason = EMobSpawnFailReason::None;

	/** GetWorld()->GetTimeSeconds() at the moment of the attempt. */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	float Timestamp = 0.0f;
};

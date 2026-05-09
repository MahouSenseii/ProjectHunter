// AI/Library/MobEnumLibrary.h
// All enums for the Mob Manager / Spawner / Modifier system.
//
// Dependency rule: no project includes — only CoreMinimal.
// Include this alone when you need to pass or switch on mob / spawn enums
// without pulling in the full manager, rules, or data-asset headers.

#pragma once

#include "CoreMinimal.h"
#include "MobEnumLibrary.generated.h"

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
// Mod type — controls how a modifier row is formatted in display names
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EMonsterModType : uint8
{
	MMT_Prefix  UMETA(DisplayName = "Prefix"),
	MMT_Suffix  UMETA(DisplayName = "Suffix"),
};

// ─────────────────────────────────────────────────────────────────────────────
// Spawn-fail reason — used for debug visualisation and analytics
// ─────────────────────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
// Manager runtime state
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EMobManagerState : uint8
{
	Active      UMETA(DisplayName = "Active"),
	Paused      UMETA(DisplayName = "Paused"),
	Disabled    UMETA(DisplayName = "Disabled"),
};

// ─────────────────────────────────────────────────────────────────────────────
// Special-spawn rule trigger source
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EMobSpawnTriggerType : uint8
{
	/** Fires when any tracked player is carrying a specific key item. */
	PlayerHasKeyItem    UMETA(DisplayName = "Player Has Key Item"),

	/** Fires when the manager's kill count for a mob class reaches the threshold. */
	KillCountReached    UMETA(DisplayName = "Kill Count Reached"),
};

// ─────────────────────────────────────────────────────────────────────────────
// Special-spawn rule action
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EMobSpawnRuleAction : uint8
{
	/** Immediately spawns SpecialMobClass via the normal spawn pipeline. */
	ForceSpawnSpecialMob    UMETA(DisplayName = "Force Spawn Special Mob"),

	/** Boosts the tier of the next normal spawn to ForcedTier. */
	BoostNextSpawnTier      UMETA(DisplayName = "Boost Next Spawn Tier"),
};

// ─────────────────────────────────────────────────────────────────────────────
// Special-spawn rule lifetime
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EMobSpawnRuleLifetime : uint8
{
	/** Fires exactly once per manager instance, then disables itself. */
	OneShot                   UMETA(DisplayName = "One Shot"),

	/** Fires, cools down for CooldownSeconds, then can fire again. */
	RepeatableWithCooldown    UMETA(DisplayName = "Repeatable With Cooldown"),
};

#pragma once

#include "CoreMinimal.h"
#include "RunEnumLibrary.generated.h"

UENUM(BlueprintType)
enum class ERunState : uint8
{
	Inactive  UMETA(DisplayName = "Inactive"),
	Active    UMETA(DisplayName = "Active"),
	Ended     UMETA(DisplayName = "Ended")
};

UENUM(BlueprintType)
enum class ERunEndReason : uint8
{
	None         UMETA(DisplayName = "None"),
	PlayerDeath  UMETA(DisplayName = "Player Death"),
	Quit         UMETA(DisplayName = "Quit"),
	Completed    UMETA(DisplayName = "Completed"),
	Extracted    UMETA(DisplayName = "Extracted"),
	Disconnect   UMETA(DisplayName = "Disconnected"),
	InvalidRun   UMETA(DisplayName = "Invalid Run"),
	/** Every player in the party is out with no revive available. */
	PartyWipe    UMETA(DisplayName = "Party Wipe")
};

/**
 * Base shape of a floor. Dungeon Architect randomises layout, rooms, paths and
 * spawn points within a type; the type decides the rules and reward contract.
 * Only Combat is expected to be fully playable at first - the rest exist so the
 * data model does not need reshaping when they are authored.
 */
UENUM(BlueprintType)
enum class EFloorType : uint8
{
	Combat    UMETA(DisplayName = "Combat"),
	Elite     UMETA(DisplayName = "Elite"),
	Boss      UMETA(DisplayName = "Boss"),
	Treasure  UMETA(DisplayName = "Treasure"),
	Survival  UMETA(DisplayName = "Survival"),
	Event     UMETA(DisplayName = "Event"),
	Puzzle    UMETA(DisplayName = "Puzzle")
};

/** What clearing the current floor requires. */
UENUM(BlueprintType)
enum class EFloorObjective : uint8
{
	ClearAllEnemies UMETA(DisplayName = "Clear All Enemies"),
	KillBoss        UMETA(DisplayName = "Kill Boss"),
	ReachExit       UMETA(DisplayName = "Reach Exit"),
	SurviveDuration UMETA(DisplayName = "Survive Duration"),
	CollectObjective UMETA(DisplayName = "Collect Objective")
};

/**
 * Where the current floor sits in the run loop:
 * Generating -> InProgress -> ObjectiveComplete -> RewardReady -> Transitioning.
 */
UENUM(BlueprintType)
enum class EFloorPhase : uint8
{
	None              UMETA(DisplayName = "None"),
	Generating        UMETA(DisplayName = "Generating"),
	InProgress        UMETA(DisplayName = "In Progress"),
	ObjectiveComplete UMETA(DisplayName = "Objective Complete"),
	RewardReady       UMETA(DisplayName = "Reward Ready"),
	Transitioning     UMETA(DisplayName = "Transitioning")
};

/**
 * Per-player run status. The party floor is owned by RunSubsystem; this is the
 * individual axis that decides whether a run continues after a death.
 *
 *   Alive -> Downed -> (revived) Alive
 *                   -> (not revived) Dead -> Out
 *   All players Out -> party wipe -> EndRun.
 */
UENUM(BlueprintType)
enum class ERunPlayerState : uint8
{
	/** Normal play. */
	Alive     UMETA(DisplayName = "Alive"),
	/** Downed but still revivable by a teammate. */
	Downed    UMETA(DisplayName = "Downed"),
	/** Dead and no longer revivable this floor, but still occupying a party slot. */
	Dead      UMETA(DisplayName = "Dead"),
	/** Out of the run entirely - spectating until the run ends. */
	Out       UMETA(DisplayName = "Out")
};

// GameModes/Library/GameModeEnumLibrary.h


#pragma once

#include "CoreMinimal.h"
#include "GameModeEnumLibrary.generated.h"

/** High-level match phases replicated to all clients via APHGameState. */
UENUM(BlueprintType)
enum class EPHMatchPhase : uint8
{
	/** Lobby / character select — no active gameplay. */
	WaitingToStart    UMETA(DisplayName = "Waiting To Start"),

	/** Active gameplay in progress. */
	InProgress        UMETA(DisplayName = "In Progress"),

	/** Boss killed / objective complete — victory state. */
	Completed         UMETA(DisplayName = "Completed"),

	/** All players dead / failed — failure state. */
	Failed            UMETA(DisplayName = "Failed"),
};

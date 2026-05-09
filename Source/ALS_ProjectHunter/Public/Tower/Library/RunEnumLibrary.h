// Tower/Library/RunEnumLibrary.h
// Shared enums for the roguelite run system.
// Include this alone when you only need to branch on run state
// without pulling in the full subsystem.

#pragma once

#include "CoreMinimal.h"
#include "RunEnumLibrary.generated.h"

/**
 * Represents the current state of a roguelite run.
 * Read via URunFunctionLibrary::GetRunState() to avoid a direct subsystem dependency.
 */
UENUM(BlueprintType)
enum class ERunState : uint8
{
	Inactive  UMETA(DisplayName = "Inactive"),  // No run in progress
	Active    UMETA(DisplayName = "Active"),    // Run is live
	Ended     UMETA(DisplayName = "Ended")      // Run finished, session data available
};

/**
 * Why the run ended. Passed with OnRunEnded for analytics and UI.
 * Extend this as new end conditions are added (e.g. Victory, Timeout).
 */
UENUM(BlueprintType)
enum class ERunEndReason : uint8
{
	PlayerDeath  UMETA(DisplayName = "Player Death"),
	Quit         UMETA(DisplayName = "Quit")
};

// Tower/Library/RunStructs.h
// Shared structs for the roguelite run system.
// Include this alone when you need to receive or pass run data
// (e.g. end-of-run screen, analytics) without a full subsystem dependency.

#pragma once

#include "CoreMinimal.h"
#include "Tower/Library/RunEnumLibrary.h"
#include "RunStructs.generated.h"

/**
 * Display-only snapshot of a completed or in-progress run.
 * Carried by OnRunEnded and readable via URunFunctionLibrary.
 *
 * Never use this to drive gameplay logic — read live values
 * from URunSubsystem directly for anything that needs to be authoritative.
 */
USTRUCT(BlueprintType)
struct FRunSessionData
{
	GENERATED_BODY()

	/** How many floors the player fully cleared this run. */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 FloorsCleared = 0;

	/** Total monster kills recorded across all floors this run. */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 TotalKills = 0;

	/** Real-time seconds elapsed from StartRun to EndRun. */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	float TimeElapsed = 0.f;

	/** Why the run ended. Set by EndRun before broadcasting OnRunEnded. */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	ERunEndReason EndReason = ERunEndReason::PlayerDeath;
};

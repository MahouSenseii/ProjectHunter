#pragma once

#include "CoreMinimal.h"
#include "Tower/Library/Enums/RunEnumLibrary.h"
#include "RunStructs.generated.h"

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FRunSessionData
{
	GENERATED_BODY()

	/** Stable identity for save/reconnect/reward bookkeeping. */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	FGuid RunID;

	/** Seed used by floor generation, encounter rolls, and reward generation. */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 RunSeed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 Difficulty = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 CurrentFloor = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 FloorsCleared = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 TotalKills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	float TimeElapsed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	ERunEndReason EndReason = ERunEndReason::None;
};

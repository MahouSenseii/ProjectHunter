#pragma once

#include "CoreMinimal.h"
#include "Tower/Library/Enums/RunEnumLibrary.h"
#include "RunStructs.generated.h"

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FRunSessionData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 FloorsCleared = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 TotalKills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	float TimeElapsed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	ERunEndReason EndReason = ERunEndReason::PlayerDeath;
};

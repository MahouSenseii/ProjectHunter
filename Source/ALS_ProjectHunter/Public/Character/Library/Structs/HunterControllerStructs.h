#pragma once

#include "CoreMinimal.h"
#include "HunterControllerStructs.generated.h"

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FDoOnceState
{
	GENERATED_BODY()

	bool bHasBeenInitialized = false;
	bool bIsClosed = false;

	FDoOnceState() = default;
};

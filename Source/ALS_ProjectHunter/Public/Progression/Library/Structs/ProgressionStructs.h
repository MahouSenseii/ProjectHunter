#pragma once

#include "CoreMinimal.h"
#include "ProgressionStructs.generated.h"

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FStatPointSpending
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName AttributeName;

	UPROPERTY(BlueprintReadOnly)
	int32 PointsSpent = 0;

	FStatPointSpending()
		: AttributeName(NAME_None)
		, PointsSpent(0)
	{}

	FStatPointSpending(FName InAttributeName, int32 InPointsSpent)
		: AttributeName(InAttributeName)
		, PointsSpent(InPointsSpent)
	{}
};

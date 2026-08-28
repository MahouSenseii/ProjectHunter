#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "TagStructs.generated.h"

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FTagConditionThresholds
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Conditions", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowResourceEnterPercent = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Conditions", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowResourceExitPercent = 0.40f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Conditions", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FullResourceEnterPercent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Conditions", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FullResourceExitPercent = 0.995f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Conditions", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MovementStartSpeed = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Conditions", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MovementStopSpeed = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Conditions", meta = (ClampMin = "0.01", Units = "s"))
	float MovementRefreshInterval = 0.1f;
};

struct FTagAttributeDelegateBinding
{
	FGameplayAttribute Attribute;
	FDelegateHandle Handle;
};

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ItemPowerSettings.generated.h"

/** Project-wide thresholds used to map total item power to the displayed F-SS grade. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Item Power Grades"))
class ALS_PROJECTHUNTER_API UItemPowerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Grade Thresholds", meta = (ClampMin = "0.0"))
	float GradeE = 20.0f;

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Grade Thresholds", meta = (ClampMin = "0.0"))
	float GradeD = 35.0f;

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Grade Thresholds", meta = (ClampMin = "0.0"))
	float GradeC = 55.0f;

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Grade Thresholds", meta = (ClampMin = "0.0"))
	float GradeB = 80.0f;

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Grade Thresholds", meta = (ClampMin = "0.0"))
	float GradeA = 110.0f;

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Grade Thresholds", meta = (ClampMin = "0.0"))
	float GradeS = 150.0f;

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Grade Thresholds", meta = (ClampMin = "0.0"))
	float GradeSS = 200.0f;
};

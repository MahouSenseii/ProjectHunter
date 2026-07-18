#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProgressionStatFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API UProgressionStatFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Progression|Stats")
	static FGameplayAttribute GetAttributeForStatName(FName StatName);
};

#pragma once

#include "CoreMinimal.h"
#include "PHCharacterStructLibrary.generated.h"

USTRUCT(BlueprintType)
struct FPHLevelUpInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	int32 LevelUpRequirement = 0;

	UPROPERTY(EditDefaultsOnly)
	int32 AttributePointAward = 1;
	
};

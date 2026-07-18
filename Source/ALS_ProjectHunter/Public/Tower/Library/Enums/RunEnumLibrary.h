#pragma once

#include "CoreMinimal.h"
#include "RunEnumLibrary.generated.h"

UENUM(BlueprintType)
enum class ERunState : uint8
{
	Inactive  UMETA(DisplayName = "Inactive"),
	Active    UMETA(DisplayName = "Active"),
	Ended     UMETA(DisplayName = "Ended")
};

UENUM(BlueprintType)
enum class ERunEndReason : uint8
{
	PlayerDeath  UMETA(DisplayName = "Player Death"),
	Quit         UMETA(DisplayName = "Quit")
};

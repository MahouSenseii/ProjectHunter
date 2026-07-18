#pragma once

#include "CoreMinimal.h"
#include "StashEnumLibrary.generated.h"

UENUM(BlueprintType)
enum class EStashTabType : uint8
{
	STT_Normal      UMETA(DisplayName = "Normal (Grid)"),
	STT_Quad        UMETA(DisplayName = "Quad (Large Grid)"),
	STT_Currency    UMETA(DisplayName = "Currency (Auto-Sort)"),
	STT_Premium     UMETA(DisplayName = "Premium (Colour/Named)"),
};

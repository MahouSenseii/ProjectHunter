// Interactable/Library/Enums/ChestEnums.h
#pragma once

#include "CoreMinimal.h"
#include "ChestEnums.generated.h"

UENUM(BlueprintType)
enum class EChestState : uint8
{
	CS_Closed UMETA(DisplayName = "Closed"),
	CS_Opening UMETA(DisplayName = "Opening"),
	CS_Open UMETA(DisplayName = "Open"),
	CS_Looted UMETA(DisplayName = "Looted"),
	CS_Closing UMETA(DisplayName = "Closing"),
	CS_Respawning UMETA(DisplayName = "Respawning")
};

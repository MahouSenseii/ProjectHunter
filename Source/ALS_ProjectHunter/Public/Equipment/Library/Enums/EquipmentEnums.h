#pragma once

#include "CoreMinimal.h"
#include "EquipmentEnums.generated.h"

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	ES_None UMETA(DisplayName = "None"),
	ES_MainHand UMETA(DisplayName = "Main Hand"),
	ES_OffHand UMETA(DisplayName = "Off Hand"),
	ES_TwoHand UMETA(DisplayName = "Two Hand"),
	ES_Head UMETA(DisplayName = "Head"),
	ES_Chest UMETA(DisplayName = "Chest"),
	ES_Hands UMETA(DisplayName = "Hands"),
	ES_Legs UMETA(DisplayName = "Legs"),
	ES_Feet UMETA(DisplayName = "Feet"),
	ES_Ring1 UMETA(DisplayName = "Ring 1"),
	ES_Ring2 UMETA(DisplayName = "Ring 2"),
	ES_Ring3 UMETA(DisplayName = "Ring 3"),
	ES_Ring4 UMETA(DisplayName = "Ring 4"),
	ES_Ring5 UMETA(DisplayName = "Ring 5"),
	ES_Ring6 UMETA(DisplayName = "Ring 6"),
	ES_Ring7 UMETA(DisplayName = "Ring 7"),
	ES_Ring8 UMETA(DisplayName = "Ring 8"),
	ES_Ring9 UMETA(DisplayName = "Ring 9"),
	ES_Ring10 UMETA(DisplayName = "Ring 10"),
	ES_Amulet UMETA(DisplayName = "Amulet"),
	ES_Belt UMETA(DisplayName = "Belt"),
};


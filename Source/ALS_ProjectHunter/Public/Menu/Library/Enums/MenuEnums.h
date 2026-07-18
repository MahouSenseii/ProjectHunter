#pragma once

#include "CoreMinimal.h"
#include "MenuEnums.generated.h"

UENUM(BlueprintType)
enum class EMenuType : uint8
{
	MT_None UMETA(DisplayName = "None"),
	MT_Equipment UMETA(DisplayName = "Equipment"),
	MT_Stats UMETA(DisplayName = "Stats"),
	MT_PassiveTree UMETA(DisplayName = "Passive Tree"),
	MT_Settings UMETA(DisplayName = "Settings")
};

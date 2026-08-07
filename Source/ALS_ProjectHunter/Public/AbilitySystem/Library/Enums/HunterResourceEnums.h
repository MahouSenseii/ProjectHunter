#pragma once

#include "CoreMinimal.h"
#include "HunterResourceEnums.generated.h"

UENUM(BlueprintType)
enum class EHunterResourceType : uint8
{
	Health UMETA(DisplayName = "Health"),
	Stamina UMETA(DisplayName = "Stamina"),
	Mana UMETA(DisplayName = "Mana"),
	ArcaneShield UMETA(DisplayName = "Arcane Shield"),
};

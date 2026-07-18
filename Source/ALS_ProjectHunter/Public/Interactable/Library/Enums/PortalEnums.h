// Interactable/Library/Enums/PortalEnums.h
#pragma once

#include "CoreMinimal.h"
#include "PortalEnums.generated.h"

UENUM(BlueprintType)
enum class EPortalState : uint8
{
	PS_Inactive UMETA(DisplayName = "Inactive"),
	PS_Active UMETA(DisplayName = "Active"),
	PS_Cooldown UMETA(DisplayName = "Cooldown"),
	PS_Disabled UMETA(DisplayName = "Disabled")
};

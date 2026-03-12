// Character/Component/Library/InteractionDebugEnumLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "InteractionDebugEnumLibrary.generated.h"

UENUM(BlueprintType)
enum class EInteractionDebugMode : uint8
{
	/** No debug visualization */
	None       UMETA(DisplayName = "None"),
    
	/** Basic - Trace lines only */
	Basic      UMETA(DisplayName = "Basic"),
    
	/** Detailed - Trace lines + hit points + distances */
	Detailed   UMETA(DisplayName = "Detailed"),
    
	/** Full - Everything including stats */
	Full       UMETA(DisplayName = "Full")
};
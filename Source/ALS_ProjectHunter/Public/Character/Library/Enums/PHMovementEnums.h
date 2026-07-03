#pragma once

#include "CoreMinimal.h"
#include "PHMovementEnums.generated.h"

/**
 * Custom movement modes for ProjectHunter traversal. Values map directly to
 * UCharacterMovementComponent::CustomMovementMode (uint8), so existing values
 * must not be reordered once content references them.
 */
UENUM(BlueprintType)
enum class EPHCustomMovementMode : uint8
{
	None = 0 UMETA(Hidden),
	WallRunning = 1 UMETA(DisplayName = "Wall Running"),
	WallClimbing = 2 UMETA(DisplayName = "Wall Climbing"),
	Gliding = 3 UMETA(DisplayName = "Gliding"),
	WallToGround = 4 UMETA(DisplayName = "Wall To Ground")
};

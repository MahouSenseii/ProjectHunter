// AbilitySystem/Library/MMCEnumLibrary.h
// Resource type enums shared by Effective and Reserved MMC calculations.
//
// Dependency rule: no project includes — only CoreMinimal.
// Both UHunterMMC_EffectiveResource and UHunterMMC_ReservedResource use a
// resource-type enum to dispatch to the correct attribute capture. They are
// kept here so neither MMC header needs to include the other.

#pragma once

#include "CoreMinimal.h"
#include "MMCEnumLibrary.generated.h"

/**
 * Which resource pool is being targeted.
 * Used by both the MMC calculations (EffectiveResource, ReservedResource)
 * and the HUD widget (HunterHUDResourceWidget) so that both reference one
 * canonical definition.
 *
 * IMPORTANT: Do NOT reorder these values — Blueprint assets may bind by index.
 */
UENUM(BlueprintType)
enum class EHunterResourceType : uint8
{
	Health   UMETA(DisplayName = "Health"),
	Stamina  UMETA(DisplayName = "Stamina"),
	Mana     UMETA(DisplayName = "Mana"),
};

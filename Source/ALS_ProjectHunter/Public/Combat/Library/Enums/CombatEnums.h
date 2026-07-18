// Shared combat enums used across ProjectHunter combat systems.
#pragma once

#include "CoreMinimal.h"
#include "CombatEnums.generated.h"

/**
 * Damage types resolved by the combat pipeline.
 * Fire, Ice, Lightning, and Light form the elemental group. Physical mitigates
 * through Armour; every other type mitigates through its resistance.
 */
UENUM(BlueprintType)
enum class EHunterDamageType : uint8
{
	Physical UMETA(DisplayName = "Physical"),
	Fire UMETA(DisplayName = "Fire"),
	Ice UMETA(DisplayName = "Ice"),
	Lightning UMETA(DisplayName = "Lightning"),
	Light UMETA(DisplayName = "Light"),
	Corruption UMETA(DisplayName = "Corruption")
};

/**
 * How an incoming hit was resolved by the target.
 * Parry keeps ailment rolls alive, Invincible negates everything, and Blocked
 * absorbs damage through a resource while still allowing contact effects.
 */
UENUM(BlueprintType)
enum class EHitResponse : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Parry UMETA(DisplayName = "Parry"),
	Invincible UMETA(DisplayName = "Invincible"),
	Blocked UMETA(DisplayName = "Blocked")
};

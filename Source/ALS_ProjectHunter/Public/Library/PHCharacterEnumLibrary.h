#pragma once

#include "CoreMinimal.h"
#include "PHCharacterEnumLibrary.generated.h"

UENUM(BlueprintType)
enum class ECombatAlignment : uint8
{
	None UMETA(DisplayName = "None"),       // Neutral or player character
	Enemy UMETA(DisplayName = "Enemy"),    // Hostile character
	Ally UMETA(DisplayName = "Ally")       // Friendly character
};


/**
 * Combat status for tracking whether a character is engaged in combat
 */
UENUM(BlueprintType)
enum class ECombatStatus : uint8
{
	OutOfCombat UMETA(DisplayName = "Out of Combat"),
	InCombat UMETA(DisplayName = "In Combat"),
	EnteringCombat UMETA(DisplayName = "Entering Combat"),  // Transition state
	LeavingCombat UMETA(DisplayName = "Leaving Combat")     // Transition state
};

UENUM(BlueprintType)
enum class EAttackType : uint8
{
	AT_None       UMETA(DisplayName = "None"),
	AT_Melee      UMETA(DisplayName = "Melee"),
	AT_Ranged     UMETA(DisplayName = "Ranged"),
	AT_Spell      UMETA(DisplayName = "Spell")
};

UENUM(BlueprintType)
enum class EVitalType: uint8
{
	VT_None       UMETA(DisplayName = "None"),
	VT_Health      UMETA(DisplayName = "Health"),
	VT_Stamina     UMETA(DisplayName = "Stamina"),
	VT_Mana    UMETA(DisplayName = "Mana"),
	VT_XP    UMETA(DisplayName = "XP")
};

UENUM(BlueprintType)
enum class EVitalRegenType : uint8
{
	Health      UMETA(DisplayName = "Health"),
	Mana        UMETA(DisplayName = "Mana"),
	Stamina     UMETA(DisplayName = "Stamina"),
	ArcaneShield UMETA(DisplayName = "Arcane Shield")
};

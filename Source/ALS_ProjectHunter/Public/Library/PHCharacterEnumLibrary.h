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
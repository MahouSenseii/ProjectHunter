// Data/Library/BaseStatsEnumLibrary.h
// Stat category enums for the BaseStats data system.
//
// Dependency rule: no project includes — only CoreMinimal.
// Include this alone when you need EHunterStatType without pulling in
// the full BaseStatsData asset header.

#pragma once

#include "CoreMinimal.h"
#include "BaseStatsEnumLibrary.generated.h"

UENUM(BlueprintType)
enum class EHunterStatType : uint8
{
	Neutral     UMETA(DisplayName = "Neutral"),
	Primary     UMETA(DisplayName = "Primary"),
	Vital       UMETA(DisplayName = "Vital"),
	Offense     UMETA(DisplayName = "Offense"),
	Defense     UMETA(DisplayName = "Defense"),
	Resource    UMETA(DisplayName = "Resource"),
	Utility     UMETA(DisplayName = "Utility"),
	Special     UMETA(DisplayName = "Special")
};

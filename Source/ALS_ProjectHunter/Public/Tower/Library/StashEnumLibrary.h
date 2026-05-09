// Tower/Library/StashEnumLibrary.h
// Enums for the lazy-loading stash system.
//
// Dependency rule: no project includes — only CoreMinimal.
// Include this alone when you need EStashTabType without pulling in
// UStashSubsystem or the full stash struct headers.

#pragma once

#include "CoreMinimal.h"
#include "StashEnumLibrary.generated.h"

/** Tab display and sorting behaviour in the stash UI. */
UENUM(BlueprintType)
enum class EStashTabType : uint8
{
	STT_Normal      UMETA(DisplayName = "Normal (Grid)"),
	STT_Quad        UMETA(DisplayName = "Quad (Large Grid)"),
	STT_Currency    UMETA(DisplayName = "Currency (Auto-Sort)"),
	STT_Premium     UMETA(DisplayName = "Premium (Colour/Named)"),
};

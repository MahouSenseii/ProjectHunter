// Combat/Library/CombatFunctionLibrary.h
// Blueprint-callable static helpers for the combat system.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CombatFunctionLibrary.generated.h"

class APHBaseCharacter;

/**
 * Static utility functions for combat queries and calculations.
 * Use from Blueprints or C++ for common combat checks without
 * needing a reference to a specific CombatManager.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UCombatFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:


	/**
	 * Get the health percentage of a character (0.0–1.0).
	 * Returns 0 if the character is null or dead.
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Utility")
	static float GetHealthPercent(const APHBaseCharacter* Character);
};

// Blueprint-callable static helpers for shared combat queries and display data.
#pragma once

#include "CoreMinimal.h"
#include "Combat/Library/Structs/CombatStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CombatFunctionLibrary.generated.h"

class APHBaseCharacter;

UCLASS()
class ALS_PROJECTHUNTER_API UCombatFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Combat|Utility")
	static float GetHealthPercent(const APHBaseCharacter* Character);

	UFUNCTION(BlueprintPure, Category = "Combat|Damage")
	static float GetResolvedDamageByType(const FCombatResolveResult& Result, EHunterDamageType DamageType);

	UFUNCTION(BlueprintPure, Category = "Combat|Damage")
	static EHunterDamageType GetDominantDamageTypeFromResolveResult(const FCombatResolveResult& Result);

	UFUNCTION(BlueprintPure, Category = "Combat|Damage")
	static FLinearColor GetDefaultDamageTypeColor(EHunterDamageType DamageType);

	UFUNCTION(BlueprintPure, Category = "Combat|Damage")
	static FText FormatDamagePopupAmount(float DamageAmount);
};

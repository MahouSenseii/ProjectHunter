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

	/**
	 * Which side of the defender the attacker is standing on.
	 *
	 * Uses the defender's facing against the horizontal direction to the
	 * attacker, so vertical separation (a hit from above or below) does not turn
	 * a front attack into a rear one. Returns Front when either actor is invalid
	 * or the two occupy the same spot, so an ambiguous case never awards the
	 * rear bonus.
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Positional")
	static EHitDirection GetHitDirection(
		const AActor* AttackerActor,
		const AActor* DefenderActor,
		const FCombatPositionalRules& Rules);

	/** Damage ratio for a hit direction under the given rules. 1.0 when disabled. */
	UFUNCTION(BlueprintPure, Category = "Combat|Positional")
	static float GetPositionalDamageMultiplier(
		EHitDirection HitDirection,
		const FCombatPositionalRules& Rules);
};

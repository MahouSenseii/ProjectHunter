#pragma once

#include "CoreMinimal.h"

class AActor;
class UAbilitySystemComponent;
class UItemInstance;

class ALS_PROJECTHUNTER_API FItemUsageHandler
{
public:
	static void ApplyAffixesToCharacter(UItemInstance& Item, UAbilitySystemComponent* ASC);
	static void RemoveAffixesFromCharacter(UItemInstance& Item, UAbilitySystemComponent* ASC);
	static bool UseConsumable(UItemInstance& Item, AActor* Target);
	static bool CanUseConsumable(const UItemInstance& Item);
	static float GetCooldownProgress(const UItemInstance& Item);
	static bool ReduceUses(UItemInstance& Item, int32 Amount = 1);
	static bool ApplyConsumableEffects(UItemInstance& Item, AActor* Target);
};

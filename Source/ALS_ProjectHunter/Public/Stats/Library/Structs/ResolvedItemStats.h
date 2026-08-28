#pragma once

#include "CoreMinimal.h"
#include "Combat/Library/Structs/CombatStructs.h"
#include "Item/Library/Structs/ItemBaseStatStructs.h"
#include "ResolvedItemStats.generated.h"

/** Immutable weapon-local values resolved from one item base plus its local affixes. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FResolvedWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Item|Resolved Stats")
	bool bIsValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Resolved Stats")
	FBaseWeaponStats Values;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Resolved Stats")
	TArray<FCombatDamageConversionRule> LocalDamageConversions;

	float GetMinDamage(EHunterDamageType DamageType) const;
	float GetMaxDamage(EHunterDamageType DamageType) const;
};

/** Immutable armour-local values resolved from one item base plus its local affixes. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FResolvedArmorStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Item|Resolved Stats")
	bool bIsValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Resolved Stats")
	FBaseArmorStats Values;
};

// Item/Library/FunctionLibraries/ItemCalculationFunctionLibrary.h

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "Item/Library/Structs/ItemCalculationStructs.h"
#include "ItemCalculationFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API UItemCalculationFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Item|Damage", meta = (
		DisplayName = "Calculate Final Damage",
		Keywords = "damage calculate final modifier"))
	static FDamageRange CalculateFinalDamage(
		FDamageRange BaseDamage,
		float FlatAdded = 0.0f,
		float IncreasedPercent = 0.0f,
		float MorePercent = 0.0f);

	UFUNCTION(BlueprintPure, Category = "Item|Damage", meta = (
		DisplayName = "Calculate DPS",
		Keywords = "dps damage per second calculate"))
	static float CalculateDPS(FDamageRange DamageRange, float AttackSpeed);

	UFUNCTION(BlueprintPure, Category = "Item|Damage", meta = (
		DisplayName = "Calculate Critical Damage",
		Keywords = "critical crit damage calculate"))
	static FDamageRange CalculateCriticalDamage(
		FDamageRange BaseDamage,
		float CritMultiplier = 1.5f);

	UFUNCTION(BlueprintPure, Category = "Item|Defense", meta = (
		DisplayName = "Calculate Final Resistance",
		Keywords = "resistance calculate final defense"))
	static float CalculateFinalResistance(
		float BaseResistance,
		float FlatAdded = 0.0f,
		float IncreasedPercent = 0.0f);

	UFUNCTION(BlueprintPure, Category = "Item|Defense", meta = (
		DisplayName = "Calculate Armor Reduction",
		Keywords = "armor damage reduction calculate defense"))
	static float CalculateArmorReduction(float Armor, float IncomingDamage);

	UFUNCTION(BlueprintPure, Category = "Item|Weight", meta = (
		DisplayName = "Calculate Max Weight From Strength",
		Keywords = "weight carry strength calculate hunter"))
	static float CalculateMaxWeightFromStrength(
		int32 Strength,
		float WeightPerStrength = 10.0f);

	UFUNCTION(BlueprintPure, Category = "Item|Weight", meta = (
		DisplayName = "Get Overweight Percentage",
		Keywords = "overweight weight percentage penalty"))
	static float GetOverweightPercentage(float CurrentWeight, float MaxWeight);

	UFUNCTION(BlueprintPure, Category = "Item|Economy", meta = (
		DisplayName = "Get Rarity Value Multiplier",
		Keywords = "rarity value multiplier grade"))
	static float GetRarityValueMultiplier(EItemRarity Rarity);
};

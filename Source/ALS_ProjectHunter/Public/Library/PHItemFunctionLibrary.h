// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PHItemStructLibrary.h"
#include "Item/BaseItem.h"
#include "Item/ConsumableItem.h"
#include "Item/EquippableItem.h"
#include "Item/WeaponItem.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PHItemFunctionLibrary.generated.h"

class APHBaseCharacter;
class UPHAttributeSet;


/**
 * @brief A function library providing utilities for item interactions and complex calculations
 *        in the ALS Project Hunter game.
 *
 * This class offers helper methods for handling items, calculating damage,
 * generating item statistics, and identifying attributes. It is designed to extend
 * Unreal Engine's `UBlueprintFunctionLibrary` for use in C++ and Blueprints.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHItemFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, Category = "Checker")
	static bool AreItemSlotsEqual(UItemDefinitionAsset* FirstItem, UItemDefinitionAsset* SecondItem);
	static UBaseItem* GetItemInformation(UItemDefinitionAsset* ItemInfo, FConsumableItemData ConsumableItemData);

	static UEquippableItem* CreateEquippableItem(UItemDefinitionAsset*& ItemInfo);

	static UConsumableItem* CreateConsumableItem( UItemDefinitionAsset*& ItemInfo);

	UFUNCTION(BlueprintCallable, Category = "Damage Calculation")
	static TMap<FString, int> CalculateTotalDamage(int MinDamage, int MaxDamage, const APHBaseCharacter* Actor);

	static FText FormatAttributeText(const FPHAttributeData& AttributeData);

	static void IdentifyStat(float ChanceToIdentify,FPHItemStats& StatsToCheck);

	static FPHItemStats GenerateStats(const UDataTable* StatsThatCanBeGenerated);
	// Convert enum to numeric value
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Rank")
	static int32 GetRankPointsValue(ERankPoints Rank);

	// Calculate total rank points for an item (replaces FPHItemStats member)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Rank")
	static int32 CalculateTotalRankPoints(int32 BaseRankPoints, const FPHItemStats& Stats);

	// Determine item rarity based on rank points
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Rank")
	static EItemRarity DetermineWeaponRank(int32 BaseRankPoints, const FPHItemStats& Stats);

	// Generate item name from affixes
	UFUNCTION(BlueprintCallable, Category = "Item|Name Generation")
	static FText GenerateItemName(const FPHItemStats& ItemStats, const UItemDefinitionAsset* ItemInfo);
	
	static void RerollModifiers(UEquippableItem* Item, const UDataTable* ModPool, bool bRerollPrefixes, bool bRerollSuffixes,
	                            const TArray<FGuid>& LockedModifiers);
	static FPHAttributeData RollSingleMod(const UDataTable* ModPool, bool bIsPrefix);

	
	UFUNCTION()
	static float GetStatValueByAttribute(const FEquippableItemData& Data, const FGameplayAttribute& Attribute);

	UFUNCTION(BlueprintPure, Category = "Equipment")
	static FName GetSocketNameForSlot(EEquipmentSlot Slot);


	UFUNCTION(BlueprintPure, Category = "Item Information")
	static bool IsItemInformationValid(const UItemDefinitionAsset* ItemInformation);

	
};

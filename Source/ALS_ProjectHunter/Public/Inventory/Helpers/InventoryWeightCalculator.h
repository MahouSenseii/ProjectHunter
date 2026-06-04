#pragma once

#include "CoreMinimal.h"

class UInventoryManager;
class UItemInstance;

class ALS_PROJECTHUNTER_API FInventoryWeightCalculator
{
public:
	static int32 GetItemCount(const UInventoryManager& Manager);
	static int32 GetAvailableSlots(const UInventoryManager& Manager);
	static float GetTotalWeight(const UInventoryManager& Manager);
	static float GetRemainingWeight(const UInventoryManager& Manager);
	static float GetWeightPercent(const UInventoryManager& Manager);
	static bool WouldExceedWeight(const UInventoryManager& Manager, UItemInstance* Item);
	static void UpdateMaxWeightFromStrength(UInventoryManager& Manager, int32 Strength);
	static void SetMaxWeight(UInventoryManager& Manager, float NewMaxWeight);
	static void BroadcastWeightChange(UInventoryManager& Manager);
};


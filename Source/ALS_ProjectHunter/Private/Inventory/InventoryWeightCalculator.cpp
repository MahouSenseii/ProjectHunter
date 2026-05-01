#include "Inventory/InventoryWeightCalculator.h"

#include "Item/ItemInstance.h"
#include "Inventory/Components/InventoryManager.h"
#include "Inventory/Library/InventoryLog.h"

int32 FInventoryWeightCalculator::GetItemCount(const UInventoryManager& Manager)
{
	int32 Count = 0;
	for (UItemInstance* Item : Manager.Items)
	{
		if (Item)
		{
			++Count;
		}
	}

	return Count;
}

int32 FInventoryWeightCalculator::GetAvailableSlots(const UInventoryManager& Manager)
{
	return Manager.MaxSlots - GetItemCount(Manager);
}

float FInventoryWeightCalculator::GetTotalWeight(const UInventoryManager& Manager)
{
	float TotalWeight = 0.0f;
	for (UItemInstance* Item : Manager.Items)
	{
		if (Item)
		{
			TotalWeight += Item->GetTotalWeight();
		}
	}

	return TotalWeight;
}

float FInventoryWeightCalculator::GetRemainingWeight(const UInventoryManager& Manager)
{
	return FMath::Max(0.0f, Manager.MaxWeight - GetTotalWeight(Manager));
}

float FInventoryWeightCalculator::GetWeightPercent(const UInventoryManager& Manager)
{
	if (Manager.MaxWeight <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(GetTotalWeight(Manager) / Manager.MaxWeight, 0.0f, 1.0f);
}

bool FInventoryWeightCalculator::WouldExceedWeight(const UInventoryManager& Manager, UItemInstance* Item)
{
	if (!Item || Manager.MaxWeight <= 0.0f)
	{
		return false;
	}

	return (GetTotalWeight(Manager) + Item->GetTotalWeight()) > Manager.MaxWeight;
}

void FInventoryWeightCalculator::UpdateMaxWeightFromStrength(UInventoryManager& Manager, int32 Strength)
{
	SetMaxWeight(Manager, Strength * Manager.WeightPerStrength);
}

void FInventoryWeightCalculator::SetMaxWeight(UInventoryManager& Manager, float NewMaxWeight)
{
	Manager.MaxWeight = FMath::Max(0.0f, NewMaxWeight);
	Manager.UpdateWeight();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Max weight set to %.1f"), Manager.MaxWeight);
}

void FInventoryWeightCalculator::BroadcastWeightChange(UInventoryManager& Manager)
{
	const float CurrentWeight = GetTotalWeight(Manager);
	Manager.OnWeightChanged.Broadcast(CurrentWeight, Manager.MaxWeight);
}


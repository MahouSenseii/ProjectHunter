// Private/Systems/Inventory/Helpers/InventoryWeightCalculator.cpp
#include "Systems/Inventory/Helpers/InventoryWeightCalculator.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Item/ItemInstance.h"
#include "Systems/Inventory/Components/InventoryManager.h"
#include "Systems/Inventory/Library/InventoryLog.h"

float FInventoryWeightCalculator::GetTotalWeight(const UInventoryManager* Inventory)
{
	if (!Inventory)
	{
		return 0.0f;
	}

	float TotalWeight = 0.0f;
	for (UItemInstance* Item : Inventory->Items)
	{
		if (Item)
		{
			TotalWeight += Item->GetTotalWeight();
		}
	}
	return TotalWeight;
}

float FInventoryWeightCalculator::GetRemainingWeight(const UInventoryManager* Inventory)
{
	if (!Inventory)
	{
		return 0.0f;
	}
	return FMath::Max(0.0f, Inventory->MaxWeight - GetTotalWeight(Inventory));
}

float FInventoryWeightCalculator::GetWeightPercent(const UInventoryManager* Inventory)
{
	if (!Inventory || Inventory->MaxWeight <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(GetTotalWeight(Inventory) / Inventory->MaxWeight, 0.0f, 1.0f);
}

bool FInventoryWeightCalculator::IsOverweight(const UInventoryManager* Inventory)
{
	if (!Inventory)
	{
		return false;
	}
	return GetTotalWeight(Inventory) > Inventory->MaxWeight;
}

bool FInventoryWeightCalculator::WouldExceedWeight(const UInventoryManager* Inventory, const UItemInstance* Item)
{
	if (!Inventory || !Item || Inventory->MaxWeight <= 0.0f)
	{
		return false;
	}
	const float ItemWeight = Item->GetTotalWeight();
	return (GetTotalWeight(Inventory) + ItemWeight) > Inventory->MaxWeight;
}

void FInventoryWeightCalculator::UpdateMaxWeightFromStrength(UInventoryManager* Inventory, int32 Strength)
{
	if (!Inventory)
	{
		PH_LOG_WARNING(LogInventoryManager, "UpdateMaxWeightFromStrength failed: Inventory was null.");
		return;
	}
	const float NewMaxWeight = static_cast<float>(Strength) * Inventory->WeightPerStrength;
	SetMaxWeight(Inventory, NewMaxWeight);
}

void FInventoryWeightCalculator::SetMaxWeight(UInventoryManager* Inventory, float NewMaxWeight)
{
	if (!Inventory)
	{
		PH_LOG_WARNING(LogInventoryManager, "SetMaxWeight failed: Inventory was null.");
		return;
	}
	Inventory->MaxWeight = FMath::Max(0.0f, NewMaxWeight);
	// Broadcast through the manager so listeners (UI, etc.) get a single
	// authoritative event.
	Inventory->BroadcastWeightChanged();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Max weight set to %.1f"), Inventory->MaxWeight);
}

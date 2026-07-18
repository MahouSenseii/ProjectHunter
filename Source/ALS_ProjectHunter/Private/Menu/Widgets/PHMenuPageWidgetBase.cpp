#include "Menu/Widgets/PHMenuPageWidgetBase.h"

#include "Character/PHBaseCharacter.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Inventory/Components/InventoryManager.h"
#include "Stats/Components/StatsManager.h"

void UPHMenuPageWidgetBase::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	Super::NativeInitializeForCharacter(Character);

	if (!Character)
	{
		EquipmentManager = nullptr;
		InventoryManager = nullptr;
		StatsManager = nullptr;
		return;
	}

	EquipmentManager = Character->GetEquipmentManager();
	InventoryManager = Character->FindComponentByClass<UInventoryManager>();
	StatsManager = Character->GetStatsManager();
}

void UPHMenuPageWidgetBase::NativeReleaseCharacter()
{
	Super::NativeReleaseCharacter();

	EquipmentManager = nullptr;
	InventoryManager = nullptr;
	StatsManager = nullptr;
}

#include "Equipment/Helpers/EquipmentDebugItemFactory.h"

#include "Equipment/Components/EquipmentManager.h"
#include "Equipment/Library/EquipmentLog.h"
#include "Item/ItemInstance.h"

UItemInstance* FEquipmentDebugItemFactory::GiveWeapon(UEquipmentManager& Manager,
	const FDataTableRowHandle& BaseItemHandle, int32 ItemLevel, EItemRarity Rarity, bool bGenerateAffixes)
{
	if (BaseItemHandle.IsNull() || BaseItemHandle.DataTable == nullptr)
	{
		UE_LOG(LogEquipmentManager, Warning,
			TEXT("GiveWeapon: BaseItemHandle is null or has no DataTable assigned."));
		return nullptr;
	}

	AActor* OwnerActor = Manager.GetOwner();
	if (!OwnerActor)
	{
		UE_LOG(LogEquipmentManager, Warning,
			TEXT("GiveWeapon: EquipmentManager has no owner; cannot create item."));
		return nullptr;
	}

	UItemInstance* NewItem = NewObject<UItemInstance>(OwnerActor);
	if (!NewItem)
	{
		UE_LOG(LogEquipmentManager, Warning,
			TEXT("GiveWeapon: NewObject<UItemInstance> returned null."));
		return nullptr;
	}

	NewItem->Initialize(BaseItemHandle, ItemLevel, Rarity, bGenerateAffixes);

	if (!NewItem->HasValidBaseData())
	{
		UE_LOG(LogEquipmentManager, Warning,
			TEXT("GiveWeapon: Initialized item '%s' has no valid base data; row handle probably points at a missing row."),
			*NewItem->GetName());
		return nullptr;
	}

	Manager.EquipItem(NewItem, EEquipmentSlot::ES_None, true);

	UE_LOG(LogEquipmentManager, Log,
		TEXT("GiveWeapon: created and equipped '%s' (level=%d, rarity=%d)."),
		*NewItem->GetName(), ItemLevel, static_cast<int32>(Rarity));

	return NewItem;
}

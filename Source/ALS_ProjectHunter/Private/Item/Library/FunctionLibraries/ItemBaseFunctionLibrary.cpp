#include "Item/Library/FunctionLibraries/ItemBaseFunctionLibrary.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Item/Library/FunctionLibraries/ItemCalculationFunctionLibrary.h"
#include "Item/Library/FunctionLibraries/ItemEnumFunctionLibrary.h"
#include "Item/Library/ItemStructsLog.h"

bool UItemBaseFunctionLibrary::IsItemBaseValid(const FItemBase& ItemBase)
{
	const bool bHasVisualRepresentation =
		(ItemBase.StaticMesh != nullptr) ||
		(ItemBase.SkeletalMesh != nullptr) ||
		(GetItemBaseRuntimeActorClass(ItemBase) != nullptr);

	return ItemBase.ItemType != EItemType::IT_None && bHasVisualRepresentation;
}

bool UItemBaseFunctionLibrary::IsItemBaseValidForInventory(const FItemBase& ItemBase)
{
	if (!IsItemBaseValid(ItemBase))
	{
		return false;
	}

	if (ItemBase.BaseWeight < 0.0f)
	{
		return false;
	}

	if (!UItemEnumFunctionLibrary::IsItemSubTypeAllowedForItemType(ItemBase.ItemType, ItemBase.ItemSubType))
	{
		PH_LOG_ERROR(LogItemStructs,
			"IsItemBaseValidForInventory failed: Item '%s' has subtype %d, which is not valid for item type %d.",
			*ItemBase.ItemID.ToString(),
			static_cast<int32>(ItemBase.ItemSubType),
			static_cast<int32>(ItemBase.ItemType));
		return false;
	}

	if (IsItemBaseEquippable(ItemBase) && ItemBase.bStackable)
	{
		PH_LOG_ERROR(LogItemStructs, "IsItemBaseValidForInventory failed: Equipment '%s' had bStackable=true.", *ItemBase.ItemID.ToString());
		return false;
	}

	if (ItemBase.bStackable && ItemBase.MaxStackSize <= 0)
	{
		return false;
	}

	if (!ItemBase.bStackable && ItemBase.MaxStackSize > 1)
	{
		return false;
	}

	return true;
}

bool UItemBaseFunctionLibrary::IsItemBaseWeapon(const FItemBase& ItemBase)
{
	return ItemBase.ItemType == EItemType::IT_Weapon;
}

bool UItemBaseFunctionLibrary::IsItemBaseArmor(const FItemBase& ItemBase)
{
	return ItemBase.ItemType == EItemType::IT_Armor;
}

bool UItemBaseFunctionLibrary::IsItemBaseAccessory(const FItemBase& ItemBase)
{
	return ItemBase.ItemType == EItemType::IT_Accessory;
}

bool UItemBaseFunctionLibrary::IsItemBaseEquippable(const FItemBase& ItemBase)
{
	return ItemBase.ItemType == EItemType::IT_Weapon
		|| ItemBase.ItemType == EItemType::IT_Armor
		|| ItemBase.ItemType == EItemType::IT_Accessory;
}

bool UItemBaseFunctionLibrary::IsItemBaseConsumable(const FItemBase& ItemBase)
{
	return ItemBase.ItemType == EItemType::IT_Consumable;
}

bool UItemBaseFunctionLibrary::IsItemBaseMaterial(const FItemBase& ItemBase)
{
	return ItemBase.ItemType == EItemType::IT_Material;
}

bool UItemBaseFunctionLibrary::IsItemBaseCurrency(const FItemBase& ItemBase)
{
	return ItemBase.ItemType == EItemType::IT_Currency;
}

bool UItemBaseFunctionLibrary::DoesItemBaseUseRuntimeActor(const FItemBase& ItemBase)
{
	return IsItemBaseWeapon(ItemBase) || ItemBase.bUseRuntimeActor || ItemBase.bUseWeaponActor;
}

TSubclassOf<AActor> UItemBaseFunctionLibrary::GetItemBaseRuntimeActorClass(const FItemBase& ItemBase)
{
	if (ItemBase.RuntimeActorClass)
	{
		return ItemBase.RuntimeActorClass.Get();
	}

	return ItemBase.WeaponActorClass;
}

FName UItemBaseFunctionLibrary::GetItemBaseSocketForContext(const FItemBase& ItemBase, FName Context)
{
	if (const FName* ContextSocket = ItemBase.ContextualSockets.Find(Context))
	{
		return *ContextSocket;
	}

	return NAME_None;
}

float UItemBaseFunctionLibrary::GetItemBaseCalculatedValue(
	const FItemBase& ItemBase,
	int32 Quantity,
	EItemRarity InstanceRarity)
{
	const EItemRarity RarityToUse =
		(InstanceRarity != EItemRarity::IR_None) ? InstanceRarity : ItemBase.ItemRarity;

	float CalculatedValue = static_cast<float>(ItemBase.Value) * (1.0f + ItemBase.ValueModifier);
	CalculatedValue *= UItemCalculationFunctionLibrary::GetRarityValueMultiplier(RarityToUse);

	if (ItemBase.bStackable)
	{
		CalculatedValue *= FMath::Max(1, Quantity);
	}

	return FMath::Max(0.0f, CalculatedValue);
}

float UItemBaseFunctionLibrary::GetItemBaseTotalWeight(const FItemBase& ItemBase, int32 Quantity)
{
	if (ItemBase.bStackable && ItemBase.bScaleWeightWithQuantity)
	{
		return ItemBase.BaseWeight * FMath::Max(1, Quantity);
	}

	return ItemBase.BaseWeight;
}

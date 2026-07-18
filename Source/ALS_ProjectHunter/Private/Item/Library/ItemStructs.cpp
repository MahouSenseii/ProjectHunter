#include "Item/Library/Structs/ItemStructs.h"
#include "Item/Library/FunctionLibraries/ItemBaseFunctionLibrary.h"

int32 FItemBase::GetMaxUses() const
{
	return ConsumableData.MaxUses;
}

float FItemBase::GetCooldown() const
{
	return ConsumableData.Cooldown;
}

bool FItemBase::IsValid() const
{
	return UItemBaseFunctionLibrary::IsItemBaseValid(*this);
}

bool FItemBase::IsValidForInventory() const
{
	return UItemBaseFunctionLibrary::IsItemBaseValidForInventory(*this);
}

bool FItemBase::IsWeapon() const
{
	return UItemBaseFunctionLibrary::IsItemBaseWeapon(*this);
}

bool FItemBase::IsArmor() const
{
	return UItemBaseFunctionLibrary::IsItemBaseArmor(*this);
}

bool FItemBase::IsAccessory() const
{
	return UItemBaseFunctionLibrary::IsItemBaseAccessory(*this);
}

bool FItemBase::IsEquippable() const
{
	return UItemBaseFunctionLibrary::IsItemBaseEquippable(*this);
}

bool FItemBase::IsConsumable() const
{
	return UItemBaseFunctionLibrary::IsItemBaseConsumable(*this);
}

bool FItemBase::IsMaterial() const
{
	return UItemBaseFunctionLibrary::IsItemBaseMaterial(*this);
}

bool FItemBase::IsCurrency() const
{
	return UItemBaseFunctionLibrary::IsItemBaseCurrency(*this);
}

bool FItemBase::UsesRuntimeActor() const
{
	return UItemBaseFunctionLibrary::DoesItemBaseUseRuntimeActor(*this);
}

TSubclassOf<AActor> FItemBase::GetRuntimeActorClass() const
{
	return UItemBaseFunctionLibrary::GetItemBaseRuntimeActorClass(*this);
}

FName FItemBase::GetSocketForContext(FName Context) const
{
	return UItemBaseFunctionLibrary::GetItemBaseSocketForContext(*this, Context);
}

float FItemBase::GetCalculatedValue(int32 Quantity, EItemRarity InstanceRarity) const
{
	return UItemBaseFunctionLibrary::GetItemBaseCalculatedValue(*this, Quantity, InstanceRarity);
}

float FItemBase::GetTotalWeight(int32 Quantity) const
{
	return UItemBaseFunctionLibrary::GetItemBaseTotalWeight(*this, Quantity);
}

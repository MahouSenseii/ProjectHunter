// Copyright@2024 Quentin Davis

#include "Item/BaseItem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

DEFINE_LOG_CATEGORY(LogBaseItem);

UBaseItem::UBaseItem()
	: bIsInitialized(false)
{
	// Generate unique ID on construction
	GenerateUniqueID();
}

void UBaseItem::Initialize( FItemInformation& ItemInfo)
{
	ItemInfos = ItemInfo;
	bIsInitialized = true;
	
	// Ensure we have a unique ID
	if (UniqueID.IsEmpty())
	{
		GenerateUniqueID();
	}
	
	UE_LOG(LogBaseItem, Verbose, TEXT("Initialized item: %s (ID: %s)"), 
		*ItemInfos.ItemInfo.ItemName.ToString(), *UniqueID);
}

void UBaseItem::GenerateUniqueID()
{
	UniqueID = FGuid::NewGuid().ToString();
}

FString UBaseItem::GetItemInstanceID() const
{
	return UniqueID;
}

FIntPoint UBaseItem::GetDimensions() const
{
	if (ItemInfos.ItemInfo.Rotated)
	{
		// Return swapped dimensions when rotated
		return FIntPoint(ItemInfos.ItemInfo.Dimensions.Y, ItemInfos.ItemInfo.Dimensions.X);
	}
	return ItemInfos.ItemInfo.Dimensions;
}

void UBaseItem::SetRotated(bool bNewRotated)
{
	if (ItemInfos.ItemInfo.Rotated != bNewRotated)
	{
		const bool bWasRotated = ItemInfos.ItemInfo.Rotated;
		ItemInfos.ItemInfo.Rotated = bNewRotated;
		OnRotationChanged(bWasRotated);
	}
}

void UBaseItem::ToggleRotation()
{
	SetRotated(!ItemInfos.ItemInfo.Rotated);
}

UMaterialInstance* UBaseItem::GetIcon() const
{
	if (ItemInfos.ItemInfo.Rotated && ItemInfos.ItemInfo.ItemImageRotated)
	{
		return ItemInfos.ItemInfo.ItemImageRotated;
	}
	return ItemInfos.ItemInfo.ItemImage;
}

int32 UBaseItem::GetQuantity() const
{
	return ItemInfos.ItemInfo.Quantity;
}

bool UBaseItem::AddQuantity(int32 InQty)
{
	if (InQty <= 0)
	{
		UE_LOG(LogBaseItem, Warning, TEXT("AddQuantity called with non-positive value: %d"), InQty);
		return false;
	}

	if (!IsStackable())
	{
		UE_LOG(LogBaseItem, Warning, TEXT("Attempted to add quantity to non-stackable item: %s"), 
			*ItemInfos.ItemInfo.ItemName.ToString());
		return false;
	}

	const int32 OldQuantity = GetQuantity();
	const int32 MaxStack = ItemInfos.ItemInfo.MaxStackSize;
	
	if (MaxStack > 0)
	{
		// Check if we can add the full amount
		const int32 SpaceAvailable = MaxStack - OldQuantity;
		if (SpaceAvailable <= 0)
		{
			return false; // Already at max
		}
		
		const int32 AmountToAdd = FMath::Min(InQty, SpaceAvailable);
		ItemInfos.ItemInfo.Quantity = OldQuantity + AmountToAdd;
		
		OnQuantityChanged(OldQuantity, ItemInfos.ItemInfo.Quantity);
		return AmountToAdd == InQty; // Return true only if we added the full amount
	}
	else
	{
		// No max stack limit
		ItemInfos.ItemInfo.Quantity = OldQuantity + InQty;
		OnQuantityChanged(OldQuantity, ItemInfos.ItemInfo.Quantity);
		return true;
	}
}

bool UBaseItem::RemoveQuantity(int32 InQty)
{
	if (InQty <= 0)
	{
		UE_LOG(LogBaseItem, Warning, TEXT("RemoveQuantity called with non-positive value: %d"), InQty);
		return false;
	}

	const int32 OldQuantity = GetQuantity();
	if (OldQuantity < InQty)
	{
		return false; // Not enough to remove
	}

	ItemInfos.ItemInfo.Quantity = OldQuantity - InQty;
	OnQuantityChanged(OldQuantity, ItemInfos.ItemInfo.Quantity);
	return true;
}

void UBaseItem::SetQuantity(int32 NewQty)
{
	if (NewQty < 0)
	{
		UE_LOG(LogBaseItem, Warning, TEXT("SetQuantity called with negative value: %d"), NewQty);
		return;
	}

	const int32 OldQuantity = GetQuantity();
	const int32 MaxStack = ItemInfos.ItemInfo.MaxStackSize;
	
	if (MaxStack > 0)
	{
		ItemInfos.ItemInfo.Quantity = FMath::Min(NewQty, MaxStack);
	}
	else
	{
		ItemInfos.ItemInfo.Quantity = NewQty;
	}
	
	if (OldQuantity != ItemInfos.ItemInfo.Quantity)
	{
		OnQuantityChanged(OldQuantity, ItemInfos.ItemInfo.Quantity);
	}
}

bool UBaseItem::CanAddQuantity(int32 Amount) const
{
	if (!IsStackable() || Amount <= 0)
	{
		return false;
	}

	const int32 MaxStack = ItemInfos.ItemInfo.MaxStackSize;
	if (MaxStack <= 0)
	{
		return true; // No limit
	}

	return (GetQuantity() + Amount) <= MaxStack;
}

void UBaseItem::SetItemInfo(const FItemInformation& NewItemInfo)
{
	const FString OldID = UniqueID;
	ItemInfos = NewItemInfo;
	
	// Preserve the unique instance ID
	if (!OldID.IsEmpty())
	{
		UniqueID = OldID;
	}
	
	bIsInitialized = true;
}

void UBaseItem::SetEquipmentData(const FEquippableItemData& InData)
{
	ItemInfos.ItemData = InData;
}

void UBaseItem::ApplyEffectToTarget(AActor* Target, TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	if (!IsValid(Target) || !GameplayEffectClass)
	{
		UE_LOG(LogBaseItem, Warning, TEXT("ApplyEffectToTarget: Invalid target or effect class"));
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
	{
		UE_LOG(LogBaseItem, Warning, TEXT("ApplyEffectToTarget: Target has no AbilitySystemComponent"));
		return;
	}

	const FGameplayEffectContextHandle EffectContext = TargetASC->MakeEffectContext();
	const FGameplayEffectSpecHandle EffectSpec = TargetASC->MakeOutgoingSpec(
		GameplayEffectClass, 1.0f, EffectContext);
	
	if (EffectSpec.IsValid())
	{
		TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
	}
}

bool UBaseItem::ValidateItemData() const
{
	bool bIsValid = true;

	if (ItemInfos.ItemInfo.ItemName.IsEmpty())
	{
		UE_LOG(LogBaseItem, Warning, TEXT("Item has no name"));
		bIsValid = false;
	}

	if (!ItemInfos.ItemInfo.ItemImage)
	{
		UE_LOG(LogBaseItem, Warning, TEXT("Item '%s' has no icon"), 
			*ItemInfos.ItemInfo.ItemName.ToString());
		bIsValid = false;
	}

	if (ItemInfos.ItemInfo.Dimensions.X <= 0 || ItemInfos.ItemInfo.Dimensions.Y <= 0)
	{
		UE_LOG(LogBaseItem, Warning, TEXT("Item '%s' has invalid dimensions: %dx%d"), 
			*ItemInfos.ItemInfo.ItemName.ToString(),
			ItemInfos.ItemInfo.Dimensions.X,
			ItemInfos.ItemInfo.Dimensions.Y);
		bIsValid = false;
	}

	if (IsStackable() && ItemInfos.ItemInfo.MaxStackSize == 1)
	{
		UE_LOG(LogBaseItem, Warning, TEXT("Item '%s' is marked as stackable but has max stack of 1"), 
			*ItemInfos.ItemInfo.ItemName.ToString());
		bIsValid = false;
	}

	return bIsValid;
}

bool UBaseItem::IsValidItem() const
{
	return bIsInitialized && !UniqueID.IsEmpty() && ValidateItemData();
}

bool UBaseItem::CanStackWith(const UBaseItem* Other) const
{
	if (!Other || !IsStackable() || !Other->IsStackable())
	{
		return false;
	}

	// Items can stack if they have the same item ID
	if (ItemInfos.ItemInfo.ItemID != Other->ItemInfos.ItemInfo.ItemID)
	{
		return false;
	}

	// Check if there's room to stack
	const int32 MaxStack = ItemInfos.ItemInfo.MaxStackSize;
	if (MaxStack > 0 && GetQuantity() >= MaxStack)
	{
		return false;
	}

	return true;
}

bool UBaseItem::IsSameItemType(const UBaseItem* Other) const
{
	if (!Other)
	{
		return false;
	}

	return ItemInfos.ItemInfo.ItemID == Other->ItemInfos.ItemInfo.ItemID;
}

void UBaseItem::OnQuantityChanged(int32 OldQuantity, int32 NewQuantity)
{
	// Base implementation - derived classes can override
	UE_LOG(LogBaseItem, Verbose, TEXT("Item '%s' quantity changed from %d to %d"), 
		*ItemInfos.ItemInfo.ItemName.ToString(), OldQuantity, NewQuantity);
}

void UBaseItem::OnRotationChanged(bool bWasRotated)
{
	// Base implementation - derived classes can override
	UE_LOG(LogBaseItem, Verbose, TEXT("Item '%s' rotation changed from %s to %s"), 
		*ItemInfos.ItemInfo.ItemName.ToString(),
		bWasRotated ? TEXT("Rotated") : TEXT("Normal"),
		ItemInfos.ItemInfo.Rotated ? TEXT("Rotated") : TEXT("Normal"));
}

void UBaseItem::InternalSetItemInfo(const FItemInformation& NewItemInfo)
{
	ItemInfos = NewItemInfo;
}

bool UBaseItem::GetIsInitialized() const
{
	return bIsInitialized;
}

bool UBaseItem::SetIsInitialized(bool bNewIsInitialized)
{
	if (bIsInitialized != bNewIsInitialized)
	{
		bIsInitialized = bNewIsInitialized;
		return true;
	}
	return false;
}

#if WITH_EDITOR
FString UBaseItem::GetDebugString() const
{
	return FString::Printf(TEXT("Item: %s | ID: %s | Type: %d | Qty: %d/%d | Dims: %dx%d | Rotated: %s"),
		*ItemInfos.ItemInfo.ItemName.ToString(),
		*UniqueID,
		(int32)ItemInfos.ItemInfo.ItemType,
		GetQuantity(),
		ItemInfos.ItemInfo.MaxStackSize,
		ItemInfos.ItemInfo.Dimensions.X,
		ItemInfos.ItemInfo.Dimensions.Y,
		ItemInfos.ItemInfo.Rotated ? TEXT("Yes") : TEXT("No"));
}

void UBaseItem::LogItemInfo() const
{
	UE_LOG(LogBaseItem, Display, TEXT("%s"), *GetDebugString());
}
#endif
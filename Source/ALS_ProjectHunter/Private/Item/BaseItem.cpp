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

void UBaseItem::Initialize( UItemDefinitionAsset*& ItemInfo)
{
	ItemInfos = ItemInfo;
	bIsInitialized = true;
	
	// Ensure we have a unique ID
	if (UniqueID.IsEmpty())
	{
		GenerateUniqueID();
	}
	
	UE_LOG(LogBaseItem, Verbose, TEXT("Initialized item: %s (ID: %s)"), 
		*ItemInfos->Base.ItemName.ToString(), *UniqueID);
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
	if (ItemInfos->Base.Rotated)
	{
		// Return swapped dimensions when rotated
		return FIntPoint(ItemInfos->Base.Dimensions.Y, ItemInfos->Base.Dimensions.X);
	}
	return ItemInfos->Base.Dimensions;
}

void UBaseItem::SetRotated(const bool bNewRotated)
{
	if (ItemInfos->Base.Rotated != bNewRotated)
	{
		const bool bWasRotated = ItemInfos->Base.Rotated;
		ItemInfos->Base.Rotated = bNewRotated;
		OnRotationChanged(bWasRotated);
	}
}

void UBaseItem::ToggleRotation()
{
	SetRotated(!ItemInfos->Base.Rotated);
}

UMaterialInstance* UBaseItem::GetIcon() 
{
	if (ItemInfos->Base.Rotated && ItemInfos->Base.ItemImageRotated)
	{
		return ItemInfos->Base.ItemImageRotated;
	}
	return ItemInfos->Base.ItemImage;
}

int32 UBaseItem::GetQuantity() 
{
	return ItemInfos->Base.Quantity;
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
			*ItemInfos->Base.ItemName.ToString());
		return false;
	}

	const int32 OldQuantity = GetQuantity();
	const int32 MaxStack = ItemInfos->Base.MaxStackSize;
	
	if (MaxStack > 0)
	{
		// Check if we can add the full amount
		const int32 SpaceAvailable = MaxStack - OldQuantity;
		if (SpaceAvailable <= 0)
		{
			return false; // Already at max
		}
		
		const int32 AmountToAdd = FMath::Min(InQty, SpaceAvailable);
		ItemInfos->Base.Quantity = OldQuantity + AmountToAdd;
		
		OnQuantityChanged(OldQuantity, ItemInfos->Base.Quantity);
		return AmountToAdd == InQty; // Return true only if we added the full amount
	}
	else
	{
		// No max stack limit
		ItemInfos->Base.Quantity = OldQuantity + InQty;
		OnQuantityChanged(OldQuantity, ItemInfos->Base.Quantity);
		return true;
	}
}

bool UBaseItem::RemoveQuantity(const int32 InQty)
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

	ItemInfos->Base.Quantity = OldQuantity - InQty;
	OnQuantityChanged(OldQuantity, ItemInfos->Base.Quantity);
	return true;
}

void UBaseItem::SetQuantity(const int32 NewQty)
{
	if (NewQty < 0)
	{
		UE_LOG(LogBaseItem, Warning, TEXT("SetQuantity called with negative value: %d"), NewQty);
		return;
	}

	const int32 OldQuantity = GetQuantity();
	const int32 MaxStack = ItemInfos->Base.MaxStackSize;
	
	if (MaxStack > 0)
	{
		ItemInfos->Base.Quantity = FMath::Min(NewQty, MaxStack);
	}
	else
	{
		ItemInfos->Base.Quantity = NewQty;
	}
	
	if (OldQuantity != ItemInfos->Base.Quantity)
	{
		OnQuantityChanged(OldQuantity, ItemInfos->Base.Quantity);
	}
}

bool UBaseItem::CanAddQuantity(int32 Amount) 
{
	if (!IsStackable() || Amount <= 0)
	{
		return false;
	}

	const int32 MaxStack = ItemInfos->Base.MaxStackSize;
	if (MaxStack <= 0)
	{
		return true; // No limit
	}

	return (GetQuantity() + Amount) <= MaxStack;
}

void UBaseItem::SetItemInfo( UItemDefinitionAsset*& NewItemInfo)
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

void UBaseItem::SetEquipmentData( FEquippableItemData& InData)
{
	ItemInfos->Equip = InData;
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

bool UBaseItem::ValidateItemData() 
{
	bool bIsValid = true;

	if (ItemInfos->Base.ItemName.IsEmpty())
	{
		UE_LOG(LogBaseItem, Warning, TEXT("Item has no name"));
		bIsValid = false;
	}

	if (!ItemInfos->Base.ItemImage)
	{
		UE_LOG(LogBaseItem, Warning, TEXT("Item '%s' has no icon"), 
			*ItemInfos->Base.ItemName.ToString());
		bIsValid = false;
	}

	if (ItemInfos->Base.Dimensions.X <= 0 || ItemInfos->Base.Dimensions.Y <= 0)
	{
		UE_LOG(LogBaseItem, Warning, TEXT("Item '%s' has invalid dimensions: %dx%d"), 
			*ItemInfos->Base.ItemName.ToString(),
			ItemInfos->Base.Dimensions.X,
			ItemInfos->Base.Dimensions.Y);
		bIsValid = false;
	}

	if (IsStackable() && ItemInfos->Base.MaxStackSize == 1)
	{
		UE_LOG(LogBaseItem, Warning, TEXT("Item '%s' is marked as stackable but has max stack of 1"), 
			*ItemInfos->Base.ItemName.ToString());
		bIsValid = false;
	}

	return bIsValid;
}

bool UBaseItem::IsValidItem() 
{
	return bIsInitialized && !UniqueID.IsEmpty() && ValidateItemData();
}

bool UBaseItem::CanStackWith( UBaseItem* Other) 
{
	if (!Other || !IsStackable() || !Other->IsStackable())
	{
		return false;
	}

	// Items can stack if they have the same item ID
	if (ItemInfos->Base.ItemID != Other->ItemInfos->Base.ItemID)
	{
		return false;
	}

	// Check if there's room to stack
	const int32 MaxStack = ItemInfos->Base.MaxStackSize;
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

	return ItemInfos->Base.ItemID == Other->ItemInfos->Base.ItemID;
}

void UBaseItem::OnQuantityChanged(int32 OldQuantity, int32 NewQuantity)
{
	// Base implementation - derived classes can override
	UE_LOG(LogBaseItem, Verbose, TEXT("Item '%s' quantity changed from %d to %d"), 
		*ItemInfos->Base.ItemName.ToString(), OldQuantity, NewQuantity);
}

void UBaseItem::OnRotationChanged(bool bWasRotated)
{
	// Base implementation - derived classes can override
	UE_LOG(LogBaseItem, Verbose, TEXT("Item '%s' rotation changed from %s to %s"), 
		*ItemInfos->Base.ItemName.ToString(),
		bWasRotated ? TEXT("Rotated") : TEXT("Normal"),
		ItemInfos->Base.Rotated ? TEXT("Rotated") : TEXT("Normal"));
}

void UBaseItem::InternalSetItemInfo( UItemDefinitionAsset*& NewItemInfo)
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
FString UBaseItem::GetDebugString() 
{
	return FString::Printf(TEXT("Item: %s | ID: %s | Type: %d | Qty: %d/%d | Dims: %dx%d | Rotated: %s"),
		*ItemInfos->Base.ItemName.ToString(),
		*UniqueID,
		(int32)ItemInfos->Base.ItemType,
		GetQuantity(),
		ItemInfos->Base.MaxStackSize,
		ItemInfos->Base.Dimensions.X,
		ItemInfos->Base.Dimensions.Y,
		ItemInfos->Base.Rotated ? TEXT("Yes") : TEXT("No"));
}

void UBaseItem::LogItemInfo() 
{
	UE_LOG(LogBaseItem, Display, TEXT("%s"), *GetDebugString());
}
#endif
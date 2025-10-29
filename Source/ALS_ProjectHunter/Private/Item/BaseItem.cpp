// Copyright@2024 Quentin Davis

#include "Item/BaseItem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Library/PHItemFunctionLibrary.h"
#include "Library/PHItemStructLibrary.h"


DEFINE_LOG_CATEGORY(LogBaseItem);

UBaseItem::UBaseItem()
	: bIsInitialized(false)
{
	// Generate unique ID on construction
	GenerateUniqueID();
}

void UBaseItem::Initialize(UItemDefinitionAsset* ItemInfo)
{
	ItemDefinition = ItemInfo;  
	bIsInitialized = true;
    
	if (RuntimeData.UniqueID.IsEmpty())
	{
		RuntimeData.UniqueID = FGuid::NewGuid().ToString();
	}
    
	
	if (!RuntimeData.bHasNameBeenGenerated && !ItemDefinition->Base.ItemName.IsEmpty())
	{
		FPHItemStats CombinedStats;
		CombinedStats.Prefixes = RuntimeData.Prefixes;
		CombinedStats.Suffixes = RuntimeData.Suffixes;
        
		RuntimeData.DisplayName = UPHItemFunctionLibrary::GenerateItemName(CombinedStats,ItemDefinition);
		RuntimeData.bHasNameBeenGenerated = true;
	}
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
	if (RuntimeData.bRotated)
	{
		// Return swapped dimensions when rotated
		return FIntPoint(ItemDefinition->Base.Dimensions.Y, ItemDefinition->Base.Dimensions.X);
	}
	return ItemDefinition->Base.Dimensions;
}

void UBaseItem::SetRotated(const bool bNewRotated)
{
	if (RuntimeData.bRotated != bNewRotated)
	{
		const bool bWasRotated = RuntimeData.bRotated;
		RuntimeData.bRotated = bNewRotated;
		OnRotationChanged(bWasRotated);
	}
}

void UBaseItem::ToggleRotation()
{
	SetRotated(!RuntimeData.bRotated);
}

UMaterialInstance* UBaseItem::GetIcon() 
{
	if (RuntimeData.bRotated && ItemDefinition->Base.ItemImageRotated)
	{
		return ItemDefinition->Base.ItemImageRotated;
	}
	return ItemDefinition->Base.ItemImage;
}

int32 UBaseItem::GetQuantity() 
{
	return RuntimeData.Quantity;
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
			*ItemDefinition->Base.ItemName.ToString());
		return false;
	}

	const int32 OldQuantity = GetQuantity();
	const int32 MaxStack = ItemDefinition->Base.MaxStackSize;
	
	if (MaxStack > 0)
	{
		// Check if we can add the full amount
		const int32 SpaceAvailable = MaxStack - OldQuantity;
		if (SpaceAvailable <= 0)
		{
			return false; // Already at max
		}
		
		const int32 AmountToAdd = FMath::Min(InQty, SpaceAvailable);
		RuntimeData.Quantity = OldQuantity + AmountToAdd;
		
		OnQuantityChanged(OldQuantity, RuntimeData.Quantity);
		return AmountToAdd == InQty; // Return true only if we added the full amount
	}
	else
	{
		// No max stack limit
		RuntimeData.Quantity = OldQuantity + InQty;
		OnQuantityChanged(OldQuantity, RuntimeData.Quantity);
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

	RuntimeData.Quantity = OldQuantity - InQty;
	OnQuantityChanged(OldQuantity, RuntimeData.Quantity);
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
	const int32 MaxStack = ItemDefinition->Base.MaxStackSize;
	
	if (MaxStack > 0)
	{
		RuntimeData.Quantity = FMath::Min(NewQty, MaxStack);
	}
	else
	{
		RuntimeData.Quantity = NewQty;
	}
	
	if (OldQuantity != RuntimeData.Quantity)
	{
		OnQuantityChanged(OldQuantity, RuntimeData.Quantity);
	}
}

bool UBaseItem::CanAddQuantity(int32 Amount) 
{
	if (!IsStackable() || Amount <= 0)
	{
		return false;
	}

	const int32 MaxStack = ItemDefinition->Base.MaxStackSize;
	if (MaxStack <= 0)
	{
		return true; // No limit
	}

	return (GetQuantity() + Amount) <= MaxStack;
}

void UBaseItem::SetItemInfo(UItemDefinitionAsset* NewItemInfo)
{
	const FString OldID = UniqueID;
	ItemDefinition = NewItemInfo;
	
	// Preserve the unique instance ID
	if (!OldID.IsEmpty())
	{
		UniqueID = OldID;
	}
	
	bIsInitialized = true;
}

void UBaseItem::SetEquipmentData( FEquippableItemData& InData)
{
	ItemDefinition->Equip = InData;
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

	if (ItemDefinition->Base.ItemName.IsEmpty())
	{
		UE_LOG(LogBaseItem, Warning, TEXT("Item has no name"));
		bIsValid = false;
	}

	if (!ItemDefinition->Base.ItemImage)
	{
		UE_LOG(LogBaseItem, Warning, TEXT("Item '%s' has no icon"), 
			*ItemDefinition->Base.ItemName.ToString());
		bIsValid = false;
	}

	if (ItemDefinition->Base.Dimensions.X <= 0 || ItemDefinition->Base.Dimensions.Y <= 0)
	{
		UE_LOG(LogBaseItem, Warning, TEXT("Item '%s' has invalid dimensions: %dx%d"), 
			*ItemDefinition->Base.ItemName.ToString(),
			ItemDefinition->Base.Dimensions.X,
			ItemDefinition->Base.Dimensions.Y);
		bIsValid = false;
	}

	if (IsStackable() && ItemDefinition->Base.MaxStackSize == 1)
	{
		UE_LOG(LogBaseItem, Warning, TEXT("Item '%s' is marked as stackable but has max stack of 1"), 
			*ItemDefinition->Base.ItemName.ToString());
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
	if (ItemDefinition->Base.ItemID != Other->ItemDefinition->Base.ItemID)
	{
		return false;
	}

	// Check if there's room to stack
	const int32 MaxStack = ItemDefinition->Base.MaxStackSize;
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

	return ItemDefinition->Base.ItemID == Other->ItemDefinition->Base.ItemID;
}

void UBaseItem::OnQuantityChanged(int32 OldQuantity, int32 NewQuantity)
{
	// Base implementation - derived classes can override
	UE_LOG(LogBaseItem, Verbose, TEXT("Item '%s' quantity changed from %d to %d"), 
		*ItemDefinition->Base.ItemName.ToString(), OldQuantity, NewQuantity);
}

void UBaseItem::OnRotationChanged(bool bWasRotated)
{
	// Base implementation - derived classes can override
	UE_LOG(LogBaseItem, Verbose, TEXT("Item '%s' rotation changed from %s to %s"), 
		*ItemDefinition->Base.ItemName.ToString(),
		bWasRotated ? TEXT("Rotated") : TEXT("Normal"),
		RuntimeData.bRotated ? TEXT("Rotated") : TEXT("Normal"));
}

void UBaseItem::InternalSetItemInfo( UItemDefinitionAsset*& NewItemInfo)
{
	ItemDefinition = NewItemInfo;
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
		*ItemDefinition->Base.ItemName.ToString(),
		*UniqueID,
		static_cast<int32>(ItemDefinition->Base.ItemType),
		GetQuantity(),
		ItemDefinition->Base.MaxStackSize,
		ItemDefinition->Base.Dimensions.X,
		ItemDefinition->Base.Dimensions.Y,
		RuntimeData.bRotated ? TEXT("Yes") : TEXT("No"));
}

void UBaseItem::LogItemInfo() 
{
	UE_LOG(LogBaseItem, Display, TEXT("%s"), *GetDebugString());
}
#endif
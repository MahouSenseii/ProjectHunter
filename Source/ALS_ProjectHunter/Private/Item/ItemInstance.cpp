// ItemInstanceObject.cpp
#include "Item/ItemInstance.h"
#include "Item/Data/UItemDefinitionAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogItemInstance, Log, All);

UItemInstanceObject::UItemInstanceObject()
{
    bCacheValid = false;
    SetIsInitialized(false);
}

/* ============================= */
/* ===      Public API       === */
/* ============================= */

UItemDefinitionAsset* UItemInstanceObject::GetItemInfo() 
{
    if (!bCacheValid)
    {
        RebuildItemInfoView();
    }
    return ItemInfoView;
}

void UItemInstanceObject::SetItemInfo( UItemDefinitionAsset*& NewItemInfo)
{
    Super::SetItemInfo(NewItemInfo);
}

bool UItemInstanceObject::ValidateItemData() 
{
    return Super::ValidateItemData();
}

void UItemInstanceObject::RebuildItemInfoView() const
{
    if (!IsValidInstance())
    {
        UE_LOG(LogItemInstance, Warning, TEXT("Attempting to rebuild view for invalid instance"));
        return;
    }

    // Rebuild the cache from the current instance data and base definition
    if (UItemDefinitionAsset* LoadedAsset = BaseDef.LoadSynchronous())
    {
        // Start with base definition
        ItemInfoView = LoadedAsset->GetBaseItemInfo();
        
        // Apply instance-specific overrides to ItemData
        ItemInfoView->Equip.Durability.CurrentDurability = Instance.Durability.CurrentDurability;
        ItemInfoView->Equip.Durability.MaxDurability = Instance.Durability.MaxDurability;
        
        // Override ItemInfo rarity with instance rarity
        ItemInfoView->Base.ItemRarity = Instance.Rarity;
        
        // Use instance display name if set, otherwise use base name
        if (!Instance.DisplayName.IsEmpty())
        {
            ItemInfoView->Base.ItemName = Instance.DisplayName;
        }
        
        // Note: Quantity and Rotated remain in ItemInfoView.ItemInfo and are maintained there
    }
    
    bCacheValid = true;
}

bool UItemInstanceObject::IsValidInstance() const
{
    return GetIsInitialized() && BaseDef.IsValid();
}

void UItemInstanceObject::InitializeFromDefinition(UItemDefinitionAsset* Definition)
{
    if (!Definition)
    {
        UE_LOG(LogItemInstance, Error, TEXT("Cannot initialize from null definition"));
        return;
    }

    BaseDef = Definition;
    
    // Initialize instance data from definition defaults
     UItemDefinitionAsset*& BaseInfo = Definition;
    
    // FItemInstance properties (rolled/runtime data)
    Instance.ItemLevel = 1;
    Instance.Rarity = BaseInfo->Base.ItemRarity;
    Instance.bIdentified = true;
    Instance.DisplayName = BaseInfo->Base.ItemName;
    Instance.Seed = FMath::Rand();
    
    // Initialize durability if the item has durability
    if (BaseInfo->Equip.Durability.MaxDurability > 0.0f)
    {
        Instance.Durability.CurrentDurability = BaseInfo->Equip.Durability.MaxDurability;
        Instance.Durability.MaxDurability = BaseInfo->Equip.Durability.MaxDurability;
    }
    
    SetIsInitialized(true);
    InvalidateCache();
    NotifyInstanceChanged(EItemChangeType::Initialization);
}

/* ============================= */
/* ===    Quantity Control   === */
/* ============================= */

int32 UItemInstanceObject::GetQuantity() 
{
    return RuntimeData.Quantity;
}

void UItemInstanceObject::SetQuantity(int32 NewQuantity)
{
    if (!ValidateQuantityChange(NewQuantity))
    {
        return static_cast<void>(false);
    }

    // Get the current cached view
    const UItemDefinitionAsset* CurrentInfo = GetItemInfo();
    if (RuntimeData.Quantity != NewQuantity)
    {
        // We need to modify the cache since quantity is in ItemInfoView, not Instance
        RuntimeData.Quantity = NewQuantity;
        NotifyInstanceChanged(EItemChangeType::Quantity);
    }
    
    return static_cast<void>(true);
}

bool UItemInstanceObject::AddQuantity(int32 Amount)
{
    if (Amount <= 0)
    {
        UE_LOG(LogItemInstance, Warning, TEXT("AddQuantity called with non-positive amount: %d"), Amount);
        return false;
    }
    SetQuantity(GetQuantity() + Amount);
    return true;
}

bool UItemInstanceObject::RemoveQuantity(int32 Amount)
{
    if (Amount <= 0)
    {
        UE_LOG(LogItemInstance, Warning, TEXT("RemoveQuantity called with non-positive amount: %d"), Amount);
        return false;
    }

    const int32 CurrentQty = GetQuantity();
    if (CurrentQty < Amount)
    {
        UE_LOG(LogItemInstance, Warning, TEXT("Cannot remove %d quantity, only have %d"), Amount, CurrentQty);
        return false;
    }
    SetQuantity(CurrentQty - Amount);
    return true;
}

/* ============================= */
/* ===   Durability Control  === */
/* ============================= */

float UItemInstanceObject::GetCurrentDurability() const
{
    return Instance.Durability.CurrentDurability;
}

float UItemInstanceObject::GetMaxDurability() const
{
    return Instance.Durability.MaxDurability;
}

bool UItemInstanceObject::SetDurability(float NewDurability)
{
    if (!ValidateDurabilityChange(NewDurability))
    {
        return false;
    }

    if (!FMath::IsNearlyEqual(Instance.Durability.CurrentDurability, NewDurability))
    {
        Instance.Durability.CurrentDurability = NewDurability;
        InvalidateCache();
        NotifyInstanceChanged(EItemChangeType::Durability);
    }

    return true;
}

bool UItemInstanceObject::DamageDurability(float DamageAmount)
{
    if (DamageAmount < 0.0f)
    {
        UE_LOG(LogItemInstance, Warning, TEXT("DamageDurability called with negative damage: %f"), DamageAmount);
        return false;
    }

    return SetDurability(FMath::Max(0.0f, Instance.Durability.CurrentDurability - DamageAmount));
}

bool UItemInstanceObject::RepairDurability(float RepairAmount)
{
    if (RepairAmount < 0.0f)
    {
        UE_LOG(LogItemInstance, Warning, TEXT("RepairDurability called with negative repair: %f"), RepairAmount);
        return false;
    }

    return SetDurability(FMath::Min(Instance.Durability.MaxDurability, Instance.Durability.CurrentDurability + RepairAmount));
}

/* ============================= */
/* ===    Rotation Control   === */
/* ============================= */

void UItemInstanceObject::SetRotated(bool bNewRotated)
{
     
    if (RuntimeData.bRotated != bNewRotated)
    {
        // Modify the cached view since rotation is in ItemInfoView, not Instance
       RuntimeData.bRotated = bNewRotated;
        NotifyInstanceChanged(EItemChangeType::Rotation);
    }
}

FIntPoint UItemInstanceObject::GetDimensions() const
{
    return Super::GetDimensions();
}

void UItemInstanceObject::ToggleRotation()
{
    SetRotated(!IsRotated());
}

void UItemInstanceObject::SetItemLevel(int32 NewLevel)
{
    Instance.ItemLevel = NewLevel;
}

/* ============================= */
/* ===    Instance Data       === */
/* ============================= */


void UItemInstanceObject::SetInstanceRarity(EItemRarity NewRarity)
{
    Instance.Rarity = NewRarity;
}

void UItemInstanceObject::SetIdentified(bool bNewIdentified)
{
    if (Instance.bIdentified != bNewIdentified)
    {
        Instance.bIdentified = bNewIdentified;
        InvalidateCache();
        NotifyInstanceChanged(EItemChangeType::Properties);
    }
}

/* ============================= */
/* ===      Events & Cache    === */
/* ============================= */

void UItemInstanceObject::InvalidateCache()
{
    bCacheValid = false;
}

/* ============================= */
/* ===    Private Helpers    === */
/* ============================= */

void UItemInstanceObject::NotifyInstanceChanged(EItemChangeType ChangeType)
{
    // Broadcast to all listeners
    OnInstanceChanged.Broadcast(this, ChangeType);
    
    UE_LOG(LogItemInstance, Verbose, TEXT("Item instance changed: %s"), 
           *UEnum::GetValueAsString(ChangeType));
}



bool UItemInstanceObject::ValidateQuantityChange(int32 NewQuantity) const
{
    if (NewQuantity < 0)
    {
        UE_LOG(LogItemInstance, Warning, TEXT("Cannot set negative quantity: %d"), NewQuantity);
        return false;
    }

    // Check against max stack size if defined
    if (UItemDefinitionAsset* LoadedAsset = BaseDef.LoadSynchronous())
    {
        const int32 MaxStack = LoadedAsset->GetBaseItemInfo()->Base.MaxStackSize;
        if (MaxStack > 0 && NewQuantity > MaxStack)
        {
            UE_LOG(LogItemInstance, Warning, TEXT("Quantity %d exceeds max stack size %d"), NewQuantity, MaxStack);
            return false;
        }
    }

    return true;
}

bool UItemInstanceObject::ValidateDurabilityChange(float NewDurability) const
{
    if (NewDurability < 0.0f)
    {
        UE_LOG(LogItemInstance, Warning, TEXT("Cannot set negative durability: %f"), NewDurability);
        return false;
    }

    // Check against max durability from instance (which may be modified from base)
    if (NewDurability > Instance.Durability.MaxDurability)
    {
        UE_LOG(LogItemInstance, Warning, TEXT("Durability %f exceeds maximum %f"), 
               NewDurability, Instance.Durability.MaxDurability);
        return false;
    }

    return true;
}
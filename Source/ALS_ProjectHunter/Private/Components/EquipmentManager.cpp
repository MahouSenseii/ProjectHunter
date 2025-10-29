// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/EquipmentManager.h"
#include "Character/ALSCharacter.h"
#include "Components/InventoryManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interactables/Pickups/ItemPickup.h"
#include "Item/EquippedObject.h"
#include "Kismet/KismetMathLibrary.h"
#include "Library/PHItemFunctionLibrary.h"
#include "PHGameplayTags.h"
#include "Library/PHDamageTypeUtils.h"

UEquipmentManager::UEquipmentManager() { }
DEFINE_LOG_CATEGORY(LogEquipmentManager);
void UEquipmentManager::BeginDestroy()
{
	// Clear all equipment data
	EquipmentData.Empty();
    
	// Destroy all equipped objects
	for (const auto& Pair : EquipmentItem)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->Destroy();
		}
	}
	EquipmentItem.Empty();
    
	// Clear any applied effects
	AppliedPassiveEffects.Empty();
	AppliedItemStats.Empty();
	
	Super::BeginDestroy();
}

void UEquipmentManager::BeginPlay()
{
	Super::BeginPlay();
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<AALSCharacter>(GetOwner());
	}
}

/* ============================= */
/* === Equipment Slot Checking === */
/* ============================= */

bool UEquipmentManager::CheckSlot(UBaseItem* Item)
{
	if (!Item) return false;

	// Check if the item's target slot exists in the EquipmentData map
	UEquippableItem* EquippableItem = Cast<UEquippableItem>(Item);
	if (!EquippableItem) return false;

	const EEquipmentSlot Slot = EquippableItem->GetItemInfo()->Equip.EquipSlot;
	return EquipmentData.Contains(Slot);
}



UEquippableItem* UEquipmentManager::GetItemInSlot(EEquipmentSlot SlotToCheck)
{
	if (SlotToCheck == EEquipmentSlot::ES_None)
	{
		return nullptr;
	}

	if (UBaseItem* const* FoundItemPtr = EquipmentData.Find(SlotToCheck))
	{
		return Cast<UEquippableItem>(*FoundItemPtr);
	}

	return nullptr;
}



TArray<UBaseItem*> UEquipmentManager::EquipmentCheck() const
{
	TArray<UBaseItem*> Items;
	EquipmentData.GenerateValueArray(Items);
	return Items;
}

bool UEquipmentManager::IsItemEquippable(UBaseItem* Item)
{
	if (!IsValid(Item)) return false;

	static const TSet<EItemType> EquippableTypes = {
		EItemType::IT_Armor, EItemType::IT_Weapon, EItemType::IT_Shield
	};

	return EquippableTypes.Contains(Item->GetItemInfo()->Base.ItemType);
}

/* ============================= */
/* === Inventory & Equipment Management === */
/* ============================= */

bool UEquipmentManager::AddItemInSlotToInventory(UBaseItem* Item)
{
	if (!IsValid(Item)) return false;

	UEquippableItem* EquippableItem = Cast<UEquippableItem>(Item);
	if (!EquippableItem) return false;

	const EEquipmentSlot Slot = EquippableItem->GetItemInfo()->Equip.EquipSlot;
	if (UBaseItem** RetrievedItem = EquipmentData.Find(Slot))
	{
		return GetInventoryManager()->TryToAddItemToInventory(*RetrievedItem, true);
	}

	return false;
}

/* ============================= */
/* === Equipping Items === */
/* ============================= */

bool UEquipmentManager::TryToEquip(UBaseItem* Item, bool HasMesh)
{
	if (!IsValid(Item))
	{
		UE_LOG(LogEquipmentManager, Error, TEXT("TryToEquip: Item is NULL!"));
		return false;
	}

	if (Item->ItemDefinition->Equip.MeetsRequirements(Cast<APHBaseCharacter>(OwnerCharacter)->GetCurrentStats()))
	{
		// Debug: Print what class the item actually is
		UE_LOG(LogEquipmentManager, Warning, TEXT("=== TryToEquip Debug ==="));
		UE_LOG(LogEquipmentManager, Warning, TEXT("Item Class: %s"), *Item->GetClass()->GetName());
		UE_LOG(LogEquipmentManager, Warning, TEXT("Item is UBaseItem: %s"), Item->IsA(UBaseItem::StaticClass()) ? TEXT("YES") : TEXT("NO"));
		UE_LOG(LogEquipmentManager, Warning, TEXT("Item is UEquippableItem: %s"), Item->IsA(UEquippableItem::StaticClass()) ? TEXT("YES") : TEXT("NO"));
    
		if (Item->ItemDefinition)
		{
			UE_LOG(LogEquipmentManager, Warning, TEXT("Item Name: %s"), *Item->ItemDefinition->Base.ItemName.ToString());
			UE_LOG(LogEquipmentManager, Warning, TEXT("Item Type: %d"), static_cast<int32>(Item->ItemDefinition->Base.ItemType));
		}

	
		HasMesh ? HandleHasMesh(Item, Item->ItemDefinition->Base.EquipmentSlot) : HandleNoMesh(Item, Item->ItemDefinition->Base.EquipmentSlot);
    
		UEquippableItem* EquipItem = Cast<UEquippableItem>(Item);
		if (!EquipItem)
		{
			UE_LOG(LogEquipmentManager, Error, TEXT("Cast to UEquippableItem FAILED!"));
			UE_LOG(LogEquipmentManager, Error, TEXT("This means Item was created as UBaseItem, not UEquippableItem"));
			OnEquipmentChangedSimple.Broadcast();
			return false;
		}
    
		UE_LOG(LogEquipmentManager, Log, TEXT("Cast successful! Applying weapon stats..."));
    
		if (APHBaseCharacter* Character = Cast<APHBaseCharacter>(GetOwner()))
		{
			ApplyWeaponBaseDamage(EquipItem, Character);
			ApplyItemStatBonuses(EquipItem, Character);
		}
    
		OnEquipmentChangedSimple.Broadcast();
	}
	else
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("=== TryToEquip Failed Requirements not met ==="));
		bool WasAddedToInventoy = false;
		 WasAddedToInventoy =  GetInventoryManager()->TryToAddItemToInventory(Item, false);
		UE_LOG(LogEquipmentManager, Warning, TEXT("=== Trying to add to Inventory ==="));
		if (WasAddedToInventoy)
		{
			UE_LOG(LogEquipmentManager, Warning, TEXT("=== Item added to Inventory ==="));
		}
		else
		{
			UE_LOG(LogEquipmentManager, Warning, TEXT("=== TryToAddItemToInventory Failed Inventory Full ==="));
		}
	}
	UE_LOG(LogEquipmentManager, Warning, TEXT("=== Try to Equip Had an Error should have never reached here please check   ==="));
	return false;
}


void UEquipmentManager::HandleHasMesh(UBaseItem* Item, EEquipmentSlot Slot)
{
	// If slot occupied, return OLD item to inventory or drop, then remove from slot
	if (CheckSlot(Item))
	{
		UBaseItem** EquippedItemPtr = EquipmentData.Find(Slot);
		if (EquippedItemPtr && *EquippedItemPtr)
		{
			if (!GetInventoryManager()->TryToAddItemToInventory(*EquippedItemPtr, true))
			{
				GetInventoryManager()->DropItemFromInventory(*EquippedItemPtr);
			}
			RemoveEquippedItem(*EquippedItemPtr, Slot);
		}
	}

	if (!Item)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("Equip Item was null in HandleHasMesh"));
		return;
	}

	Item->SetRotated(false);
	AttachItem(Item->GetItemInfo()->Equip.EquipClass, Item, Slot);
}


void UEquipmentManager::HandleNoMesh(UBaseItem* Item, EEquipmentSlot Slot)
{
	if (!CheckSlot(Item) || !IsValid(Item)) return;

	if (UBaseItem** EquippedItem = EquipmentData.Find(Slot))
	{
		if (!GetInventoryManager()->TryToAddItemToInventory(*EquippedItem, true))
		{
			GetInventoryManager()->DropItemFromInventory(*EquippedItem);
		}
		EquipmentData.Remove(Slot);
	}

	EquipmentData.Add(Slot, Item);
}


void UEquipmentManager::RemoveEquippedItem(UBaseItem* Item, EEquipmentSlot Slot)
{
	if (!IsValid(Item)) return;

	RemoveItemInSlot(Slot);

	if (OwnerCharacter)
	{
		// Back to default overlay when an item leaves the slot
		OwnerCharacter->SetOverlayState(EALSOverlayState::Default);

		if (UEquippableItem* Equip = Cast<UEquippableItem>(Item))
		{
			if (APHBaseCharacter* PHCharacter = Cast<APHBaseCharacter>(OwnerCharacter))
			{
				RemoveItemStatBonuses(Equip, PHCharacter);
				RemoveWeaponBaseDamage(Equip, PHCharacter);
			}
		}
	}
}


void UEquipmentManager::RemoveWeaponBaseDamage(UEquippableItem* WeaponItem, APHBaseCharacter* Character)
{
	if (!WeaponItem)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("RemoveWeaponBaseDamage failed: WeaponItem is nullptr"));
		return;
	}

	if (!Character)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("RemoveWeaponBaseDamage failed: Character is nullptr"));
		return;
	}

	UStatsManager* StatsManager = Character->GetStatsManager();
	if (!StatsManager)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("RemoveWeaponBaseDamage failed: Character has no StatsManager"));
		return;
	}

	const TMap<EDamageTypes, FDamageRange>& BaseDamageMap = WeaponItem->GetItemInfo()->Equip.WeaponBaseStats.BaseDamage;
	if (BaseDamageMap.Num() == 0)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("Weapon has no base damage defined, nothing to remove."));
		return;
	}

	for (const TPair<EDamageTypes, FDamageRange>& Pair : BaseDamageMap)
	{
		const FString TypeName = StaticEnum<EDamageTypes>()->GetNameByValue((int64)Pair.Key).ToString().RightChop(17);

		const FString MinKey = FString::Printf(TEXT("Min %s"), *TypeName);
		const FString MaxKey = FString::Printf(TEXT("Max %s"), *TypeName);

		if (const FGameplayAttribute* MinAttr = FPHGameplayTags::BaseDamageToAttributesMap.Find(MinKey))
		{
			StatsManager->ApplyFlatModifier(*MinAttr, -Pair.Value.Min);
			UE_LOG(LogEquipmentManager, Log, TEXT("Removed weapon base damage MIN: %s = %.2f"), *MinAttr->GetName(), -Pair.Value.Min);
		}
		else
		{
			UE_LOG(LogEquipmentManager, Warning, TEXT("No Min attribute found to remove for key: %s"), *MinKey);
		}

		if (const FGameplayAttribute* MaxAttr = FPHGameplayTags::BaseDamageToAttributesMap.Find(MaxKey))
		{
			StatsManager->ApplyFlatModifier(*MaxAttr, -Pair.Value.Max);
			UE_LOG(LogEquipmentManager, Log, TEXT("Removed weapon base damage MAX: %s = %.2f"), *MaxAttr->GetName(), -Pair.Value.Max);
		}
		else
		{
			UE_LOG(LogEquipmentManager, Warning, TEXT("No Max attribute found to remove for key: %s"), *MaxKey);
		}
	}
}

void UEquipmentManager::RemoveItemStatBonuses(UEquippableItem* Item, APHBaseCharacter* Character)
{
	if (!Item || !Character)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("RemoveItemStatBonuses: Invalid parameters"));
		return;
	}

	UStatsManager* StatsManager = Character->GetStatsManager();
	if (!StatsManager)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("RemoveItemStatBonuses: No StatsManager"));
		return;
	}

	// Check if we have applied stats for this item
	if (!AppliedItemStats.Contains(Item))
	{
		UE_LOG(LogEquipmentManager, Log, TEXT("No stats to remove for this item"));
		return;
	}

	// Get the full item stats
	const FPHItemStats& Stats = Item->GetFullItemStats();
	const TArray<FPHAttributeData> AllStats = Stats.GetAllStats();

	// Remove each stat modifier by applying negative values
	for (const FPHAttributeData& Attr : AllStats)
	{
		const FGameplayAttribute& TargetAttr = Attr.ModifiedAttribute;
		if (!TargetAttr.IsValid()) continue;

		const float RolledValue = Attr.RolledStatValue;
		
		// Apply negative value to remove the bonus
		StatsManager->ApplyFlatModifier(TargetAttr, -RolledValue);
		
		UE_LOG(LogEquipmentManager, Log, TEXT("Removed stat: %s = -%.2f"), 
			*TargetAttr.GetName(), RolledValue);
	}

	// Remove from tracking
	AppliedItemStats.Remove(Item);

	UE_LOG(LogEquipmentManager, Log, TEXT("Removed item stat bonuses for: %s"), 
		*Item->GetItemInfo()->Base.ItemName.ToString());
}


void UEquipmentManager::RemoveItemInSlot(EEquipmentSlot Slot)
{
	// Check if there's an equipped object in the given slot.
	AEquippedObject** pRetrievedItem = EquipmentItem.Find(Slot);
	if (!pRetrievedItem || !IsValid(*pRetrievedItem)) return;

	// Destroy the object and remove it from the mapping.
	(*pRetrievedItem)->Destroy();
	EquipmentData.Remove(Slot);
}

bool UEquipmentManager::IsSocketEmpty(EEquipmentSlot Slot) const
{
	return !EquipmentData.Contains(Slot);
}

void UEquipmentManager::AttachItem(TSubclassOf<AEquippedObject> Class, UBaseItem* Item, EEquipmentSlot Slot)
{
	if (!IsValid(Item) || !IsValid(OwnerCharacter) || !Class)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("Invalid parameters in AttachItem"));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		// Spawn the equipped actor
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AEquippedObject* SpawnedActor =
			World->SpawnActor<AEquippedObject>(Class, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (!IsValid(SpawnedActor))
		{
			UE_LOG(LogEquipmentManager, Error, TEXT("Failed to spawn equipped object"));
			return;
		}

		// Feed the item's current view into the equipped actor
		const UItemDefinitionAsset* Info = Item->GetItemInfo();
		FItemBase Base = Info->Base;
		FEquippableItemData& Equip = Info->Equip;

		SpawnedActor->InitializeFromItem(Info, Item->RuntimeData);


		FName SocketToAttach = Base.AttachmentSocket;

		if (SocketToAttach.IsNone())
		{
			if (Base.ContextualSockets.Num() > 0)
			{
				// pull first authored contextual socket as a generic default
				for (const TPair<FName, FName>& Kvp : Base.ContextualSockets)
				{
					if (!Kvp.Value.IsNone())
					{
						SocketToAttach = Kvp.Value;
						break;
					}
				}
			}
		}

		if (SocketToAttach.IsNone())
		{
			SocketToAttach = UPHItemFunctionLibrary::GetSocketNameForSlot(Slot);
		}

		if (!OwnerCharacter->GetMesh()->DoesSocketExist(SocketToAttach))
		{
			UE_LOG(LogEquipmentManager, Warning, TEXT("Socket %s does not exist on character"), *SocketToAttach.ToString());
			SpawnedActor->Destroy();
			return;
		}

		// Mesh assignment (kept as StaticMesh for now)
		if (UStaticMesh* Mesh = Base.StaticMesh)
		{
			SpawnedActor->SetMesh(Mesh);
		}
		else
		{
			UE_LOG(LogEquipmentManager, Error, TEXT("Mesh for item is null!"));
			SpawnedActor->Destroy();
			return;
		}

		// Use authorable attachment rules & offset if present, else snap like before
		const FAttachmentTransformRules AttachRules =
			Base.AttachmentRules.ToAttachmentRules(); // KeepRelative|SnapToTarget based on your struct
		SpawnedActor->AttachToComponent(OwnerCharacter->GetMesh(), AttachRules, SocketToAttach);

		// If using KeepRelative, apply per-item offset
		SpawnedActor->SetActorRelativeTransform(Base.AttachmentRules.AttachmentOffset);

		// Hand back references
		SpawnedActor->SetOwningCharacter(OwnerCharacter);
		EquipmentData.Add(Slot, Item);
		EquipmentItem.Add(Slot, SpawnedActor);

		// Drive overlay from the item's equip data (DA-backed)
		if (Equip.OverlayState != EALSOverlayState::Default)
		{
			OwnerCharacter->SetOverlayState(Equip.OverlayState);
		}
		
	}
}


void UEquipmentManager::ApplyAllEquipmentStatsToAttributes(APHBaseCharacter* Character)
{
	if (!Character) return;

	if (const UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent(); !ASC) return;

	TMap<FGameplayAttribute, float> ModifiedAttributes;

	for (UBaseItem* Item : EquipmentCheck())
	{
		if (const UEquippableItem* Equip = Cast<UEquippableItem>(Item))
		{
			const FPHItemStats& Stats = Equip->GetFullItemStats();  // Presumed getter

			for (const TArray<FPHAttributeData> AllStats = Stats.GetAllStats();
				const FPHAttributeData& Attr : AllStats)
			{
				const FGameplayAttribute& TargetAttr = Attr.ModifiedAttribute;
				if (!TargetAttr.IsValid()) continue;

				const float Rolled = Attr.RolledStatValue;

				ModifiedAttributes.FindOrAdd(TargetAttr) += Rolled;
			}
		}
	}

	// Apply all accumulated modifiers
	for (const auto& Pair : ModifiedAttributes)
	{
		Character->GetStatsManager()->ApplyFlatModifier(Pair.Key, Pair.Value);
	}
}

void UEquipmentManager::ApplyItemStatBonuses(UEquippableItem* Item, APHBaseCharacter* Character)
{
}


void UEquipmentManager::ApplyWeaponBaseDamage(UEquippableItem* WeaponItem, const APHBaseCharacter* Character)
{
	if (!WeaponItem)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("ApplyWeaponBaseDamage failed: WeaponItem is nullptr"));
		return;
	}

	if (!Character)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("ApplyWeaponBaseDamage failed: Character is nullptr"));
		return;
	}

	// Example: if your stat manager does the applying
	UStatsManager* StatsManager = Character->GetStatsManager();
	if (!StatsManager)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("ApplyWeaponBaseDamage failed: Character has no StatsManager"));
		return;
	}

	const TMap<EDamageTypes, FDamageRange>& BaseDamageMap = WeaponItem->GetItemInfo()->Equip.WeaponBaseStats.BaseDamage;
	if (BaseDamageMap.Num() == 0)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("Weapon has no base damage defined."));
		return;
	}

	// === Apply the min/max damage stats ===
	for (const TPair<EDamageTypes, FDamageRange>& Pair : BaseDamageMap)
	{
		FString TypeName = StaticEnum<EDamageTypes>()->GetNameByValue(static_cast<int64>(Pair.Key)).ToString();
		TypeName.RemoveFromStart(TEXT("EDamageTypes::"));
		TypeName.RemoveFromStart(TEXT("DT_"));


		const FString MinKey = FString::Printf(TEXT("Min %s"), *TypeName);
		const FString MaxKey = FString::Printf(TEXT("Max %s"), *TypeName);

		if (const FGameplayAttribute* MinAttr = FPHGameplayTags::BaseDamageToAttributesMap.Find(MinKey))
		{
			StatsManager->ApplyFlatModifier(*MinAttr, Pair.Value.Min);
			UE_LOG(LogEquipmentManager, Log, TEXT("Applied weapon base damage MIN: %s = %.2f"), *MinAttr->GetName(), Pair.Value.Min);
		}
		else
		{
			UE_LOG(LogEquipmentManager, Warning, TEXT("No Min attribute found for damage type key: %s"), *MinKey);
		}

		if (const FGameplayAttribute* MaxAttr = FPHGameplayTags::BaseDamageToAttributesMap.Find(MaxKey))
		{
			StatsManager->ApplyFlatModifier(*MaxAttr, Pair.Value.Max);
			UE_LOG(LogEquipmentManager, Log, TEXT("Applied weapon base damage MAX: %s = %.2f"), *MaxAttr->GetName(), Pair.Value.Max);
		}
		else
		{
			UE_LOG(LogEquipmentManager, Warning, TEXT("No Max attribute found for damage type key: %s"), *MaxKey);
		}
	}
}

// EquipmentManager.cpp
void UEquipmentManager::EquipItem(UBaseItem* Item, EEquipmentSlot Slot)
{
  	AEquippedObject** EquippedObjectPtr = EquipmentItem.Find(Slot);
	if (!EquippedObjectPtr || !*EquippedObjectPtr)
	{
		return;
	}

	AEquippedObject* EquippedObject = *EquippedObjectPtr;
	UBaseItem* LItem = EquipmentData.FindRef(Slot);

	// Unbind from equipped object's events
	EquippedObject->OnEquipped.RemoveDynamic(this, &UEquipmentManager::HandleItemEquipped);
	EquippedObject->OnUnequipped.RemoveDynamic(this, &UEquipmentManager::HandleItemUnequipped);

	// Perform unequip logic (remove stat bonuses, etc.)
	if (UEquippableItem* EquippableItem = Cast<UEquippableItem>(LItem))
	{
		if (APHBaseCharacter* Character = Cast<APHBaseCharacter>(GetOwner()))
		{
			RemoveItemStatBonuses(EquippableItem, Character);
			RemoveWeaponBaseDamage(EquippableItem, Character);
		}
	}

	// Broadcast the delegate (notify listeners that item was unequipped)
	EquippedObject->OnUnequipped.Broadcast(EquippedObject);

	// Destroy the equipped object
	EquippedObject->Destroy();

	// Remove references
	EquipmentItem.Remove(Slot);
	EquipmentData.Remove(Slot);

	// Broadcast manager-level events
	if (LItem)
	{
		OnItemUnequipped.Broadcast(Slot, LItem);
		OnEquipmentChanged.Broadcast(Slot, LItem, nullptr);
	}

	UE_LOG(LogEquipmentManager, Log, TEXT("Unequipped item from slot %s"),
		   *UEnum::GetValueAsString(Slot));
}


void UEquipmentManager::UnequipItem(EEquipmentSlot Slot)
{
	AEquippedObject** EquippedObjectPtr = EquipmentItem.Find(Slot);
	if (!EquippedObjectPtr || !*EquippedObjectPtr)
	{
		return;
	}

	AEquippedObject* EquippedObject = *EquippedObjectPtr;
	UBaseItem* Item = EquipmentData.FindRef(Slot);

	// Unbind from equipped object's events
	EquippedObject->OnEquipped.RemoveDynamic(this, &UEquipmentManager::HandleItemEquipped);
	EquippedObject->OnUnequipped.RemoveDynamic(this, &UEquipmentManager::HandleItemUnequipped);

	// Perform unequip logic (remove stat bonuses, etc.)
	if (UEquippableItem* EquippableItem = Cast<UEquippableItem>(Item))
	{
		if (APHBaseCharacter* Character = Cast<APHBaseCharacter>(GetOwner()))
		{
			RemoveItemStatBonuses(EquippableItem, Character);
			RemoveWeaponBaseDamage(EquippableItem, Character);
		}
	}

	// Broadcast the delegate (notify listeners that item was unequipped)
	EquippedObject->OnUnequipped.Broadcast(EquippedObject);

	// Destroy the equipped object
	EquippedObject->Destroy();

	// Remove references
	EquipmentItem.Remove(Slot);
	EquipmentData.Remove(Slot);

	// Broadcast manager-level events
	if (Item)
	{
		OnItemUnequipped.Broadcast(Slot, Item);
		OnEquipmentChanged.Broadcast(Slot, Item, nullptr);
	}

	UE_LOG(LogEquipmentManager, Log, TEXT("Unequipped item from slot %s"),
		   *UEnum::GetValueAsString(Slot));
}

void UEquipmentManager::SwapEquipment(EEquipmentSlot FromSlot, EEquipmentSlot ToSlot)
{
	if (FromSlot == ToSlot)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("Cannot swap equipment to the same slot"));
		return;
	}

	UBaseItem* FromItem = EquipmentData.FindRef(FromSlot);
	UBaseItem* ToItem = EquipmentData.FindRef(ToSlot);

	if (!FromItem)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("No item in FromSlot to swap"));
		return;
	}

	// Unequip both items
	if (FromItem) UnequipItem(FromSlot);
	if (ToItem) UnequipItem(ToSlot);

	// Re-equip in swapped positions
	if (FromItem) EquipItem(FromItem, ToSlot);
	if (ToItem) EquipItem(ToItem, FromSlot);

	UE_LOG(LogEquipmentManager, Log, TEXT("Swapped equipment between slots"));
}

void UEquipmentManager::HandleItemEquipped(AEquippedObject* EquippedObject, AActor* Owner)
{
	UE_LOG(LogEquipmentManager, Log, TEXT("Item equipped event: %s"),
		   *EquippedObject->GetDisplayName().ToString());
    
	// Custom behavior when item is equipped

}

void UEquipmentManager::HandleItemUnequipped(AEquippedObject* EquippedObject)
{
	UE_LOG(LogEquipmentManager, Log, TEXT("Item unequipped event: %s"),
		   *EquippedObject->GetDisplayName().ToString());
    
	// Custom behavior when item is unequipped
}

// EquipmentManager.cpp


void UEquipmentManager::ResetAllEquipmentBonuses(APHBaseCharacter* Character)
{
	// Copy keys to avoid modifying the map during iteration
	TArray<const UEquippableItem*> Keys;
	AppliedItemStats.GetKeys(Keys);

	for (const UEquippableItem* Item : Keys)
	{
		RemoveItemStatBonuses(const_cast<UEquippableItem*>(Item), Character); // Cast safe for removal
	}

	AppliedItemStats.Empty();
}


bool UEquipmentManager::DropItem(UBaseItem* Item)
{
	if (!IsValid(Item))
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("DropItem failed: Item is invalid"));
		return false;
	}

	if (!GetWorld())
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("DropItem failed: World context is null"));
		return false;
	}

	// Ensure the pickup class exists
	const TSubclassOf<AItemPickup> Class = Item->GetItemInfo()->Base.PickupClass;
	if (!Class)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("DropItem failed: Pickup class is null"));
		return false;
	}

	// Spawn parameters
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn the item pickup
	AItemPickup* DroppedItem = GetWorld()->SpawnActor<AItemPickup>(Class, GetGroundSpawnLocation(), FRotator::ZeroRotator, SpawnParams);
	if (!IsValid(DroppedItem))
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("DropItem failed: Unable to spawn item pickup"));
		return false;
	}

	// Set properties of the dropped item
	DroppedItem->ObjItem->ItemDefinition = Item->GetItemInfo();
	DroppedItem->SetNewMesh(Item->GetItemInfo()->Base.StaticMesh);
	DroppedItem->SetSkeletalMesh(Item->GetItemInfo()->Base.SkeletalMesh);

	return true;
}

UInventoryManager* UEquipmentManager::GetInventoryManager() const
{
	// Validate OwnerCharacter first
	if (!IsValid(OwnerCharacter))
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("GetInventoryManager failed: OwnerCharacter is null"));
		return nullptr;
	}

	// Get the InventoryManager component
	return OwnerCharacter->FindComponentByClass<UInventoryManager>();
}

FVector UEquipmentManager::GetGroundSpawnLocation() const
{
	// Validate OwnerCharacter
	if (!IsValid(OwnerCharacter))
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("GetGroundSpawnLocation failed: OwnerCharacter is null"));
		return FVector::ZeroVector;
	}

	// Generate a random forward offset within a reasonable range
	const FVector ForwardOffset = OwnerCharacter->GetActorForwardVector() * 200.0f;
	const FVector RandomOffset = UKismetMathLibrary::RandomUnitVector() * 50.0f;

	// Compute initial target location
	FVector TargetLocation = OwnerCharacter->GetActorLocation() + ForwardOffset + RandomOffset;

	// Perform a downward trace to find the ground
	FFindFloorResult FloorResult;
	OwnerCharacter->GetCharacterMovement()->FindFloor(TargetLocation, FloorResult, true);

	// If we find a valid floor, adjust the Z position
	if (FloorResult.bBlockingHit)
	{
		TargetLocation.Z = FloorResult.HitResult.ImpactPoint.Z + 120.0f; // Small offset above ground
		return TargetLocation;
	}

	// If no valid floor is found, log a warning and return a safe default location
	UE_LOG(LogEquipmentManager, Warning, TEXT("GetGroundSpawnLocation: No valid floor found, using fallback location"));
	return OwnerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f); // Slightly above the character
}



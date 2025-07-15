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
	if (!IsValid(Item)) return false;

	// Check if the item's target slot exists in the EquipmentData map
	UEquippableItem* EquippableItem = Cast<UEquippableItem>(Item);
	if (!EquippableItem) return false;

	const EEquipmentSlot Slot = EquippableItem->GetItemInfo().ItemData.EquipSlot;
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

	return EquippableTypes.Contains(Item->GetItemInfo().ItemInfo.ItemType);
}

/* ============================= */
/* === Inventory & Equipment Management === */
/* ============================= */

bool UEquipmentManager::AddItemInSlotToInventory(UBaseItem* Item)
{
	if (!IsValid(Item)) return false;

	UEquippableItem* EquippableItem = Cast<UEquippableItem>(Item);
	if (!EquippableItem) return false;

	const EEquipmentSlot Slot = EquippableItem->GetItemInfo().ItemData.EquipSlot;
	if (UBaseItem** RetrievedItem = EquipmentData.Find(Slot))
	{
		return GetInventoryManager()->TryToAddItemToInventory(*RetrievedItem, true);
	}

	return false;
}

/* ============================= */
/* === Equipping Items === */
/* ============================= */

void UEquipmentManager::TryToEquip(UBaseItem* Item, bool HasMesh, EEquipmentSlot Slot)
{
	if (!IsValid(Item)) return;

	(HasMesh) ? HandleHasMesh(Item, Slot) : HandleNoMesh(Item, Slot);

	// === Apply Stat Bonuses ===
	if (UEquippableItem* EquipItem = Cast<UEquippableItem>(Item))
	{
		if (APHBaseCharacter* Character = Cast<APHBaseCharacter>(GetOwner()))
		{
			// Apply all affix and passive bonuses
			ApplyItemStatBonuses(EquipItem, Character);
		}
	}
	OnEquipmentChanged.Broadcast();
}


void UEquipmentManager::HandleHasMesh(UBaseItem* Item, EEquipmentSlot Slot)
{
	if (!CheckSlot(Item)) return;

	// Retrieve the currently equipped item in the slot
	UBaseItem** EquippedItemPtr = EquipmentData.Find(Slot);

	if (UBaseItem* EquippedItem = (EquippedItemPtr) ? *EquippedItemPtr : nullptr; IsValid(EquippedItem))
	{
		// Try adding old equipment back to inventory; if it fails, drop it
		if (!GetInventoryManager()->TryToAddItemToInventory(EquippedItem, true))
		{
			GetInventoryManager()->DropItemInInventory(EquippedItem);
		}

		// Remove the currently equipped item from the slot
		RemoveEquippedItem(EquippedItem, Slot);
	}

	// Attach the new item
	if (Item)
	{
		Item->SetRotated(false);
		AttachItem(Item->GetItemInfo().ItemData.EquipClass, Item, Slot);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Equip Item came back null (Check EquipmentManager 107)"));
	}
}

void UEquipmentManager::HandleNoMesh(UBaseItem* Item, EEquipmentSlot Slot)
{
	if (!CheckSlot(Item) || !IsValid(Item)) return;

	// Try to find the currently equipped item in the given slot.
	if (UBaseItem** EquippedItem = EquipmentData.Find(Slot))
	{
		// If an item is equipped, attempt to return it to inventory.
		if (!GetInventoryManager()->TryToAddItemToInventory(*EquippedItem, true))
		{
			// If adding to inventory fails, drop the item.
			GetInventoryManager()->DropItemInInventory(*EquippedItem);
		}
		
		// Remove the equipped item from the slot.
		EquipmentData.Remove(Slot);
	}

	// Equip the new item.
	EquipmentData.Add(Slot, Item);
}

void UEquipmentManager::RemoveEquippedItem(UBaseItem* Item, EEquipmentSlot Slot)
{
	if (!IsValid(Item)) return;

	RemoveItemInSlot(Slot); // Handles detaching from the equipment slot

	if (OwnerCharacter)
	{
		OwnerCharacter->SetOverlayState(EALSOverlayState::Default);

		// Safely cast to equippable and remove flat stats
		if (UEquippableItem* Equip = Cast<UEquippableItem>(Item))
		{
			if (APHBaseCharacter* PHCharacter = Cast<APHBaseCharacter>(OwnerCharacter))
			{
				// Optional: remove passive effects
				RemoveItemStatBonuses(Equip, PHCharacter);
			}
		}
	}
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
		UE_LOG(LogTemp, Warning, TEXT("Invalid parameters in AttachItem"));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		// Spawn parameters
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// Spawn the actor
		AEquippedObject* SpawnedActor = World->SpawnActor<AEquippedObject>(Class, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (!IsValid(SpawnedActor))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to spawn equipped object"));
			return;
		}

		// Set item info
		SpawnedActor->SetItemInfo(Item->GetItemInfo());

		// Find the correct socket name and attach the actor
		const FName SlotToAttachTo = UPHItemFunctionLibrary::GetSocketNameForSlot(Slot);
		if (!OwnerCharacter->GetMesh()->DoesSocketExist(SlotToAttachTo))
		{
			UE_LOG(LogTemp, Warning, TEXT("Socket %s does not exist on character"), *SlotToAttachTo.ToString());
			return;
		}

		// Set the mesh and validate it
		if (UStaticMesh* Mesh = Item->GetItemInfo().ItemInfo.StaticMesh)
		{
			SpawnedActor->SetMesh(Mesh);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Mesh for item is null!"));
			return;
		}

		// Attach the object and update equipment data
		SpawnedActor->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SlotToAttachTo);
		SpawnedActor->SetOwningCharacter(OwnerCharacter);

		// Update equipment mappings
		EquipmentData.Add(Slot, Item);
		EquipmentItem.Add(Slot, SpawnedActor);
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
		Character->GetStatsManager()->ApplyFlatStatModifier(Pair.Key, Pair.Value);
	}
}



void UEquipmentManager::ApplyItemStatBonuses(UEquippableItem* Item, APHBaseCharacter* Character)
{
	if (!Item || !Character) return;

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC) return;

	// === Apply Attribute Bonuses from FPHItemStats ===
	const FPHItemStats& ItemStats = Item->GetItemInfo().ItemData.Affixes;
	const TArray<FPHAttributeData>& Stats = ItemStats.GetAllStats();

	FAppliedStats AppliedStats;
	AppliedStats.Stats = Stats;

        for (const FPHAttributeData& Attr : Stats)
        {
                if (!Attr.ModifiedAttribute.IsValid()) continue;

                const float Rolled = Attr.RolledStatValue;
                AppliedStats.RolledValues.Add(Rolled);

                const float CurrentValue = ASC->GetNumericAttribute(Attr.ModifiedAttribute);
                ASC->SetNumericAttributeBase(Attr.ModifiedAttribute, CurrentValue + Rolled);
        }

        // === Apply Base Weapon Damage to Attributes ===
        const TMap<EDamageTypes, FDamageRange>& BaseDamageMap = Item->GetItemInfo().ItemData.WeaponBaseStats.BaseDamage;
        for (const TPair<EDamageTypes, FDamageRange>& Pair : BaseDamageMap)
        {
                const FString TypeName = DamageTypeToString(Pair.Key);

                const FString MinKey = FString::Printf(TEXT("Min %s"), *TypeName);
                const FString MaxKey = FString::Printf(TEXT("Max %s"), *TypeName);

                if (const FGameplayAttribute* MinAttr = FPHGameplayTags::BaseDamageToAttributesMap.Find(MinKey))
                {
                        Character->GetStatsManager()->ApplyFlatStatModifier(*MinAttr, Pair.Value.Min);
                        AppliedStats.BaseDamageAttributes.Add(*MinAttr);
                        AppliedStats.BaseDamageValues.Add(Pair.Value.Min);
                }

                if (const FGameplayAttribute* MaxAttr = FPHGameplayTags::BaseDamageToAttributesMap.Find(MaxKey))
                {
                        Character->GetStatsManager()->ApplyFlatStatModifier(*MaxAttr, Pair.Value.Max);
                        AppliedStats.BaseDamageAttributes.Add(*MaxAttr);
                        AppliedStats.BaseDamageValues.Add(Pair.Value.Max);
                }
        }

        if (AppliedStats.Stats.Num() > 0 || AppliedStats.BaseDamageAttributes.Num() > 0)
        {
                AppliedItemStats.Add(Item, AppliedStats);
        }

	// === Apply Passive Effects ===
	const TArray<FItemPassiveEffectInfo>& Passives = Item->GetItemInfo().ItemData.PassiveEffects;
	FPassiveEffectHandleList PassiveHandles;

	for (const FItemPassiveEffectInfo& Passive : Passives)
	{
		if (!Passive.EffectClass) continue;

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(Item);

		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Passive.EffectClass, Passive.Level, Context);
		if (Spec.IsValid())
		{
			FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			PassiveHandles.Handles.Add(Handle);
		}
	}

	if (PassiveHandles.Handles.Num() > 0)
	{
		AppliedPassiveEffects.Add(Item, PassiveHandles);
	}
}

void UEquipmentManager::RemoveItemStatBonuses(UEquippableItem* Item, APHBaseCharacter* Character)
{
	if (!Item || !Character) return;

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC) return;

	// === Remove Stat Bonuses ===
	if (AppliedItemStats.Contains(Item))
	{
		const FAppliedStats& Stats = AppliedItemStats[Item];

                for (int32 i = 0; i < Stats.Stats.Num(); ++i)
                {
                        const FPHAttributeData& Attr = Stats.Stats[i];
                        if (!Attr.ModifiedAttribute.IsValid()) continue;

                        const float Rolled = Stats.RolledValues.IsValidIndex(i) ? Stats.RolledValues[i] : 0.0f;
                        Character->GetStatsManager()->ApplyFlatStatModifier(Attr.ModifiedAttribute, -Rolled);
                        UE_LOG(LogTemp, Log, TEXT("Removing %.2f from %s"), Rolled, *Attr.ModifiedAttribute.GetName());
                }

                for (int32 i = 0; i < Stats.BaseDamageAttributes.Num(); ++i)
                {
                        const FGameplayAttribute& Attr = Stats.BaseDamageAttributes[i];
                        const float Value = Stats.BaseDamageValues.IsValidIndex(i) ? Stats.BaseDamageValues[i] : 0.0f;
                        if (!Attr.IsValid()) continue;

                        Character->GetStatsManager()->ApplyFlatStatModifier(Attr, -Value);
                }

                AppliedItemStats.Remove(Item);
	}

	// === Remove Passive Effects ===
	if (AppliedPassiveEffects.Contains(Item))
	{
		for (const FActiveGameplayEffectHandle& Handle : AppliedPassiveEffects[Item].Handles)
		{
			if (Handle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(Handle);
			}
		}
		AppliedPassiveEffects.Remove(Item);
	}
}


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
		UE_LOG(LogTemp, Warning, TEXT("DropItem failed: Item is invalid"));
		return false;
	}

	if (!GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("DropItem failed: World context is null"));
		return false;
	}

	// Ensure the pickup class exists
	const TSubclassOf<AItemPickup> Class = Item->GetItemInfo().ItemInfo.PickupClass;
	if (!Class)
	{
		UE_LOG(LogTemp, Warning, TEXT("DropItem failed: Pickup class is null"));
		return false;
	}

	// Spawn parameters
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn the item pickup
	AItemPickup* DroppedItem = GetWorld()->SpawnActor<AItemPickup>(Class, GetGroundSpawnLocation(), FRotator::ZeroRotator, SpawnParams);
	if (!IsValid(DroppedItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("DropItem failed: Unable to spawn item pickup"));
		return false;
	}

	// Set properties of the dropped item
	DroppedItem->ItemInfo = Item->GetItemInfo();
	DroppedItem->SetNewMesh(Item->GetItemInfo().ItemInfo.StaticMesh);
	DroppedItem->SetSkeletalMesh(Item->GetItemInfo().ItemInfo.SkeletalMesh);

	return true;
}

UInventoryManager* UEquipmentManager::GetInventoryManager() const
{
	// Validate OwnerCharacter first
	if (!IsValid(OwnerCharacter))
	{
		UE_LOG(LogTemp, Warning, TEXT("GetInventoryManager failed: OwnerCharacter is null"));
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
		UE_LOG(LogTemp, Warning, TEXT("GetGroundSpawnLocation failed: OwnerCharacter is null"));
		return FVector::ZeroVector;
	}

	// Generate a random forward offset within a reasonable range
	const FVector ForwardOffset = OwnerCharacter->GetActorForwardVector() * 200.0f;
	const FVector RandomOffset = UKismetMathLibrary::RandomUnitVector() * 50.0f; // Slight randomness

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
	UE_LOG(LogTemp, Warning, TEXT("GetGroundSpawnLocation: No valid floor found, using fallback location"));
	return OwnerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f); // Slightly above the character
}



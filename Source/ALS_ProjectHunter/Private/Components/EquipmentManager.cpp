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

	HasMesh ? HandleHasMesh(Item, Slot) : HandleNoMesh(Item, Slot);

	// Apply stats (unchanged)
	if (UEquippableItem* EquipItem = Cast<UEquippableItem>(Item))
	{
		if (APHBaseCharacter* Character = Cast<APHBaseCharacter>(GetOwner()))
		{
			ApplyWeaponBaseDamage(EquipItem, Character);
			ApplyItemStatBonuses(EquipItem, Character);
		}
	}

	OnEquipmentChanged.Broadcast();
}



void UEquipmentManager::HandleHasMesh(UBaseItem* Item, EEquipmentSlot Slot)
{
	if (!CheckSlot(Item)) return;

	// If slot occupied, return to inventory or drop, then remove from slot
	if (UBaseItem** EquippedItemPtr = EquipmentData.Find(Slot))
	{
		if (!GetInventoryManager()->TryToAddItemToInventory(*EquippedItemPtr, true))
		{
			GetInventoryManager()->DropItemFromInventory(*EquippedItemPtr);
		}
		RemoveEquippedItem(*EquippedItemPtr, Slot);
	}

	if (!Item)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("Equip Item was null in HandleHasMesh"));
		return;
	}

	Item->SetRotated(false);
	AttachItem(Item->GetItemInfo().ItemData.EquipClass, Item, Slot);
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
		// Back to default overlay when item leaves the slot
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

	const TMap<EDamageTypes, FDamageRange>& BaseDamageMap = WeaponItem->GetItemInfo().ItemData.WeaponBaseStats.BaseDamage;
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
			StatsManager->ApplyFlatStatModifier(*MinAttr, -Pair.Value.Min);
			UE_LOG(LogEquipmentManager, Log, TEXT("Removed weapon base damage MIN: %s = %.2f"), *MinAttr->GetName(), -Pair.Value.Min);
		}
		else
		{
			UE_LOG(LogEquipmentManager, Warning, TEXT("No Min attribute found to remove for key: %s"), *MinKey);
		}

		if (const FGameplayAttribute* MaxAttr = FPHGameplayTags::BaseDamageToAttributesMap.Find(MaxKey))
		{
			StatsManager->ApplyFlatStatModifier(*MaxAttr, -Pair.Value.Max);
			UE_LOG(LogEquipmentManager, Log, TEXT("Removed weapon base damage MAX: %s = %.2f"), *MaxAttr->GetName(), -Pair.Value.Max);
		}
		else
		{
			UE_LOG(LogEquipmentManager, Warning, TEXT("No Max attribute found to remove for key: %s"), *MaxKey);
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
		const FItemInformation& Info = Item->GetItemInfo();
		const FItemBase&       Base = Info.ItemInfo;
		const FEquippableItemData& Equip = Info.ItemData;

		SpawnedActor->SetItemInfo(Info);

		// Resolve the desired socket:
		// 1) Use explicit per-item socket if provided
		// 2) (Optional) Use the first ContextualSockets entry if any were authored
		// 3) Fallback to your library's mapping by slot
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

void UEquipmentManager::ApplyWeaponBaseDamage(UEquippableItem* WeaponItem, APHBaseCharacter* Character)
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

	const TMap<EDamageTypes, FDamageRange>& BaseDamageMap = WeaponItem->GetItemInfo().ItemData.WeaponBaseStats.BaseDamage;
	if (BaseDamageMap.Num() == 0)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("Weapon has no base damage defined."));
		return;
	}

	// === Apply the min/max damage stats ===
	for (const TPair<EDamageTypes, FDamageRange>& Pair : BaseDamageMap)
	{
		const FString TypeName = StaticEnum<EDamageTypes>()->GetNameByValue((int64)Pair.Key).ToString().RightChop(17);


		const FString MinKey = FString::Printf(TEXT("Min %s"), *TypeName);
		const FString MaxKey = FString::Printf(TEXT("Max %s"), *TypeName);

		if (const FGameplayAttribute* MinAttr = FPHGameplayTags::BaseDamageToAttributesMap.Find(MinKey))
		{
			StatsManager->ApplyFlatStatModifier(*MinAttr, Pair.Value.Min);
			UE_LOG(LogEquipmentManager, Log, TEXT("Applied weapon base damage MIN: %s = %.2f"), *MinAttr->GetName(), Pair.Value.Min);
		}
		else
		{
			UE_LOG(LogEquipmentManager, Warning, TEXT("No Min attribute found for damage type key: %s"), *MinKey);
		}

		if (const FGameplayAttribute* MaxAttr = FPHGameplayTags::BaseDamageToAttributesMap.Find(MaxKey))
		{
			StatsManager->ApplyFlatStatModifier(*MaxAttr, Pair.Value.Max);
			UE_LOG(LogEquipmentManager, Log, TEXT("Applied weapon base damage MAX: %s = %.2f"), *MaxAttr->GetName(), Pair.Value.Max);
		}
		else
		{
			UE_LOG(LogEquipmentManager, Warning, TEXT("No Max attribute found for damage type key: %s"), *MaxKey);
		}
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
                        UE_LOG(LogEquipmentManager, Log, TEXT("Removing %.2f from %s"), Rolled, *Attr.ModifiedAttribute.GetName());
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
		UE_LOG(LogEquipmentManager, Warning, TEXT("DropItem failed: Item is invalid"));
		return false;
	}

	if (!GetWorld())
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("DropItem failed: World context is null"));
		return false;
	}

	// Ensure the pickup class exists
	const TSubclassOf<AItemPickup> Class = Item->GetItemInfo().ItemInfo.PickupClass;
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



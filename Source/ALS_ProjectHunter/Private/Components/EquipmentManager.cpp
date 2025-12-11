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
#include "AbilitySystem/PHAttributeSet.h"
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
			UE_LOG(LogEquipmentManager, Warning, TEXT("Item Name: %s"), *Item->RuntimeData.DisplayName.ToString());
			UE_LOG(LogEquipmentManager, Warning, TEXT("Item Type: %s"), *UEnum::GetValueAsString(Item->ItemDefinition->Base.ItemType));
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
		return true;
	}
	else
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("=== TryToEquip Failed Requirements not met ==="));

		bool WasAddedToInventoy = GetInventoryManager()->TryToAddItemToInventory(Item, false);
		
		UE_LOG(LogEquipmentManager, Warning, TEXT("=== Trying to add to Inventory ==="));
		if (WasAddedToInventoy)
		{
			UE_LOG(LogEquipmentManager, Warning, TEXT("=== Item added to Inventory ==="));
			return true;
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
	if (!WeaponItem || !Character)
	{
		return;
	}

	UStatsManager* StatsManager = Character->GetStatsManager();
	if (!StatsManager || !StatsManager->GetASC())
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("RemoveWeaponBaseDamage: No StatsManager or ASC"));
		return;
	}

	// Find and remove all weapon effects
	if (FWeaponEffectHandles* WeaponHandles = AppliedWeaponEffect.Find(WeaponItem))
	{
		int32 RemovedCount = 0;
		for (const FActiveGameplayEffectHandle& Handle : WeaponHandles->Handles)
		{
			if (Handle.IsValid())
			{
				if (StatsManager->GetASC()->RemoveActiveGameplayEffect(Handle))
				{
					RemovedCount++;
				}
			}
		}
        
		UE_LOG(LogEquipmentManager, Log, TEXT("Removed %d weapon effects for %s"), 
			RemovedCount, *WeaponItem->GetItemInfo()->Base.ItemName.ToString());
        
		// Remove from tracking map
		AppliedWeaponEffect.Remove(WeaponItem);
	}
	else
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("No applied effects found for weapon %s"), 
			*WeaponItem->GetItemInfo()->Base.ItemName.ToString());
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
    if (!Item || !Character)
    {
        UE_LOG(LogEquipmentManager, Warning, TEXT("ApplyItemStatBonuses: Invalid parameters"));
        return;
    }

    UStatsManager* StatsManager = Character->GetStatsManager();
    if (!StatsManager)
    {
        UE_LOG(LogEquipmentManager, Warning, TEXT("ApplyItemStatBonuses: No StatsManager"));
        return;
    }

    const UItemDefinitionAsset* ItemDefinition = Item->GetItemInfo();
    if (!ItemDefinition)
    {
        UE_LOG(LogEquipmentManager, Warning, TEXT("ApplyItemStatBonuses: No ItemDefinition"));
        return;
    }

    
    FAppliedItemStats& AppliedStats = AppliedItemStats.FindOrAdd(Item);
    AppliedStats.Stats.Empty();
    AppliedStats.Values.Empty();
	
    auto ApplyStatWithFallback = [&](const FGameplayAttribute& Attribute, float Value, const FString& StatName)
    {
        if (FMath::IsNearlyZero(Value))
        {
            return;
        }

        // Try to get the effect class for this attribute
        TSubclassOf<UGameplayEffect> EffectClass = StatsManager->GetEffectClassForAttribute(Attribute);
        
        if (EffectClass)
        {
           
            FActiveGameplayEffectHandle Handle = StatsManager->ApplyFlatModifier(Attribute, Value, false);
            
            if (Handle.IsValid())
            {
                AppliedStats.Stats.Add(Attribute);
                AppliedStats.Values.Add(Value);
                UE_LOG(LogEquipmentManager, Log, TEXT("Applied %s via GE: %.2f"), *StatName, Value);
            }
            else
            {
                UE_LOG(LogEquipmentManager, Warning, TEXT("Failed to apply %s GE modifier"), *StatName);
            }
        }
        else
        {
            // Fallback: Direct attribute modification (not recommended for multiplayer)
            UE_LOG(LogEquipmentManager, Warning, TEXT("No effect class mapped for %s, using direct modification"), *StatName);
            
            float CurrentValue = StatsManager->GetAttributeBase(Attribute);
            StatsManager->SetAttributeBase(Attribute, CurrentValue + Value);
            
            AppliedStats.Stats.Add(Attribute);
            AppliedStats.Values.Add(Value);
            UE_LOG(LogEquipmentManager, Log, TEXT("Applied %s directly: %.2f"), *StatName, Value);
        }
    };

    // === Apply Base Armor/Weapon Stats ===
    ApplyStatWithFallback(UPHAttributeSet::GetArmourAttribute(), 
        ItemDefinition->Equip.ArmorBaseStats.Armor, TEXT("Armor"));
    
    ApplyStatWithFallback(UPHAttributeSet::GetPoiseAttribute(), 
        ItemDefinition->Equip.ArmorBaseStats.Poise, TEXT("Poise"));
    
    ApplyStatWithFallback(UPHAttributeSet::GetCritChanceAttribute(), 
        ItemDefinition->Equip.WeaponBaseStats.CritChance, TEXT("CritChance"));
    
    ApplyStatWithFallback(UPHAttributeSet::GetAttackSpeedAttribute(), 
        ItemDefinition->Equip.WeaponBaseStats.AttackSpeed, TEXT("AttackSpeed"));
    
    ApplyStatWithFallback(UPHAttributeSet::GetAttackRangeAttribute(), 
        ItemDefinition->Equip.WeaponBaseStats.WeaponRange, TEXT("WeaponRange"));

    // === Apply Base Resistances ===
    for (const TPair<EDefenseTypes, float>& Resistance : ItemDefinition->Equip.ArmorBaseStats.Resistances)
    {
        if (FMath::IsNearlyZero(Resistance.Value))
        {
            continue;
        }

        FGameplayAttribute ResistanceAttr;
        FString ResistanceName;

        switch (Resistance.Key)
        {
        case EDefenseTypes::DFT_FireResistance:
            ResistanceAttr = UPHAttributeSet::GetFireResistanceFlatBonusAttribute();
            ResistanceName = TEXT("FireResistance");
            break;
        case EDefenseTypes::DFT_IceResistance:
            ResistanceAttr = UPHAttributeSet::GetIceResistanceFlatBonusAttribute();
            ResistanceName = TEXT("IceResistance");
            break;
        case EDefenseTypes::DFT_LightningResistance:
            ResistanceAttr = UPHAttributeSet::GetLightningResistanceFlatBonusAttribute();
            ResistanceName = TEXT("LightningResistance");
            break;
        case EDefenseTypes::DFT_LightResistance:
            ResistanceAttr = UPHAttributeSet::GetLightResistanceFlatBonusAttribute();
            ResistanceName = TEXT("LightResistance");
            break;
        case EDefenseTypes::DFT_CorruptionResistance:
            ResistanceAttr = UPHAttributeSet::GetCorruptionResistanceFlatBonusAttribute();
            ResistanceName = TEXT("CorruptionResistance");
            break;
        default:
            continue;
        }

        ApplyStatWithFallback(ResistanceAttr, Resistance.Value, ResistanceName);
    }

    // === Apply ALL Affix Stats (Generic) ===
    const FPHItemStats& Stats = Item->GetFullItemStats();
    const TArray<FPHAttributeData> AllStats = Stats.GetAllStats();

    for (const FPHAttributeData& Attr : AllStats)
    {
      
        if (Attr.bAffectsBaseWeaponStatsDirectly)
        {
            UE_LOG(LogEquipmentManager, Verbose, TEXT("Skipping weapon-local stat: %s"), 
                *Attr.ModifiedAttribute.GetName());
            continue;
        }

        const FGameplayAttribute& TargetAttr = Attr.ModifiedAttribute;
        if (!TargetAttr.IsValid())
        {
            UE_LOG(LogEquipmentManager, Warning, TEXT("Invalid attribute in affix"));
            continue;
        }

        const float RolledValue = Attr.RolledStatValue;
        if (FMath::IsNearlyZero(RolledValue))
        {
            continue;
        }

        ApplyStatWithFallback(TargetAttr, RolledValue, TargetAttr.GetName());
    }

    UE_LOG(LogEquipmentManager, Log, TEXT("=== Applied %d stat bonuses for item: %s ==="), 
        AppliedStats.Stats.Num(), *ItemDefinition->Base.ItemName.ToString());
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

    // Get or create the weapon effect handles struct
    FWeaponEffectHandles& WeaponHandles = AppliedWeaponEffect.FindOrAdd(WeaponItem);
    WeaponHandles.Handles.Empty(); // Clear any existing handles first

    // === Apply the min/max damage stats ===
    for (const TPair<EDamageTypes, FDamageRange>& Pair : BaseDamageMap)
    {
        FString TypeName = StaticEnum<EDamageTypes>()->GetNameByValue(static_cast<int64>(Pair.Key)).ToString();
        TypeName.RemoveFromStart(TEXT("EDamageTypes::"));
        TypeName.RemoveFromStart(TEXT("DT_"));

    	UE_LOG(LogEquipmentManager, Log, TEXT("Type name used is : %s"), *TypeName);
        const FString MinKey = FString::Printf(TEXT("Min %s"), *TypeName);
        const FString MaxKey = FString::Printf(TEXT("Max %s"), *TypeName);
    	UE_LOG(LogEquipmentManager, Log, TEXT("Type name used for min is : %s"), *MinKey);
    	UE_LOG(LogEquipmentManager, Log, TEXT("Type name used for max is : %s"), *MaxKey);
        // Apply MIN damage
        if (const FGameplayAttribute* MinAttr = FPHGameplayTags::BaseDamageToAttributesMap.Find(MinKey))
        {
            FActiveGameplayEffectHandle MinHandle = StatsManager->ApplyFlatModifier(*MinAttr, Pair.Value.Min, false);
            
            if (MinHandle.IsValid())
            {
                WeaponHandles.Handles.Add(MinHandle);
                UE_LOG(LogEquipmentManager, Log, TEXT("Applied weapon base damage MIN: %s = %.2f"), *MinAttr->GetName(), Pair.Value.Min);
            }
            else
            {
                UE_LOG(LogEquipmentManager, Error, TEXT("Failed to apply MIN damage modifier for %s"), *MinKey);
            }
        }
        else
        {
            UE_LOG(LogEquipmentManager, Warning, TEXT("No Min attribute found for damage type key: %s"), *MinKey);
        }

        // Apply MAX damage
        if (const FGameplayAttribute* MaxAttr = FPHGameplayTags::BaseDamageToAttributesMap.Find(MaxKey))
        {
            FActiveGameplayEffectHandle MaxHandle = StatsManager->ApplyFlatModifier(*MaxAttr, Pair.Value.Max, false);
            
            if (MaxHandle.IsValid())
            {
                WeaponHandles.Handles.Add(MaxHandle);
                UE_LOG(LogEquipmentManager, Log, TEXT("Applied weapon base damage MAX: %s = %.2f"), *MaxAttr->GetName(), Pair.Value.Max);
            }
            else
            {
                UE_LOG(LogEquipmentManager, Error, TEXT("Failed to apply MAX damage modifier for %s"), *MaxKey);
            }
        }
        else
        {
            UE_LOG(LogEquipmentManager, Warning, TEXT("No Max attribute found for damage type key: %s"), *MaxKey);
        }
    }

    // Verify the effects were applied
    UE_LOG(LogEquipmentManager, Log, TEXT("Total weapon effects applied: %d"), WeaponHandles.Handles.Num());
}

void UEquipmentManager::ApplyAffixesEffects(const UEquippableItem* Item, APHBaseCharacter* Character)
{
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



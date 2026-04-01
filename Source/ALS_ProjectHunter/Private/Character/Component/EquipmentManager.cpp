// Character/Component/EquipmentManager.cpp

#include "Character/Component/EquipmentManager.h"
#include "Character/Component/InventoryManager.h"
#include "Character/Component/StatsManager.h"
#include "Item/ItemInstance.h"
#include "Item/Library/ItemFunctionLibrary.h"
#include "Item/Library/ItemStructs.h"
#include "Item/Runtime/EquippedItemRuntimeActor.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogEquipmentManager);

namespace EquipmentManagerPrivate
{
	const EEquipmentSlot ManagedEquipmentSlots[] =
	{
		EEquipmentSlot::ES_MainHand,
		EEquipmentSlot::ES_OffHand,
		EEquipmentSlot::ES_TwoHand,
		EEquipmentSlot::ES_Head,
		EEquipmentSlot::ES_Chest,
		EEquipmentSlot::ES_Hands,
		EEquipmentSlot::ES_Legs,
		EEquipmentSlot::ES_Feet,
		EEquipmentSlot::ES_Ring1,
		EEquipmentSlot::ES_Ring2,
		EEquipmentSlot::ES_Ring3,
		EEquipmentSlot::ES_Ring4,
		EEquipmentSlot::ES_Ring5,
		EEquipmentSlot::ES_Ring6,
		EEquipmentSlot::ES_Ring7,
		EEquipmentSlot::ES_Ring8,
		EEquipmentSlot::ES_Ring9,
		EEquipmentSlot::ES_Ring10,
		EEquipmentSlot::ES_Amulet,
		EEquipmentSlot::ES_Belt
	};
}

UEquipmentManager::UEquipmentManager(): InventoryManager(nullptr), AbilitySystemComponent(nullptr),
                                        StatsManager(nullptr), CharacterMesh(nullptr)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEquipmentManager::BeginPlay()
{
	Super::BeginPlay();
	CacheComponents();
	
	// Rebuild map from array (for save game loads or late joiners)
	RebuildEquipmentMap();

	if (bAutoUpdateWeapons)
	{
		for (const EEquipmentSlot Slot : EquipmentManagerPrivate::ManagedEquipmentSlots)
		{
			UpdateEquippedWeapon(Slot, GetEquippedItem(Slot));
		}
	}
}

void UEquipmentManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UEquipmentManager, EquippedItemsArray);
}

void UEquipmentManager::CacheComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Cache inventory manager
	InventoryManager = Owner->FindComponentByClass<UInventoryManager>();
	if (!InventoryManager)
	{
		UE_LOG(LogEquipmentManager, Error, TEXT("EquipmentManager: No InventoryManager found on %s"), *Owner->GetName());
	}

	// Cache stats manager
	StatsManager = Owner->FindComponentByClass<UStatsManager>();
	if (!StatsManager)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager: No StatsManager found on %s"), *Owner->GetName());
	}

	// Cache ability system component
	AbilitySystemComponent = Owner->FindComponentByClass<UAbilitySystemComponent>();
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogEquipmentManager, Error, TEXT("EquipmentManager: No AbilitySystemComponent found on %s"), *Owner->GetName());
	}

	// Cache character mesh
	ACharacter* Character = Cast<ACharacter>(Owner);
	if (Character)
	{
		CharacterMesh = Character->GetMesh();
	}
	
	if (!CharacterMesh)
	{
		UE_LOG(LogEquipmentManager, Error, TEXT("EquipmentManager: No CharacterMesh found on %s"), *Owner->GetName());
	}
}

// ═══════════════════════════════════════════════════════════════════════
// PUBLIC API
// ═══════════════════════════════════════════════════════════════════════

UItemInstance* UEquipmentManager::EquipItem(UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag)
{
	if (!Item)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager::EquipItem: Null item"));
		return nullptr;
	}

	// Server authority
	if (!GetOwner()->HasAuthority())
	{
		ServerEquipItem(Item, Slot, bSwapToBag);
		return nullptr;
	}

	return EquipItemInternal(Item, Slot, bSwapToBag);
}

UItemInstance* UEquipmentManager::UnequipItem(EEquipmentSlot Slot, bool bMoveToBag)
{
	// Server authority
	if (!GetOwner()->HasAuthority())
	{
		ServerUnequipItem(Slot, bMoveToBag);
		return nullptr;
	}

	// Get currently equipped item
	UItemInstance* CurrentItem = GetEquippedItem(Slot);
	if (!CurrentItem)
	{
		UE_LOG(LogEquipmentManager, Verbose, TEXT("EquipmentManager: Slot %d is already empty"), static_cast<int32>(Slot));
		return nullptr;
	}

	// Remove from slot
	RemoveEquipment(Slot);

	// Remove stats
	if (bApplyStatsOnEquip)
	{
		RemoveItemStats(CurrentItem);
	}

	// Add to inventory if requested
	if (bMoveToBag && InventoryManager)
	{
		if (!InventoryManager->AddItem(CurrentItem))
		{
			UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager: Failed to move unequipped item to inventory"));
		}
	}

	// Update weapon visual
	if (bAutoUpdateWeapons)
	{
		UpdateEquippedWeapon(Slot, nullptr);
	}

	// I-06 FIX: Only broadcast the server-local event here. Clients receive the
	// change via OnRep_EquippedItems diff broadcast — calling MulticastEquipmentChanged
	// as well caused every client to fire OnEquipmentChanged twice per unequip.
	OnEquipmentChanged.Broadcast(Slot, nullptr, CurrentItem);

	UE_LOG(LogEquipmentManager, Log, TEXT("EquipmentManager: Unequipped %s from slot %d"),
		*CurrentItem->GetName(), static_cast<int32>(Slot));

	return CurrentItem;
}

UItemInstance* UEquipmentManager::SwapEquipment(UItemInstance* Item, EEquipmentSlot Slot)
{
	// Just equip with swap enabled
	return EquipItem(Item, Slot, true);
}

UItemInstance* UEquipmentManager::GetEquippedItem(EEquipmentSlot Slot) const
{
	UItemInstance* const* FoundItem = EquippedItemsMap.Find(Slot);
	return FoundItem ? *FoundItem : nullptr;
}

bool UEquipmentManager::IsSlotOccupied(EEquipmentSlot Slot) const
{
	return EquippedItemsMap.Contains(Slot);
}

TArray<UItemInstance*> UEquipmentManager::GetAllEquippedItems() const
{
	TArray<UItemInstance*> Items;
	EquippedItemsMap.GenerateValueArray(Items);
	return Items;
}

void UEquipmentManager::UnequipAll(bool bMoveToBag)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager::UnequipAll: Must be called on server"));
		return;
	}

	// Copy keys to avoid modifying map during iteration
	TArray<EEquipmentSlot> Slots;
	EquippedItemsMap.GetKeys(Slots);

	for (EEquipmentSlot Slot : Slots)
	{
		UnequipItem(Slot, bMoveToBag);
	}
}

AEquippedItemRuntimeActor* UEquipmentManager::GetActiveRuntimeItemActor(EEquipmentSlot Slot) const
{
	if (AActor* const* FoundActor = ActiveRuntimeItemActors.Find(Slot))
	{
		return Cast<AEquippedItemRuntimeActor>(*FoundActor);
	}

	return nullptr;
}

bool UEquipmentManager::BeginPrimaryItemAction(EEquipmentSlot Slot)
{
	AEquippedItemRuntimeActor* RuntimeItemActor = GetActiveRuntimeItemActor(Slot);
	const bool bStartedLocally = RuntimeItemActor ? RuntimeItemActor->BeginPrimaryAction() : false;

	if (!GetOwner()->HasAuthority())
	{
		ServerBeginPrimaryItemAction(Slot);
	}

	return bStartedLocally;
}

bool UEquipmentManager::EndPrimaryItemAction(EEquipmentSlot Slot)
{
	AEquippedItemRuntimeActor* RuntimeItemActor = GetActiveRuntimeItemActor(Slot);
	if (RuntimeItemActor)
	{
		RuntimeItemActor->EndPrimaryAction();
	}

	if (!GetOwner()->HasAuthority())
	{
		ServerEndPrimaryItemAction(Slot);
	}

	return RuntimeItemActor != nullptr;
}

// ═══════════════════════════════════════════════════════════════════════
// SLOT DETERMINATION
// ═══════════════════════════════════════════════════════════════════════

EEquipmentSlot UEquipmentManager::DetermineEquipmentSlot(UItemInstance* Item) const
{
	if (!Item)
	{
		return EEquipmentSlot::ES_None;
	}

	// Get base data
	FItemBase* BaseData = Item->GetBaseData();
	if (!BaseData)
	{
		return EEquipmentSlot::ES_None;
	}

	// Map SubType to Equipment Slot
	switch (BaseData->ItemSubType)
	{
	case EItemSubType::IST_Helmet:
		return EEquipmentSlot::ES_Head;
		
	case EItemSubType::IST_Chest:
		return EEquipmentSlot::ES_Chest;
		
	case EItemSubType::IST_Gloves:
		return EEquipmentSlot::ES_Hands;
		
	case EItemSubType::IST_Boots:
		return EEquipmentSlot::ES_Feet;
		
	case EItemSubType::IST_Belt:
		return EEquipmentSlot::ES_Belt;
		
	case EItemSubType::IST_Amulet:
		return EEquipmentSlot::ES_Amulet;
		
	case EItemSubType::IST_Ring:
		return GetNextAvailableRingSlot();
		
	// Weapons
	case EItemSubType::IST_Sword:
	case EItemSubType::IST_Axe:
	case EItemSubType::IST_Mace:
	case EItemSubType::IST_Dagger:
		// Check if two-handed
		if (Item->bIsTwoHanded())
		{
			return EEquipmentSlot::ES_TwoHand;
		}
		return EEquipmentSlot::ES_MainHand;
		
	case EItemSubType::IST_Bow:
	case EItemSubType::IST_Staff:
		return EEquipmentSlot::ES_TwoHand;
		
	case EItemSubType::IST_Shield:
		return EEquipmentSlot::ES_OffHand;
		
	default:
		return EEquipmentSlot::ES_None;
	}
}

bool UEquipmentManager::CanEquipToSlot(UItemInstance* Item, EEquipmentSlot Slot) const
{
	if (!Item || Slot == EEquipmentSlot::ES_None)
	{
		return false;
	}

	// Get base data
	FItemBase* BaseData = Item->GetBaseData();
	if (!BaseData)
	{
		return false;
	}

	// Check if item is equipment
	if (!BaseData->IsEquippable())
	{
		return false;
	}

	// Check slot compatibility
	EEquipmentSlot DeterminedSlot = DetermineEquipmentSlot(Item);
	
	// If item is ring, can go in any ring slot
	if (IsRingSlot(DeterminedSlot) && IsRingSlot(Slot))
	{
		return true;
	}

	// Otherwise must match exactly
	return DeterminedSlot == Slot;
}

EEquipmentSlot UEquipmentManager::GetNextAvailableRingSlot() const
{
	// Check ring slots in order
	for (int32 i = 1; i <= MaxRingSlots; ++i)
	{
		EEquipmentSlot RingSlot = static_cast<EEquipmentSlot>(
			static_cast<int32>(EEquipmentSlot::ES_Ring1) + (i - 1)
		);
		
		if (!IsSlotOccupied(RingSlot))
		{
			return RingSlot;
		}
	}

	// All ring slots occupied
	return EEquipmentSlot::ES_None;
}

bool UEquipmentManager::IsRingSlot(EEquipmentSlot Slot) const
{
	return Slot >= EEquipmentSlot::ES_Ring1 && Slot <= EEquipmentSlot::ES_Ring10;
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL EQUIPPING
// ═══════════════════════════════════════════════════════════════════════

UItemInstance* UEquipmentManager::EquipItemInternal(UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag)
{
	if (!Item)
	{
		return nullptr;
	}

	// Get base data
	FItemBase* BaseData = Item->GetBaseData();
	if (!BaseData)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager: Item has no base data"));
		return nullptr;
	}

	// Verify it's equipment
	if (!BaseData->IsEquippable() )
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager: Item %s is not equipment"), *Item->GetName());
		return nullptr;
	}

	// Auto-determine slot if needed
	if (Slot == EEquipmentSlot::ES_None && bAutoSlotSelection)
	{
		Slot = DetermineEquipmentSlot(Item);
	}

	if (Slot == EEquipmentSlot::ES_None)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager: Could not determine slot for item %s"), *Item->GetName());
		return nullptr;
	}

	// Verify slot compatibility
	if (!CanEquipToSlot(Item, Slot))
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager: Item %s cannot be equipped to slot %d"), 
			*Item->GetName(), static_cast<int32>(Slot));
		return nullptr;
	}

	// Handle two-handed weapon (occupies both MainHand + OffHand)
	if (Item->bIsTwoHanded() && Slot == EEquipmentSlot::ES_TwoHand)
	{
		UItemInstance* OldMainHand = nullptr;
		UItemInstance* OldOffHand = nullptr;
		
		if (HandleTwoHandedWeapon(Item, bSwapToBag, OldMainHand, OldOffHand))
		{
			UE_LOG(LogEquipmentManager, Log, TEXT("EquipmentManager: Equipped two-handed weapon %s"), *Item->GetName());
			return OldMainHand ? OldMainHand : OldOffHand;
		}
		return nullptr;
	}

	// Get currently equipped item in this slot
	UItemInstance* OldItem = GetEquippedItem(Slot);

	// Equip new item
	AddEquipment(Slot, Item);

	// FIX: Remove the newly equipped item from the inventory so it only exists
	// in the equipment slot. Without this, the item remained in both inventory
	// and equipment simultaneously, causing duplicate stat applications and
	// allowing exploits via the double-item state.
	if (InventoryManager)
	{
		InventoryManager->RemoveItem(Item);
	}

	// Apply stats
	if (bApplyStatsOnEquip)
	{
		ApplyItemStats(Item);
	}

	// Handle old item
	if (OldItem)
	{
		// Remove old stats
		if (bApplyStatsOnEquip)
		{
			RemoveItemStats(OldItem);
		}

		// Move to inventory if requested
		if (bSwapToBag && InventoryManager)
		{
			if (!InventoryManager->AddItem(OldItem))
			{
				UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager: Failed to move old item to inventory"));
			}
		}
	}

	// Update weapon visual
	if (bAutoUpdateWeapons)
	{
		UpdateEquippedWeapon(Slot, Item);
	}

	// I-06 FIX: Server-local broadcast only. Client broadcast handled by OnRep_EquippedItems.
	OnEquipmentChanged.Broadcast(Slot, Item, OldItem);

	UE_LOG(LogEquipmentManager, Log, TEXT("EquipmentManager: Equipped %s to slot %d"),
		*Item->GetName(), static_cast<int32>(Slot));

	return OldItem;
}

bool UEquipmentManager::HandleTwoHandedWeapon(UItemInstance* Item, bool bSwapToBag, 
                                               UItemInstance*& OutOldMainHand, UItemInstance*& OutOldOffHand)
{
	// Get items in both hands
	OutOldMainHand = GetEquippedItem(EEquipmentSlot::ES_MainHand);
	OutOldOffHand = GetEquippedItem(EEquipmentSlot::ES_OffHand);

	// Remove both slots
	RemoveEquipment(EEquipmentSlot::ES_MainHand);
	RemoveEquipment(EEquipmentSlot::ES_OffHand);

	// Equip to TwoHand slot (occupies both visually)
	AddEquipment(EEquipmentSlot::ES_TwoHand, Item);

	// FIX: Remove the newly equipped two-handed weapon from the inventory
	// so it exists only in the equipment slot, not in both simultaneously.
	if (InventoryManager)
	{
		InventoryManager->RemoveItem(Item);
	}

	// Apply new stats
	if (bApplyStatsOnEquip)
	{
		ApplyItemStats(Item);
	}

	// Handle old items
	if (OutOldMainHand)
	{
		if (bApplyStatsOnEquip)
		{
			RemoveItemStats(OutOldMainHand);
		}
		
		if (bSwapToBag && InventoryManager)
		{
			InventoryManager->AddItem(OutOldMainHand);
		}
	}

	if (OutOldOffHand)
	{
		if (bApplyStatsOnEquip)
		{
			RemoveItemStats(OutOldOffHand);
		}
		
		if (bSwapToBag && InventoryManager)
		{
			InventoryManager->AddItem(OutOldOffHand);
		}
	}

	// Update weapon visual
	if (bAutoUpdateWeapons)
	{
		UpdateEquippedWeapon(EEquipmentSlot::ES_MainHand, nullptr);  // Clear main hand
		UpdateEquippedWeapon(EEquipmentSlot::ES_OffHand, nullptr);   // Clear off hand
		UpdateEquippedWeapon(EEquipmentSlot::ES_TwoHand, Item);      // Show two-handed weapon
	}

	// I-06 + I-12 FIX: Removed MulticastEquipmentChanged calls. Client broadcast is now
	// exclusively handled by OnRep_EquippedItems diff logic, which already covers the
	// TwoHand equip and the MainHand/OffHand clears. The old code only multicast the
	// TwoHand event, silently skipping the cleared-hand notifications on clients.
	OnEquipmentChanged.Broadcast(EEquipmentSlot::ES_TwoHand, Item, nullptr);

	if (OutOldMainHand || OutOldOffHand)
	{
		OnEquipmentChanged.Broadcast(EEquipmentSlot::ES_MainHand, nullptr, OutOldMainHand);
		OnEquipmentChanged.Broadcast(EEquipmentSlot::ES_OffHand, nullptr, OutOldOffHand);
	}

	return true;
}

// Continued in Part 2...
// Part 2 - Stats and Visual Updates

// ═══════════════════════════════════════════════════════════════════════
// STATS
// ═══════════════════════════════════════════════════════════════════════

void UEquipmentManager::ApplyItemStats(UItemInstance* Item)
{
	if (!Item || !StatsManager)
	{
		return;
	}

	// Delegate to StatsManager to apply equipment stats
	// StatsManager handles GAS attribute modifications
	StatsManager->ApplyEquipmentStats(Item);

	UE_LOG(LogEquipmentManager, Verbose, TEXT("EquipmentManager: Applied stats for %s"), *Item->GetName());
}

void UEquipmentManager::RemoveItemStats(UItemInstance* Item)
{
	if (!Item || !StatsManager)
	{
		return;
	}

	// Delegate to StatsManager to remove equipment stats
	StatsManager->RemoveEquipmentStats(Item);

	UE_LOG(LogEquipmentManager, Verbose, TEXT("EquipmentManager: Removed stats for %s"), *Item->GetName());
}

// ═══════════════════════════════════════════════════════════════════════
// WEAPON VISUAL + COMBAT UPDATES
// ═══════════════════════════════════════════════════════════════════════

void UEquipmentManager::UpdateEquippedWeapon(EEquipmentSlot Slot, UItemInstance* Item)
{
	if (!CharacterMesh)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager::UpdateEquippedWeapon: No CharacterMesh found"));
		return;
	}

	// Clean up the previous representation before building the new one.
	FName ComponentTag = FName(*FString::Printf(TEXT("EquippedWeapon_%s"), *UEnum::GetValueAsString(Slot)));
	CleanupWeapon(ComponentTag, Slot);

	// Early exit if unequipping
	if (!Item)
	{
		UE_LOG(LogEquipmentManager, Verbose, TEXT("EquipmentManager: Cleared weapon for slot %s"), 
			*UEnum::GetValueAsString(Slot));
		OnWeaponUpdated.Broadcast(Slot, Item);
		return;
	}

	// Get base data
	FItemBase* BaseData = Item->GetBaseData();
	if (!BaseData)
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager::UpdateEquippedWeapon: Item has no base data"));
		return;
	}

	const FName SocketContext = GetSocketContextForSlot(Slot);
	FName SocketName = SocketContext != NAME_None
		? BaseData->GetSocketForContext(SocketContext)
		: BaseData->AttachmentSocket;

	if (SocketName == NAME_None)
	{
		SocketName = BaseData->AttachmentSocket;
	}

	if (!CharacterMesh || SocketName == NAME_None || !CharacterMesh->DoesSocketExist(SocketName))
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager::UpdateEquippedWeapon: Invalid socket '%s'"), 
			*SocketName.ToString());
		return;
	}

	// Spawn appropriate representation
	if (BaseData->UsesRuntimeActor() && BaseData->GetRuntimeActorClass())
	{
		// Spawn runtime actor (visual + combat/special logic)
		SpawnWeaponActor(Slot, Item, BaseData, SocketName, ComponentTag);
	}
	else
	{
		if (BaseData->UsesRuntimeActor())
		{
			UE_LOG(LogEquipmentManager, Warning,
				TEXT("EquipmentManager: Item %s requested a runtime actor but no RuntimeActorClass is configured. Falling back to mesh."),
				*Item->GetName());
		}

		// Spawn mesh-only representation
		SpawnWeaponMesh(Slot, Item, BaseData, SocketName, ComponentTag);
	}

	// Broadcast update event
	OnWeaponUpdated.Broadcast(Slot, Item);
}

FName UEquipmentManager::GetSocketContextForSlot(EEquipmentSlot Slot) const
{
	switch (Slot)
	{
	case EEquipmentSlot::ES_MainHand:
		return FName("MainHand");
	case EEquipmentSlot::ES_OffHand:
		return FName("OffHand");
	case EEquipmentSlot::ES_TwoHand:
		return FName("TwoHand");
	default:
		return NAME_None;
	}
}

void UEquipmentManager::SpawnWeaponActor(EEquipmentSlot Slot, UItemInstance* Item,
                                         FItemBase* BaseData, FName SocketName, FName ComponentTag)
{
	if (!GetOwner())
	{
		return;
	}

	const TSubclassOf<AActor> RuntimeActorClass = BaseData->GetRuntimeActorClass();
	if (!RuntimeActorClass)
	{
		return;
	}

	// OPT-SAFE: Cache the owner pawn once for Instigator + InitializeFromItem.
	APawn* OwnerPawn = Cast<APawn>(GetOwner());

	// Spawn parameters
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = OwnerPawn;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn weapon actor
	AActor* WeaponActor = GetWorld()->SpawnActor<AActor>(
		RuntimeActorClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (!WeaponActor)
	{
		UE_LOG(LogEquipmentManager, Error, TEXT("EquipmentManager: Failed to spawn weapon actor"));
		return;
	}

	// Tag and store
	WeaponActor->Tags.Add(ComponentTag);
	ActiveRuntimeItemActors.Add(Slot, WeaponActor);

	// Attach to socket
	FAttachmentTransformRules AttachRules = ConvertAttachmentRules(BaseData->AttachmentRules);
	if (CharacterMesh)
	{
		WeaponActor->AttachToComponent(CharacterMesh, AttachRules, SocketName);
	}
	else
	{
		UE_LOG(LogEquipmentManager, Warning,
			TEXT("EquipmentManager: CharacterMesh is null — weapon actor '%s' spawned but not attached."),
			*WeaponActor->GetName());
	}

	if (AEquippedItemRuntimeActor* RuntimeItemActor = Cast<AEquippedItemRuntimeActor>(WeaponActor))
	{
		RuntimeItemActor->InitializeFromItem(Item, OwnerPawn, Slot);
		RuntimeItemActor->OnEquipped();
	}
	else
	{
		UE_LOG(LogEquipmentManager, Warning,
			TEXT("EquipmentManager: Runtime actor class %s does not derive from AEquippedItemRuntimeActor; initialization hooks were skipped."),
			*GetNameSafe(RuntimeActorClass));
	}

	UE_LOG(LogEquipmentManager, Log, TEXT("EquipmentManager: Spawned weapon actor '%s' to socket '%s'"), 
		*WeaponActor->GetName(), *SocketName.ToString());
}

void UEquipmentManager::SpawnWeaponMesh(EEquipmentSlot Slot, UItemInstance* Item,
                                        FItemBase* BaseData, FName SocketName, FName ComponentTag)
{
	USceneComponent* NewWeaponComponent = nullptr;

	if (BaseData->SkeletalMesh)
	{
		// Skeletal mesh (animated weapons)
		USkeletalMeshComponent* SkeletalComp = NewObject<USkeletalMeshComponent>(
			GetOwner(), USkeletalMeshComponent::StaticClass(), ComponentTag);
		
		if (SkeletalComp)
		{
			SkeletalComp->SetSkeletalMesh(BaseData->SkeletalMesh);
			SkeletalComp->ComponentTags.Add(ComponentTag);
			SkeletalComp->RegisterComponent();
			
			FAttachmentTransformRules AttachRules = ConvertAttachmentRules(BaseData->AttachmentRules);
			SkeletalComp->AttachToComponent(CharacterMesh, AttachRules, SocketName);
			
			NewWeaponComponent = SkeletalComp;
			
			UE_LOG(LogEquipmentManager, Verbose, TEXT("EquipmentManager: Attached SkeletalMesh '%s' to socket '%s'"), 
				*BaseData->SkeletalMesh->GetName(), *SocketName.ToString());
		}
	}
	else if (BaseData->StaticMesh)
	{
		// Static mesh (non-animated weapons)
		UStaticMeshComponent* StaticComp = NewObject<UStaticMeshComponent>(
			GetOwner(), UStaticMeshComponent::StaticClass(), ComponentTag);
		
		if (StaticComp)
		{
			StaticComp->SetStaticMesh(BaseData->StaticMesh);
			StaticComp->ComponentTags.Add(ComponentTag);
			StaticComp->RegisterComponent();
			
			FAttachmentTransformRules AttachRules = ConvertAttachmentRules(BaseData->AttachmentRules);
			StaticComp->AttachToComponent(CharacterMesh, AttachRules, SocketName);
			
			NewWeaponComponent = StaticComp;
			
			UE_LOG(LogEquipmentManager, Verbose, TEXT("EquipmentManager: Attached StaticMesh '%s' to socket '%s'"), 
				*BaseData->StaticMesh->GetName(), *SocketName.ToString());
		}
	}
	else
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager::SpawnWeaponMesh: Item has no mesh"));
	}

	// B-2 FIX: Store the component for O(1) cleanup and external access.
	// Previously NewWeaponComponent was set but never saved anywhere, making
	// it impossible to retrieve without an expensive tag-based component scan.
	if (NewWeaponComponent)
	{
		ActiveMeshComponents.Add(Slot, NewWeaponComponent);
	}

	// TODO: Apply material customization based on item rarity, affixes, durability, etc.
}

void UEquipmentManager::CleanupWeapon(FName ComponentTag, EEquipmentSlot Slot)
{
	// Clean up runtime actor
	if (AActor** FoundActor = ActiveRuntimeItemActors.Find(Slot))
	{
		if (*FoundActor && IsValid(*FoundActor))
		{
			if (AEquippedItemRuntimeActor* RuntimeItemActor = Cast<AEquippedItemRuntimeActor>(*FoundActor))
			{
				RuntimeItemActor->OnUnequipped();
			}

			(*FoundActor)->Destroy();
		}
		ActiveRuntimeItemActors.Remove(Slot);
	}

	// B-2 FIX: Also remove from the direct-reference map so the slot is clean
	// and the next SpawnWeaponMesh can safely Add() to this slot.
	if (USceneComponent** FoundMesh = ActiveMeshComponents.Find(Slot))
	{
		if (*FoundMesh && IsValid(*FoundMesh))
		{
			(*FoundMesh)->DestroyComponent();
		}
		ActiveMeshComponents.Remove(Slot);
	}

	// Tag-based sweep as a safety net (catches any components missed by the map,
	// e.g. if the component was spawned before ActiveMeshComponents was introduced).
	TArray<UActorComponent*> Components = GetOwner()->GetComponentsByTag(
		USceneComponent::StaticClass(), ComponentTag);

	for (UActorComponent* Component : Components)
	{
		if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
		{
			SceneComp->DestroyComponent();
		}
	}
}

FAttachmentTransformRules UEquipmentManager::ConvertAttachmentRules(const FItemAttachmentRules& ItemRules) const
{
	auto ConvertRule = [](EPHAttachmentRule Rule) -> EAttachmentRule
	{
		switch (Rule)
		{
		case EPHAttachmentRule::AR_KeepRelative:
			return EAttachmentRule::KeepRelative;
		case EPHAttachmentRule::AR_KeepWorld:
			return EAttachmentRule::KeepWorld;
		case EPHAttachmentRule::AR_SnapToTarget:
			return EAttachmentRule::SnapToTarget;
		default:
			return EAttachmentRule::KeepRelative;
		}
	};

	return FAttachmentTransformRules(
		ConvertRule(ItemRules.LocationRule),
		ConvertRule(ItemRules.RotationRule),
		ConvertRule(ItemRules.ScaleRule),
		ItemRules.bWeldSimulatedBodies
	);
}

// ═══════════════════════════════════════════════════════════════════════
// REPLICATION
// ═══════════════════════════════════════════════════════════════════════

void UEquipmentManager::OnRep_EquippedItems()
{
	// B-3 FIX: Capture the current (pre-replication) state so we can broadcast
	// correct OldItem values. Previously this always broadcast nullptr for OldItem
	// because the map was rebuilt before the diff was performed.
	TMap<EEquipmentSlot, UItemInstance*> OldEquipmentMap = EquippedItemsMap;

	// Rebuild TMap from replicated TArray (this reflects the new server state).
	RebuildEquipmentMap();

	// Build the set of slots present in the new array for O(1) lookup.
	TSet<EEquipmentSlot> NewSlots;
	for (const FEquipmentSlotEntry& Entry : EquippedItemsArray)
	{
		NewSlots.Add(Entry.Slot);
	}

	// Broadcast changes for newly equipped / swapped items — with correct OldItem.
	// OPT: Only call UpdateEquippedWeapon for slots that actually changed, instead of
	// iterating all 20 managed slots unconditionally on every replication event.
	for (const FEquipmentSlotEntry& Entry : EquippedItemsArray)
	{
		UItemInstance* OldItem = OldEquipmentMap.FindRef(Entry.Slot);
		if (OldItem != Entry.Item)
		{
			if (bAutoUpdateWeapons)
			{
				UpdateEquippedWeapon(Entry.Slot, Entry.Item);
			}
			OnEquipmentChanged.Broadcast(Entry.Slot, Entry.Item, OldItem);
		}
	}

	// Handle slots that were cleared (item removed, not replaced).
	for (const TPair<EEquipmentSlot, UItemInstance*>& OldEntry : OldEquipmentMap)
	{
		if (!NewSlots.Contains(OldEntry.Key))
		{
			if (bAutoUpdateWeapons)
			{
				UpdateEquippedWeapon(OldEntry.Key, nullptr);
			}
			OnEquipmentChanged.Broadcast(OldEntry.Key, nullptr, OldEntry.Value);
		}
	}

	UE_LOG(LogEquipmentManager, Verbose, TEXT("EquipmentManager: Replicated equipment changes"));
}

// ═══════════════════════════════════════════════════════════════════════
// NETWORK RPCS
// ═══════════════════════════════════════════════════════════════════════

void UEquipmentManager::ServerEquipItem_Implementation(UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag)
{
	if (!Item)
	{
		return;
	}

	// B-4 FIX: Verify ownership before acting on the client's equip request.
	// Without this check a malicious client could equip an item from another player's
	// inventory by sending a crafted RPC with a foreign UItemInstance pointer.
	// N-16 NOTE: ContainsItem() previously always returned false on the server because
	// InventoryManager.Items was not replicated (server array was always empty).
	// N-04 FIX (InventoryManager replication) resolved this — Items now replicates
	// COND_OwnerOnly so the server has the authoritative copy and this check works.
	if (InventoryManager && !InventoryManager->ContainsItem(Item))
	{
		UE_LOG(LogEquipmentManager, Warning,
			TEXT("ServerEquipItem: Rejected equip request for item '%s' — item is not in %s's inventory. "
				 "Possible cheat attempt."),
			*GetNameSafe(Item), *GetNameSafe(GetOwner()));
		return;
	}

	// Validate the item can actually be equipped (slot compatibility check).
	if (!CanEquipToSlot(Item, Slot == EEquipmentSlot::ES_None ? DetermineEquipmentSlot(Item) : Slot))
	{
		UE_LOG(LogEquipmentManager, Warning,
			TEXT("ServerEquipItem: Rejected equip request — item '%s' cannot be equipped to slot %d."),
			*GetNameSafe(Item), static_cast<int32>(Slot));
		return;
	}

	EquipItem(Item, Slot, bSwapToBag);
}

void UEquipmentManager::ServerUnequipItem_Implementation(EEquipmentSlot Slot, bool bMoveToBag)
{
	UnequipItem(Slot, bMoveToBag);
}

void UEquipmentManager::ServerBeginPrimaryItemAction_Implementation(EEquipmentSlot Slot)
{
	if (AEquippedItemRuntimeActor* RuntimeItemActor = GetActiveRuntimeItemActor(Slot))
	{
		RuntimeItemActor->BeginPrimaryAction();
	}
}

void UEquipmentManager::ServerEndPrimaryItemAction_Implementation(EEquipmentSlot Slot)
{
	if (AEquippedItemRuntimeActor* RuntimeItemActor = GetActiveRuntimeItemActor(Slot))
	{
		RuntimeItemActor->EndPrimaryAction();
	}
}

void UEquipmentManager::MulticastEquipmentChanged_Implementation(EEquipmentSlot Slot, UItemInstance* NewItem, UItemInstance* OldItem)
{
	// I-06 FIX: No-op. Client-side equipment change notifications are now delivered
	// exclusively via the OnRep_EquippedItems diff broadcast, which fires correctly
	// whenever the replicated EquippedItemsArray changes. Keeping this stub so
	// existing Blueprint callers compile without modification.
}

// ═══════════════════════════════════════════════════════════════════════
// HELPER FUNCTIONS (TArray/TMap Management)
// ═══════════════════════════════════════════════════════════════════════

void UEquipmentManager::RebuildEquipmentMap()
{
	// Clear map
	EquippedItemsMap.Empty();

	// Rebuild from replicated array
	for (const FEquipmentSlotEntry& Entry : EquippedItemsArray)
	{
		if (Entry.Item)
		{
			EquippedItemsMap.Add(Entry.Slot, Entry.Item);
		}
	}
}

void UEquipmentManager::AddEquipment(EEquipmentSlot Slot, UItemInstance* Item)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager::AddEquipment: Must be called on server"));
		return;
	}

	// Remove old entry if exists (prevents duplicates)
	RemoveEquipment(Slot);

	// Add to array (for replication)
	EquippedItemsArray.Add(FEquipmentSlotEntry(Slot, Item));

	// Add to map (for fast lookup)
	EquippedItemsMap.Add(Slot, Item);
}

void UEquipmentManager::RemoveEquipment(EEquipmentSlot Slot)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager::RemoveEquipment: Must be called on server"));
		return;
	}

	// OPT: Use index-based RemoveAtSwap (O(1)) instead of RemoveAll with a lambda
	// (O(n)). Each slot appears at most once in the array, so a single pass find + swap
	// is sufficient. Order is not meaningful for the replicated flat array.
	for (int32 i = EquippedItemsArray.Num() - 1; i >= 0; --i)
	{
		if (EquippedItemsArray[i].Slot == Slot)
		{
			EquippedItemsArray.RemoveAtSwap(i, 1, EAllowShrinking::No);
			break;
		}
	}

	// Remove from map
	EquippedItemsMap.Remove(Slot);
}

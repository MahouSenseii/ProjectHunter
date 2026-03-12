// Character/Component/EquipmentManager.cpp

#include "Character/Component/EquipmentManager.h"
#include "Character/Component/InventoryManager.h"
#include "Character/Component/StatsManager.h"
#include "Item/ItemInstance.h"
#include "Item/Library/ItemFunctionLibrary.h"
#include "Item/Library/ItemStructs.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogEquipmentManager);
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

	// Broadcast change
	OnEquipmentChanged.Broadcast(Slot, nullptr, CurrentItem);
	MulticastEquipmentChanged(Slot, nullptr, CurrentItem);

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

	// Broadcast change
	OnEquipmentChanged.Broadcast(Slot, Item, OldItem);
	MulticastEquipmentChanged(Slot, Item, OldItem);

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

	// Broadcast changes
	OnEquipmentChanged.Broadcast(EEquipmentSlot::ES_TwoHand, Item, nullptr);
	MulticastEquipmentChanged(EEquipmentSlot::ES_TwoHand, Item, nullptr);

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

	// Determine socket context
	FName SocketContext = GetSocketContextForSlot(Slot);
	if (SocketContext == NAME_None)
	{
		return; // Not a weapon slot
	}

	// Clean up old weapon
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

	// Get socket
	FName SocketName = BaseData->GetSocketForContext(SocketContext);
	if (SocketName == NAME_None)
	{
		SocketName = BaseData->AttachmentSocket;
	}

	if (SocketName == NAME_None || !CharacterMesh->DoesSocketExist(SocketName))
	{
		UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager::UpdateEquippedWeapon: Invalid socket '%s'"), 
			*SocketName.ToString());
		return;
	}

	// Spawn appropriate representation
	if (BaseData->bUseWeaponActor && BaseData->WeaponActorClass)
	{
		// Spawn weapon actor (visual + combat)
		SpawnWeaponActor(Slot, Item, BaseData, SocketName, ComponentTag);
	}
	else
	{
		// Spawn weapon mesh (visual only)
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
	if (!GetOwner() || !BaseData->WeaponActorClass)
	{
		return;
	}

	// Spawn parameters
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn weapon actor
	AActor* WeaponActor = GetWorld()->SpawnActor<AActor>(
		BaseData->WeaponActorClass,
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
	ActiveWeaponActors.Add(Slot, WeaponActor);

	// Attach to socket
	FAttachmentTransformRules AttachRules = ConvertAttachmentRules(BaseData->AttachmentRules);
	WeaponActor->AttachToComponent(CharacterMesh, AttachRules, SocketName);

	// Initialize weapon with item data (for damage, stats, VFX, etc.)
	// TODO: Implement ICombatWeapon interface
	// if (WeaponActor->Implements<UICombatWeapon>())
	// {
	//     IICombatWeapon::Execute_InitializeWeapon(WeaponActor, Item);
	// }

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

	// TODO: Apply material customization based on item rarity, affixes, durability, etc.
}

void UEquipmentManager::CleanupWeapon(FName ComponentTag, EEquipmentSlot Slot)
{
	// Clean up weapon actor
	if (AActor** FoundActor = ActiveWeaponActors.Find(Slot))
	{
		if (*FoundActor && IsValid(*FoundActor))
		{
			(*FoundActor)->Destroy();
		}
		ActiveWeaponActors.Remove(Slot);
	}

	// Clean up mesh components
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
	// Rebuild TMap from replicated TArray
	RebuildEquipmentMap();

	// Update visuals on clients
	if (bAutoUpdateWeapons)
	{
		for (const FEquipmentSlotEntry& Entry : EquippedItemsArray)
		{
			if (Entry.Item)
			{
				UpdateEquippedWeapon(Entry.Slot, Entry.Item);
			}
		}
	}

	// Broadcast events
	for (const FEquipmentSlotEntry& Entry : EquippedItemsArray)
	{
		OnEquipmentChanged.Broadcast(Entry.Slot, Entry.Item, nullptr);
	}

	UE_LOG(LogEquipmentManager, Verbose, TEXT("EquipmentManager: Replicated equipment changes"));
}

// ═══════════════════════════════════════════════════════════════════════
// NETWORK RPCS
// ═══════════════════════════════════════════════════════════════════════

void UEquipmentManager::ServerEquipItem_Implementation(UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag)
{
	// Validation
	if (!Item)
	{
		return;
	}

	// TODO: Add anti-cheat validation
	// - Verify item is in client's inventory
	// - Verify item can be equipped
	// - Check for rapid equip/unequip exploits

	EquipItem(Item, Slot, bSwapToBag);
}

void UEquipmentManager::ServerUnequipItem_Implementation(EEquipmentSlot Slot, bool bMoveToBag)
{
	UnequipItem(Slot, bMoveToBag);
}

void UEquipmentManager::MulticastEquipmentChanged_Implementation(EEquipmentSlot Slot, UItemInstance* NewItem, UItemInstance* OldItem)
{
	// Broadcast to all clients for UI/visual updates
	OnEquipmentChanged.Broadcast(Slot, NewItem, OldItem);
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

	// Remove from array
	EquippedItemsArray.RemoveAll([Slot](const FEquipmentSlotEntry& Entry)
	{
		return Entry.Slot == Slot;
	});

	// Remove from map
	EquippedItemsMap.Remove(Slot);
}
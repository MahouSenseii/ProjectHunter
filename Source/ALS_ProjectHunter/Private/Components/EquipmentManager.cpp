// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/EquipmentManager.h"

#include "Character/ALSCharacter.h"
#include "Components/InventoryManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interactables/Pickups/ItemPickup.h"
#include "Item/EquippedObject.h"
#include "Kismet/KismetMathLibrary.h"


UEquipmentManager::UEquipmentManager()
{

}

void UEquipmentManager::BeginPlay()
{
	Super::BeginPlay();
	if(!OwnerCharacter)
	{
		OwnerCharacter = Cast<AALSCharacter>(GetOwner());
	}
}


bool UEquipmentManager::CheckSlot(UBaseItem* Item)
{
	if (IsValid(Item))
	{
		// Check if the EquipmentSlot key exists in the EquipmentData map.
		// The Find method returns a pointer to the value if found, otherwise nullptr.
		return (EquipmentData.Find(Cast<UEquippableItem>(Item)->GetEquippableData().EquipSlot) != nullptr);
	}
	return false;
}


TArray<UBaseItem*> UEquipmentManager::EquipmentCheck() const
{
	TArray<UBaseItem*> Items;
	EquipmentData.GenerateValueArray(Items);
	return Items;
}

bool UEquipmentManager::IsItemEquippable(UBaseItem* Item)
{
	static const TSet<EItemType> EquippableTypes = { EItemType::IS_Armor, EItemType::IS_Weapon, EItemType::IS_Shield };
	return EquippableTypes.Contains(Item->GetItemInfo().ItemType);
}

bool UEquipmentManager::AddItemInSlotToInventory(UBaseItem* Item)
{
	if(IsValid(Item))
	{
		Cast<UEquippableItem>(Item);
		const EEquipmentSlot Slot = Cast<UEquippableItem>(Item)->GetEquippableData().EquipSlot;
		UBaseItem** PRetrievedItem = EquipmentData.Find(Slot);
		UBaseItem* RetrievedItem = PRetrievedItem ? *PRetrievedItem : nullptr;
		return GetInventoryManager()->TryToAddItemToInventory(RetrievedItem, true);
	}
	return false;
}

void UEquipmentManager::TryToEquip(UBaseItem* Item, bool HasMesh, EEquipmentSlot Slot)
{

	if (!IsValid(Item)) { return; }

	if (HasMesh)
	{          
		HandleHasMesh(Item, Slot);
	}
	else
	{
		HandleNoMesh(Item, Slot);
	}
	OnEquipmentChanged.Broadcast();
}

void UEquipmentManager::HandleHasMesh(UBaseItem* Item, EEquipmentSlot Slot)
{
	// Check if the slot is valid for the given item.
	if (CheckSlot(Item))
	{
		// Try to find the currently equipped item in the given slot.
		UBaseItem** EquippedItem = EquipmentData.Find(Slot);

		if (IsValid(*EquippedItem))
		{
			// If there's an item equipped in the slot, try to add it back to the inventory.
			if (!GetInventoryManager()->TryToAddItemToInventory(*EquippedItem, true))
			{
				// If adding back to inventory fails, drop the item.
				GetInventoryManager()->DropItemInInventory(Item);
			}

			// Remove the currently equipped item from the slot.
			RemoveEquippedItem(*EquippedItem, Slot);
		}

		// Attach the new item to the character and update the mesh.
		if(const UEquippableItem* EquipItem = Cast<UEquippableItem>(Item))
		{
			AttachItem(EquipItem->GetEquippableData().EquipClass, Item, Slot);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Equip Item came back null (Check EquipmentManager 107)"));
		}
	}
}

void UEquipmentManager::HandleNoMesh(UBaseItem* Item, EEquipmentSlot Slot)
{
	// Check if the slot is valid for the given item.
	if (CheckSlot(Item))
	{
		// Try to find the currently equipped item in the given slot.

		if (UBaseItem** EquippedItem = EquipmentData.Find(Slot))
		{
			// If there's an item equipped in the slot, try to add it back to the inventory.
			if (!GetInventoryManager()-> TryToAddItemToInventory(*EquippedItem, true))
			{
				// If adding back to inventory fails, drop the item.
				GetInventoryManager()->DropItemInInventory(Item);
			}
			EquipmentData.Remove(Slot);
		}
		EquipmentData.Add(Slot,Item);
	}
}



void UEquipmentManager::RemoveEquippedItem(UBaseItem* Item, EEquipmentSlot Slot)
{
	if (IsValid(Item))
	{
		RemoveItemInSlot(Slot);  // This should already remove the item from EquipmentData

		// It seems like you are setting a default overlay state upon item removal.
		// Ensure this is what you intend to do in all scenarios.
		OwnerCharacter->SetOverlayState(EALSOverlayState::Default);

		// Broadcasting the event that the equipment has changed.
		OnEquipmentChanged.Broadcast();
	}
}


void UEquipmentManager::RemoveItemInSlot(EEquipmentSlot Slot)
{
	// Use a single call to Find to get the pointer.
	AEquippedObject** pRetrievedItem = EquipmentItem.Find(Slot);

	// Use early return to reduce nesting.
	if (!pRetrievedItem)
	{
		return;
	}

	AEquippedObject* RetrievedItem = *pRetrievedItem;

	// Check for validity of the RetrievedItem.
	if (!IsValid(RetrievedItem))
	{
		return;
	}

	// Destroy the object, update the mesh, and remove the item from the mapping.
	RetrievedItem->Destroy();
	EquipmentData[Slot] = nullptr;
	OnEquipmentChanged.Broadcast();
}

bool UEquipmentManager::IsSocketEmpty(EEquipmentSlot Slot)
{
	return (EquipmentData.Find(Slot) != nullptr);
}

void UEquipmentManager::AttachItem(TSubclassOf<AEquippedObject> Class, UBaseItem* Item, EEquipmentSlot Slot)
{
	if (UWorld* World = GetWorld())
	{
		// Spawn parameters
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// Spawn location and rotation (you can customize these)
		const FVector Location = FVector(0.f, 0.f, 0.f);
		const FRotator Rotation = FRotator(0.f, 0.f, 0.f);

		// Spawn the actor
		
		AEquippedObject* SpawnedActor = World->SpawnActor<AEquippedObject>(Class, Location, Rotation, SpawnParams);

		
		
		if (IsValid(SpawnedActor))
		{
			// add item info to spawned actor 
			SpawnedActor->SetItemInfo(Item->GetItemInfo());

			// Check if the owner character is valid
			if (IsValid(OwnerCharacter))
			{
				// Find the slot to attach to based on the provided enum
				const FName SlotToAttachTo = FindSlotName(Slot);
				if (!OwnerCharacter->GetMesh()->DoesSocketExist(SlotToAttachTo)) {
					// Handle the error, the socket does not exist
					UE_LOG(LogTemp, Warning, TEXT("Socket doesnt exist."));

				}
				
				// Set the mesh
				if (UStaticMesh* Mesh = Item->GetItemInfo().StaticMesh)
				{
					SpawnedActor->SetMesh(Mesh);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Mesh for item is null!"));
					return;
				}
				SpawnedActor->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SlotToAttachTo);
				SpawnedActor->SetOwningCharacter(OwnerCharacter);

				// Set additional properties, execute functions, etc.
				EquipmentData.Add(Slot, Item);
				EquipmentItem.Add(Slot, SpawnedActor);

				// Notify that equipment has changed
				OnEquipmentChanged.Broadcast();
			}
			else
			{
				// Handle the case where OwnerCharacter is not valid (e.g., log an error)
			}
		}
		else
		{
			// Handle the case where SpawnedActor is not valid (e.g., log an error)
		}
	}
}

FName UEquipmentManager::FindSlotName(const EEquipmentSlot Slot)
{
	FName SlotToAttachTo;
	switch (Slot)
	{
	case EEquipmentSlot::ES_None:
		SlotToAttachTo = "None";
		break;
	case EEquipmentSlot::ES_Head:
		SlotToAttachTo = "HeadSocket";
		break;
	case EEquipmentSlot::ES_Gloves:
		SlotToAttachTo = "GlovesSocket";
		break;
	case EEquipmentSlot::ES_Neck:
		SlotToAttachTo = "NeckSocket";
		break;
	case EEquipmentSlot::ES_Chestplate:
		SlotToAttachTo = "ChestSocket";
		break;
	case EEquipmentSlot::ES_Legs:
		SlotToAttachTo = "LegsSocket";
		break;
	case EEquipmentSlot::ES_Boots:
		SlotToAttachTo = "BootsSocket";
		break;
	case EEquipmentSlot::ES_MainHand:
		SlotToAttachTo = "MainHandSocket";
		break;
	case EEquipmentSlot::ES_OffHand:
		SlotToAttachTo = "OffHandSocket";
		break;
	case EEquipmentSlot::ES_Ring:
		SlotToAttachTo = "RingSocket";
		break;
	case EEquipmentSlot::ES_Flask:
		SlotToAttachTo = "FlaskSocket";
		break;
	case EEquipmentSlot::ES_Belt:
		SlotToAttachTo = "BeltSocket";
		break;
	default:
		SlotToAttachTo = "None"; // Or handle it differently
		break;
	}
	return SlotToAttachTo;
}

bool UEquipmentManager::DropItem(UBaseItem* Item)
{
	// Ensure the Item is valid.
	if (!IsValid(Item))
	{
		UE_LOG(LogTemp, Warning, TEXT("Item is invalid"));
		return false;
	}

	// Get the world context.
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("World context is null"));
		return false;
	}

	// Ensure Class is valid.
	const TSubclassOf<AItemPickup> Class = Item->GetItemInfo().PickupClass; 
	if (!Class)
	{
		UE_LOG(LogTemp, Warning, TEXT("Class is null"));
		return false;
	}

	const FRotator Rotation(0.0f, 0.0f, 0.0f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn the actor
	AItemPickup* DroppedItem = World->SpawnActor<AItemPickup>(Class, GetGroundSpawnLocation(), Rotation, SpawnParams);
	if (!DroppedItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn DroppedItem"));
		return false;
	}

	// Set properties of the spawned item.
	DroppedItem->ItemInfo = Item->GetItemInfo();
	DroppedItem->SetNewMesh(Item->GetItemInfo().StaticMesh);
	DroppedItem->SetSkeletalMesh(Item->GetItemInfo().SkeletalMesh);

	return true;
}

UInventoryManager* UEquipmentManager::GetInventoryManager() const
{
	// Assuming Owner is an AActor* and UInventoryManager is a UActorComponent subclass
	if (OwnerCharacter)
	{
		return Cast<UInventoryManager>(OwnerCharacter->GetComponentByClass(UInventoryManager::StaticClass()));
	}
	return nullptr; // Return nullptr if Owner is null or the component is not found
}

FVector UEquipmentManager::GetGroundSpawnLocation() const
{
	// Ensure OwnerCharacter is valid to avoid potential crashes.
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("OwnerCharacter is null"));
		return FVector::ZeroVector;
	}

	// Simplify vector arithmetic and avoid unnecessary transformations.
	FVector RdmForwardVector = OwnerCharacter->GetActorForwardVector() + UKismetMathLibrary::RandomUnitVector();
	RdmForwardVector *= 200;  // Directly scale the vector.

	// Directly use the transformed location rather than decomposing it into components.
	FVector CapsuleLocation = OwnerCharacter->GetActorLocation() + RdmForwardVector;
	CapsuleLocation.Z = 0;  // Reset Z to ground level if necessary.

	FFindFloorResult FloorResult;
	OwnerCharacter->GetCharacterMovement()->FindFloor(CapsuleLocation, FloorResult, true);

	if (FloorResult.bBlockingHit)  // Check if the floor was found.
	{
		FVector FoundImpactPoint = FloorResult.HitResult.ImpactPoint;
		FoundImpactPoint.Z += 120;  // Offset Z to position above the floor.
		return FoundImpactPoint;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No floor found at location"));
		return CapsuleLocation;  // Return the original calculated location or a default.
	}
}



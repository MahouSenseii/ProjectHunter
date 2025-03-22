// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/Pickups/ItemPickup.h"

#include "Character/Player/PHPlayerController.h"
#include "Components/InteractableManager.h"
#include "Components/InventoryManager.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Library/InteractionEnumLibrary.h"
#include "Library/PHItemFunctionLibrary.h"
#include "Particles/ParticleSystemComponent.h"


AItemPickup::AItemPickup()
{
	// Create RotatingMovementComponent and attach it to RootComponent
	RotatingMovementComponent = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovementComponent"));

	// Create ParticleComponent and attach it to RootComponent
	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystemComponent"));
	ParticleSystemComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	// Set rotation rate (can also be changed later during gameplay)
	RotatingMovementComponent->RotationRate = FRotator(0.f, 180.f, 0.f); // Rotate 180 degrees per second around the yaw axis.

	StaticMesh->SetStaticMesh(ItemInfo.StaticMesh);
}

void AItemPickup::BeginPlay()
{
	Super::BeginPlay();

	StaticMesh->SetStaticMesh(ItemInfo.StaticMesh);
	StaticMesh->CanCharacterStepUpOn = ECB_No;
	StaticMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	if(InteractableManager)
	{
		InteractableManager->IsInteractableChangeable = true;
		InteractableManager->DestroyAfterInteract = true;
		InteractableManager->InputType = EInteractType::Single;
		InteractableManager->InteractableResponse = EInteractableResponseType::OnlyOnce;
	}
}

void AItemPickup::BPIInteraction_Implementation(AActor* Interactor, bool WasHeld)
{
	FItemInformation PassedItemInfo;
	FEquippableItemData EquippableItemData;
	FWeaponItemData WeaponItemData;
	FConsumableItemData ConsumableItemData;
	HandleInteraction(Interactor, WasHeld, PassedItemInfo,EquippableItemData, WeaponItemData,ConsumableItemData);
}

UBaseItem* AItemPickup::GenerateItem() const
{
	// Ensure we are operating within the correct world context
	if (GetWorld())
	{
		if (UBaseItem* NewItem = NewObject<UBaseItem>(GetTransientPackage(), UBaseItem::StaticClass()))
		{
			NewItem->SetItemInfo(ItemInfo);			
			// Initialize your item's properties here, if necessary
			return NewItem;
		}
	}
	return nullptr;
}

bool AItemPickup::HandleInteraction(AActor* Actor, bool WasHeld, FItemInformation PassedItemInfo, FEquippableItemData EquippableItemData, FWeaponItemData WeaponItemData, FConsumableItemData ConsumableItemData) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	APHPlayerController* PlayerController = Cast<APHPlayerController>(Actor);
	
	APHBaseCharacter* OwnerCharacter = Cast<APHBaseCharacter>(PlayerController->AcknowledgedPawn);
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("Owner is not a valid APHBaseCharacter."));
		return false;
	}

	// Get the item information
	UBaseItem* ReturnItem = UPHItemFunctionLibrary::GetItemInformation( PassedItemInfo, EquippableItemData, WeaponItemData, ConsumableItemData);
	if (!ReturnItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to create item from ItemInfo."));
		return false;
	}

	// Set the Owner ID for the item
	FItemInformation ReturnItemInfo = ReturnItem->GetItemInfo();
	ReturnItemInfo.OwnerID = OwnerCharacter->GetInventoryManager()->GetID();
	ReturnItem->SetItemInfo(ReturnItemInfo);

	// If the interaction was held, try to equip the item
	if (WasHeld)
	{
		if (OwnerCharacter->GetEquipmentManager()->IsItemEquippable(ReturnItem))
		{
			OwnerCharacter->GetEquipmentManager()->TryToEquip(ReturnItem, true, ReturnItemInfo.EquipmentSlot);
		}
		else
		{
			OwnerCharacter->GetEquipmentManager()->TryToEquip(ReturnItem, false, ReturnItemInfo.EquipmentSlot);
		}

		if (InteractableManager)
		{
			InteractableManager->RemoveInteraction();
		}
	}
	else
	{
		// Otherwise, try to add the item to the inventory
		if (OwnerCharacter->GetInventoryManager()->TryToAddItemToInventory(ReturnItem, true))
		{
			if (InteractableManager && InteractableManager->DestroyAfterInteract)
			{
				InteractableManager->RemoveInteraction();
			}
		}
	}

	return false;
}


void AItemPickup::HandleHeldInteraction(APHBaseCharacter* Character) const
{

}

void AItemPickup::HandleSimpleInteraction(APHBaseCharacter* Character) const
{
	
}

void AItemPickup::SetWidgetRarity()
{
	
	if (InteractionWidget->IsWidgetVisible())
	{
		SetWidgetRarity();
	}
}


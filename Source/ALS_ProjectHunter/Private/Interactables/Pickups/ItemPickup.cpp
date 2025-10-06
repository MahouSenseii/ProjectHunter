// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/Pickups/ItemPickup.h"

#include "Character/Player/PHPlayerController.h"
#include "Components/InteractableManager.h"
#include "Components/InventoryManager.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Library/InteractionEnumLibrary.h"
#include "Library/PHItemFunctionLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "UI/InteractableWidget.h"


AItemPickup::AItemPickup()
{
	// Create RotatingMovementComponent and attach it to RootComponent
	RotatingMovementComponent = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovementComponent"));

	// Create ParticleComponent and attach it to RootComponent
	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystemComponent"));
	ParticleSystemComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	// Set rotation rate (can also be changed later during gameplay)
	RotatingMovementComponent->RotationRate = FRotator(0.f, 180.f, 0.f);
	// Rotate 180 degrees per second around the yaw axis.

	if (ItemInfo)
	{
		StaticMesh->SetStaticMesh(ItemInfo->Base.StaticMesh);
	};
}

UBaseItem* AItemPickup::CreateItemObject(UObject* Outer)
{
	if (!Outer)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateItemObject: Outer is null"));
		return nullptr;
	}


	UEquippableItem* Item = NewObject<UEquippableItem>(Outer);
	if (!Item)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateItemObject: Failed to create UEquippableItem"));
		return nullptr;
	}

	return Item;
}

void AItemPickup::BeginPlay()
{
	Super::BeginPlay();

	if (ItemInfo)
	{
		StaticMesh->SetStaticMesh(ItemInfo->Base.StaticMesh);
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
}

void AItemPickup::BPIInteraction_Implementation(AActor* Interactor, bool WasHeld)
{
	UItemDefinitionAsset* PassedItemInfo = nullptr ;
	FEquippableItemData EquippableItemData;
	FConsumableItemData ConsumableItemData;
	HandleInteraction(Interactor, WasHeld, PassedItemInfo, ConsumableItemData);
}

UBaseItem* AItemPickup::GenerateItem() 
{
	
	if (GetWorld())
	{
		if (UBaseItem* NewItem = NewObject<UBaseItem>(GetTransientPackage(), UBaseItem::StaticClass()))
		{
			NewItem->SetItemInfo(ItemInfo);			
			// Initialize  item's properties here 
			return NewItem;
		}
	}
	return nullptr;
}

bool AItemPickup::HandleInteraction(AActor* Actor, bool WasHeld, UItemDefinitionAsset*& PassedItemInfo, FConsumableItemData ConsumableItemData) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	const APHPlayerController* PlayerController = Cast<APHPlayerController>(Actor);
	
	APHBaseCharacter* OwnerCharacter = Cast<APHBaseCharacter>(PlayerController->AcknowledgedPawn);
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("Owner is not a valid APHBaseCharacter."));
		return false;
	}

	// Get the item information
	if (!ItemInfo)
	{
		return false;
	}
	PassedItemInfo = ItemInfo;
	UBaseItem* ReturnItem = UPHItemFunctionLibrary::GetItemInformation( PassedItemInfo, ConsumableItemData);
	if (!ReturnItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to create item from ItemInfo."));
		return false;
	}

	// Set the Owner ID for the item
	UItemDefinitionAsset* ReturnItemInfo = ReturnItem->GetItemInfo();
	ReturnItemInfo->Base.OwnerID = OwnerCharacter->GetInventoryManager()->GetID();
	ReturnItem->SetItemInfo(ReturnItemInfo);

	// If the interaction was held, try to equip the item
	if (WasHeld)
	{
		if (OwnerCharacter->GetEquipmentManager()->IsItemEquippable(ReturnItem))
		{
			OwnerCharacter->GetEquipmentManager()->TryToEquip(ReturnItem, true, ReturnItemInfo->Base.EquipmentSlot);
		}
		else
		{
			OwnerCharacter->GetEquipmentManager()->TryToEquip(ReturnItem, false, ReturnItemInfo->Base.EquipmentSlot);
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

        if (!InteractableManager)
        {
                return;
        }

       
        if (InteractionWidget && InteractionWidget->GetWidget() && InteractionWidget->IsWidgetVisible())
        {
                if (UInteractableWidget* InteractableWidget = InteractableManager->InteractionWidgetRef)
                {
                        InteractableWidget->SetGrade(ItemInfo->Base.ItemRarity);
                }
        }
}


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

	if (ObjItem)
	{
		StaticMesh->SetStaticMesh(ObjItem->ItemDefinition->Base.StaticMesh);
	};
}

UBaseItem* AItemPickup::CreateItemObjectWClass(TSubclassOf<UBaseItem> ItemClass, UObject* Outer)
{
	if (!ItemClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateItemObject: ItemClass is null"));
		return nullptr;
	}

	if (!Outer)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateItemObject: Outer is null"));
		return nullptr;
	}

	UBaseItem* Item = NewObject<UBaseItem>(Outer, ItemClass);
	if (!Item)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateItemObject: Failed to create UEquippableItem"));
		return nullptr;
	}

	return Item;
}


 UEquippableItem* AItemPickup::CreateItemObject(UObject* Outer)
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
	if (ObjItem)
	{
		StaticMesh->SetStaticMesh(ObjItem->ItemDefinition->Base.StaticMesh);
		StaticMesh->CanCharacterStepUpOn = ECB_No;
		StaticMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		if(InteractableManager)
		{
			InteractableManager->IsInteractableChangeable = true;
			InteractableManager->DestroyAfterInteract = true;
			InteractableManager->InputType = EInteractType::Single;
			InteractableManager->InteractableResponse = EInteractableResponseType::OnlyOnce;
			if (!ObjItem)
			{
				ObjItem = CreateItemObject(ObjItemClass);
			}
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
			NewItem->SetItemInfo(ObjItem->ItemDefinition);			
			// Initialize  item's properties here 
			return NewItem;
		}
	}
	return nullptr;
}

bool AItemPickup::HandleInteraction(AActor* Actor, bool WasHeld,
	UItemDefinitionAsset*& PassedItemInfo, FConsumableItemData ConsumableItemData)
{
    if (!IsValid(Actor))
    {
        return false;
    }

    // NULL CHECK HERE!
    const APHPlayerController* PlayerController = Cast<APHPlayerController>(Actor);
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("Actor is not a valid APHPlayerController."));
        return false;
    }
    
    APHBaseCharacter* OwnerCharacter = Cast<APHBaseCharacter>(PlayerController->AcknowledgedPawn);
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("Owner is not a valid APHBaseCharacter."));
        return false;
    }

    // Use the already-created item with generated affixes!
    if (!ObjItem || !ObjItem->ItemDefinition)
    {
        UE_LOG(LogTemp, Error, TEXT("ObjItem or ItemDefinition is null!"));
        return false;
    }

    // Set the Owner ID for the item
    ObjItem->RuntimeData.OwnerID = OwnerCharacter->GetInventoryManager()->GetID();
    
    // Pass the item definition if needed
    PassedItemInfo = ObjItem->ItemDefinition;

    // If the interaction was held, try to equip the item
    if (WasHeld)
    {
        const UItemDefinitionAsset* ItemInfo = ObjItem->ItemDefinition;
        
        if (OwnerCharacter->GetEquipmentManager()->IsItemEquippable(ObjItem))   
        {
            HandleHeldInteraction(OwnerCharacter);
        	
        }
        else
        {
            HandleSimpleInteraction(OwnerCharacter);
        }

        if (InteractableManager)
        {
            InteractableManager->RemoveInteraction();
        }
        
        return true; // Success!
    }
    else
    {
        // Try to add the item to the inventory
        if (OwnerCharacter->GetInventoryManager()->TryToAddItemToInventory(ObjItem, true))
        {
            if (InteractableManager && InteractableManager->DestroyAfterInteract)
            {
                InteractableManager->RemoveInteraction();
            }
            return true; // Success!
        }
    }

    return false; // Failed to add to inventory
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
                        InteractableWidget->SetGrade(ObjItem->ItemDefinition->Base.ItemRarity);
                }
        }
}


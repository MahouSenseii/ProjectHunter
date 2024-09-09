// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/Pickups/ItemPickup.h"

#include "Components/InteractableManager.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Library/InteractionEnumLibrary.h"
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

UBaseItem* AItemPickup::GenerateItem() const
{
	// Ensure we are operating within the correct world context
	if (GetWorld())
	{
		if (UBaseItem* NewItem = NewObject<UBaseItem>(GetTransientPackage(), UBaseItem::StaticClass()))
		{
			NewItem->ItemInfos = ItemInfo;			
			// Initialize your item's properties here, if necessary
			return NewItem;
		}
	}
	return nullptr;
}

bool AItemPickup::InteractionHandle(AActor* Actor, bool WasHeld) const
{
	Super::InteractionHandle(Actor, WasHeld);

	// Get item information once.
	ObjItem = GenerateItem();

	if (IsValid(Actor))
	{
		// Cast to ALSBaseCharacter once.
		APHBaseCharacter* Character = Cast<APHBaseCharacter>(Actor);
		if (Character == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Actor is not an ALSBaseCharacter."));
			return false;
		}
		
		// Handle held interaction
		if (WasHeld)
		{
			HandleHeldInteraction(Character);
		}
		else // Handle simple interaction
		{
			HandleSimpleInteraction(Character);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid actor passed to InteractionHandle."));
		return false;
	}
	return true;
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

// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Player/PHPlayerController.h"

#include "Components/InteractableManager.h"

APHPlayerController::APHPlayerController(const FObjectInitializer& ObjectInitializer) : AALSPlayerController(ObjectInitializer)
{
	InteractionManager = CreateDefaultSubobject<UInteractionManager>(TEXT("Interaction Manager"));

	InteractionManager->OnNewInteractableAssigned.AddDynamic(this, &APHPlayerController::SetCurrentInteractable);
	InteractionManager->OnRemoveCurrentInteractable.AddDynamic(this, &APHPlayerController::RemoveCurrentInteractable);
}

AActor* APHPlayerController::GetCurrentInteractableObject_Implementation()
{
	if(IsValid(CurrentInteractable))
	{
		return CurrentInteractable->GetOwner();
	}
	return nullptr;
}

void APHPlayerController::InitializeInteractionWithObject_Implementation(UInteractableManager* Interactable)
{
	if(IsValid(Interactable))
	{
		ClientInitializeInteractionWithObject(Interactable);
	}
}

void APHPlayerController::ClientInitializeInteractionWithObject_Implementation(UInteractableManager* Interactable)
{
	InitializeInteraction(Interactable);
}

void APHPlayerController::InitializeInteraction(UInteractableManager* Interactable)
{
	if(IsValid(Interactable))
	{
		Interactable->PreInteraction(this);
	}
}

void APHPlayerController::StartInteractionWithObject_Implementation(UInteractableManager* Interactable)
{
	if(IsValid(Interactable))
	{
		ServerStartInteractionWithObject(Interactable);
		ClientStartInteractionWithObject(Interactable);
	}
}



void APHPlayerController::ServerStartInteractionWithObject_Implementation(UInteractableManager* Interactable)
{
	StartInteraction(Interactable);
}

void APHPlayerController::ClientStartInteractionWithObject_Implementation(UInteractableManager* Interactable)
{
	Interactable->ClientInteraction(this, false);
}

void APHPlayerController::EndInteractionWithObject_Implementation(UInteractableManager* Interactable)
{
	if(IsValid(Interactable))
	{
		ServerEndInteractionWithObject(Interactable);
		ClientEndInteractionWithObject(Interactable);
	}
}

void APHPlayerController::RemoveInteractionFromObject_Implementation(UInteractableManager* Interactable)
{
	if(IsValid(Interactable))
	{
		ServerRemoveInteractionFromObject(Interactable);
		ClientRemoveInteractionFromObject(Interactable);
	}
}

void APHPlayerController::ServerRemoveInteractionFromObject_Implementation(UInteractableManager* Interactable)
{
	RemoveInteraction(Interactable);
}

void APHPlayerController::ServerEndInteractionWithObject_Implementation(UInteractableManager* Interactable)
{
	EndInteraction(Interactable);
}

void APHPlayerController::ClientEndInteractionWithObject_Implementation(UInteractableManager* Interactable)
{
	Interactable->ClientEndInteraction(this);
}

void APHPlayerController::ClientRemoveInteractionFromObject_Implementation(UInteractableManager* Interactable)
{
	Interactable->ClientRemoveInteraction();
}

void APHPlayerController::EndInteraction(const UInteractableManager* Interactable)
{
	if(IsValid(Interactable))
	{
		AActor* LObject =  Interactable->GetOwner();
		Interactable->EndInteraction(this);
	}
}

void APHPlayerController::StartInteraction(UInteractableManager* Interactable)
{
	if(IsValid(Interactable))
	{
		Interactable->Interaction(this, false);
		
	}
}

void APHPlayerController::RemoveInteraction(UInteractableManager* Interactable)
{
	if(IsValid(Interactable))
	{
		if(Interactable->GetOwner()->GetIsReplicated())
		{
			Interactable->RemoveInteraction();
		}
	}
}

void APHPlayerController::SetCurrentInteractable(UInteractableManager* InInteractable)
{
	CurrentInteractable = InInteractable;
}

void APHPlayerController::RemoveCurrentInteractable(UInteractableManager* RemovedInteractable)
{
	CurrentInteractable = nullptr;
}

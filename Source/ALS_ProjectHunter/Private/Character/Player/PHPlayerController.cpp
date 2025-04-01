// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Player/PHPlayerController.h"

#include "Character/Player/PHPlayerCharacter.h"
#include "Components/InteractableManager.h"
#include "Components/WidgetManager.h"

APHPlayerController::APHPlayerController(const FObjectInitializer& ObjectInitializer) : AALSPlayerController(ObjectInitializer)
{
	InteractionManager = CreateDefaultSubobject<UInteractionManager>(TEXT("Interaction Manager"));

	InteractionManager->OnNewInteractableAssigned.AddDynamic(this, &APHPlayerController::SetCurrentInteractable);
	InteractionManager->OnRemoveCurrentInteractable.AddDynamic(this, &APHPlayerController::RemoveCurrentInteractable);
	WidgetManager = CreateDefaultSubobject<UWidgetManager>(TEXT("WidgetManager"));

}

void APHPlayerController::BeginPlay()
{
	Super::BeginPlay();
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

void APHPlayerController::StartInteractionWithObject_Implementation(UInteractableManager* Interactable, const bool WasHeld)
{
	if(IsValid(Interactable))
	{
		ServerStartInteractionWithObject(Interactable, WasHeld);
		ClientStartInteractionWithObject(Interactable, WasHeld);
	}
}



void APHPlayerController::ServerStartInteractionWithObject_Implementation(UInteractableManager* Interactable, const bool WasHeld)
{
 	StartInteraction(Interactable, WasHeld);
}

void APHPlayerController::ClientStartInteractionWithObject_Implementation(UInteractableManager* Interactable, const bool WasHeld)
{
		Interactable->ClientInteraction(this, WasHeld);
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

void APHPlayerController::StartInteraction(UInteractableManager* Interactable, const bool WasHeld)
{
	if(IsValid(Interactable))
	{
		Interactable->Interaction(this,  WasHeld);
		
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

/*
 *Project Hunter Input Actions
 * - Interact is added to the ALS controller
 */
void APHPlayerController::Menu(const FInputActionValue& Value) const
{
	if (PossessedCharacter && Value.Get<bool>())
	{
		WidgetManager->WidgetCheck(Cast<APHBaseCharacter>(PossessedCharacter));
	}
}



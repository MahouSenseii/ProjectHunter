// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Player/PHPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

#include "Components/InteractableManager.h"
#include "Components/WidgetManager.h"

struct FEnhancedActionKeyMapping;

APHPlayerController::APHPlayerController(const FObjectInitializer& ObjectInitializer) : AALSPlayerController(ObjectInitializer)
{
	InteractionManager = CreateDefaultSubobject<UInteractionManager>(TEXT("Interaction Manager"));

	InteractionManager->OnNewInteractableAssigned.AddDynamic(this, &APHPlayerController::SetCurrentInteractable);
	InteractionManager->OnRemoveCurrentInteractable.AddDynamic(this, &APHPlayerController::RemoveCurrentInteractable);
	WidgetManager = CreateDefaultSubobject<UWidgetManager>(TEXT("WidgetManager"));

}

void APHPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

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

EInteractType APHPlayerController::CheckInputType(const float Elapsed, const float InRequiredHeldTime, const bool bReset)
{
	if (bReset)
	{
		bHasCheckedInputType = false;
	}

	if (!(Elapsed > RequiredHeldTime))
	{
		return EInteractType::Single;
	}

	if(DoOnce(MyDoOnce, bReset, false))
	{
		return EInteractType::Holding;
	}
	return EInteractType::Mashing; // this is used as a continue
}

float APHPlayerController::GetElapsedSeconds(const UInputAction* Action) const
{
	// Attempt to retrieve the Enhanced Input subsystem associated with the local player.
	// This subsystem manages input actions and mappings for the player.
	if (const auto EnhancedInput = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(this->GetLocalPlayer()))
	{
		if (const auto LocalPlayerInput = EnhancedInput->GetPlayerInput())
		{
			if (const auto ActionData = LocalPlayerInput->FindActionInstanceData(Action))
			{
				return ActionData->GetElapsedTime();
			}
		}
	}
	// If any step fails (e.g., the action is not found), return 0 to indicate no elapsed time.
	return 0;
}

const UInputAction* APHPlayerController::GetInputActionByName(const FString& InString) const
{
	const UInputMappingContext* Context = DefaultInputMappingContext;
	TObjectPtr<const UInputAction> FoundAction  = nullptr;

	if (Context)
	{
		const TArray<FEnhancedActionKeyMapping>& Mappings = Context->GetMappings();
		for (const FEnhancedActionKeyMapping& Keymapping : Mappings)
		{
			if (Keymapping.Action && Keymapping.Action->GetFName() == InString)
			{
				FoundAction = Keymapping.Action;
				break; // Found the Interact action, no need to search further.
			}
		}
		return FoundAction;
	}
	return nullptr;
}

bool APHPlayerController::DoOnce(FDoOnceState& State, bool bReset, bool bStartClosed)
{
	if (bReset)
	{
		State.bHasBeenInitialized = true;
		State.bIsClosed = false;
		return false; // Nothing should execute on reset
	}

	if (!State.bHasBeenInitialized)
	{
		State.bHasBeenInitialized = true;
		if (bStartClosed)
		{
			State.bIsClosed = true;
			return false; // Starts closed, skip execution
		}
	}

	if (!State.bIsClosed)
	{
		State.bIsClosed = true;
		return true;
	}

	return false; // Already done once, do nothing
}

void APHPlayerController::Interact(const FInputActionValue& Value)
{
	if (Value.Get<bool>()) // Button Pressed
	{
		bHasInteractBeenReleased = false;
		GetWorld()->GetTimerManager().ClearTimer(InteractionDelayHandle);

		UInteractableManager* Interactable = InteractionManager->GetCurrentInteractable();

		// Block if holding and interactable has changed
		if (SavedInteractable && Interactable != SavedInteractable && !bHasInteractBeenReleased)
		{
			return;
		}

		// Save the current interactable at press
		SavedInteractable = Interactable;

		// Trigger visuals immediately
		if (Interactable && Interactable->IsInteractable)
		{
			Interactable->InputType = EInteractType::Holding;
			Interactable->PreInteraction(this);
		}

		// Start delayed check for hold input
		GetWorld()->GetTimerManager().SetTimer(
			InteractionDelayHandle,
			this,
			&APHPlayerController::RunInteractionCheck,
			RequiredHeldTime,
			false
		);
	}
	else // Button Released
	{
		bHasInteractBeenReleased = true;
		GetWorld()->GetTimerManager().ClearTimer(InteractionDelayHandle);

		float HeldTime = GetElapsedSeconds(GetInputActionByName("Interact"));
		if (HeldTime < RequiredHeldTime)
		{
			if (UInteractableManager* Interactable = InteractionManager->GetCurrentInteractable())
			{
				Interactable->InputType = EInteractType::Single;
				Execute_InitializeInteractionWithObject(this, Interactable);
			}
		}

		// Reset for next use
		DoOnce(MyDoOnce, true, false);
		SavedInteractable = nullptr;
	}
}


void APHPlayerController::RunInteractionCheck()
{
	CachedHoldTime = GetElapsedSeconds(GetInputActionByName("Interact"));

	UInteractableManager* Interactable = InteractionManager->GetCurrentInteractable();\
	SavedInteractable = Interactable;
	if (!Interactable || !Interactable->IsInteractable)
	{
		return;
	}

	if (!Interactable->IsInteractableChangeable)
	{
		InitializeInteractionWithObject_Implementation(Interactable);
		bHasInteractBeenReleased = false;
		bHasCheckedInputType = false;
		return;
	}

	EInteractType InputType = EInteractType::Mashing;

	// Decide input type based on state
	if (bHasInteractBeenReleased && CachedHoldTime < RequiredHeldTime)
	{
		InputType = EInteractType::Single;
	}
	else if (!bHasInteractBeenReleased && CachedHoldTime >= RequiredHeldTime && DoOnce(MyDoOnce, false, false))
	{
		InputType = EInteractType::Holding;
	}

	Interactable->InputType = InputType;

	switch (InputType)
	{
	case EInteractType::Single:
		InitializeInteractionWithObject_Implementation(Interactable);
		break;

	case EInteractType::Holding:
		Interactable->ToggleHighlight(true, this);
		InitializeInteractionWithObject_Implementation(Interactable);
		break;

	case EInteractType::Mashing:
	default:
		break; // Do nothing (used for ongoing spam or fallback)
	}

	bHasInteractBeenReleased = false;
	bHasCheckedInputType = false;
}

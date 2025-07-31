// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InteractionManager.h"

#include "Character/PHBaseCharacter.h"
#include "Math/Vector.h"
#include "Components/InteractableManager.h"
#include "Character/Player/PHPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Library/FL_InteractUtility.h"


DEFINE_LOG_CATEGORY(LogInteract);
// Sets default values for this component's properties
UInteractionManager::UInteractionManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	// ...
}


void UInteractionManager::BeginPlay()
{
	Super::BeginPlay();
	OwnerController = Cast<APHPlayerController>(GetOwner());

	// Initialize a timer to call UpdateInteraction method
	FTimerHandle UpdateInteractionTimerHandle;
	constexpr float LoopInterval = 0.1f; // Time between each call to UpdateInteraction
	constexpr float InitialDelay = 1.0f; // Initial delay before the timer starts

	// Set the timer to call UpdateInteraction. This timer will call UpdateInteraction every 0.1 seconds after a 1.0 second delay.
	GetWorld()->GetTimerManager().SetTimer(UpdateInteractionTimerHandle, this,
		&UInteractionManager::UpdateInteraction, LoopInterval, true, InitialDelay);
}

// Determines whether interaction should be updated and updates if needed.
void UInteractionManager::UpdateInteraction()
{
	if(OwnerController == nullptr)
	{
		OwnerController = Cast<APHPlayerController>(GetOwner());
	}
	if (ShouldUpdateInteraction())
	{
		AssignCurrentInteraction();
	}
}

// Sets the current interactable object. If the new interactable differs from the current,
// the interaction with the current is removed, and the new interactable is set.
// Useful for managing interactions in networked multiplayer scenarios and accessible via Blueprints.
void UInteractionManager::SetCurrentInteraction(UInteractableManager* NewInteractable)
{
	// Log the attempt to set a new interactable
	UE_LOG(LogInteract, Warning, TEXT("Setting Current Interaction"));

	// If the new interactable is the same as the current, no action is needed
	if (NewInteractable == CurrentInteractable)
	{
		UE_LOG(LogInteract, Warning, TEXT("New and Current Interactables are the same. No change needed."));
		return;
	}

	// Remove interaction from the current interactable, if it exists
	if (IsValid(CurrentInteractable))
	{
		RemoveInteractionFromCurrent(CurrentInteractable);
		UE_LOG(LogInteract, Warning, TEXT("Removed Interaction from Current Interactable"));
	}

	// Update to the new interactable and add interaction if valid
	CurrentInteractable = NewInteractable;
	if (IsValid(CurrentInteractable))
	{
		AddInteraction(CurrentInteractable);
		UE_LOG(LogInteract, Warning, TEXT("Added Interaction to New Interactable"));
	}
}



void UInteractionManager::AddInteraction(UInteractableManager* Interactable)
{
	if (IsValid(Interactable))
	{
		if(CurrentInteractable != Interactable)
		{
			// Optionally remove previous interaction; can be conditional based on your requirements
			OnRemoveCurrentInteractable.Broadcast(CurrentInteractable);
			RemoveInteractionFromCurrent(nullptr);
		}
				
		// Highlight the new interactable object, assuming OwnerCharacter and its Controller are always valid
		Interactable->ToggleHighlight(true, OwnerController);
		
		// Set the current interactable object
		CurrentInteractable = Interactable;
		OnNewInteractableAssigned.Broadcast(Interactable);
	}
	
}






/**
 * Find the maximum value and its index in a given array of floats.
 *
 * @param FloatArray - The array of floats to search through.
 * @param OutMaxValue - The maximum value found, or -FLT_MAX if array is empty.
 * @param OutMaxIndex - The index of the maximum value, or -1 if array is empty.
 */

void UInteractionManager::MaxOfFloatArray(const TArray<float>& FloatArray, float& OutMaxValue, int32& OutMaxIndex)
{
	// Check if the float array is empty, and if so, return defaults
	if (FloatArray.Num() == 0)
	{
		OutMaxValue = -FLT_MAX;
		OutMaxIndex = -1;
		return;
	}

	// Initialize maximum value and index to the first element in the array
	OutMaxValue = FloatArray[0];
	OutMaxIndex = 0;

	// Loop through each element in the float array to find the maximum value and its index
	for (int32 Index = 0; Index < FloatArray.Num(); ++Index)
	{
		if (const float& Value = FloatArray[Index]; Value >= OutMaxValue)
		{
			OutMaxValue = Value;
			OutMaxIndex = Index;
			UE_LOG(LogInteract, Warning, TEXT("New maximum value found: %f at index %d."), OutMaxValue, OutMaxIndex);
		}
	}
}

void UInteractionManager::AssignCurrentInteraction()
{
	if (!IsValid(OwnerController) || !IsValid(OwnerController->PossessedCharacter)) return;

	const APHBaseCharacter* OwnerPawn = Cast<APHBaseCharacter>(OwnerController->PossessedCharacter);
	const FVector OwnerLocation = OwnerPawn->GetActorLocation();
	const FVector OwnerForward = OwnerPawn->GetCamera()->GetActorForwardVector();

	TArray<float> Scores;
	TArray<UInteractableManager*> ValidElements;

	UFL_InteractUtility::GetInteractableScores(
		InteractableList,
		OwnerLocation,
		OwnerForward,
		InteractMaxDistance > 0.f ? InteractMaxDistance : 800.f,
		InteractThreshold,
		Scores,
		ValidElements,
		bDebugMode
	);

	float MaxScore;
	int32 MaxIndex;
	MaxOfFloatArray(Scores, MaxScore, MaxIndex);

	if (MaxIndex != INDEX_NONE)
	{
		SetCurrentInteraction(ValidElements[MaxIndex]);

		UE_LOG(LogInteract, Warning, TEXT("Selected: %s with Score %.3f"),
			*ValidElements[MaxIndex]->GetOwner()->GetName(), MaxScore);
	}
	else
	{
		SetCurrentInteraction(nullptr);
		UE_LOG(LogInteract, Warning, TEXT("No valid interactable found"));
	}
}


void UInteractionManager::RemoveInteractionFromCurrent(UInteractableManager* Interactable)
{
	// Early exit if any of the involved objects are not valid.
	// This could be a critical error and should not happen.
	if (!IsValid(Interactable) || !IsValid(OwnerController) || !IsValid(CurrentInteractable))
	{
		UE_LOG(LogInteract, Error, TEXT("Invalid parameters in RemoveInteractionFromCurrent. This should not happen under normal conditions."));
		return;
	}

	
	// Toggle off the highlight for the current interactable object.
	ToggleHighlight(false);
	CurrentInteractable = nullptr;

	// Perform any cleanup or state reset required when an interaction ends.
	OnRemoveCurrentInteractable.Broadcast(Interactable);
}


void UInteractionManager::ToggleHighlight(const bool bShouldHighlight) const
{
	if (IsValid(CurrentInteractable) && IsValid(OwnerController) && IsValid(OwnerController))
	{
		CurrentInteractable->ToggleHighlight(bShouldHighlight, OwnerController);
	}
}



bool UInteractionManager::ShouldUpdateInteraction()
{
	if (IsValid(OwnerController) && IsValid(OwnerController->PossessedCharacter))
	{
		
		const APHBaseCharacter* OwnerPawn = Cast<APHBaseCharacter>(OwnerController->PossessedCharacter);
		const bool bCanUpdate = (! OwnerPawn->GetCharacterMovement()->IsFalling()) && (!InteractableList.IsEmpty());
		if ( OwnerPawn->GetCharacterMovement()->IsFalling())
		{
			UE_LOG(LogInteract, Warning, TEXT("Cannot update interaction. Character might be falling."));
		}
		if(InteractableList.IsEmpty())
		{
			UE_LOG(LogInteract, Warning, TEXT("Cannot update interaction. InteractionList is Empty"));
		}
		return bCanUpdate;
	}
	
	return false;
}

void UInteractionManager::AddToInteractionList(UInteractableManager* InInteractable)
{
	InteractableList.Add(InInteractable);
}

void UInteractionManager::RemoveFromInteractionList(UInteractableManager* InInteractable)
{
	InteractableList.Remove(InInteractable);
}



// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InteractionManager.h"

#include "Character/PHBaseCharacter.h"
#include "Math/Vector.h"
#include "Components/InteractableManager.h"
#include "Components/SphereComponent.h"
#include "Character/Player/PHPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Item/EquippableItem.h"

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
	UE_LOG(LogTemp, Warning, TEXT("Setting Current Interaction"));

	// If the new interactable is the same as the current, no action is needed
	if (NewInteractable == CurrentInteractable)
	{
		UE_LOG(LogTemp, Warning, TEXT("New and Current Interactables are the same. No change needed."));
		return;
	}

	// Remove interaction from the current interactable, if it exists
	if (IsValid(CurrentInteractable))
	{
		RemoveInteractionFromCurrent(CurrentInteractable);
		UE_LOG(LogTemp, Warning, TEXT("Removed Interaction from Current Interactable"));
	}

	// Update to the new interactable and add interaction if valid
	CurrentInteractable = NewInteractable;
	if (IsValid(CurrentInteractable))
	{
		AddInteraction(CurrentInteractable);
		UE_LOG(LogTemp, Warning, TEXT("Added Interaction to New Interactable"));
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
			UE_LOG(LogTemp, Warning, TEXT("New maximum value found: %f at index %d."), OutMaxValue, OutMaxIndex);
		}
	}
}

void UInteractionManager::AssignCurrentInteraction()
{
	if (IsValid(OwnerController) && IsValid(OwnerController->PossessedCharacter))
	{
		TArray<float> DotProducts;
		const APHBaseCharacter* OwnerPawn = Cast<APHBaseCharacter>(OwnerController->PossessedCharacter);
		const FVector OwnerLocation = OwnerPawn ? OwnerPawn->GetRootComponent()->GetComponentLocation() : FVector::ZeroVector;
	
		// Calculate Dot Products for interactable objects only
		for (const UInteractableManager* Element : InteractableList)
		{
			constexpr float Tolerance = 0.0001f;
			if (!Element || !Element->IsInteractable ) continue;  // Skip non-interactable or null element
			
				FVector InteractableLocation = Element->InteractableArea->GetComponentLocation();
				FVector ForwardVector = OwnerPawn->GetCamera()->GetActorForwardVector();
				FVector DirectionVector = (InteractableLocation - OwnerLocation).GetSafeNormal(Tolerance);
				float DotProductResult = FVector::DotProduct(DirectionVector, ForwardVector);
				DotProducts.Add(DotProductResult);
		}

		// Find the maximum DotProduct and its index
		float MaxValue = FLT_MIN;
		int MaxIndex = INDEX_NONE;
		MaxOfFloatArray(DotProducts, MaxValue, MaxIndex);

		// Check if a suitable interactable has been found
		if (MaxValue >= InteractThreshold)
		{
			SetCurrentInteraction(InteractableList[MaxIndex]);
			UE_LOG(LogTemp, Warning, TEXT("Set a new interaction based on dot product."));
		}
		else
		{
			SetCurrentInteraction(nullptr);
			UE_LOG(LogTemp, Warning, TEXT("No suitable interaction found based on dot product."));
		}
	}
}


void UInteractionManager::RemoveInteractionFromCurrent(UInteractableManager* Interactable)
{
	// Early exit if any of the involved objects are not valid.
	// This could be a critical error and should not happen.
	if (!IsValid(Interactable) || !IsValid(OwnerController) || !IsValid(CurrentInteractable))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid parameters in RemoveInteractionFromCurrent. This should not happen under normal conditions."));
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
			UE_LOG(LogTemp, Warning, TEXT("Cannot update interaction. Character might be falling."));
		}
		if(InteractableList.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("Cannot update interaction. InteractionList is Empty"));
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



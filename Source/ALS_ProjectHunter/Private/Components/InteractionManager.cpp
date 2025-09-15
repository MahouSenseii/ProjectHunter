// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InteractionManager.h"

#include "Character/PHBaseCharacter.h"
#include "Math/Vector.h"
#include "Components/InteractableManager.h"
#include "Character/Player/PHPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Library/FL_InteractUtility.h"


struct FSpatialHash
{
	TMap<FIntPoint, TArray<UInteractableManager*>> Grid;
	float CellSize = 200.0f; // Adjust based on your interaction range
    
	FIntPoint GetCell(const FVector& Location)
	{
		return FIntPoint(
			FMath::FloorToInt(Location.X / CellSize),
			FMath::FloorToInt(Location.Y / CellSize)
		);
	}
    
	void Add(UInteractableManager* Interactable)
	{
		FIntPoint Cell = GetCell(Interactable->GetOwner()->GetActorLocation());
		Grid.FindOrAdd(Cell).Add(Interactable);
	}
    
	void Remove(UInteractableManager* Interactable)
	{
		FIntPoint Cell = GetCell(Interactable->GetOwner()->GetActorLocation());
		if (auto* CellArray = Grid.Find(Cell))
		{
			CellArray->Remove(Interactable);
			if (CellArray->Num() == 0)
			{
				Grid.Remove(Cell);
			}
		}
	}
    
	TArray<UInteractableManager*> GetNearby(const FVector& Location, float Radius)
	{
		TArray<UInteractableManager*> Result;
		int32 CellRadius = FMath::CeilToInt(Radius / CellSize);
		FIntPoint CenterCell = GetCell(Location);
        
		// Check all cells that could contain items within radius
		for (int32 X = -CellRadius; X <= CellRadius; X++)
		{
			for (int32 Y = -CellRadius; Y <= CellRadius; Y++)
			{
				FIntPoint Cell(CenterCell.X + X, CenterCell.Y + Y);
				if (auto* CellArray = Grid.Find(Cell))
				{
					Result.Append(*CellArray);
				}
			}
		}
        
		return Result;
	}
};


DEFINE_LOG_CATEGORY(LogInteract);
// Sets default values for this component's properties
UInteractionManager::UInteractionManager(): LastLookDirection()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	// ...
}


void UInteractionManager::BeginPlay()
{
	Super::BeginPlay();
	ScheduleNextUpdate();

}

void UInteractionManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
    
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateInteractionTimerHandle);
	}
}

void UInteractionManager::ScheduleNextUpdate()
{
	// Adapt update rate based on number of nearby interactables
	float UpdateInterval = 0.1f; // Default
    
	if (InteractableList.Num() == 0)
	{
		UpdateInterval = 0.5f; // Slow when nothing around
	}
	else if (InteractableList.Num() < 3)
	{
		UpdateInterval = 0.15f; // Moderate for few items
	}
	else if (InteractableList.Num() < 10)
	{
		UpdateInterval = 0.1f; // Normal for several items
	}
	else
	{
		UpdateInterval = 0.05f; // Fast for many items (high density loot)
	}
    
	GetWorld()->GetTimerManager().SetTimer(
		UpdateInteractionTimerHandle,
		this,
		&UInteractionManager::UpdateInteractionAdaptive,
		UpdateInterval,
		false // Non-looping, we'll reschedule each time
	);
}

void UInteractionManager::UpdateInteractionAdaptive()
{
	UpdateInteraction();
	ScheduleNextUpdate(); 
}


// Determines whether interaction should be updated and updates if needed.
void UInteractionManager::UpdateInteraction()
{
	// Lazy initialization of controller
	if (!OwnerController)
	{
		OwnerController = Cast<APHPlayerController>(GetOwner());
		if (!OwnerController) return;
	}
    
	bool bShouldUpdate = false;
    
	// Check 1: Has the list changed? (using version counter instead of copying array)
	if (CurrentListVersion != LastListVersion)
	{
		LastListVersion = CurrentListVersion;
		bShouldUpdate = true;
		UE_LOG(LogInteract, Verbose, TEXT("Interactable list changed"));
	}
    
	// Check 2: Is the player looking in a different direction? (critical for high item density)
	if (!bShouldUpdate && OwnerController->PossessedCharacter)
	{
		// Get current look direction (camera forward vector)
		FVector CurrentLookDirection;
		if (APHBaseCharacter* Character = Cast<APHBaseCharacter>(OwnerController->PossessedCharacter))
		{
			CurrentLookDirection = Character->GetCamera()->GetActorForwardVector();
		}
		else
		{
			CurrentLookDirection = OwnerController->PossessedCharacter->GetActorForwardVector();
		}
        
		// Check if look direction changed enough to matter
		float DotProduct = FVector::DotProduct(CurrentLookDirection, LastLookDirection);
		if (DotProduct < LookSensitivity) // Player looked away enough
		{
			bShouldUpdate = true;
			LastLookDirection = CurrentLookDirection;
			UE_LOG(LogInteract, Verbose, TEXT("Player look direction changed (dot: %.3f)"), DotProduct);
		}
	}
    
	// Perform the update if needed
	if (bShouldUpdate && ShouldUpdateInteraction())
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

bool UInteractionManager::IsValidForInteraction() const
{
	// Check if we have a valid controller
	if (!IsValid(OwnerController))
	{
		UE_LOG(LogInteract, Warning, TEXT("IsValidForInteraction: OwnerController is invalid"));
		return false;
	}
    
	// Check if controller has a possessed character
	if (!IsValid(OwnerController->PossessedCharacter))
	{
		UE_LOG(LogInteract, Warning, TEXT("IsValidForInteraction: PossessedCharacter is invalid"));
		return false;
	}
    
	// Optional: Check if character is in a valid state for interaction
	APHBaseCharacter* Character = Cast<APHBaseCharacter>(OwnerController->PossessedCharacter);
	if (!Character)
	{
		UE_LOG(LogInteract, Warning, TEXT("IsValidForInteraction: Failed to cast to PHBaseCharacter"));
		return false;
	}
	
	return true;
}

void UInteractionManager::AssignCurrentInteraction()
{
	    if (!IsValidForInteraction()) return;

    const APHBaseCharacter* OwnerPawn = Cast<APHBaseCharacter>(OwnerController->PossessedCharacter);
    const FVector OwnerLocation = OwnerPawn->GetActorLocation();
    const FVector OwnerForward = OwnerPawn->GetCamera()->GetActorForwardVector();
    
    // Early filtering for performance with many items
    TArray<UInteractableManager*> CandidateInteractables;
    CandidateInteractables.Reserve(InteractableList.Num());
    
    // First pass: Quick distance culling
    const float MaxDistanceSq = FMath::Square(InteractMaxDistance > 0.f ? InteractMaxDistance : 800.f);
    
    for (UInteractableManager* Interactable : InteractableList)
    {
        if (!IsValid(Interactable) || !Interactable->IsInteractable)
            continue;
            
        // Quick distance check (squared to avoid sqrt)
        float DistanceSq = FVector::DistSquared(OwnerLocation, Interactable->GetOwner()->GetActorLocation());
        if (DistanceSq <= MaxDistanceSq)
        {
            CandidateInteractables.Add(Interactable);
        }
    }
    
    // If we filtered down to a reasonable number, do the full scoring
    if (CandidateInteractables.Num() > 0)
    {
        TArray<float> Scores;
        TArray<UInteractableManager*> ValidElements;
        
        UFL_InteractUtility::GetInteractableScores(
            CandidateInteractables, // Use filtered list instead of full list
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
            UE_LOG(LogInteract, Verbose, TEXT("Selected: %s with Score %.3f (from %d candidates)"),
                *ValidElements[MaxIndex]->GetOwner()->GetName(), MaxScore, CandidateInteractables.Num());
        }
        else
        {
            SetCurrentInteraction(nullptr);
        }
    }
    else
    {
        SetCurrentInteraction(nullptr);
        UE_LOG(LogInteract, Verbose, TEXT("No interactables in range"));
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
	if (IsValid(CurrentInteractable) && IsValid(OwnerController))
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
	if (!InInteractable) return;
    
	InteractableList.Add(InInteractable);
	CurrentListVersion++; // Increment version on change
    
	UE_LOG(LogInteract, Verbose, TEXT("Added %s to interaction list (count: %d)"), 
		   *InInteractable->GetOwner()->GetName(), InteractableList.Num());
}

void UInteractionManager::RemoveFromInteractionList(UInteractableManager* InInteractable)
{
	if (!InInteractable) return;
    
	InteractableList.Remove(InInteractable);
	CurrentListVersion++; // Increment version on change
    
	// If we removed the current interactable, clear it immediately
	if (CurrentInteractable == InInteractable)
	{
		SetCurrentInteraction(nullptr);
	}
    
	UE_LOG(LogInteract, Verbose, TEXT("Removed %s from interaction list (count: %d)"), 
		   *InInteractable->GetOwner()->GetName(), InteractableList.Num());
}



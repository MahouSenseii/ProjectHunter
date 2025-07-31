// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "InteractableManager.h"
#include "Components/ActorComponent.h"
#include "InteractionManager.generated.h"


class APHPlayerController;
class APHPlayerCharacter;
class UInteractableManager;

DECLARE_LOG_CATEGORY_EXTERN(LogInteract, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNewInteractableAssigned, UInteractableManager*, NewInteractable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemoveCurrentInteractable, UInteractableManager*, RemovedIntercatable);



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALS_PROJECTHUNTER_API UInteractionManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInteractionManager();

	// The threshold for interaction
	UPROPERTY(EditAnywhere)
	float InteractThreshold = 0.25f;


	UPROPERTY(EditAnywhere)
	float InteractMaxDistance = 400.0f;

	UPROPERTY(EditAnywhere, Category = "Scoring")
	float DotWeight = 0.9f;

	UPROPERTY(EditAnywhere, Category = "Scoring")
	float DistanceWeight = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDebugMode = false;
	
	UPROPERTY()
	TObjectPtr<APHPlayerController> OwnerController;

	UPROPERTY(BlueprintAssignable, Category="Events") FOnNewInteractableAssigned OnNewInteractableAssigned;
	UPROPERTY(BlueprintAssignable, Category="Events") FOnRemoveCurrentInteractable OnRemoveCurrentInteractable;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Making this BlueprintReadOnly as well
	UPROPERTY(BlueprintReadWrite) TObjectPtr<UInteractableManager> CurrentInteractable;

	// Array for storing interactable objects
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly) TArray<UInteractableManager*> InteractableList;

public:	

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	UInteractableManager* GetCurrentInteractable() const { return CurrentInteractable; }

	// Update the current interaction
	UFUNCTION(BlueprintCallable, Category = "InteractionControl") void UpdateInteraction();

	// Set the current interaction
	UFUNCTION(BlueprintCallable, Category = "InteractionControl")
	void SetCurrentInteraction(UInteractableManager* NewInteractable);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void AddInteraction(UInteractableManager* Interactable);

	// Find the maximum value in a float array and its index
	/// Finds the maximum value in a given float array and its corresponding index
	UFUNCTION(BlueprintCallable, Category = "Math Utils")
	static void MaxOfFloatArray(const TArray<float>& FloatArray, float& OutMaxValue, int32& OutMaxIndex);

	// Assign the current interaction
	UFUNCTION(BlueprintCallable, Category = "Interactable")
	void AssignCurrentInteraction();
	
	// Remove an interaction from the current interaction
	UFUNCTION(BlueprintCallable, Category = "InteractionControl")
	void RemoveInteractionFromCurrent(UInteractableManager* Interactable);

	UFUNCTION(BlueprintCallable, Category = "InteractionControl")
	void ToggleHighlight(bool bShouldHighlight) const;

	// Check if the interaction should be updated
	UFUNCTION(BlueprintCallable, Category = "InteractionControl") bool ShouldUpdateInteraction();

	UFUNCTION(BlueprintGetter) TArray<UInteractableManager*> GetInteractableList() const { return InteractableList; }
	UFUNCTION(BlueprintCallable) void AddToInteractionList(UInteractableManager* InInteractable);
	UFUNCTION(BlueprintCallable) void RemoveFromInteractionList(UInteractableManager* InInteractable);
	
		
};

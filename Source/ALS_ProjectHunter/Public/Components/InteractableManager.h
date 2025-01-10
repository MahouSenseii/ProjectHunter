// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TimelineComponent.h"
#include "Engine/TargetPoint.h"
#include "Library/InteractionEnumLibrary.h"
#include "InteractableManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHoldingChangedDelegate, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMashingChangedDelegate, float, Value);

// Forward declarations
class UInputAction;
enum class EInteractType : uint8;
enum class EInteractableResponseType : uint8;
class UInteractableWidget;
class ABaseInteractable;
class AALSPlayerController;

/*
 *This Manager will handle all interactable allowing them to be found and set by InteractionManagers
 */

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALS_PROJECTHUNTER_API UInteractableManager : public UActorComponent
{
	GENERATED_BODY()
	
//Functions 
public:	
	// Sets default values for this component's properties
	UInteractableManager();
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
public:
	
	UFUNCTION(BlueprintCallable) void Initialize();

	//Before Interaction  Start 
	// Set up references for interactable components

	UFUNCTION() void SetupInteractableReferences(USphereComponent*& InInteractableArea, UWidgetComponent*& InInteractionWidget, const TSet<UPrimitiveComponent*>& InHighlightableObject);
	UFUNCTION() void SetupHighlightableObjects(TSet<UPrimitiveComponent*> InHighlightableObject);
	UFUNCTION(BlueprintCallable) void ToggleHighlight(bool Highlight, AActor* Interactor);
	UFUNCTION() void SetWidgetLocalOwner(APlayerController* OwnerPlayerController);
	UFUNCTION() void PreInteraction(AActor* Interactor);
	UFUNCTION() void DurationPress(AActor* Interactor);
	UFUNCTION() void IsKeyDown();
	UFUNCTION() void ClearInteractionTimer();
	
	UFUNCTION() static bool GetPressedByAction(const UInputAction* Action, AActor* Interactor, FKey& OutKey);
	UFUNCTION() void  HoldingInputHandle();
	UFUNCTION() bool GetKeyTimeDown(float& TimeDown) const;
	UFUNCTION() void MultiplePress();
	UFUNCTION() void  DelayedStartInteraction();
	UFUNCTION() bool MashingInput (float& Value);


	//Before Interaction End
	
	// On Interaction Start
	UFUNCTION() void Interaction(AActor* Interactor, bool WasHeld);
	UFUNCTION() void  ClientInteraction(AActor* Interactor, bool WasHeld) const;
	UFUNCTION() void AssociatedActorInteraction(AActor* Interactor);
	UFUNCTION() void CheckForInteractionWithAssociate(AActor* Interactor);
	UFUNCTION() bool IsTargetInteractableValue();
	UFUNCTION() void AssociateActorRemoveHandle(bool IsTemp) const;
	// On Interaction End

	// After Interaction
	UFUNCTION() void EndInteraction(AActor* Interactor) const;
	UFUNCTION() void ClientEndInteraction(AActor* Interactor) const;
	UFUNCTION() void AssociatedActorEndInteraction() const;
	UFUNCTION(Reliable, NetMulticast ) void RemoveInteraction();
	UFUNCTION() void ClientRemoveInteraction();
	UFUNCTION() void Re_Initialize();
	UFUNCTION() void Re_InitializeAssociatedActors();
	//After Interaction End 

	//Other
	UFUNCTION() void ToggleCanBeReInitialized(bool bCond);
	UFUNCTION() void ToggleIsInteractable(bool bCond);
	UFUNCTION() void ToggleInteractionWidget(bool Condition) const;
	UFUNCTION() void ChangeInteractableValue(bool Increment);
	// The delegate for Holding value
    FOnHoldingChangedDelegate OnUpdateHoldingValue;
	FOnMashingChangedDelegate OnUpdateMashingValue;

	UFUNCTION(BlueprintCallable) void SetHasInteracted(const bool bValue) {DoOnce = bValue;}

		
private:
	UFUNCTION() void RemoveInteractionByResponse();
	UFUNCTION() void DecreaseMashingProgress();
	
//Variables
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MashingAmount = 10;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Class") // Must set
	TSubclassOf<UInteractableWidget> WidgetClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere) // Must set
	UInputAction* ActionToCheckForInteract;
	
	UPROPERTY(BlueprintReadOnly, Category = "Components")
	USphereComponent* InteractableArea;

	UPROPERTY(BlueprintReadOnly, Category = "Components")
	UWidgetComponent* InteractionWidget;

	// is object interactable.
	UPROPERTY(BlueprintReadOnly,EditInstanceOnly, Replicated, Category = "Main")
	bool IsInteractable = true;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	bool IsInteractableChangeable;

	// should Interactable be destroyed after interacting.
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	bool DestroyAfterInteract = false;

	//Text shown on Interaction widget.
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	FText InteractionText = FText::FromString(TEXT("Interact"));

	UPROPERTY(BlueprintReadOnly,  Category = "Interaction")
	FName InteractableTag = "Interactable";

	// An array holding references to primitive components that should be highlighted during gameplay.
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TArray<UPrimitiveComponent*> ObjectsToHighlight;

	// A map holding references to actors that should associated with the main interactable object.
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	TMap<AActor*, int> AssociatedInteractableActors;
	
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	FName DestroyableTag = "Destroyable";

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool AlreadyInteracted = false;
	
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	float MaxKeyTimeDown = 1.0f;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	float MashingKeyRetriggerableTime = 1.0f;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	float MaxKeyRetriggerableTime = 0.2;

	UPROPERTY(BlueprintReadOnly,  Category = "Interaction")
	TObjectPtr<UInteractableWidget> InteractionWidgetRef;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FTimerHandle KeyDownTimer;

	UPROPERTY(BlueprintReadWrite, EditInstanceOnly, Category = "Interaction")
	EInteractType InputType = EInteractType::Single ;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	EInteractableResponseType InteractableResponse = EInteractableResponseType::Persistent;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Replicated, Category = "Interaction")
	int32 InteractableValue = 0;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	int32 InteractableTargetValue = 0;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	int32 InteractableLimitValue = 0;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	bool CheckForAssociatedActors = false;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool CanBeReInitialized = true;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	bool RemoveAssociatedInteractablesOnComplete = false;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	FKey PressedInteractionKey;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Moveable Object")
	TObjectPtr<ATargetPoint> DestinationPoint;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Moveable Object")
	float TravelDuration = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Moveable Object")
	float TimelinePosition = 0.0;

	UPROPERTY(BlueprintReadOnly,Category = "Moveable Object")
	TEnumAsByte<ETimelineDirection::Type> TimelineDirection;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Door")
	FString KeyID;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Door")
	bool RemoveItemAfterUnlock = false;
	

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor>  RefInteractor;

protected:
	
	UPROPERTY() bool bHastInteracted = false;
private:
	
	UPROPERTY() bool DoOnce = false;
	UPROPERTY()float MashingProgress = 0.0f;
	UPROPERTY() FTimerHandle MashingDecreaseTimerHandle;
};

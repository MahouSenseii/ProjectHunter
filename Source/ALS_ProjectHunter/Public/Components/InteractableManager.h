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
DECLARE_LOG_CATEGORY_EXTERN(LogInteractableManager, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHoldingChangedDelegate, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMashingChangedDelegate, float, Value);

class UInputAction;
enum class EInteractType : uint8;
enum class EInteractableResponseType : uint8;
class UInteractableWidget;
class ABaseInteractable;
class AALSPlayerController;

/**
 * Handles interactables and manages interaction behavior.
 */

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UInteractableManager : public UActorComponent
{
	GENERATED_BODY()

	/* ============================= */
	/* ========  Functions  ======== */
	/* ============================= */
public:
	// Sets default values for this component's properties
	UInteractableManager();

	void ResetInteractable();

protected:
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	/** Initialize Interactable Manager */
	UFUNCTION(BlueprintCallable)
	void Initialize();

	/* ============================= */
	/* ===== Setup functions  ===== */
	/* ============================= */
	
	UFUNCTION(BlueprintCallable)
	void SetupInteractableReferences(USphereComponent* InInteractableArea, UWidgetComponent* InInteractionWidget, const TSet<UPrimitiveComponent*> InHighlightableObject);
	
	UFUNCTION()
	void SetupHighlightableObjects(TSet<UPrimitiveComponent*> InHighlightableObject);
	
	UFUNCTION(BlueprintCallable)
	void ToggleHighlight(bool Highlight, AActor* Interactor);
	
	UFUNCTION()
	void SetWidgetLocalOwner(APlayerController* OwnerPlayerController);
	
	UFUNCTION()
	void PreInteraction(AActor* Interactor);
	
	UFUNCTION()
	void DurationPress(AActor* Interactor);
	
	UFUNCTION()
	void IsKeyDown();
	
	UFUNCTION()
	void ClearInteractionTimer();

	/* ============================= */
	/* === Input Handling  === */
	/* ============================= */
	
	UFUNCTION()
	static bool GetPressedByAction(const UInputAction* Action, AActor* Interactor, FKey& OutKey);

	UFUNCTION()
	void HoldingInputHandle();
	
	UFUNCTION()
	bool GetKeyTimeDown(float& TimeDown) const;
	
	UFUNCTION()
	void MultiplePress();
	
	UFUNCTION()
	void DelayedStartInteraction();
	
	UFUNCTION()
	bool MashingInput(float& Value);

	/* ============================= */
	/* === Interaction Handling === */
	/* ============================= */

	
	UFUNCTION()
	void Interaction(AActor* Interactor, bool WasHeld);
	
	UFUNCTION()
	void ClientInteraction(AActor* Interactor, bool WasHeld) const;
	
	UFUNCTION()
	void AssociatedActorInteraction(AActor* Interactor);
	
	UFUNCTION()
	void CheckForInteractionWithAssociate(AActor* Interactor);
	
	UFUNCTION()
	bool IsTargetInteractableValue();
	
	UFUNCTION()
	void AssociateActorRemoveHandle(bool IsTemp) const;

	
	/* ============================= */
	/* === End Interaction Handling === */
	/* ============================= */
	
	UFUNCTION()
	void EndInteraction(AActor* Interactor) const;
	
	UFUNCTION()
	void ClientEndInteraction(AActor* Interactor) const;
	
	UFUNCTION()
	void AssociatedActorEndInteraction() const;
	
	UFUNCTION(Reliable, NetMulticast)
	void RemoveInteraction();
	
	UFUNCTION()
	void ClientRemoveInteraction();
	
	UFUNCTION()
	void Re_Initialize();
	
	UFUNCTION()
	void Re_InitializeAssociatedActors();

	/* ============================= */
	/* === Miscellaneous === */
	/* ============================= */
	
	UFUNCTION()
	void ToggleCanBeReInitialized(bool bCond);
	
	UFUNCTION()
	void ToggleIsInteractable(bool bCond);
	
	UFUNCTION()
	void ToggleInteractionWidget(bool Condition) const;
	
	UFUNCTION()
	void ChangeInteractableValue(bool Increment);

	UFUNCTION()
	void RemoveInteractionByResponse();

	UFUNCTION()
	void DecreaseMashingProgress();
	
	UFUNCTION(BlueprintCallable)
	void SetHasInteracted(bool bValue) { DoOnce = bValue; }
	
	UFUNCTION(BlueprintCallable)
	void SetInteractionText(const FText NewText) { InteractionText = NewText; }
	
	UFUNCTION(BlueprintCallable)
	void SetInteractionType(EInteractType InInteractType) { InputType = InInteractType; }
	
	UFUNCTION(BlueprintCallable)
	void SetInteractableResponse(EInteractableResponseType InInteractableResponse) { InteractableResponse = InInteractableResponse; }
	
	UFUNCTION(BlueprintCallable)
	void SetIsDestroyable(bool InResponse) { DestroyAfterInteract = InResponse; }

	/* ============================= */
	/* === * Delegates === */
	/* ============================= */
	
	FOnHoldingChangedDelegate OnUpdateHoldingValue;
	FOnMashingChangedDelegate OnUpdateMashingValue;


	/* ============================= */
	/* ======= Variables =========== */
	/* ============================= */
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MashingAmount = 10;

	UPROPERTY() 
	TSubclassOf<UInteractableWidget> WidgetClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere) 
	UInputAction* ActionToCheckForInteract;

	UPROPERTY(BlueprintReadWrite, Category = "Components")
	USphereComponent* InteractableArea;

	UPROPERTY(BlueprintReadWrite, Category = "Components")
	UWidgetComponent* InteractionWidget;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Replicated, Category = "Main")
	bool IsInteractable = true;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	bool IsInteractableChangeable;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	bool DestroyAfterInteract = false;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Interaction")
	FText InteractionText = FText::FromString(TEXT("Interact"));

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FName InteractableTag = "Interactable";

	UPROPERTY(BlueprintReadWrite, Category = "Interaction")
	TArray<UPrimitiveComponent*> ObjectsToHighlight;

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
	float MaxKeyRetriggerableTime = 0.2f;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UInteractableWidget> InteractionWidgetRef;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FTimerHandle KeyDownTimer;

	UPROPERTY(BlueprintReadWrite, EditInstanceOnly, Category = "Interaction")
	EInteractType InputType = EInteractType::Single;

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
	float TravelDuration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Moveable Object")
	float TimelinePosition = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Moveable Object")
	TEnumAsByte<ETimelineDirection::Type> TimelineDirection;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Door")
	FString KeyID;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Category = "Door")
	bool RemoveItemAfterUnlock = false;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> RefInteractor;

protected:
	UPROPERTY()
	bool bHasInteracted = false;

private:
	UPROPERTY()
	bool DoOnce = false;

	UPROPERTY()
	float MashingProgress = 0.0f;

	UPROPERTY()
	FTimerHandle MashingDecreaseTimerHandle;
};

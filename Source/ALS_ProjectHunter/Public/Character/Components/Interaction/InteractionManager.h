#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/Components/Interaction/InteractionTraceManager.h"
#include "Character/Components/Interaction/InteractionValidatorManager.h"
#include "Character/Components/Interaction/GroundItemPickupManager.h"
#include "Character/Components/Interaction/InteractionDebugManager.h"
#include "Character/Components/Interaction/InteractionWidgetPresenter.h"

#include "Interactable/Library/Structs/InteractionStructs.h"
#include "InteractionManager.generated.h"
class IInteractable;
class UInteractableManager;
class UInteractableWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogInteractionManager, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentInteractableChanged, UInteractableManager*, NewInteractable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGroundItemFocusChanged, int32, GroundItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnGroundItemSelectionChanged,
	int32, GroundItemID,
	int32, SelectionNumber,
	int32, CandidateCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHoldProgressChanged, float, Progress);

// EManagedInteractionMode and FActiveInteraction are shared interaction data types.

// Owns interaction focus and active-interaction state; embedded helpers handle tracing, validation, pickup execution, debug drawing, and presentation.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UInteractionManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionManager();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Interaction|Setup")
	void Initialize();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Setup")
	bool bInteractionEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Managers",
		meta = (ShowOnlyInnerProperties))
	FInteractionTraceManager TraceManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Managers",
		meta = (ShowOnlyInnerProperties))
	FInteractionValidatorManager ValidatorManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Managers",
		meta = (ShowOnlyInnerProperties))
	FGroundItemPickupManager PickupManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Managers",
		meta = (ShowOnlyInnerProperties))
	FInteractionDebugManager DebugManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Presentation",
		meta = (ShowOnlyInnerProperties))
	FInteractionWidgetPresenter WidgetPresenter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Quick Settings")
	bool bDebugEnabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction|State")
	TScriptInterface<IInteractable> CurrentInteractable;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction|State")
	int32 CurrentGroundItemID = INDEX_NONE;

	/** Ranked ground items currently inside the aim bubble. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Ground Items")
	TArray<FGroundItemInteractionCandidate> GroundItemCandidates;

	/** Zero-based index into GroundItemCandidates, or INDEX_NONE. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Ground Items")
	int32 SelectedGroundItemCandidateIndex = INDEX_NONE;

	/** Item the latest aim-scoring pass would select without a manual lock. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Ground Items")
	int32 AutomaticGroundItemID = INDEX_NONE;

	/**
	 * How long a manually cycled item remains selected while it is still valid.
	 * After this expires, normal aim scoring may select a better item again.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Ground Items",
		meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float ManualGroundItemSelectionLockDuration = 0.75f;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnCurrentInteractableChanged OnCurrentInteractableChanged;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnGroundItemFocusChanged OnGroundItemFocusChanged;

	/** Supplies the selected item plus a one-based position suitable for a "2 / 5" UI label. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnGroundItemSelectionChanged OnGroundItemSelectionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnHoldProgressChanged OnHoldProgressChanged;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void OnInteractPressed();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void OnInteractReleased();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void CheckForInteractables();

	/** Select the next (positive) or previous (negative) ranked ground item. */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Ground Items")
	void CycleGroundItemFocus(int32 Direction);

	UFUNCTION(BlueprintPure, Category = "Interaction|Ground Items")
	int32 GetGroundItemCandidateCount() const { return GroundItemCandidates.Num(); }

	/** One-based selected position for UI, or zero when nothing is selected. */
	UFUNCTION(BlueprintPure, Category = "Interaction|Ground Items")
	int32 GetGroundItemSelectionNumber() const
	{
		return GroundItemCandidates.IsValidIndex(SelectedGroundItemCandidateIndex)
			? SelectedGroundItemCandidateIndex + 1
			: 0;
	}

	UFUNCTION(BlueprintPure, Category = "Interaction|Widget")
	UInteractableWidget* GetInteractionWidget() const { return WidgetPresenter.GetHUDWidget(); }

	UFUNCTION(BlueprintCallable, Category = "Interaction|Widget")
	void SetWidgetVisible(bool bVisible) { WidgetPresenter.SetWidgetVisible(bVisible); }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	UInteractableManager* GetCurrentInteractable() const;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	TScriptInterface<IInteractable> GetCurrentInteractableInterface() const
	{
		return HasValidCurrentInteractable() ? CurrentInteractable : TScriptInterface<IInteractable>();
	}

	UFUNCTION(BlueprintPure, Category = "Interaction")
	int32 GetCurrentGroundItemID() const { return CurrentGroundItemID; }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsLocallyControlled() const { return TraceManager.IsLocallyControlled(); }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsSystemInitialized() const { return bSystemInitialized; }

	/**
	 * True while the active interaction requires the button to be held
	 * (Hold, TapOrHold, Continuous). False for Mash and None.
	 */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsHoldingInteraction() const;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	float GetCurrentHoldProgress() const;

	UFUNCTION(BlueprintCallable, Category = "Interaction|Debug")
	void PrintDebugStats() { DebugManager.PrintDebugStats(); }

protected:
	void InitializeInteractionSystem();
	void InitializeSubManagers();
	void InitializeWidget();
	void ApplyQuickSettings();
	void UpdateInteractionCheckRate(bool bHasProximityCandidates);

	void UpdateFocusState(TScriptInterface<IInteractable> NewInteractable);
	void UpdateGroundItemFocus(int32 NewGroundItemID);
	void ApplyGroundItemCandidates(
		const TArray<FGroundItemInteractionCandidate>& NewCandidates,
		int32 NewAutomaticGroundItemID);
	void SelectGroundItemCandidateIndex(int32 NewIndex, bool bManualSelection);
	void AdvanceGroundItemFocusAfterPickup(int32 PickedUpItemID);
	void BroadcastGroundItemSelectionChanged();
	int32 FindGroundItemCandidateIndex(int32 ItemID) const;
	bool IsManualGroundItemSelectionLocked() const;
	float GetManualGroundItemSelectionLockRemaining() const;

	bool InteractWithActor(AActor* TargetActor);
	bool PickupGroundItemToInventory(int32 ItemID);
	bool PickupGroundItemAndEquip(int32 ItemID);

	void BeginGroundTapOrHoldInteraction(int32 GroundItemID);
	void BeginActorHoldInteraction(const TScriptInterface<IInteractable>& Interactable, bool bAllowTapOnRelease);
	void BeginActorContinuousInteraction(const TScriptInterface<IInteractable>& Interactable);
	void BeginOrAdvanceActorMashInteraction(const TScriptInterface<IInteractable>& Interactable);

	void ResetActiveInteractionState();
	void StartHoldPhaseIfNeeded();
	void PushActiveProgress(float NewProgress, bool bForce = false);
	void CompleteActiveHoldInteraction();
	void CancelActiveHoldInteraction(bool bShowCancelledState);
	void EndActorContinuousInteraction();
	void UpdateActiveInteraction(float DeltaTime);

	void RefreshFocusedWidget();

	AActor*  ResolveInteractionActor(const TScriptInterface<IInteractable>& Interactable) const;
	AActor*  ResolveInteractionActor(UObject* InteractableObject) const;
	float    GetRequiredHoldSeconds() const;
	bool     HasActiveInteraction() const;
	bool     HasValidCurrentInteractable() const;
	bool     HasValidActiveInteractable() const;
	UObject* GetCurrentInteractableObject() const;
	UObject* GetActiveInteractableObject() const;
	bool     UsesHoldLifecycle(EManagedInteractionMode Mode) const;
	bool     UsesTapThreshold(EManagedInteractionMode Mode) const;
	bool     UsesActorTarget(EManagedInteractionMode Mode) const;
	bool     ShouldUpdatePromptWidgetFromFocus() const;

	// All gameplay-relevant interaction crosses the network HERE, on the
	// player-owned component - never via Server RPCs on the target actor.
	// (A client calling a Server RPC on an actor it doesn't own - chest,
	// portal - is silently dropped by the engine. Ground pickups always did
	// this correctly; actor interaction now follows the same pattern.)
	//
	// Flow: client executes the interface call locally for presentation
	// (widget states, component BP events), and the server re-validates
	// (distance + LOS via ValidatorManager) then executes the SAME interface
	// call authoritatively. Target actors keep guarding their gameplay with
	// HasAuthority, so the local client execution stays cosmetic.

	UFUNCTION(Server, Reliable, Category = "Interaction|Pickup")
	void Server_PickupToInventory(int32 ItemID, FVector ClientLocation);

	UFUNCTION(Server, Reliable, Category = "Interaction|Pickup")
	void Server_PickupAndEquip(int32 ItemID, FVector ClientLocation);

	UFUNCTION(Server, Reliable, Category = "Interaction|Actor")
	void Server_InteractWithActor(AActor* TargetActor, FVector ClientLocation);

	/**
	 * Hold completed on an actor interactable (OnHoldInteractionComplete).
	 * The hold timing itself is client-driven (cosmetic gating); the server
	 * validates reachability, not the elapsed time.
	 */
	UFUNCTION(Server, Reliable, Category = "Interaction|Actor")
	void Server_NotifyHoldComplete(AActor* TargetActor, FVector ClientLocation);

	UFUNCTION(Server, Reliable, Category = "Interaction|Actor")
	void Server_NotifyMashComplete(AActor* TargetActor, FVector ClientLocation);

private:
	void Server_PickupAndEquip_Implementation(int32 ItemID, FVector ClientLocation);
	bool ValidateServerInteraction(AActor* TargetActor);

	/**
	 * Resolve the object that implements IInteractable on a target actor -
	 * the UInteractableManager component when present (preferred), otherwise
	 * the actor itself if it implements the interface directly.
	 */
	UObject* ResolveInteractableObjectOnActor(AActor* TargetActor) const;

	bool bSystemInitialized = false;

	bool bInteractInputHeld = false;

	/** Weak validation handle for the currently focused interactable UObject. */
	TWeakObjectPtr<UObject> CurrentInteractableObject;

	float LastInteractionCheckTimeSeconds = -1.0f;
	float ManualGroundItemSelectionLockEndTime = -1.0f;
	float CurrentInteractionCheckInterval = -1.0f;

	// All per-interaction transient data lives in ActiveInteraction.
	// Call ActiveInteraction.Reset() to clear every field at once.

	FActiveInteraction ActiveInteraction;

	FTimerHandle InteractionCheckTimer;
};

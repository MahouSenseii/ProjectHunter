// Character/Component/Interaction/InteractionManager.h
// PH-4.5 — Thin facade: focus state, input routing, and active-interaction state machine.
//
// OWNER:    Focus state (CurrentInteractable, CurrentGroundItemID) and
//           active interaction state (ActiveInteraction).
//
// DELEGATES TO:
//   FInteractionTraceManager      — all trace / camera-viewpoint queries
//   FInteractionValidatorManager  — server-side distance / LOS validation
//   FGroundItemPickupManager      — ground item pickup routing
//   FInteractionDebugManager      — debug visualization
//   FInteractionWidgetPresenter   — widget instances, highlight/outline (PH-4.3)
//
// What this manager must NOT do after PH-4.3/4.5:
//   × Create, position, or destroy widget instances or WidgetComponents.
//   × Compute screen-space positions for ground-item widgets.
//   × Write UMaterialInstanceDynamic outline parameters directly.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/Component/Interaction/InteractionTraceManager.h"
#include "Character/Component/Interaction/InteractionValidatorManager.h"
#include "Character/Component/Interaction/GroundItemPickupManager.h"
#include "Character/Component/Interaction/InteractionDebugManager.h"
#include "Character/Component/Interaction/InteractionWidgetPresenter.h"

#include "Interactable/Library/InteractionEnumLibrary.h"
#include "InteractionManager.generated.h"

// Forward declarations
class IInteractable;
class UInteractableManager;
class UInteractableWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogInteractionManager, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentInteractableChanged, UInteractableManager*, NewInteractable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGroundItemFocusChanged, int32, GroundItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHoldProgressChanged, float, Progress);

// EManagedInteractionMode and FActiveInteraction are defined in InteractionEnumLibrary.h.

/**
 * UInteractionManager — Interaction system facade component.
 *
 * Owns focus state and active-interaction state machine.
 * All tracing, validation, pickup routing, widget presentation, and debug
 * visualization are delegated to embedded sub-manager USTRUCTs.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UInteractionManager : public UActorComponent
{
	GENERATED_BODY()

public:

	// ═══════════════════════════════════════════════
	// LIFECYCLE
	// ═══════════════════════════════════════════════

	UInteractionManager();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Interaction|Setup")
	void Initialize();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ═══════════════════════════════════════════════
	// CONFIGURATION
	// ═══════════════════════════════════════════════

	/** Enable / disable the entire interaction system. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Setup")
	bool bInteractionEnabled = true;

	// ── Sub-managers ─────────────────────────────────────────────────────────

	/** Trace Manager — traces for actor interactables and ground items. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Managers",
		meta = (ShowOnlyInnerProperties))
	FInteractionTraceManager TraceManager;

	/** Validator Manager — server-side distance/LOS validation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Managers",
		meta = (ShowOnlyInnerProperties))
	FInteractionValidatorManager ValidatorManager;

	/** Pickup Manager — ground item pickup routing (inventory / equip). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Managers",
		meta = (ShowOnlyInnerProperties))
	FGroundItemPickupManager PickupManager;

	/** Debug Manager — debug visualization and logging. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Managers",
		meta = (ShowOnlyInnerProperties))
	FInteractionDebugManager DebugManager;

	/**
	 * Widget Presenter — owns all widget instances and highlight/outline state.
	 * Configure InteractionWidgetClass, GroundItemWorldWidgetClass, OutlineMaterial,
	 * highlight colors, and other presentation settings here (PH-4.3).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Presentation",
		meta = (ShowOnlyInnerProperties))
	FInteractionWidgetPresenter WidgetPresenter;

	// ── Quick settings ────────────────────────────────────────────────────────

	/** Quick toggle for debug visualization. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Quick Settings")
	bool bDebugEnabled = false;

	// ═══════════════════════════════════════════════
	// STATE
	// ═══════════════════════════════════════════════

	/** Current actor interactable in crosshair. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|State")
	TScriptInterface<IInteractable> CurrentInteractable;

	/** Current ground item ID (INDEX_NONE if none). */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|State")
	int32 CurrentGroundItemID = INDEX_NONE;

	// ═══════════════════════════════════════════════
	// EVENTS
	// ═══════════════════════════════════════════════

	/** Broadcast when CurrentInteractable changes. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnCurrentInteractableChanged OnCurrentInteractableChanged;

	/** Broadcast when the focused ground item ID changes. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnGroundItemFocusChanged OnGroundItemFocusChanged;

	/** Broadcast when hold / mash progress changes. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnHoldProgressChanged OnHoldProgressChanged;

	// ═══════════════════════════════════════════════
	// PRIMARY INTERFACE
	// ═══════════════════════════════════════════════

	/** Called when the interact button is pressed. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void OnInteractPressed();

	/** Called when the interact button is released. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void OnInteractReleased();

	/** Check for interactables (called on timer). */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void CheckForInteractables();

	// ═══════════════════════════════════════════════
	// WIDGET ACCESS (pass-through to WidgetPresenter)
	// ═══════════════════════════════════════════════

	/** Get the screen-space HUD interaction widget (creates if needed). */
	UFUNCTION(BlueprintPure, Category = "Interaction|Widget")
	UInteractableWidget* GetInteractionWidget() const { return WidgetPresenter.GetHUDWidget(); }

	/** Force show / hide the screen-space HUD widget. */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Widget")
	void SetWidgetVisible(bool bVisible) { WidgetPresenter.SetWidgetVisible(bVisible); }

	// ═══════════════════════════════════════════════
	// GETTERS
	// ═══════════════════════════════════════════════

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

	// ═══════════════════════════════════════════════
	// DEBUG
	// ═══════════════════════════════════════════════

	UFUNCTION(BlueprintCallable, Category = "Interaction|Debug")
	void PrintDebugStats() { DebugManager.PrintDebugStats(); }

protected:

	// ═══════════════════════════════════════════════
	// INITIALIZATION
	// ═══════════════════════════════════════════════

	void InitializeInteractionSystem();
	void InitializeSubManagers();
	void InitializeWidget();
	void ApplyQuickSettings();

	// ═══════════════════════════════════════════════
	// FOCUS MANAGEMENT
	// ═══════════════════════════════════════════════

	void UpdateFocusState(TScriptInterface<IInteractable> NewInteractable);
	void UpdateGroundItemFocus(int32 NewGroundItemID);

	// ═══════════════════════════════════════════════
	// INTERACTION ROUTING (actor / pickup)
	// ═══════════════════════════════════════════════

	bool InteractWithActor(AActor* TargetActor);
	bool PickupGroundItemToInventory(int32 ItemID);
	bool PickupGroundItemAndEquip(int32 ItemID);

	// ═══════════════════════════════════════════════
	// ACTIVE INTERACTION STATE MACHINE
	// ═══════════════════════════════════════════════

	void BeginGroundTapOrHoldInteraction(int32 GroundItemID);
	void BeginActorHoldInteraction(const TScriptInterface<IInteractable>& Interactable, bool bAllowTapOnRelease);
	void BeginActorContinuousInteraction(const TScriptInterface<IInteractable>& Interactable);
	void BeginOrAdvanceActorMashInteraction(const TScriptInterface<IInteractable>& Interactable);

	void ResetActiveInteractionState(bool bForceCancelHoldOnPickupManager = false);
	void StartHoldPhaseIfNeeded();
	void PushActiveProgress(float NewProgress, bool bForce = false);
	void CompleteActiveHoldInteraction();
	void CancelActiveHoldInteraction(bool bShowCancelledState);
	void EndActorContinuousInteraction();
	void UpdateActiveInteraction(float DeltaTime);

	void RefreshFocusedWidget();

	// ── State machine helpers ─────────────────────────────────────────────────

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

	// ═══════════════════════════════════════════════
	// SERVER RPCs
	// ═══════════════════════════════════════════════

	UFUNCTION(Server, Reliable, Category = "Interaction|Pickup")
	void Server_PickupToInventory(int32 ItemID, FVector ClientLocation);

	UFUNCTION(Server, Reliable, Category = "Interaction|Pickup")
	void Server_PickupAndEquip(int32 ItemID, FVector ClientLocation);

private:
	// ═══════════════════════════════════════════════
	// SYSTEM FLAGS
	// ═══════════════════════════════════════════════

	/** Guard flag to prevent double initialization. */
	bool bSystemInitialized = false;

	/** Is interact input currently held down. */
	bool bInteractInputHeld = false;

	// ═══════════════════════════════════════════════
	// FOCUS TRACKING
	// ═══════════════════════════════════════════════

	/** Weak validation handle for the currently focused interactable UObject. */
	TWeakObjectPtr<UObject> CurrentInteractableObject;

	/** Last time a focus trace was performed (world time seconds). */
	float LastInteractionCheckTimeSeconds = -1.0f;

	// ═══════════════════════════════════════════════
	// ACTIVE INTERACTION STATE
	// All per-interaction transient data lives in ActiveInteraction.
	// Call ActiveInteraction.Reset() to clear every field at once.
	// ═══════════════════════════════════════════════

	FActiveInteraction ActiveInteraction;

	// ═══════════════════════════════════════════════
	// TIMERS
	// ═══════════════════════════════════════════════

	FTimerHandle InteractionCheckTimer;
};

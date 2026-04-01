// Character/Component/Interaction/InteractionManager.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/Component/Interaction/InteractionTraceManager.h"
#include "Character/Component/Interaction/InteractionValidatorManager.h"
#include "Character/Component/Interaction/GroundItemPickupManager.h"
#include "Character/Component/Interaction/InteractionDebugManager.h"

#include "Interactable/Library/InteractionEnumLibrary.h"
#include "InteractionManager.generated.h"

struct FInteractableHighlightStyle;
// Forward declarations
class IInteractable;
class UInteractableManager;
class UInteractableWidget;
class UItemInstance;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class APostProcessVolume;

DECLARE_LOG_CATEGORY_EXTERN(LogInteractionManager, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentInteractableChanged, UInteractableManager*, NewInteractable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGroundItemFocusChanged, int32, GroundItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHoldProgressChanged, float, Progress);

// EManagedInteractionMode is defined in InteractionEnumLibrary.h (alongside FActiveInteraction).

/**
 * Interaction Manager Component
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UInteractionManager : public UActorComponent
{
	GENERATED_BODY()

public:
	
	// ─────────────────────────────────────────────────────────────
	// HIGHLIGHT / OUTLINE
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight")
	TObjectPtr<UMaterialInterface> OutlineMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight")
	TObjectPtr<APostProcessVolume> TargetPostProcessVolume = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight")
	FLinearColor PlayerHighlightColor = FLinearColor::Yellow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float PlayerHighlightWidth = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight", meta = (ClampMin = "0.0", ClampMax = "50.0"))
	float PlayerHighlightThreshold = 50.0f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OutlineMID = nullptr;


	
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

	/** Enable interaction system */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Setup")
	bool bInteractionEnabled = true;

	/** Trace Manager - Handles tracing for interactables */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Managers", meta = (ShowOnlyInnerProperties))
	FInteractionTraceManager TraceManager;

	/** Validator Manager - Validates interactions server-side */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Managers", meta = (ShowOnlyInnerProperties))
	FInteractionValidatorManager ValidatorManager;

	/** Pickup Manager - Handles ground item pickup */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Managers", meta = (ShowOnlyInnerProperties))
	FGroundItemPickupManager PickupManager;

	/** Debug Manager - Handles debug visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Managers", meta = (ShowOnlyInnerProperties))
	FInteractionDebugManager DebugManager;

	// ═══════════════════════════════════════════════
	// WIDGET CONFIGURATION
	// ═══════════════════════════════════════════════

	/** Widget class to spawn for interaction prompts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Widget")
	TSubclassOf<UInteractableWidget> InteractionWidgetClass;

	/** Z-Order for the widget (higher = on top) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Widget")
	int32 WidgetZOrder = 10;

	/** Default action name for ground items */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Widget")
	UInputAction* GroundItemActionInput;

	/** Default text for ground item pickup */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Widget")
	FText GroundItemDefaultText = FText::FromString(TEXT("Pick Up"));

	/** Text format for ground items with name (use {0} for item name) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Widget")
	FText GroundItemNameFormat = FText::FromString(TEXT("Pick Up {0}"));

	// ═══════════════════════════════════════════════
	// QUICK SETTINGS
	// ═══════════════════════════════════════════════

	/** Quick toggle for debug visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Quick Settings")
	bool bDebugEnabled = false;

	// ═══════════════════════════════════════════════
	// STATE
	// ═══════════════════════════════════════════════

	/** Current interactable being looked at */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|State")
	TScriptInterface<IInteractable> CurrentInteractable;

	/** Current ground item ID (INDEX_NONE if none) */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|State")
	int32 CurrentGroundItemID = INDEX_NONE;

	// ═══════════════════════════════════════════════
	// EVENTS
	// ═══════════════════════════════════════════════

	/** Called when CurrentInteractable changes */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnCurrentInteractableChanged OnCurrentInteractableChanged;

	/** Called when focused ground item changes */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnGroundItemFocusChanged OnGroundItemFocusChanged;

	/** Called when hold progress changes */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnHoldProgressChanged OnHoldProgressChanged;

	// ═══════════════════════════════════════════════
	// PRIMARY INTERFACE
	// ═══════════════════════════════════════════════

	/** Called when interact button is pressed */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void OnInteractPressed();

	/** Called when interact button is released */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void OnInteractReleased();

	/** Check for interactables (called on timer) */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void CheckForInteractables();

	// ═══════════════════════════════════════════════
	// WIDGET ACCESS
	// ═══════════════════════════════════════════════

	/** Get the interaction widget (creates if needed) */
	UFUNCTION(BlueprintPure, Category = "Interaction|Widget")
	UInteractableWidget* GetInteractionWidget() const { return InteractionWidget; }

	/** Force show/hide widget */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Widget")
	void SetWidgetVisible(bool bVisible);

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
	 * True while the active interaction is one that requires the button to be held
	 * (Hold, TapOrHold, Continuous). False for Mash (press-based) and None.
	 * Derived directly from ActiveInteraction.Mode — no separate flag needed.
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
	// OUTLINE
	// ═══════════════════════════════════════════════
	
	bool InitializeOutlineMID();
	void ApplyHighlightStyle(const FInteractableHighlightStyle& Style);
	void ResetHighlightStyle();
	
	// ═══════════════════════════════════════════════
	// INITIALIZATION
	// ═══════════════════════════════════════════════

	void InitializeInteractionSystem();
	void InitializeSubManagers();
	void InitializeWidget();
	void ApplyQuickSettings();

	// ═══════════════════════════════════════════════
	// INTERNAL LOGIC
	// ═══════════════════════════════════════════════

	bool InteractWithActor(AActor* TargetActor);
	bool PickupGroundItemToInventory(int32 ItemID);
	bool PickupGroundItemAndEquip(int32 ItemID);
	void UpdateFocusState(TScriptInterface<IInteractable> NewInteractable);
	void UpdateGroundItemFocus(int32 NewGroundItemID);
	void UpdateActiveInteraction(float DeltaTime);
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
	void RefreshFocusedWidget();
	AActor* ResolveInteractionActor(const TScriptInterface<IInteractable>& Interactable) const;
	AActor* ResolveInteractionActor(UObject* InteractableObject) const;
	float GetRequiredHoldSeconds() const;
	bool HasActiveInteraction() const;
	bool HasValidCurrentInteractable() const;
	bool HasValidActiveInteractable() const;
	UObject* GetCurrentInteractableObject() const;
	UObject* GetActiveInteractableObject() const;
	bool UsesHoldLifecycle(EManagedInteractionMode Mode) const;
	bool UsesTapThreshold(EManagedInteractionMode Mode) const;
	bool UsesActorTarget(EManagedInteractionMode Mode) const;
	bool ShouldUpdatePromptWidgetFromFocus() const;

	// ═══════════════════════════════════════════════
	// WIDGET MANAGEMENT
	// ═══════════════════════════════════════════════

	/** Update widget for actor interactable focus */
	void UpdateWidgetForActorInteractable(const TScriptInterface<IInteractable>& Interactable);

	/** Update widget for ground item focus */
	void UpdateWidgetForGroundItem(int32 GroundItemID);

	/** Hide widget (no focus) */
	void HideWidget();

	/** Set widget to holding state with progress */
	void SetWidgetHoldingState(float Progress);

	/** Set widget to completed state */
	void SetWidgetCompletedState();

	/** Set widget to cancelled state */
	void SetWidgetCancelledState();

	/** Get item instance for ground item ID */
	UItemInstance* GetGroundItemInstance(int32 GroundItemID) const;

	// ═══════════════════════════════════════════════
	// SERVER RPCs  (multiplayer foundation)
	// ═══════════════════════════════════════════════

	/**
	 * Ask the server to pick up a ground item into inventory.
	 * Called from clients; the server validates proximity and executes.
	 */
	UFUNCTION(Server, Reliable, Category = "Interaction|Pickup")
	void Server_PickupToInventory(int32 ItemID, FVector ClientLocation);

	/**
	 * Ask the server to pick up a ground item and equip it.
	 */
	UFUNCTION(Server, Reliable, Category = "Interaction|Pickup")
	void Server_PickupAndEquip(int32 ItemID, FVector ClientLocation);

private:
	// ═══════════════════════════════════════════════
	// WIDGET INSTANCE
	// ═══════════════════════════════════════════════

	/** The screen-space interaction widget instance */
	UPROPERTY()
	TObjectPtr<UInteractableWidget> InteractionWidget;

	// ═══════════════════════════════════════════════
	// SYSTEM FLAGS
	// ═══════════════════════════════════════════════

	/** Guard flag to prevent double initialization */
	bool bSystemInitialized = false;

	/** Is interact input currently held down */
	bool bInteractInputHeld = false;

	/**
	 * Set to true after a failed TActorIterator<APostProcessVolume> scan so we
	 * don't repeat the expensive full-level iteration every time ApplyHighlightStyle
	 * or ResetHighlightStyle is called without a volume present in the level.
	 */
	bool bPostProcessSearchFailed = false;

	// ═══════════════════════════════════════════════
	// FOCUS TRACKING (current interactable in crosshair)
	// ═══════════════════════════════════════════════

	/** Weak validation handle for the currently focused interactable UObject */
	TWeakObjectPtr<UObject> CurrentInteractableObject;

	/** Last time a focus trace was performed (world time seconds) */
	float LastInteractionCheckTimeSeconds = -1.0f;

	// ═══════════════════════════════════════════════
	// ACTIVE INTERACTION STATE
	// All per-interaction transient data lives here.
	// Call ActiveInteraction.Reset() to clear every field at once.
	// ═══════════════════════════════════════════════

	FActiveInteraction ActiveInteraction;

	// ═══════════════════════════════════════════════
	// TIMERS
	// ═══════════════════════════════════════════════

	FTimerHandle InteractionCheckTimer;
};

// Character/Component/Interaction/InteractionWidgetPresenter.h
// PH-4.3 — Owns all widget state and presentation logic for UInteractionManager.
//
// OWNER:    Widget instance lifecycle (screen HUD + world-space ground item widget),
//           post-process outline/highlight for focused interactables.
//           Follows the embedded-USTRUCT sub-manager pattern established in this codebase.
//
// What InteractionManager must NOT do after PH-4.3:
//   × Create, position, or destroy UInteractableWidget or UWidgetComponent instances.
//   × Read or write UMaterialInstanceDynamic outline parameters directly.
//   × Compute screen-space positions for ground-item widgets.
//
// Usage:
//   1. Declare as UPROPERTY(EditAnywhere, ..., meta=(ShowOnlyInnerProperties)) on UInteractionManager.
//   2. Call Initialize(this, GetWorld()) from UInteractionManager::InitializeWidget().
//   3. Delegate all widget/highlight calls to this presenter.

#pragma once

#include "CoreMinimal.h"
#include "Interactable/Library/InteractionEnumLibrary.h"
#include "InteractionWidgetPresenter.generated.h"

// Forward declarations
class UInteractableWidget;
class UWidgetComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class APostProcessVolume;
class UInputAction;
class UActorComponent;
class UItemInstance;
struct FInteractableHighlightStyle;
class IInteractable;

DECLARE_LOG_CATEGORY_EXTERN(LogInteractionWidgetPresenter, Log, All);

/**
 * FInteractionWidgetPresenter
 *
 * Embedded sub-manager that owns all widget and outline/highlight presentation
 * for UInteractionManager. Stores the screen-space HUD widget (hold-progress,
 * cancellation feedback, key prompt), the world-space floating widget above
 * ground items, and the post-process outline MID used for focus highlights.
 *
 * Call Initialize() once from UInteractionManager::InitializeWidget() on the
 * locally-controlled pawn, after the PlayerController is available.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FInteractionWidgetPresenter
{
	GENERATED_BODY()

public:
	FInteractionWidgetPresenter();

	// ═══════════════════════════════════════════════
	// WIDGET CONFIGURATION (Blueprint-editable via outer component)
	// ═══════════════════════════════════════════════

	/** Widget class for the screen-space HUD overlay (hold-progress, cancellation, key prompt). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	TSubclassOf<UInteractableWidget> InteractionWidgetClass;

	/**
	 * Widget class for the world-space floating prompt above ground items.
	 * Falls back to InteractionWidgetClass when unset.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	TSubclassOf<UInteractableWidget> GroundItemWorldWidgetClass;

	/** Z-Order for the screen-space HUD widget (higher = on top of other HUD). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	int32 WidgetZOrder = 10;

	/** Input action shown on the ground item pickup prompt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	TObjectPtr<UInputAction> GroundItemActionInput;

	/** Fallback display text when no item name is available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	FText GroundItemDefaultText = FText::FromString(TEXT("Pick Up"));

	/** Format string for the pickup prompt ({0} = item display name). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	FText GroundItemNameFormat = FText::FromString(TEXT("Pick Up {0}"));

	/**
	 * Height offset (cm) added above the ground item's base location.
	 * Controls how high the floating widget appears above the mesh.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	float GroundItemWidgetHeightOffset = 120.0f;

	/**
	 * Physical draw size of the world-space widget that floats above ground items
	 * (mirrors InteractableManager's WidgetDrawSize). Width × Height in cm.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	FVector2D GroundItemWidgetDrawSize = FVector2D(300.0f, 80.0f);

	// ═══════════════════════════════════════════════
	// HIGHLIGHT / OUTLINE CONFIGURATION
	// ═══════════════════════════════════════════════

	/** Post-process material for the focus outline effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	TObjectPtr<UMaterialInterface> OutlineMaterial;

	/**
	 * Optional explicit post-process volume target.
	 * Auto-discovered via TActorIterator when null (cached on first use).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	TObjectPtr<APostProcessVolume> TargetPostProcessVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	FLinearColor PlayerHighlightColor = FLinearColor::Yellow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight",
		meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float PlayerHighlightWidth = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight",
		meta = (ClampMin = "0.0", ClampMax = "50.0"))
	float PlayerHighlightThreshold = 50.0f;

	// ═══════════════════════════════════════════════
	// INITIALIZATION
	// ═══════════════════════════════════════════════

	/**
	 * Create widget instances and register the world-space widget component.
	 * Must be called once on the locally-controlled pawn after the PlayerController
	 * is available (called from UInteractionManager::InitializeWidget()).
	 *
	 * @param InOwnerComponent  The owning UInteractionManager (used to resolve actor/PC).
	 * @param InWorld           The world context.
	 */
	void Initialize(UActorComponent* InOwnerComponent, UWorld* InWorld);

	// ═══════════════════════════════════════════════
	// WIDGET UPDATE
	// ═══════════════════════════════════════════════

	/** Update the HUD widget for the currently focused actor interactable. */
	void UpdateForActorInteractable(const TScriptInterface<IInteractable>& Interactable);

	/** Update both the HUD and the world-space widget for a focused ground item. */
	void UpdateForGroundItem(int32 GroundItemID);

	/**
	 * Project the ground item's world position to screen space and
	 * reposition the HUD widget so it floats above the item.
	 * Safe to call every frame; no-ops when the item is behind the camera.
	 */
	void PositionWidgetAtGroundItem(int32 GroundItemID);

	/** Show and position the world-space floating widget above the given ground item. */
	void ShowGroundItemWorldWidget(int32 GroundItemID);

	/** Hide the world-space floating widget (called when ground item focus ends). */
	void HideGroundItemWorldWidget();

	/**
	 * Update the world-space widget's location and camera-facing rotation.
	 * Called from TickComponent while a ground item is focused.
	 */
	void TickGroundItemWorldWidget(int32 GroundItemID);

	/** Hide all widgets (no interactable or ground item in focus). */
	void HideAll();

	/** Show or hide the screen-space HUD widget. */
	void SetWidgetVisible(bool bVisible);

	/**
	 * Drive the HUD + world-space widget to the holding/mash progress state.
	 * @param Progress  Normalized hold progress [0..1].
	 * @param Mode      Active interaction mode (determines IWS_Holding vs IWS_Mashing).
	 */
	void SetHoldingState(float Progress, EManagedInteractionMode Mode);

	/** Drive both widgets to the completed state. */
	void SetCompletedState();

	/** Drive both widgets to the cancelled state. */
	void SetCancelledState();

	// ═══════════════════════════════════════════════
	// OUTLINE / HIGHLIGHT
	// ═══════════════════════════════════════════════

	/** Lazily create and register the OutlineMID on the PostProcessVolume. */
	bool InitializeOutlineMID();

	/** Push the given highlight style parameters to the OutlineMID. */
	void ApplyHighlightStyle(const FInteractableHighlightStyle& Style);

	/** Reset outline parameters to the player's default highlight settings. */
	void ResetHighlightStyle();

	// ═══════════════════════════════════════════════
	// ACCESSORS
	// ═══════════════════════════════════════════════

	/** Return the screen-space HUD widget (may be nullptr if not initialized). */
	UInteractableWidget* GetHUDWidget() const { return InteractionWidget; }

	/** True if the HUD widget exists and is currently visible. */
	bool IsHUDWidgetShown() const;

	// ═══════════════════════════════════════════════
	// CLEANUP
	// ═══════════════════════════════════════════════

	/** Remove the HUD widget from viewport and destroy the world-space widget component. */
	void Shutdown();

private:
	// ─────────────────────────────────────────────
	// WIDGET INSTANCES (GC-tracked via outer UPROPERTY on UInteractionManager)
	// ─────────────────────────────────────────────

	/** Screen-space HUD overlay — hold progress, cancellation feedback, key prompt. */
	UPROPERTY()
	TObjectPtr<UInteractableWidget> InteractionWidget;

	/** World-space UWidgetComponent floating above the focused ground item. */
	UPROPERTY()
	TObjectPtr<UWidgetComponent> GroundItemWorldWidget;

	/** Dynamic material instance for the post-process outline effect. */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> OutlineMID;

	// ─────────────────────────────────────────────
	// CACHED CONTEXT (raw pointers — valid for lifetime of owner component)
	// ─────────────────────────────────────────────

	UActorComponent* OwnerComponent = nullptr;
	UWorld*          WorldContext   = nullptr;

	/**
	 * Set to true after a failed TActorIterator<APostProcessVolume> scan so we
	 * don't repeat the expensive full-level iteration every time outline methods
	 * are called without a volume present in the level.
	 */
	bool bPostProcessSearchFailed = false;

	// ─────────────────────────────────────────────
	// INTERNAL HELPERS
	// ─────────────────────────────────────────────

	/** Resolve the PlayerController from the owning pawn. May return nullptr. */
	APlayerController* GetOwnerPlayerController() const;

	/** Resolve a UItemInstance from the GroundItemSubsystem for display-name formatting. */
	UItemInstance* GetGroundItemInstance(int32 GroundItemID) const;
};

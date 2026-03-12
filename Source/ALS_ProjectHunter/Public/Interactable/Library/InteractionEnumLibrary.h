// Interactable/Library/InteractionEnumLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "InteractionEnumLibrary.generated.h"

// ═══════════════════════════════════════════════════════════════════════
// LEGACY ENUMS (For backwards compatibility)
// ═══════════════════════════════════════════════════════════════════════

/**
 * Legacy Interact Type (kept for backwards compatibility with existing code)
 * Used by CheckInputType() in HunterController
 */
UENUM(BlueprintType)
enum class EInteractType : uint8
{
	Single      UMETA(DisplayName = "Single"),
	Holding     UMETA(DisplayName = "Holding"),
	Mashing     UMETA(DisplayName = "Mashing")
};

// ═══════════════════════════════════════════════════════════════════════
// INTERACTION ENUMS - Single Responsibility: Define Interaction Types
// ═══════════════════════════════════════════════════════════════════════

/**
 * Interaction Type
 * Defines how the interaction is performed
 */
UENUM(BlueprintType)
enum class EInteractionType : uint8
{
	/** Single press interaction (immediate) */
	IT_Tap UMETA(DisplayName = "Tap"),
	
	/** Hold button to complete (shows progress bar) */
	IT_Hold UMETA(DisplayName = "Hold"),
	
	/** Rapidly press button (gate wheel, struggle) */
	IT_Mash UMETA(DisplayName = "Mash"),
	
	/** Both tap AND hold (ground items: tap=inventory, hold=equip) */
	IT_TapOrHold UMETA(DisplayName = "Tap or Hold"),
	
	/** Toggle interaction (switches, levers) */
	IT_Toggle UMETA(DisplayName = "Toggle"),
	
	/** Continuous hold (channeling, reading) */
	IT_Continuous UMETA(DisplayName = "Continuous"),
	
	/** Disabled (can't interact) */
	IT_None UMETA(DisplayName = "None")
};

/**
 * Widget Space Type
 * Defines where the interaction widget is rendered
 */
UENUM(BlueprintType)
enum class EInteractionWidgetSpace : uint8
{
	IWS_World       UMETA(DisplayName = "World Space"),    // 3D world widget
	IWS_Screen      UMETA(DisplayName = "Screen Space"),   // 2D HUD overlay
	IWS_Component   UMETA(DisplayName = "Component Space") // Attached to component (future)
};

/**
 * Interaction State - Current state of an ongoing interaction
 */
UENUM(BlueprintType)
enum class EInteractionState : uint8
{
	/** Not interacting */
	IS_Idle UMETA(DisplayName = "Idle"),
	
	/** Interaction started */
	IS_Started UMETA(DisplayName = "Started"),
	
	/** In progress (hold/mash) */
	IS_InProgress UMETA(DisplayName = "In Progress"),
	
	/** Successfully completed */
	IS_Completed UMETA(DisplayName = "Completed"),
	
	/** Cancelled by player */
	IS_Cancelled UMETA(DisplayName = "Cancelled"),
	
	/** Failed (mash decay, interrupted) */
	IS_Failed UMETA(DisplayName = "Failed")
};

/**
 * Input Device Type
 * For dynamic icon switching
 */
UENUM(BlueprintType)
enum class EInputDeviceType : uint8
{
	IDT_Keyboard    UMETA(DisplayName = "Keyboard & Mouse"),
	IDT_Gamepad     UMETA(DisplayName = "Gamepad"),
	IDT_Touch       UMETA(DisplayName = "Touch") // Future mobile support
};

/**
 * Progress Bar Color Mode
 * Visual feedback for different interaction states
 */
UENUM(BlueprintType)
enum class EProgressColorMode : uint8
{
	PCM_Filling     UMETA(DisplayName = "Filling"),     // Normal progress (green/cyan)
	PCM_Depleting   UMETA(DisplayName = "Depleting"),   // Decaying progress (orange/red)
	PCM_Warning     UMETA(DisplayName = "Warning"),     // Nearly failed (red)
	PCM_Success     UMETA(DisplayName = "Success"),     // Completed (bright green)
	PCM_Disabled    UMETA(DisplayName = "Disabled")     // Cannot interact (gray)
};

/**
 * Highlight Type
 * Visual feedback when interactable is focused
 */
UENUM(BlueprintType)
enum class EInteractableHighlightType : uint8
{
	IHT_None            UMETA(DisplayName = "None"),
	IHT_CustomDepth     UMETA(DisplayName = "Custom Depth Stencil"),
	IHT_Outline         UMETA(DisplayName = "Outline Effect"),
	IHT_Emission        UMETA(DisplayName = "Emission Glow"),
	IHT_Material        UMETA(DisplayName = "Material Swap"),
	IHT_Overlay         UMETA(DisplayName = "Overlay Effect")
};

/**
 * Widget Anchor Position
 * Where widget appears relative to interactable
 */
UENUM(BlueprintType)
enum class EWidgetAnchor : uint8
{
	WA_Top          UMETA(DisplayName = "Top"),
	WA_Bottom       UMETA(DisplayName = "Bottom"),
	WA_Center       UMETA(DisplayName = "Center"),
	WA_Custom       UMETA(DisplayName = "Custom Offset")
};

/**
 * Validation Result
 * Server-side interaction validation
 */
UENUM(BlueprintType)
enum class EInteractionValidation : uint8
{
	IV_Valid            UMETA(DisplayName = "Valid"),
	IV_TooFar           UMETA(DisplayName = "Too Far"),
	IV_Obstructed       UMETA(DisplayName = "Obstructed"),
	IV_Disabled         UMETA(DisplayName = "Disabled"),
	IV_OnCooldown       UMETA(DisplayName = "On Cooldown"),
	IV_RequirementFailed UMETA(DisplayName = "Requirement Failed"),
	IV_InvalidTarget    UMETA(DisplayName = "Invalid Target")
};

/**
 * Interaction Result - What happened when player tried to interact
 */
UENUM(BlueprintType)
enum class EInteractionResult : uint8
{
	/** Interaction succeeded */
	IR_Success UMETA(DisplayName = "Success"),
	
	/** Can't interact right now */
	IR_CannotInteract UMETA(DisplayName = "Cannot Interact"),
	
	/** Wrong interaction type (tried to tap a hold interaction) */
	IR_WrongType UMETA(DisplayName = "Wrong Type"),
	
	/** Inventory full (for pickups) */
	IR_InventoryFull UMETA(DisplayName = "Inventory Full"),
	
	/** Requirements not met */
	IR_RequirementsNotMet UMETA(DisplayName = "Requirements Not Met"),
	
	/** Too far away */
	IR_TooFar UMETA(DisplayName = "Too Far"),
	
	/** Generic failure */
	IR_Failed UMETA(DisplayName = "Failed")
};

// ═══════════════════════════════════════════════════════════════════════
// STRUCTS
// ═══════════════════════════════════════════════════════════════════════

/**
 * Interaction Configuration - Settings for each interaction type
 */
USTRUCT(BlueprintType)
struct FInteractionConfig
{
	GENERATED_BODY()

	/** Which interaction type this uses */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	EInteractionType InteractionType = EInteractionType::IT_Tap;

	// ═══════════════════════════════════════════════
	// INPUT ACTION (REFACTORED)
	// ═══════════════════════════════════════════════
	
	/**
	 * Input Action for this interaction
	 * Widget will query this to get currently bound key
	 * Example: IA_Interact, IA_PickUp, IA_Open, etc.
	 * 
	 * REPLACES: FName ActionName
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	TObjectPtr<UInputAction> InputAction = nullptr;

	// ═══════════════════════════════════════════════
	// HOLD INTERACTION SETTINGS
	// ═══════════════════════════════════════════════
	
	/** Threshold time to distinguish tap from hold (seconds) - Time before hold starts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Hold", 
		meta = (EditCondition = "InteractionType == EInteractionType::IT_Hold || InteractionType == EInteractionType::IT_TapOrHold", ClampMin = "0.1", ClampMax = "1.0"))
	float TapHoldThreshold = 0.3f;

	/** How long to hold after threshold (seconds) - Total hold time = Threshold + Duration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Hold", 
		meta = (EditCondition = "InteractionType == EInteractionType::IT_Hold || InteractionType == EInteractionType::IT_TapOrHold", ClampMin = "0.1"))
	float HoldDuration = 1.0f;

	/** Can cancel hold by releasing button? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Hold",
		meta = (EditCondition = "InteractionType == EInteractionType::IT_Hold || InteractionType == EInteractionType::IT_TapOrHold"))
	bool bCanCancelHold = true;

	/** Text to show during hold */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Hold",
		meta = (EditCondition = "InteractionType == EInteractionType::IT_Hold || InteractionType == EInteractionType::IT_TapOrHold"))
	FText HoldText = FText::FromString("Hold to Interact");

	// ═══════════════════════════════════════════════
	// MASH INTERACTION SETTINGS
	// ═══════════════════════════════════════════════
	
	/** How many button presses required */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Mash",
		meta = (EditCondition = "InteractionType == EInteractionType::IT_Mash", ClampMin = "1"))
	int32 RequiredMashCount = 10;

	/** Progress decay per second when not mashing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Mash",
		meta = (EditCondition = "InteractionType == EInteractionType::IT_Mash", ClampMin = "0.0"))
	float MashDecayRate = 2.0f;

	/** Text to show during mashing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Mash",
		meta = (EditCondition = "InteractionType == EInteractionType::IT_Mash"))
	FText MashText = FText::FromString("Mash to Open!");

	// ═══════════════════════════════════════════════
	// TAP OR HOLD SETTINGS (Ground Items)
	// ═══════════════════════════════════════════════
	
	/** Text for tap action (e.g., "Tap: Add to Inventory") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|TapOrHold",
		meta = (EditCondition = "InteractionType == EInteractionType::IT_TapOrHold"))
	FText TapText = FText::FromString("Tap: Pickup");

	/** Text for hold action (e.g., "Hold: Equip") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|TapOrHold",
		meta = (EditCondition = "InteractionType == EInteractionType::IT_TapOrHold"))
	FText HoldActionText = FText::FromString("Hold: Equip");

	// ═══════════════════════════════════════════════
	// GENERAL SETTINGS
	// ═══════════════════════════════════════════════
	
	/** Display text for basic interactions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText InteractionText = FText::FromString("Press To Interact");

	/** Can currently interact? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bCanInteract = true;

	FInteractionConfig() = default;
};
/**
 * Active Interaction Data - Runtime state for ongoing interaction
 */
USTRUCT(BlueprintType)
struct FActiveInteraction
{
	GENERATED_BODY()

	/** Current interaction state */
	UPROPERTY(BlueprintReadOnly)
	EInteractionState State = EInteractionState::IS_Idle;

	/** Current interaction type being performed */
	UPROPERTY(BlueprintReadOnly)
	EInteractionType Type = EInteractionType::IT_None;

	/** Who is interacting */
	UPROPERTY(BlueprintReadOnly)
	AActor* Interactor = nullptr;

	/** What is being interacted with */
	UPROPERTY(BlueprintReadOnly)
	TScriptInterface<class IInteractable> Target;

	/** Elapsed time (for hold) */
	UPROPERTY(BlueprintReadOnly)
	float ElapsedTime = 0.0f;

	/** Current progress [0.0 - 1.0] */
	UPROPERTY(BlueprintReadOnly)
	float Progress = 0.0f;

	/** Current mash count */
	UPROPERTY(BlueprintReadOnly)
	int32 MashCount = 0;

	/** Last mash time (for decay) */
	UPROPERTY(BlueprintReadOnly)
	float LastMashTime = 0.0f;

	FActiveInteraction() = default;

	/** Is interaction active? */
	bool IsActive() const
	{
		return State == EInteractionState::IS_Started || State == EInteractionState::IS_InProgress;
	}

	/** Reset to idle */
	void Reset()
	{
		State = EInteractionState::IS_Idle;
		Type = EInteractionType::IT_None;
		Interactor = nullptr;
		Target = nullptr;
		ElapsedTime = 0.0f;
		Progress = 0.0f;
		MashCount = 0;
		LastMashTime = 0.0f;
	}
};

/**
 * DoOnce State - Helper for DoOnce pattern
 * Used to ensure code executes only once until reset
 */
USTRUCT(BlueprintType)
struct FDoOnceState
{
	GENERATED_BODY()

	/** Has been initialized? */
	bool bHasBeenInitialized = false;

	/** Is gate closed? */
	bool bIsClosed = false;

	FDoOnceState() = default;
};
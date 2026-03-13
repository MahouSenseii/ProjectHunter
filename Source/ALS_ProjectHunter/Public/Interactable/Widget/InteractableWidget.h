// Interactable/Widget/InteractableWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputAction.h"
#include "InteractableWidget.generated.h"

class UImage;
class UTextBlock;
class UOverlay;
class UMaterialInstanceDynamic;
class UInputAction;
class UEnhancedInputLocalPlayerSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogInteractableWidget, Log, All);

/**
 * EInteractionWidgetState - Widget display states
 */
UENUM(BlueprintType)
enum class EInteractionWidgetState : uint8
{
	IWS_Idle        UMETA(DisplayName = "Idle"),           // Tap interaction - animated border
	IWS_Holding     UMETA(DisplayName = "Holding"),        // Hold interaction - progress fills
	IWS_Mashing     UMETA(DisplayName = "Mashing"),        // Mash interaction - progress fills
	IWS_Completed   UMETA(DisplayName = "Completed"),      // Action completed - flash effect
	IWS_Cancelled   UMETA(DisplayName = "Cancelled")       // Action cancelled - deplete effect
};

/**
 * UInteractableWidget - Universal interaction prompt widget
 */
UCLASS()
class ALS_PROJECTHUNTER_API UInteractableWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ═══════════════════════════════════════════════
	// WIDGET BINDINGS (Set in Blueprint)
	// ═══════════════════════════════════════════════
	
	/** Background image (the inner square/circle area) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UImage> Img_Background;

	/** 
	 * Fill border image - THIS IS THE PROGRESS INDICATOR
	 * Uses material with "Progress" parameter (0.0-1.0)
	 * Fills clockwise around the border like reference image
	 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UImage> Img_FillBorder;

	/** Key icon (E, F, or gamepad button) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UImage> Img_Key;

	/** Interaction text ("Interact", "Pick Up", etc.) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UTextBlock> InteractionDescription;

	/** Root overlay container */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UOverlay> RootOverlay;

	// ═══════════════════════════════════════════════
	// MATERIALS
	// ═══════════════════════════════════════════════
	
	/** Material for square border (keyboard mode) - needs "Progress" parameter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Materials")
	TObjectPtr<UMaterialInterface> SquareBorderMaterial;

	/** Material for circle border (gamepad mode) - needs "Progress" parameter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Materials")
	TObjectPtr<UMaterialInterface> CircleBorderMaterial;

	// ═══════════════════════════════════════════════
	// ICONS - REFACTORED TO USE FKey
	// ═══════════════════════════════════════════════

	/** 
	 * Keyboard key textures mapped by FKey
	 * Example setup in editor:
	 *   E → Texture_E_Key.png
	 *   F → Texture_F_Key.png
	 *   R → Texture_R_Key.png
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icons")
	TMap<FKey, TObjectPtr<UTexture2D>> KeyboardIcons;

	/** 
	 * Gamepad button textures mapped by FKey
	 * Example setup in editor:
	 *   Gamepad_FaceButton_Bottom (A/X) → Texture_A_Button.png
	 *   Gamepad_FaceButton_Right (B/Circle) → Texture_B_Button.png
	 *   Gamepad_LeftShoulder (LB/L1) → Texture_LB_Button.png
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icons")
	TMap<FKey, TObjectPtr<UTexture2D>> GamepadIcons;

	/**
	 * Fallback icon when no key found (optional)
	 * Shows generic "Press Button" icon if specific key not mapped
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icons")
	TObjectPtr<UTexture2D> FallbackIcon;

	// ═══════════════════════════════════════════════
	// COLORS
	// ═══════════════════════════════════════════════

	/** Border fill color - idle/holding state (green from reference) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Colors")
	FLinearColor FillColorNormal = FLinearColor(0.45f, 0.76f, 0.26f, 1.0f);  // Green

	/** Border fill color - completed state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Colors")
	FLinearColor FillColorCompleted = FLinearColor(0.2f, 1.0f, 0.5f, 1.0f);  // Bright green

	/** Border fill color - cancelled/depleting state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Colors")
	FLinearColor FillColorCancelled = FLinearColor(1.0f, 0.3f, 0.1f, 1.0f);  // Orange-red

	/** Border background color (unfilled portion) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Colors")
	FLinearColor BorderBackgroundColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.6f);  // Gray

	// ═══════════════════════════════════════════════
	// ANIMATION
	// ═══════════════════════════════════════════════

	/** Idle animation speed (shimmer/pulse when waiting for input) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Animation", meta = (ClampMin = "0.0"))
	float IdleAnimationSpeed = 2.0f;

	/** Enable idle animation when in Idle state? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Animation")
	bool bEnableIdleAnimation = true;

	/** Completion flash duration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Animation", meta = (ClampMin = "0.1"))
	float CompletionFlashDuration = 0.3f;

	// ═══════════════════════════════════════════════
	// PUBLIC API
	// ═══════════════════════════════════════════════
	
	/**
	 * Set interaction data using InputAction (automatically gets bound key)
	 * @param InputAction - The Enhanced Input Action (e.g., IA_Interact)
	 * @param Description - Display text shown to player
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionData(UInputAction* InputAction, const FText& Description);

	/**
	 * Set interaction data using FKey directly (manual override)
	 * Use this if you already know the specific key to show
	 * @param Key - The key to display (e.g., EKeys::E, EKeys::Gamepad_FaceButton_Bottom)
	 * @param Description - Display text shown to player.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionDataWithKey(const FKey& Key, const FText& Description);

	/**
	 * Set widget state (determines visual behavior)
	 * @param NewState - Target state
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetWidgetState(EInteractionWidgetState NewState);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetProgressBarVisible(bool bVisible);

	/**
	 * Set progress value (for Hold/Mash interactions)
	 * Updates the border fill material
	 * @param Progress - Progress value (0.0 = empty, 1.0 = full)
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetProgress(float Progress);

	/**
	 * Get current progress value
	 */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	float GetProgress() const { return CurrentProgress; }

	/**
	 * Get current widget state
	 */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	EInteractionWidgetState GetWidgetState() const { return CurrentState; }

	/**
	 * Force refresh input mode (keyboard vs gamepad)
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void RefreshInputMode();

	/**
	 * Show widget with optional fade
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Show();

	/**
	 * Hide widget with optional fade
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Hide();

	/**
	 * Check if widget is currently visible
	 */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsShown() const { return GetVisibility() == ESlateVisibility::HitTestInvisible || GetVisibility() == ESlateVisibility::Visible; }

protected:
	// ═══════════════════════════════════════════════
	// STATE
	// ═══════════════════════════════════════════════

	/** Current widget state */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|State")
	EInteractionWidgetState CurrentState = EInteractionWidgetState::IWS_Idle;

	/** Current progress (0.0 - 1.0) */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|State")
	float CurrentProgress = 0.0f;

	/** Is using gamepad input? */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|State")
	bool bIsUsingGamepad = false;

	/** 
	 * Current input key to display
	 * REFACTORED: Changed from FName to FKey for direct key lookup
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|State")
	FKey CurrentInputKey;

	// ═══════════════════════════════════════════════
	// INTERNAL
	// ═══════════════════════════════════════════════

	/** Dynamic material instance for border fill */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BorderMID;

	/** Animation accumulator */
	float AnimationTime = 0.0f;

	/** State timer (for completion flash, etc.) */
	float StateTimer = 0.0f;

	/** Cached last progress to avoid redundant updates */
	float LastSetProgress = -1.0f;

	/** Cached last input mode to detect changes */
	bool bLastInputModeGamepad = false;

	// ═══════════════════════════════════════════════
	// INTERNAL METHODS
	// ═══════════════════════════════════════════════

	/** Create/update material instance for current input mode */
	void UpdateBorderMaterial();

	/** Update key icon based on CurrentInputKey and input mode */
	void UpdateKeyIcon();

	/** Update material parameters (progress, color, animation) */
	void UpdateMaterialParameters();

	/** Get current fill color based on state */
	FLinearColor GetCurrentFillColor() const;

	/** Detect current input mode */
	bool DetectGamepadMode() const;

	/** Handle state-specific tick logic */
	void TickState(float DeltaTime);

	/**
	 * Get the bound key for an InputAction from Enhanced Input system
	 * Automatically gets keyboard or gamepad key based on current input mode
	 * @param InputAction - The Enhanced Input Action to query
	 * @return The currently bound FKey, or EKeys::Invalid if not found
	 */
	FKey GetBoundKeyForInputAction(UInputAction* InputAction) const;

	/**
	 * Get Enhanced Input Subsystem
	 * Helper to reduce code duplication
	 */
	UEnhancedInputLocalPlayerSubsystem* GetEnhancedInputSubsystem() const;
};
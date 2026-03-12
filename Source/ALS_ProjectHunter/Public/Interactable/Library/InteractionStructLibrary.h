// Interactable/Library/InteractionStructLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Interactable/Library/InteractionEnumLibrary.h"
#include "InteractionStructLibrary.generated.h"

// Forward declarations
class UTexture2D;
class UMaterialInterface;

// ═══════════════════════════════════════════════════════════════════════
// INTERACTION STRUCTS - Single Responsibility: Define Interaction Data
// ═══════════════════════════════════════════════════════════════════════

/**
 * Input Icon Mapping
 * Maps action names to input device icons
 */
USTRUCT(BlueprintType)
struct FInputIconMapping
{
	GENERATED_BODY()

	/** Action name (e.g., "Interact", "Open", "PickUp") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FName ActionName;

	/** Keyboard/Mouse icon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UTexture2D* KeyboardIcon = nullptr;

	/** Gamepad icon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UTexture2D* GamepadIcon = nullptr;

	/** Touch icon (future mobile support) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UTexture2D* TouchIcon = nullptr;

	FInputIconMapping()
		: ActionName(NAME_None)
	{}

	FInputIconMapping(FName InActionName, UTexture2D* InKeyboardIcon, UTexture2D* InGamepadIcon)
		: ActionName(InActionName)
		, KeyboardIcon(InKeyboardIcon)
		, GamepadIcon(InGamepadIcon)
	{}
};

/**
 * Widget Visual Configuration
 * Defines how the interaction widget looks
 */
USTRUCT(BlueprintType)
struct FInteractionWidgetConfig
{
	GENERATED_BODY()

	/** Widget space (world vs screen) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	EInteractionWidgetSpace WidgetSpace = EInteractionWidgetSpace::IWS_World;

	/** Widget size */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	FVector2D DrawSize = FVector2D(300.0f, 80.0f);

	/** Widget offset from actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	FVector WorldOffset = FVector(0.0f, 0.0f, 100.0f);

	/** Screen space offset (if using screen space) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	FVector2D ScreenOffset = FVector2D(0.0f, -100.0f);

	/** Should widget face camera? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	bool bFaceCamera = true;

	/** Widget anchor position */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	EWidgetAnchor AnchorPosition = EWidgetAnchor::WA_Top;

	/** Widget scale */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	float Scale = 1.0f;

	FInteractionWidgetConfig() = default;
};

/**
 * Hold Interaction Configuration
 * Settings for hold-type interactions
 */
USTRUCT(BlueprintType)
struct FHoldInteractionConfig
{
	GENERATED_BODY()

	/** How long to hold (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hold", meta = (ClampMin = "0.1"))
	float HoldDuration = 2.0f;

	/** Can release and continue? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hold")
	bool bCanPause = false;

	/** If can pause, how long before progress resets? (0 = never resets) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hold", meta = (EditCondition = "bCanPause", ClampMin = "0.0"))
	float PauseResetDelay = 2.0f;

	/** Show progress bar? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hold")
	bool bShowProgress = true;

	/** Progress bar color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hold")
	FLinearColor ProgressColor = FLinearColor(0.0f, 1.0f, 0.8f, 1.0f);

	FHoldInteractionConfig() = default;
};

/**
 * Mash Interaction Configuration
 * Settings for button-mashing interactions
 */
USTRUCT(BlueprintType)
struct FMashInteractionConfig
{
	GENERATED_BODY()

	/** Required number of button presses */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mash", meta = (ClampMin = "1"))
	int32 RequiredPresses = 10;

	/** Progress decay rate per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mash", meta = (ClampMin = "0.0"))
	float DecayRate = 0.2f;

	/** Minimum time between valid presses (anti-spam) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mash", meta = (ClampMin = "0.0"))
	float MinTimeBetweenPresses = 0.05f;

	/** Maximum time between presses before failure */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mash", meta = (ClampMin = "0.0"))
	float MaxIdleTime = 3.0f;

	/** Show progress bar? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mash")
	bool bShowProgress = true;

	/** Progress bar color (filling) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mash")
	FLinearColor FillingColor = FLinearColor(0.0f, 1.0f, 0.8f, 1.0f);

	/** Progress bar color (depleting) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mash")
	FLinearColor DepletingColor = FLinearColor(1.0f, 0.3f, 0.0f, 1.0f);

	FMashInteractionConfig() = default;
};

/**
 * Interaction Trace Configuration
 * Settings for finding interactables
 */
USTRUCT(BlueprintType)
struct FInteractionTraceConfig
{
	GENERATED_BODY()

	/** Max interaction distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace", meta = (ClampMin = "0.0"))
	float MaxDistance = 300.0f;

	/** Sphere trace radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace", meta = (ClampMin = "0.0"))
	float SphereRadius = 50.0f;

	/** Check frequency (times per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace", meta = (ClampMin = "0.01"))
	float CheckFrequency = 0.1f;

	/** Use line trace first (optimization)? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	bool bUseLineTraceFirst = true;

	/** Weight for dot product in scoring */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Scoring", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DotProductWeight = 0.7f;

	/** Weight for distance in scoring */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Scoring", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DistanceWeight = 0.3f;

	/** Server validation buffer (for lag compensation) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Multiplayer", meta = (ClampMin = "0.0"))
	float ServerValidationBuffer = 100.0f;

	/** Debug draw? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Debug")
	bool bDebugDraw = false;

	FInteractionTraceConfig() = default;
};

/**
 * Highlight Configuration
 * Settings for visual feedback
 */
USTRUCT(BlueprintType)
struct FHighlightConfig
{
	GENERATED_BODY()

	/** Highlight type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	EInteractableHighlightType HighlightType = EInteractableHighlightType::IHT_CustomDepth;

	/** Custom depth stencil value (if using custom depth) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight", meta = (EditCondition = "HighlightType == EInteractableHighlightType::IHT_CustomDepth"))
	int32 StencilValue = 250;

	/** Highlight color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	FLinearColor HighlightColor = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);

	/** Pulse effect? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	bool bPulse = true;

	/** Pulse speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight", meta = (EditCondition = "bPulse", ClampMin = "0.1"))
	float PulseSpeed = 2.0f;

	/** Pulse intensity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight", meta = (EditCondition = "bPulse", ClampMin = "0.0", ClampMax = "1.0"))
	float PulseIntensity = 0.5f;

	FHighlightConfig() = default;
};

/**
 * Complete Interaction Configuration
 * All settings for an interactable object
 */
USTRUCT(BlueprintType)
struct FInteractionConfiguration
{
	GENERATED_BODY()

	/** Action name (for icon lookup) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	FName ActionName = FName("Interact");

	/** Display text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	FText InteractionText = FText::FromString("Press To Interact");

	/** Interaction type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	EInteractionType InteractionType = EInteractionType::IT_None;

	/** Currently enabled? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	bool bEnabled = true;

	/** Widget configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	FInteractionWidgetConfig WidgetConfig;

	/** Hold configuration (if type is Hold) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hold", meta = (EditCondition = "InteractionType == EInteractionType::IT_Hold"))
	FHoldInteractionConfig HoldConfig;

	/** Mash configuration (if type is Mash) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mash", meta = (EditCondition = "InteractionType == EInteractionType::IT_Mash"))
	FMashInteractionConfig MashConfig;

	/** Highlight configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	FHighlightConfig HighlightConfig;

	FInteractionConfiguration() = default;
};

/**
 * Interaction Progress Data
 * Runtime data for tracking interaction progress
 */
USTRUCT(BlueprintType)
struct FInteractionProgressData
{
	GENERATED_BODY()

	/** Current progress [0.0 - 1.0] */
	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	float Progress = 0.0f;

	/** Current state */
	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	EInteractionState State = EInteractionState::IS_Idle;

	/** Is depleting? (for color feedback) */
	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	bool bIsDepleting = false;

	/** Time elapsed (for hold interactions) */
	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	float TimeElapsed = 0.0f;

	/** Press count (for mash interactions) */
	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	int32 PressCount = 0;

	/** Time since last press (for mash timeout detection) */
	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	float TimeSinceLastPress = 0.0f;

	FInteractionProgressData() = default;

	void Reset()
	{
		Progress = 0.0f;
		State = EInteractionState::IS_Idle;
		bIsDepleting = false;
		TimeElapsed = 0.0f;
		PressCount = 0;
		TimeSinceLastPress = 0.0f;
	}
};


/**
 * Data-driven highlight style.
 * Interactable provides defaults/hints.
 * Interactor / InteractionManager can override these with player settings.
 */
USTRUCT(BlueprintType)
struct FInteractableHighlightStyle
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Highlight")
	UMaterialInstanceDynamic* OutlineMID = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight")
	bool bEnableHighlight = true;

	/** Stencil value written to the focused mesh/components */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight")
	int32 StencilValue = 250;

	/** Desired outline color (can be overridden by player settings) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight")
	FLinearColor Color = FLinearColor::Yellow;

	/** Desired outline width (can be overridden by player settings) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float OutlineWidth = 3.0f;

	/** Desired depth threshold/sensitivity (can be overridden by player settings) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight", meta = (ClampMin = "0.0", ClampMax = "50.0"))
	float Threshold = 50.0f;
};
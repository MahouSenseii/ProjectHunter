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
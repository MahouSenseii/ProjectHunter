// Interactable/Library/Structs/InteractionStructs.h
#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "Interactable/Library/Enums/InteractionEnums.h"
#include "InteractionStructs.generated.h"

class UMaterialInstanceDynamic;
class UTexture2D;

USTRUCT(BlueprintType)
struct FInteractionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	EInteractionType InteractionType = EInteractionType::IT_Tap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	TObjectPtr<UInputAction> InputAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Hold",
		meta = (EditCondition = "InteractionType == EInteractionType::IT_Hold || InteractionType == EInteractionType::IT_TapOrHold", ClampMin = "0.1", ClampMax = "1.0"))
	float TapHoldThreshold = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Hold",
		meta = (EditCondition = "InteractionType == EInteractionType::IT_Hold || InteractionType == EInteractionType::IT_TapOrHold", ClampMin = "0.1"))
	float HoldDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Hold",
		meta = (EditCondition = "InteractionType == EInteractionType::IT_Hold || InteractionType == EInteractionType::IT_TapOrHold"))
	bool bCanCancelHold = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Hold",
		meta = (EditCondition = "InteractionType == EInteractionType::IT_Hold || InteractionType == EInteractionType::IT_TapOrHold"))
	FText HoldText = FText::FromString("Hold to Interact");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Mash",
		meta = (EditCondition = "InteractionType == EInteractionType::IT_Mash", ClampMin = "1"))
	int32 RequiredMashCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Mash",
		meta = (EditCondition = "InteractionType == EInteractionType::IT_Mash", ClampMin = "0.0"))
	float MashDecayRate = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Mash",
		meta = (EditCondition = "InteractionType == EInteractionType::IT_Mash"))
	FText MashText = FText::FromString("Mash to Open!");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|TapOrHold",
		meta = (EditCondition = "InteractionType == EInteractionType::IT_TapOrHold"))
	FText TapText = FText::FromString("Tap: Pickup");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|TapOrHold",
		meta = (EditCondition = "InteractionType == EInteractionType::IT_TapOrHold"))
	FText HoldActionText = FText::FromString("Hold: Equip");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText InteractionText = FText::FromString("Press To Interact");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bCanInteract = true;

	FInteractionConfig() = default;
};

USTRUCT(BlueprintType)
struct FInputIconMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FName ActionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UTexture2D* KeyboardIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UTexture2D* GamepadIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UTexture2D* TouchIcon = nullptr;

	FInputIconMapping()
		: ActionName(NAME_None)
	{
	}

	FInputIconMapping(FName InActionName, UTexture2D* InKeyboardIcon, UTexture2D* InGamepadIcon)
		: ActionName(InActionName)
		, KeyboardIcon(InKeyboardIcon)
		, GamepadIcon(InGamepadIcon)
	{
	}
};

USTRUCT(BlueprintType)
struct FInteractionWidgetConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	EInteractionWidgetSpace WidgetSpace = EInteractionWidgetSpace::IWS_World;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	FVector2D DrawSize = FVector2D(300.0f, 80.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	FVector WorldOffset = FVector(0.0f, 0.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	FVector2D ScreenOffset = FVector2D(0.0f, -100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	bool bFaceCamera = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	EWidgetAnchor AnchorPosition = EWidgetAnchor::WA_Top;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	float Scale = 1.0f;

	FInteractionWidgetConfig() = default;
};

USTRUCT(BlueprintType)
struct FHighlightConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	EInteractableHighlightType HighlightType = EInteractableHighlightType::IHT_CustomDepth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight", meta = (EditCondition = "HighlightType == EInteractableHighlightType::IHT_CustomDepth"))
	int32 StencilValue = 250;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	FLinearColor HighlightColor = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	bool bPulse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight", meta = (EditCondition = "bPulse", ClampMin = "0.1"))
	float PulseSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight", meta = (EditCondition = "bPulse", ClampMin = "0.0", ClampMax = "1.0"))
	float PulseIntensity = 0.5f;

	FHighlightConfig() = default;
};

USTRUCT(BlueprintType)
struct FInteractableHighlightStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight")
	UMaterialInstanceDynamic* OutlineMID = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight")
	bool bEnableHighlight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight")
	int32 StencilValue = 250;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight")
	FLinearColor Color = FLinearColor::Yellow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float OutlineWidth = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight", meta = (ClampMin = "0.0", ClampMax = "50.0"))
	float Threshold = 50.0f;
};

USTRUCT(BlueprintType)
struct FActiveInteraction
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EManagedInteractionMode Mode = EManagedInteractionMode::None;

	UPROPERTY(BlueprintReadOnly)
	EInteractionType Type = EInteractionType::IT_None;

	UPROPERTY(BlueprintReadOnly)
	EInteractionState State = EInteractionState::IS_Idle;

	UPROPERTY(BlueprintReadOnly)
	AActor* Interactor = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TScriptInterface<class IInteractable> Target;

	UPROPERTY()
	TWeakObjectPtr<UObject> TargetObject;

	UPROPERTY(BlueprintReadOnly)
	int32 GroundItemID = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	float ElapsedTime = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float TapThresholdSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float HoldDurationSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	bool bHoldStarted = false;

	UPROPERTY(BlueprintReadOnly)
	bool bHoldCompleted = false;

	UPROPERTY(BlueprintReadOnly)
	float Progress = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float LastProgress = -1.0f;

	UPROPERTY(BlueprintReadOnly)
	float MashProgressUnits = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	int32 MashCount = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 MashRequiredCount = 0;

	UPROPERTY(BlueprintReadOnly)
	float MashDecayRate = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float LastMashTime = -1.0f;

	FActiveInteraction() = default;

	bool IsActive() const
	{
		return Mode != EManagedInteractionMode::None;
	}

	void Reset()
	{
		Mode = EManagedInteractionMode::None;
		Type = EInteractionType::IT_None;
		State = EInteractionState::IS_Idle;
		Interactor = nullptr;
		Target = nullptr;
		TargetObject.Reset();
		GroundItemID = INDEX_NONE;
		ElapsedTime = 0.0f;
		TapThresholdSeconds = 0.0f;
		HoldDurationSeconds = 0.0f;
		bHoldStarted = false;
		bHoldCompleted = false;
		Progress = 0.0f;
		LastProgress = -1.0f;
		MashProgressUnits = 0.0f;
		MashCount = 0;
		MashRequiredCount = 0;
		MashDecayRate = 0.0f;
		LastMashTime = -1.0f;
	}
};

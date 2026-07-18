// Interactable/Library/Enums/InteractionEnums.h
#pragma once

#include "CoreMinimal.h"
#include "InteractionEnums.generated.h"

UENUM(BlueprintType)
enum class EInteractionType : uint8
{
	IT_Tap UMETA(DisplayName = "Tap"),
	IT_Hold UMETA(DisplayName = "Hold"),
	IT_Mash UMETA(DisplayName = "Mash"),
	IT_TapOrHold UMETA(DisplayName = "Tap or Hold"),
	IT_Toggle UMETA(DisplayName = "Toggle"),
	IT_Continuous UMETA(DisplayName = "Continuous"),
	IT_None UMETA(DisplayName = "None")
};

UENUM(BlueprintType)
enum class EInteractionWidgetSpace : uint8
{
	IWS_World UMETA(DisplayName = "World Space"),
	IWS_Screen UMETA(DisplayName = "Screen Space"),
	IWS_Component UMETA(DisplayName = "Component Space")
};

UENUM(BlueprintType)
enum class EInteractionState : uint8
{
	IS_Idle UMETA(DisplayName = "Idle"),
	IS_Started UMETA(DisplayName = "Started"),
	IS_InProgress UMETA(DisplayName = "In Progress"),
	IS_Completed UMETA(DisplayName = "Completed"),
	IS_Cancelled UMETA(DisplayName = "Cancelled"),
	IS_Failed UMETA(DisplayName = "Failed")
};

UENUM(BlueprintType)
enum class EInputDeviceType : uint8
{
	IDT_Keyboard UMETA(DisplayName = "Keyboard & Mouse"),
	IDT_Gamepad UMETA(DisplayName = "Gamepad"),
	IDT_Touch UMETA(DisplayName = "Touch")
};

UENUM(BlueprintType)
enum class EProgressColorMode : uint8
{
	PCM_Filling UMETA(DisplayName = "Filling"),
	PCM_Depleting UMETA(DisplayName = "Depleting"),
	PCM_Warning UMETA(DisplayName = "Warning"),
	PCM_Success UMETA(DisplayName = "Success"),
	PCM_Disabled UMETA(DisplayName = "Disabled")
};

UENUM(BlueprintType)
enum class EInteractableHighlightType : uint8
{
	IHT_None UMETA(DisplayName = "None"),
	IHT_CustomDepth UMETA(DisplayName = "Custom Depth Stencil"),
	IHT_Outline UMETA(DisplayName = "Outline Effect"),
	IHT_Emission UMETA(DisplayName = "Emission Glow"),
	IHT_Material UMETA(DisplayName = "Material Swap"),
	IHT_Overlay UMETA(DisplayName = "Overlay Effect")
};

UENUM(BlueprintType)
enum class EWidgetAnchor : uint8
{
	WA_Top UMETA(DisplayName = "Top"),
	WA_Bottom UMETA(DisplayName = "Bottom"),
	WA_Center UMETA(DisplayName = "Center"),
	WA_Custom UMETA(DisplayName = "Custom Offset")
};

UENUM(BlueprintType)
enum class EInteractionValidation : uint8
{
	IV_Valid UMETA(DisplayName = "Valid"),
	IV_TooFar UMETA(DisplayName = "Too Far"),
	IV_Obstructed UMETA(DisplayName = "Obstructed"),
	IV_Disabled UMETA(DisplayName = "Disabled"),
	IV_OnCooldown UMETA(DisplayName = "On Cooldown"),
	IV_RequirementFailed UMETA(DisplayName = "Requirement Failed"),
	IV_InvalidTarget UMETA(DisplayName = "Invalid Target")
};

UENUM(BlueprintType)
enum class EInteractionResult : uint8
{
	IR_Success UMETA(DisplayName = "Success"),
	IR_CannotInteract UMETA(DisplayName = "Cannot Interact"),
	IR_WrongType UMETA(DisplayName = "Wrong Type"),
	IR_InventoryFull UMETA(DisplayName = "Inventory Full"),
	IR_RequirementsNotMet UMETA(DisplayName = "Requirements Not Met"),
	IR_TooFar UMETA(DisplayName = "Too Far"),
	IR_Failed UMETA(DisplayName = "Failed")
};

// Tracks the active lifecycle branch without overloading the player-facing interaction type.
UENUM()
enum class EManagedInteractionMode : uint8
{
	None,
	GroundTapOrHold,
	ActorHold,
	ActorTapOrHold,
	ActorMash,
	ActorContinuous
};

UENUM(BlueprintType)
enum class EInteractionWidgetState : uint8
{
	IWS_Idle UMETA(DisplayName = "Idle"),
	IWS_Holding UMETA(DisplayName = "Holding"),
	IWS_Mashing UMETA(DisplayName = "Mashing"),
	IWS_Completed UMETA(DisplayName = "Completed"),
	IWS_Cancelled UMETA(DisplayName = "Cancelled")
};

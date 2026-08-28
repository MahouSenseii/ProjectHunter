// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Equipment/Library/Enums/EquipmentEnums.h"
#include "UI/Menu/Library/Enums/MenuEnums.h"
#include "PHMenuCameraBehavior.generated.h"

/**
 * Anim instance that lets the menu camera be authored in an animation graph,
 * the way ALS authors the gameplay camera.
 *
 * ALS runs UALSPlayerCameraBehavior on a hidden skeletal mesh and reads named
 * curves off it every frame; the anim graph's state transitions are what blend
 * the camera. This is the same trick for the menu: push the menu's state in,
 * build a state machine on it, and put the framing on each state's pose as
 * curves.
 *
 * Nothing here is required. With no anim Blueprint assigned to the rig, the
 * Menu_Override curve reads 0 and UPHMenuCameraComponent uses its PageViews
 * data exactly as before.
 *
 * Curves read by UPHMenuCameraComponent:
 *   Menu_Override      0 keeps the PageViews value, 1 takes the curve fully
 *   Menu_Distance      cm from the pivot
 *   Menu_PivotHeight   raises what the camera looks at
 *   Menu_YawOffset     degrees around the character from their facing
 *   Menu_Pitch         negative looks up at them
 *   Menu_LateralOffset slides the character off centre
 *   Menu_FOV           degrees
 *
 * A curve that is absent is skipped rather than treated as zero, so a state can
 * override just the distance and leave everything else to the data.
 */
UCLASS(Blueprintable, BlueprintType)
class ALS_PROJECTHUNTER_API UPHMenuCameraBehavior : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Called by UPHMenuCameraComponent once per frame, before the curves are read. */
	void UpdateMenuState(
		EMenuType InMenuType,
		EEquipmentSlot InFocusedSlot,
		float InZoomMultiplier,
		float InTurntableYaw,
		bool bInDragging,
		bool bInMenuCameraActive);

	/** The page currently open. Drive the state machine from this. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Menu Camera|State")
	EMenuType MenuType = EMenuType::MT_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Menu Camera|State")
	EEquipmentSlot FocusedEquipmentSlot = EEquipmentSlot::ES_None;

	/** Convenience for a transition rule; true whenever a body part is focused. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Menu Camera|State")
	bool bHasEquipmentFocus = false;

	/** Below 1 the player has zoomed in. Useful as a blend alpha. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Menu Camera|State")
	float ZoomMultiplier = 1.0f;

	/** Degrees the character has been spun on the turntable. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Menu Camera|State")
	float TurntableYaw = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Menu Camera|State")
	bool bDragging = false;

	/** False while the camera is blending back out to gameplay. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Menu Camera|State")
	bool bMenuCameraActive = false;
};

/** Curve names the component samples. Match these in the anim Blueprint. */
namespace PHMenuCameraCurves
{
	inline const FName Override{TEXT("Menu_Override")};
	inline const FName Distance{TEXT("Menu_Distance")};
	inline const FName PivotHeight{TEXT("Menu_PivotHeight")};
	inline const FName YawOffset{TEXT("Menu_YawOffset")};
	inline const FName Pitch{TEXT("Menu_Pitch")};
	inline const FName LateralOffset{TEXT("Menu_LateralOffset")};
	inline const FName FOV{TEXT("Menu_FOV")};
}

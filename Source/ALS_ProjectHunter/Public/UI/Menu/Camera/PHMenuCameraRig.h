// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PHMenuCameraRig.generated.h"

class UCameraComponent;
class UPHMenuCameraBehavior;
class USkeletalMeshComponent;
class USpotLightComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * The view target the player controller blends to while the menu is open.
 *
 * The actor's origin sits on the character's pivot and its yaw is the
 * direction the camera looks, so the camera hangs off it at -X and every light
 * is a fixed child around the subject. That keeps the key/rim lighting
 * identical no matter where in the world the menu was opened, and it means
 * spinning the character on the turntable spins them *through* the lighting
 * instead of dragging the lights with them.
 *
 * Nothing here is required to be a Blueprint - the defaults are usable as-is,
 * and a Blueprint child only exists if you want to tune lights or add VFX.
 */
UCLASS(Blueprintable, BlueprintType, NotPlaceable)
class ALS_PROJECTHUNTER_API APHMenuCameraRig : public AActor
{
	GENERATED_BODY()

public:
	APHMenuCameraRig();

	UFUNCTION(BlueprintPure, Category = "Menu Camera Rig")
	UCameraComponent* GetCameraComponent() const { return Camera; }

	/**
	 * The anim instance driving the framing curves, or null when no anim
	 * Blueprint is assigned - in which case the camera stays fully data-driven.
	 */
	UFUNCTION(BlueprintPure, Category = "Menu Camera Rig")
	UPHMenuCameraBehavior* GetCameraBehavior() const;

	/**
	 * Places the camera relative to the pivot this actor sits on.
	 * Distance runs backwards along -X, Lateral along +Y, Height along +Z.
	 */
	void ApplyCameraOffset(float Distance, float LateralOffset, float HeightOffset, float Pitch, float FOV);

	/** Focuses depth of field on the subject so the world behind falls away. */
	void ApplyDepthOfField(float FocalDistance);

	/** Key/fill/rim lights. Hidden with the rig so they never leak into gameplay. */
	void SetStudioLightingVisible(bool bNewVisible);

	/** Only ever shows when BackdropMaterial is assigned - an unskinned plane looks worse than the world. */
	void SetBackdropVisible(bool bNewVisible);

	/** Hides the whole rig, lights included. */
	void SetRigVisible(bool bNewVisible);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Menu Camera Rig")
	TObjectPtr<USceneComponent> PivotRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Menu Camera Rig")
	TObjectPtr<UCameraComponent> Camera;

	/**
	 * Hidden mesh that exists only to run the camera anim graph, mirroring how
	 * AALSPlayerCameraManager carries its own CameraBehavior component.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Menu Camera Rig|Behavior")
	TObjectPtr<USkeletalMeshComponent> CameraBehavior;

	/** Assign an anim Blueprint here to author the framing in a state machine. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Camera Rig|Behavior")
	TSubclassOf<UPHMenuCameraBehavior> CameraBehaviorClass;

	/** Front-left and above. The light that describes the silhouette. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Menu Camera Rig|Lighting")
	TObjectPtr<USpotLightComponent> KeyLight;

	/** Front-right and low. Lifts the shadow side so armour detail survives. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Menu Camera Rig|Lighting")
	TObjectPtr<USpotLightComponent> FillLight;

	/** Behind the subject. The bright edge that separates them from the background. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Menu Camera Rig|Lighting")
	TObjectPtr<USpotLightComponent> RimLight;

	/** Optional clean card behind the character. Stays hidden until it has a material. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Menu Camera Rig|Backdrop")
	TObjectPtr<UStaticMeshComponent> Backdrop;

	// CONFIG

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Camera Rig|Lighting")
	bool bUseStudioLighting = true;

	/** Candelas. Tune against your exposure - these are a starting point, not gospel. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Camera Rig|Lighting",
		meta = (ClampMin = "0.0"))
	float KeyLightIntensity = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Camera Rig|Lighting",
		meta = (ClampMin = "0.0"))
	float FillLightIntensity = 7.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Camera Rig|Lighting",
		meta = (ClampMin = "0.0"))
	float RimLightIntensity = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Camera Rig|Lighting")
	FLinearColor KeyLightColor = FLinearColor(1.0f, 0.97f, 0.92f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Camera Rig|Lighting")
	FLinearColor FillLightColor = FLinearColor(0.72f, 0.80f, 1.0f);

	/**
	 * Tinted towards the panel azure so the rim reads as the system window's
	 * glow spilling onto the character rather than generic studio lighting.
	 * Kept bright: a light colour is a multiplier, so feeding it the panel's
	 * dark linear azure would just turn the rim off.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Camera Rig|Lighting")
	FLinearColor RimLightColor = FLinearColor(0.35f, 0.62f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Camera Rig|Post Process")
	bool bUseDepthOfField = true;

	/** Lower is a shallower, more cinematic falloff. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Camera Rig|Post Process",
		meta = (ClampMin = "0.5", ClampMax = "32.0"))
	float DepthOfFieldFstop = 1.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Camera Rig|Post Process",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VignetteIntensity = 0.55f;

	/** Assign a dark translucent gradient to switch the backdrop on. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Camera Rig|Backdrop")
	TObjectPtr<UMaterialInterface> BackdropMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Camera Rig|Backdrop",
		meta = (ClampMin = "0.0"))
	float BackdropDistance = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Camera Rig|Backdrop",
		meta = (ClampMin = "1.0"))
	float BackdropScale = 14.0f;

	/** Only applied to the component once BackdropMaterial is set. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu Camera Rig|Backdrop")
	TObjectPtr<UStaticMesh> BackdropMesh;

private:
	void ApplyLightingSettings();
};

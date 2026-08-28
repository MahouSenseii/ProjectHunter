// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/ActorComponent.h"
#include "Equipment/Library/Enums/EquipmentEnums.h"
#include "InputCoreTypes.h"
#include "UI/Menu/Camera/PHMenuCameraTypes.h"
#include "UI/Menu/Library/Enums/MenuEnums.h"
#include "PHMenuCameraComponent.generated.h"

class APHBaseCharacter;
class APHMenuCameraRig;
class APlayerController;
class UUserWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogMenuCamera, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMenuCameraActiveChanged, bool, bIsActive);

/**
 * Swings the view onto the player's character while the menu is open, the way
 * a character screen does, without a render target or a duplicate mesh.
 *
 * ALS drives the gameplay camera from an animation Blueprint inside
 * AALSPlayerCameraManager, but UpdateViewTargetInternal only does that when the
 * view target *is* an AALSBaseCharacter - anything else falls through to
 * CalcCamera. So pointing the controller at a plain camera actor hands the
 * camera over cleanly, and pointing it back hands it back.
 *
 * Lives on AHunterHUD next to the menu's input-mode switch so the widget, the
 * input mode, and the camera can never disagree about whether a menu is open.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UPHMenuCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPHMenuCameraComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// ACTIVATION

	/** Blends onto the character using the view configured for MenuType. */
	UFUNCTION(BlueprintCallable, Category = "Menu Camera")
	void ActivateMenuCamera(EMenuType MenuType = EMenuType::MT_Equipment);

	/** Blends back to the ALS gameplay camera and unwinds the turntable. */
	UFUNCTION(BlueprintCallable, Category = "Menu Camera")
	void DeactivateMenuCamera();

	/** Re-frames for a different tab. Pages in CameraDisabledPages blend back out. */
	UFUNCTION(BlueprintCallable, Category = "Menu Camera")
	void SetMenuPage(EMenuType MenuType);

	UFUNCTION(BlueprintPure, Category = "Menu Camera")
	bool IsMenuCameraActive() const { return bMenuCameraActive; }

	UFUNCTION(BlueprintPure, Category = "Menu Camera")
	APHMenuCameraRig* GetCameraRig() const { return CameraRig; }

	/** The menu camera on the widget's owning HUD, or null when there is none. */
	UFUNCTION(BlueprintPure, Category = "Menu Camera", meta = (DefaultToSelf = "Widget"))
	static UPHMenuCameraComponent* GetForWidget(const UUserWidget* Widget);

	// FOCUS

	/** Drifts the framing towards the body part that wears EquipmentSlot. */
	UFUNCTION(BlueprintCallable, Category = "Menu Camera|Focus")
	void FocusEquipmentSlot(EEquipmentSlot EquipmentSlot);

	/** Eases back to the page's default framing. */
	UFUNCTION(BlueprintCallable, Category = "Menu Camera|Focus")
	void ClearEquipmentSlotFocus();

	UFUNCTION(BlueprintPure, Category = "Menu Camera|Focus")
	EEquipmentSlot GetFocusedEquipmentSlot() const { return FocusedEquipmentSlot; }

	// TURNTABLE

	/**
	 * Spins the character in place. Degrees, applied to the mesh rather than the
	 * camera so lighting and framing stay put at every angle - orbiting the
	 * camera instead would swing it through walls and kill the rim light.
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu Camera|Turntable")
	void AddTurntableInput(float DeltaDegrees);

	/** Returns the character to the facing they had when the menu opened. */
	UFUNCTION(BlueprintCallable, Category = "Menu Camera|Turntable")
	void ResetTurntable();

	/**
	 * Zooms within the clamp range. Positive pushes in.
	 * Call this from the menu widget if Slate eats the wheel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu Camera|Turntable")
	void AddZoomInput(float DeltaZoom);

	UPROPERTY(BlueprintAssignable, Category = "Menu Camera|Events")
	FOnMenuCameraActiveChanged OnMenuCameraActiveChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// CONFIG - FRAMING

	/** Master switch. Off leaves the gameplay camera alone entirely. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Config")
	bool bEnableMenuCamera = true;

	/** Used for any page without an entry in PageViews. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Config")
	FPHMenuCameraView DefaultView;

	/**
	 * Per-tab framing. Switching tabs interpolates between entries rather than
	 * cutting, which is most of the polish for none of the work.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Config")
	TMap<EMenuType, FPHMenuCameraView> PageViews;

	/** Pages that keep the gameplay camera - a settings screen has no subject. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Config")
	TArray<EMenuType> CameraDisabledPages;

	/** How fast framing chases a new page or focus. Higher is snappier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Config",
		meta = (ClampMin = "0.5", UIMax = "20.0"))
	float ViewInterpSpeed = 6.0f;

	// CONFIG - BLEND

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Blend",
		meta = (ClampMin = "0.0", UIMax = "3.0"))
	float BlendInTime = 0.6f;

	/** Closing should be quicker than opening - symmetric blends feel sluggish. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Blend",
		meta = (ClampMin = "0.0", UIMax = "3.0"))
	float BlendOutTime = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Blend")
	TEnumAsByte<EViewTargetBlendFunction> BlendFunction = VTBlend_EaseInOut;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Blend",
		meta = (ClampMin = "0.1", UIMax = "6.0"))
	float BlendExponent = 2.0f;

	// CONFIG - INPUT

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Turntable")
	bool bAllowTurntable = true;

	/**
	 * Reads cursor movement rather than the look axis, because the menu runs
	 * with a visible uncaptured cursor and the axis reports nothing there.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Turntable")
	bool bPollCursorForTurntable = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Turntable")
	FKey TurntableDragKey = EKeys::RightMouseButton;

	/** Negative flips the drag direction. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Turntable")
	float TurntableSensitivity = 0.45f;

	/** Spin-down after the drag is released. Higher stops sooner. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Turntable",
		meta = (ClampMin = "0.1", UIMax = "20.0"))
	float TurntableInertiaDamping = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Turntable")
	bool bAllowZoom = true;

	/** The wheel is polled, but Slate may consume it - AddZoomInput always works. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Turntable")
	bool bPollMouseWheelForZoom = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Turntable",
		meta = (ClampMin = "0.0"))
	float ZoomStep = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Turntable",
		meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float MinZoomMultiplier = 0.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Turntable",
		meta = (ClampMin = "1.0", ClampMax = "4.0"))
	float MaxZoomMultiplier = 1.6f;

	// CONFIG - COLLISION

	/** Without this the camera happily sets up inside the wall behind the player. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Collision")
	bool bTraceForBlockingGeometry = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Collision",
		meta = (ClampMin = "1.0"))
	float BlockingTraceRadius = 18.0f;

	/** Never pull closer than this, even when boxed in. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Collision",
		meta = (ClampMin = "20.0"))
	float MinBlockedDistance = 70.0f;

	// CONFIG - FOCUS

	/**
	 * Filled with body-part offsets in the constructor. Rings are deliberately
	 * absent: pushing in on a finger reads as a bug, not a feature.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Focus")
	TMap<EEquipmentSlot, FPHMenuCameraFocus> SlotFocusOffsets;

	// CONFIG - RIG / WORLD

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Rig")
	TSubclassOf<APHMenuCameraRig> CameraRigClass;

	/**
	 * Global time dilation while the menu is open. 1 leaves the world running.
	 * Do not use 0 - the idle animation freezes and you are looking at a statue.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Menu Camera|Rig",
		meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float MenuTimeDilation = 1.0f;

private:
	APlayerController* GetOwningPlayerController() const;
	APHBaseCharacter* GetTargetCharacter() const;

	bool EnsureCameraRig();

	/** DefaultView, overridden by PageViews, with the focus delta folded in. */
	FPHMenuCameraView ResolveTargetView() const;

	/** Feeds the anim graph the menu state its transitions are built on. */
	void PushBehaviorState() const;

	/**
	 * Blends the anim graph's framing curves over the data-driven view by the
	 * Menu_Override curve. Absent curves are left alone rather than read as
	 * zero, so a state can override only what it cares about.
	 */
	FPHMenuCameraView ApplyBehaviorCurves(const FPHMenuCameraView& DataView) const;

	void UpdateFraming(float DeltaTime);
	void UpdateTurntable(float DeltaTime);
	void PollCursorInput();

	/** Pulls the camera in when geometry sits between it and the character. */
	float ResolveUnblockedDistance(const FVector& PivotLocation, const FVector& DesiredLocation,
		float DesiredDistance) const;

	void RestoreTimeDilation();

	/**
	 * ALS stops updating its camera lag while something else is the view target,
	 * so its pivot goes stale and the return blend visibly settles. OnPossess
	 * re-seeds exactly those values.
	 */
	void ReseedAlsCamera(APHBaseCharacter* Character) const;

	void CacheCharacterState(APHBaseCharacter* Character);
	void RestoreCharacterState();
	void HandleBlendOutFinished();

	UPROPERTY()
	TObjectPtr<APHMenuCameraRig> CameraRig;

	TWeakObjectPtr<APHBaseCharacter> TargetCharacter;

	bool bMenuCameraActive = false;
	bool bBlendingOut = false;
	bool bTimeDilationApplied = false;

	EMenuType ActiveMenuType = EMenuType::MT_None;
	EEquipmentSlot FocusedEquipmentSlot = EEquipmentSlot::ES_None;

	/** Interpolated framing actually in use this frame. */
	FPHMenuCameraView CurrentView;

	/** Character facing when the menu opened - the camera anchors to this. */
	float CharacterYawAtOpen = 0.0f;

	/**
	 * Pivot height above the actor origin, sampled once on open. Sampling the
	 * head socket every frame instead would let the turntable feed back into
	 * the framing as the mesh spins.
	 */
	float PivotHeightFromActor = 90.0f;

	FRotator CachedMeshRelativeRotation = FRotator::ZeroRotator;

	float TurntableYaw = 0.0f;
	float TurntableVelocity = 0.0f;
	float ZoomMultiplier = 1.0f;
	float TargetZoomMultiplier = 1.0f;

	bool bTurntableDragActive = false;
	FVector2D LastCursorPosition = FVector2D::ZeroVector;

	FTimerHandle BlendOutTimerHandle;
};

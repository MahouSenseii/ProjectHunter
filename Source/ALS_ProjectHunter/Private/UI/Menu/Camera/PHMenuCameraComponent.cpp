// Author: Quentin Davis

#include "UI/Menu/Camera/PHMenuCameraComponent.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Character/ALSPlayerCameraManager.h"
#include "Character/PHBaseCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UI/HUD/HunterHUD.h"
#include "UI/Menu/Camera/PHMenuCameraRig.h"

DEFINE_LOG_CATEGORY(LogMenuCamera);

UPHMenuCameraComponent::UPHMenuCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// The menu is allowed to run the world slowed or paused; the framing still
	// has to update or the camera freezes mid-blend.
	PrimaryComponentTick.bTickEvenWhenPaused = true;

	CameraRigClass = APHMenuCameraRig::StaticClass();

	// A settings page has no subject worth looking at.
	CameraDisabledPages = { EMenuType::MT_Settings };

	// Heights are relative to the ALS pivot, which sits around the waist.
	SlotFocusOffsets.Add(EEquipmentSlot::ES_Head, FPHMenuCameraFocus(55.0f, 0.55f));
	SlotFocusOffsets.Add(EEquipmentSlot::ES_Amulet, FPHMenuCameraFocus(45.0f, 0.55f));
	SlotFocusOffsets.Add(EEquipmentSlot::ES_Chest, FPHMenuCameraFocus(25.0f, 0.75f));
	SlotFocusOffsets.Add(EEquipmentSlot::ES_Hands, FPHMenuCameraFocus(0.0f, 0.70f));
	SlotFocusOffsets.Add(EEquipmentSlot::ES_Belt, FPHMenuCameraFocus(-10.0f, 0.70f));
	SlotFocusOffsets.Add(EEquipmentSlot::ES_Legs, FPHMenuCameraFocus(-35.0f, 0.80f));
	SlotFocusOffsets.Add(EEquipmentSlot::ES_Feet, FPHMenuCameraFocus(-60.0f, 0.70f));

	// Weapons swing round to the hand that holds them.
	SlotFocusOffsets.Add(EEquipmentSlot::ES_MainHand, FPHMenuCameraFocus(10.0f, 0.75f, -18.0f));
	SlotFocusOffsets.Add(EEquipmentSlot::ES_OffHand, FPHMenuCameraFocus(10.0f, 0.75f, 18.0f));
	SlotFocusOffsets.Add(EEquipmentSlot::ES_TwoHand, FPHMenuCameraFocus(10.0f, 0.85f));
}

void UPHMenuCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentView = DefaultView;
}

void UPHMenuCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bMenuCameraActive || bBlendingOut)
	{
		// Leaving time dilation clamped would outlive the HUD.
		RestoreTimeDilation();
		RestoreCharacterState();
	}

	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BlendOutTimerHandle);
	}

	if (CameraRig)
	{
		CameraRig->Destroy();
		CameraRig = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

UPHMenuCameraComponent* UPHMenuCameraComponent::GetForWidget(const UUserWidget* Widget)
{
	const APlayerController* PC = Widget ? Widget->GetOwningPlayer() : nullptr;
	const AHunterHUD* HunterHUD = PC ? Cast<AHunterHUD>(PC->GetHUD()) : nullptr;
	return HunterHUD ? HunterHUD->GetMenuCameraComponent() : nullptr;
}

APlayerController* UPHMenuCameraComponent::GetOwningPlayerController() const
{
	if (const AHUD* OwningHUD = Cast<AHUD>(GetOwner()))
	{
		return OwningHUD->GetOwningPlayerController();
	}

	return GetOwner() ? GetOwner()->GetInstigatorController<APlayerController>() : nullptr;
}

APHBaseCharacter* UPHMenuCameraComponent::GetTargetCharacter() const
{
	const APlayerController* PC = GetOwningPlayerController();
	return PC ? Cast<APHBaseCharacter>(PC->GetPawn()) : nullptr;
}

bool UPHMenuCameraComponent::EnsureCameraRig()
{
	if (CameraRig)
	{
		return true;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.ObjectFlags |= RF_Transient;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TSubclassOf<APHMenuCameraRig> RigClass = CameraRigClass;
	if (!RigClass)
	{
		RigClass = APHMenuCameraRig::StaticClass();
	}

	CameraRig = World->SpawnActor<APHMenuCameraRig>(RigClass, FTransform::Identity, SpawnParams);
	if (!CameraRig)
	{
		UE_LOG(LogMenuCamera, Warning,
			TEXT("EnsureCameraRig: failed to spawn %s; the menu will keep the gameplay camera."),
			*GetNameSafe(RigClass));
		return false;
	}

	return true;
}

// ACTIVATION

void UPHMenuCameraComponent::ActivateMenuCamera(const EMenuType MenuType)
{
	if (!bEnableMenuCamera)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	APHBaseCharacter* Character = GetTargetCharacter();
	if (!PC || !Character)
	{
		UE_LOG(LogMenuCamera, Warning,
			TEXT("ActivateMenuCamera: aborted - PlayerController=%s Pawn=%s (class %s). "
			     "The menu camera only frames an APHBaseCharacter; the gameplay camera is left alone."),
			*GetNameSafe(PC),
			PC ? *GetNameSafe(PC->GetPawn()) : TEXT("None"),
			(PC && PC->GetPawn()) ? *GetNameSafe(PC->GetPawn()->GetClass()) : TEXT("None"));
		return;
	}

	if (CameraDisabledPages.Contains(MenuType))
	{
		ActiveMenuType = MenuType;
		DeactivateMenuCamera();
		return;
	}

	if (!EnsureCameraRig())
	{
		return;
	}

	ActiveMenuType = MenuType;

	if (bMenuCameraActive)
	{
		// Already framed - a page change only needs to retarget the framing.
		ClearEquipmentSlotFocus();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BlendOutTimerHandle);
	}
	bBlendingOut = false;

	TargetCharacter = Character;
	CacheCharacterState(Character);

	TurntableYaw = 0.0f;
	TurntableVelocity = 0.0f;
	bTurntableDragActive = false;
	ZoomMultiplier = 1.0f;
	TargetZoomMultiplier = 1.0f;
	FocusedEquipmentSlot = EEquipmentSlot::ES_None;

	// Snap rather than interpolate, so the blend starts from the real framing
	// instead of chasing it while the camera is already moving.
	CurrentView = ResolveTargetView();

	CameraRig->SetRigVisible(true);
	UpdateFraming(0.0f);

	PC->SetViewTargetWithBlend(CameraRig, BlendInTime, BlendFunction.GetValue(), BlendExponent, false);

	if (!FMath::IsNearlyEqual(MenuTimeDilation, 1.0f))
	{
		UGameplayStatics::SetGlobalTimeDilation(this, MenuTimeDilation);
		bTimeDilationApplied = true;
	}

	bMenuCameraActive = true;
	OnMenuCameraActiveChanged.Broadcast(true);

	const UCameraComponent* RigCamera = CameraRig->GetCameraComponent();
	UE_LOG(LogMenuCamera, Log,
		TEXT("ActivateMenuCamera: page=%d character=%s characterLoc=%s pivotHeight=%.1f "
		     "yawAtOpen=%.1f rigLoc=%s cameraLoc=%s distance=%.1f fov=%.1f"),
		static_cast<int32>(MenuType), *GetNameSafe(Character),
		*Character->GetActorLocation().ToCompactString(), PivotHeightFromActor, CharacterYawAtOpen,
		*CameraRig->GetActorLocation().ToCompactString(),
		RigCamera ? *RigCamera->GetComponentLocation().ToCompactString() : TEXT("no camera component"),
		CurrentView.Distance, CurrentView.FOV);
}

void UPHMenuCameraComponent::DeactivateMenuCamera()
{
	if (!bMenuCameraActive)
	{
		return;
	}

	bMenuCameraActive = false;
	bBlendingOut = true;
	FocusedEquipmentSlot = EEquipmentSlot::ES_None;

	APlayerController* PC = GetOwningPlayerController();
	APHBaseCharacter* Character = TargetCharacter.Get();

	if (PC)
	{
		if (Character)
		{
			ReseedAlsCamera(Character);
			PC->SetViewTargetWithBlend(Character, BlendOutTime, BlendFunction.GetValue(), BlendExponent, false);
		}
		else if (APawn* Pawn = PC->GetPawn())
		{
			// Character died or was unpossessed behind the menu - just get back.
			PC->SetViewTarget(Pawn);
		}
	}

	RestoreTimeDilation();

	// The rig stays alive and lit until the blend lands; hiding it now would
	// black out the lighting halfway through the move back.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(BlendOutTimerHandle, this,
			&UPHMenuCameraComponent::HandleBlendOutFinished,
			FMath::Max(0.01f, BlendOutTime), false);
	}
	else
	{
		HandleBlendOutFinished();
	}

	OnMenuCameraActiveChanged.Broadcast(false);
}

void UPHMenuCameraComponent::SetMenuPage(const EMenuType MenuType)
{
	if (!bEnableMenuCamera)
	{
		return;
	}

	if (CameraDisabledPages.Contains(MenuType))
	{
		ActiveMenuType = MenuType;
		DeactivateMenuCamera();
		return;
	}

	if (!bMenuCameraActive)
	{
		// Coming back from a page that had the camera turned off.
		ActivateMenuCamera(MenuType);
		return;
	}

	if (MenuType == ActiveMenuType)
	{
		return;
	}

	ActiveMenuType = MenuType;

	// A new page should not inherit the framing of whatever was hovered last.
	ClearEquipmentSlotFocus();
}

void UPHMenuCameraComponent::HandleBlendOutFinished()
{
	RestoreCharacterState();

	if (CameraRig)
	{
		CameraRig->SetRigVisible(false);
	}

	bBlendingOut = false;
	TargetCharacter.Reset();
}

// FOCUS

void UPHMenuCameraComponent::FocusEquipmentSlot(const EEquipmentSlot EquipmentSlot)
{
	if (!bMenuCameraActive || FocusedEquipmentSlot == EquipmentSlot)
	{
		return;
	}

	// Slots without an entry (the rings) simply hold the page framing.
	FocusedEquipmentSlot = SlotFocusOffsets.Contains(EquipmentSlot)
		? EquipmentSlot
		: EEquipmentSlot::ES_None;
}

void UPHMenuCameraComponent::ClearEquipmentSlotFocus()
{
	FocusedEquipmentSlot = EEquipmentSlot::ES_None;
}

// TURNTABLE

void UPHMenuCameraComponent::AddTurntableInput(const float DeltaDegrees)
{
	if (!bAllowTurntable || !bMenuCameraActive || FMath::IsNearlyZero(DeltaDegrees))
	{
		return;
	}

	TurntableYaw += DeltaDegrees;

	// Remember the rate so a flick keeps rolling after the button comes up.
	const UWorld* World = GetWorld();
	const float DeltaTime = World ? World->GetDeltaSeconds() : 0.0f;
	TurntableVelocity = DeltaTime > KINDA_SMALL_NUMBER ? DeltaDegrees / DeltaTime : 0.0f;
}

void UPHMenuCameraComponent::ResetTurntable()
{
	TurntableYaw = 0.0f;
	TurntableVelocity = 0.0f;
}

void UPHMenuCameraComponent::AddZoomInput(const float DeltaZoom)
{
	if (!bAllowZoom || !bMenuCameraActive)
	{
		return;
	}

	// Positive pushes in, so it comes off the distance multiplier.
	TargetZoomMultiplier = FMath::Clamp(
		TargetZoomMultiplier - DeltaZoom, MinZoomMultiplier, MaxZoomMultiplier);
}

// TICK

void UPHMenuCameraComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bMenuCameraActive && !bBlendingOut)
	{
		return;
	}

	if (bMenuCameraActive && !TargetCharacter.IsValid())
	{
		// Died or was unpossessed with the menu still open.
		DeactivateMenuCamera();
		return;
	}

	// Menu motion should keep its own pace when the world is running slowed.
	const float Dilation = FMath::Max(KINDA_SMALL_NUMBER, UGameplayStatics::GetGlobalTimeDilation(this));
	const float UnscaledDelta = DeltaTime / Dilation;

	if (bMenuCameraActive)
	{
		PollCursorInput();
	}

	UpdateTurntable(UnscaledDelta);
	UpdateFraming(UnscaledDelta);
}

void UPHMenuCameraComponent::PollCursorInput()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	if (bAllowZoom && bPollMouseWheelForZoom)
	{
		if (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp))
		{
			AddZoomInput(ZoomStep);
		}
		else if (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown))
		{
			AddZoomInput(-ZoomStep);
		}
	}

	if (!bAllowTurntable || !bPollCursorForTurntable)
	{
		return;
	}

	FVector2D CursorPosition = FVector2D::ZeroVector;
	const bool bHasCursor = PC->GetMousePosition(CursorPosition.X, CursorPosition.Y);

	if (PC->IsInputKeyDown(TurntableDragKey) && bHasCursor)
	{
		if (bTurntableDragActive)
		{
			// Set TurntableSensitivity negative to flip the drag direction.
			AddTurntableInput((CursorPosition.X - LastCursorPosition.X) * TurntableSensitivity);
		}

		bTurntableDragActive = true;
		LastCursorPosition = CursorPosition;
	}
	else
	{
		bTurntableDragActive = false;
	}
}

// FRAMING

FPHMenuCameraView UPHMenuCameraComponent::ResolveTargetView() const
{
	FPHMenuCameraView View = DefaultView;

	if (const FPHMenuCameraView* PageView = PageViews.Find(ActiveMenuType))
	{
		View = *PageView;
	}

	if (const FPHMenuCameraFocus* Focus = SlotFocusOffsets.Find(FocusedEquipmentSlot))
	{
		View.PivotHeightOffset += Focus->PivotHeightOffset;
		View.Distance *= Focus->DistanceScale;
		View.YawOffset += Focus->YawOffset;
	}

	return View;
}

void UPHMenuCameraComponent::UpdateTurntable(const float DeltaTime)
{
	if (bBlendingOut)
	{
		// Unwind while the camera pulls away, so gameplay resumes on the facing
		// the player actually had.
		const float UnwindSpeed = BlendOutTime > KINDA_SMALL_NUMBER ? 3.0f / BlendOutTime : 20.0f;
		TurntableYaw = FMath::FInterpTo(TurntableYaw, 0.0f, DeltaTime, UnwindSpeed);
		TurntableVelocity = 0.0f;
	}
	else if (bAllowTurntable && !bTurntableDragActive && !FMath::IsNearlyZero(TurntableVelocity))
	{
		TurntableYaw += TurntableVelocity * DeltaTime;
		TurntableVelocity = FMath::FInterpTo(TurntableVelocity, 0.0f, DeltaTime, TurntableInertiaDamping);

		// Stop rather than creep for the rest of the session.
		if (FMath::Abs(TurntableVelocity) < 1.0f)
		{
			TurntableVelocity = 0.0f;
		}
	}

	const APHBaseCharacter* Character = TargetCharacter.Get();
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!Mesh)
	{
		return;
	}

	// The mesh spins, not the actor: rotating the actor would replicate, fight
	// ALS's rotation modes, and drag the camera pivot around with it.
	Mesh->SetRelativeRotation(FRotator(
		CachedMeshRelativeRotation.Pitch,
		CachedMeshRelativeRotation.Yaw + TurntableYaw,
		CachedMeshRelativeRotation.Roll));
}

void UPHMenuCameraComponent::UpdateFraming(const float DeltaTime)
{
	const APHBaseCharacter* Character = TargetCharacter.Get();
	if (!CameraRig || !Character)
	{
		return;
	}

	const FPHMenuCameraView TargetView = ResolveTargetView();

	if (DeltaTime > 0.0f)
	{
		CurrentView.Distance = FMath::FInterpTo(CurrentView.Distance, TargetView.Distance, DeltaTime, ViewInterpSpeed);
		CurrentView.PivotHeightOffset = FMath::FInterpTo(CurrentView.PivotHeightOffset, TargetView.PivotHeightOffset, DeltaTime, ViewInterpSpeed);
		CurrentView.YawOffset = FMath::FInterpTo(CurrentView.YawOffset, TargetView.YawOffset, DeltaTime, ViewInterpSpeed);
		CurrentView.Pitch = FMath::FInterpTo(CurrentView.Pitch, TargetView.Pitch, DeltaTime, ViewInterpSpeed);
		CurrentView.LateralOffset = FMath::FInterpTo(CurrentView.LateralOffset, TargetView.LateralOffset, DeltaTime, ViewInterpSpeed);
		CurrentView.FOV = FMath::FInterpTo(CurrentView.FOV, TargetView.FOV, DeltaTime, ViewInterpSpeed);

		ZoomMultiplier = FMath::FInterpTo(ZoomMultiplier, TargetZoomMultiplier, DeltaTime, ViewInterpSpeed * 1.5f);
	}
	else
	{
		CurrentView = TargetView;
		ZoomMultiplier = TargetZoomMultiplier;
	}

	// Built from the actor origin plus a height sampled once on open, rather
	// than from the head socket every frame - otherwise the turntable moves the
	// pivot it is being framed against.
	const FVector PivotLocation = Character->GetActorLocation() + FVector(0.0f, 0.0f, PivotHeightFromActor);

	// The rig looks down its own +X, so it sits at the pivot facing the
	// character and the camera hangs off the back of it.
	const FRotator RigRotation(0.0f, CharacterYawAtOpen + CurrentView.YawOffset + 180.0f, 0.0f);

	const float DesiredDistance = FMath::Max(20.0f, CurrentView.Distance * ZoomMultiplier);
	const FVector DesiredLocation = PivotLocation + RigRotation.RotateVector(
		FVector(-DesiredDistance, CurrentView.LateralOffset, CurrentView.PivotHeightOffset));

	const float FinalDistance = ResolveUnblockedDistance(PivotLocation, DesiredLocation, DesiredDistance);

	CameraRig->SetActorLocationAndRotation(PivotLocation, RigRotation);
	CameraRig->ApplyCameraOffset(FinalDistance, CurrentView.LateralOffset,
		CurrentView.PivotHeightOffset, CurrentView.Pitch, CurrentView.FOV);

	const FVector FinalLocation = PivotLocation + RigRotation.RotateVector(
		FVector(-FinalDistance, CurrentView.LateralOffset, CurrentView.PivotHeightOffset));
	CameraRig->ApplyDepthOfField(FVector::Dist(FinalLocation, PivotLocation));
}

float UPHMenuCameraComponent::ResolveUnblockedDistance(const FVector& PivotLocation,
	const FVector& DesiredLocation, const float DesiredDistance) const
{
	if (!bTraceForBlockingGeometry)
	{
		return DesiredDistance;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return DesiredDistance;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MenuCameraBlocking), false);
	if (const APHBaseCharacter* Character = TargetCharacter.Get())
	{
		Params.AddIgnoredActor(Character);
	}
	if (CameraRig)
	{
		Params.AddIgnoredActor(CameraRig);
	}

	FHitResult Hit;
	const bool bBlocked = World->SweepSingleByChannel(Hit, PivotLocation, DesiredLocation,
		FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(BlockingTraceRadius), Params);

	if (!bBlocked)
	{
		return DesiredDistance;
	}

	// The swept path is not purely along the boom, so pull the boom in by the
	// same proportion rather than using the hit distance directly.
	const float PathLength = FVector::Dist(PivotLocation, DesiredLocation);
	const float Alpha = PathLength > KINDA_SMALL_NUMBER
		? FVector::Dist(PivotLocation, Hit.Location) / PathLength
		: 1.0f;

	return FMath::Clamp(DesiredDistance * Alpha,
		FMath::Min(MinBlockedDistance, DesiredDistance), DesiredDistance);
}

// STATE

void UPHMenuCameraComponent::CacheCharacterState(APHBaseCharacter* Character)
{
	if (!Character)
	{
		return;
	}

	CharacterYawAtOpen = Character->GetActorRotation().Yaw;

	// ALS already knows where a good pivot is - reuse it, but store it as a
	// height above the actor so the turntable cannot move it later.
	const FTransform PivotTransform = Character->GetThirdPersonPivotTarget();
	PivotHeightFromActor = PivotTransform.GetLocation().Z - Character->GetActorLocation().Z;

	if (const USkeletalMeshComponent* Mesh = Character->GetMesh())
	{
		CachedMeshRelativeRotation = Mesh->GetRelativeRotation();
	}
}

void UPHMenuCameraComponent::RestoreCharacterState()
{
	if (const APHBaseCharacter* Character = TargetCharacter.Get())
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			Mesh->SetRelativeRotation(CachedMeshRelativeRotation);
		}
	}

	TurntableYaw = 0.0f;
	TurntableVelocity = 0.0f;
	bTurntableDragActive = false;
}

void UPHMenuCameraComponent::RestoreTimeDilation()
{
	if (!bTimeDilationApplied)
	{
		return;
	}

	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	bTimeDilationApplied = false;
}

void UPHMenuCameraComponent::ReseedAlsCamera(APHBaseCharacter* Character) const
{
	const APlayerController* PC = GetOwningPlayerController();
	if (!PC || !Character)
	{
		return;
	}

	// While the rig was the view target ALS never ran CustomCameraBehavior, so
	// SmoothedPivotTarget and TargetCameraLocation are stale and its FInterpTo
	// lag would visibly settle on the way back. OnPossess re-seeds them.
	if (AALSPlayerCameraManager* AlsCameraManager = Cast<AALSPlayerCameraManager>(PC->PlayerCameraManager))
	{
		AlsCameraManager->OnPossess(Character);
	}
}

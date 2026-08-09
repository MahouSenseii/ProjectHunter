#include "Interactable/Components/InteractableManager.h"

#include "UI/Interaction/InteractableWidget.h"
#include "Interactable/Helpers/InteractableWidgetPresentationHelper.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "Interactable/Library/FunctionLibraries/InteractionFunctionLibrary.h"
#include "Interactable/Library/Structs/InteractionStructs.h"
DEFINE_LOG_CATEGORY(LogInteractable);

UInteractableManager::UInteractableManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractableManager::BeginPlay()
{
	Super::BeginPlay();

	if (MeshesToHighlight.Num() == 0)
	{
		AutoFindMeshes();
	}

	if (bShowWidget && InteractionWidgetClass)
	{
		CreateWidgetComponent();
	}


}

void UInteractableManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopCameraFacingUpdates();

	Super::EndPlay(EndPlayReason);
}

void UInteractableManager::CreateWidgetComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogInteractable, Error, TEXT("InteractableManager: No owner actor!"));
		return;
	}

	WidgetComponent = NewObject<UWidgetComponent>(Owner, UWidgetComponent::StaticClass(), TEXT("InteractionWidget"));
	if (!WidgetComponent)
	{
		UE_LOG(LogInteractable, Error, TEXT("InteractableManager: Failed to create WidgetComponent!"));
		return;
	}

	WidgetComponent->RegisterComponent();

	WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	WidgetComponent->SetGeometryMode(EWidgetGeometryMode::Plane);
	WidgetComponent->SetBlendMode(EWidgetBlendMode::Transparent);
	WidgetComponent->SetTwoSided(false);
	// Tick mode stays Enabled, but we drive the component tick EXPLICITLY with
	// focus visibility (see OnBeginFocus/OnEndFocus). Do NOT use
	// ETickMode::Automatic here: a widget component that starts hidden
	// self-disables its tick, and SetVisibility(true) never re-enables it -
	// widget components only draw during tick, so the prompt never renders.
	// Explicit enable/disable gets the same perf win (hidden prompts cost
	// nothing) without the engine quirk.
	WidgetComponent->SetTickMode(ETickMode::Enabled);
	WidgetComponent->SetWindowFocusable(false);
	WidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));

	// The render target's PIXEL resolution equals DrawSize, and the quad's
	// WORLD size is DrawSize (cm) x component scale. Sharpness therefore comes
	// from rendering at DrawSize x ResolutionScale pixels and compensating the
	// component scale by 1/ResolutionScale so the world size stays identical.
	// (The old "high res" path skipped the compensation - it just made a
	// physically larger widget. And desired-size mode lets the engine shrink
	// the render target to the authored widget size every frame, which is the
	// classic source of blurry prompts.)
	if (bUseDesiredSize)
	{
		WidgetComponent->SetDrawSize(WidgetDrawSize);
		WidgetComponent->SetDrawAtDesiredSize(true);
	}
	else
	{
		const float Scale = FMath::Clamp(ResolutionScale, 0.5f, 4.0f);
		WidgetComponent->SetDrawSize(WidgetDrawSize * Scale);
		WidgetComponent->SetDrawAtDesiredSize(false);
		WidgetComponent->SetRelativeScale3D(FVector(1.0f / Scale));
	}

	WidgetComponent->SetWidgetClass(InteractionWidgetClass);
	WidgetComponent->SetRelativeLocation(WidgetOffset);
	WidgetComponent->AttachToComponent(
		Owner->GetRootComponent(),
		FAttachmentTransformRules::KeepRelativeTransform
	);

	WidgetComponent->InitWidget();
	WidgetComponent->RequestRedraw();
	WidgetComponent->SetBackgroundColor(FLinearColor::Transparent);

	// Start dormant: hidden AND not ticking. OnBeginFocus wakes both up.
	WidgetComponent->SetVisibility(false);
	WidgetComponent->SetComponentTickEnabled(false);

	UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Created high-quality widget for %s (Type: %s, CameraFacing: %s)"),
		*Owner->GetName(),
		*UEnum::GetValueAsString(Config.InteractionType),
		bAlwaysFaceCamera ? TEXT("Enabled") : TEXT("Disabled"));
}

void UInteractableManager::AutoFindMeshes()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UStaticMeshComponent*> StaticMeshes;
	Owner->GetComponents<UStaticMeshComponent>(StaticMeshes);
	for (UStaticMeshComponent* StaticMesh : StaticMeshes)
	{
		if (IsValid(StaticMesh))
		{
			MeshesToHighlight.AddUnique(StaticMesh);
		}
	}

	TArray<USkeletalMeshComponent*> SkeletalMeshes;
	Owner->GetComponents<USkeletalMeshComponent>(SkeletalMeshes);
	for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshes)
	{
		if (IsValid(SkeletalMesh))
		{
			MeshesToHighlight.AddUnique(SkeletalMesh);
		}
	}

	UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Auto-found %d meshes on %s"),
		MeshesToHighlight.Num(), *Owner->GetName());
}

void UInteractableManager::OnInteract_Implementation(AActor* Interactor)
{
	switch (Config.InteractionType)
	{
	case EInteractionType::IT_Tap:
		OnTapInteracted.Broadcast(Interactor);
		UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Tap interact on %s"), *GetOwner()->GetName());
		break;

	case EInteractionType::IT_Toggle:
		OnTapInteracted.Broadcast(Interactor);
		UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Toggle interact on %s"), *GetOwner()->GetName());
		break;

	case EInteractionType::IT_TapOrHold:
		OnTapInteracted.Broadcast(Interactor);
		UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Tap path used on tap-or-hold interactable %s"), *GetOwner()->GetName());
		break;

	default:
		UE_LOG(LogInteractable, Warning, TEXT("InteractableManager: OnInteract called on non-tap interaction type"));
		break;
	}
}

bool UInteractableManager::CanInteract_Implementation(AActor* Interactor) const
{
	return Config.bCanInteract;
}

void UInteractableManager::OnBeginFocus_Implementation(AActor* Interactor)
{
	// Refcounted focus: visuals turn ON with the FIRST focuser only.
	// CurrentInteractor always tracks the most recent one (camera facing).
	const int32 LiveFocusersBefore = CompactFocusingInteractors();
	if (Interactor)
	{
		FocusingInteractors.AddUnique(Interactor);
	}
	CurrentInteractor = Interactor;

	const bool bFirstFocuser = (LiveFocusersBefore == 0);

	if (bEnableHighlight && bFirstFocuser)
	{
		ApplyHighlight(true);
	}

	if (WidgetComponent)
	{
		// Wake the component: tick must come back on with visibility - widget
		// components only draw during tick (see CreateWidgetComponent note).
		WidgetComponent->SetComponentTickEnabled(true);
		WidgetComponent->SetVisibility(true);

		if (UInteractableWidget* Widget = Cast<UInteractableWidget>(WidgetComponent->GetWidget()))
		{
			UpdateWidgetText();
			FInteractableWidgetPresentationHelper::SetIdle(*Widget, false);
			WidgetComponent->RequestRedraw();
		}

		if (bAlwaysFaceCamera && Interactor)
		{
			UpdateWidgetRotationToFaceCamera(Interactor, 0.0f);

			if (CameraFacingUpdateRate > 0.0f)
			{
				StartCameraFacingUpdates();
			}
		}
	}

	OnFocusBegin.Broadcast(Interactor);

	UE_LOG(LogInteractable, Verbose, TEXT("InteractableManager: Begin focus on %s (Type: %s, Focusers: %d)"),
		*GetOwner()->GetName(), *UEnum::GetValueAsString(Config.InteractionType), FocusingInteractors.Num());
}

void UInteractableManager::OnEndFocus_Implementation(AActor* Interactor)
{
	// Refcounted focus: visuals turn OFF only when the LAST focuser leaves.
	FocusingInteractors.RemoveAll([Interactor](const TWeakObjectPtr<AActor>& Weak)
	{
		return !Weak.IsValid() || Weak.Get() == Interactor;
	});
	const int32 LiveFocusers = CompactFocusingInteractors();

	if (LiveFocusers > 0)
	{
		// Someone is still looking - retarget camera facing to a remaining focuser.
		if (CurrentInteractor == Interactor)
		{
			CurrentInteractor = FocusingInteractors.Last().Get();
		}

		OnFocusEnd.Broadcast(Interactor);
		return;
	}

	CurrentInteractor = nullptr;

	StopCameraFacingUpdates();

	if (bEnableHighlight)
	{
		ApplyHighlight(false);
	}

	if (WidgetComponent)
	{
		if (UInteractableWidget* Widget = Cast<UInteractableWidget>(WidgetComponent->GetWidget()))
		{
			FInteractableWidgetPresentationHelper::SetIdle(*Widget, true);
		}

		// Back to dormant: hidden prompts shouldn't tick (perf with many
		// interactables); OnBeginFocus re-enables both.
		WidgetComponent->SetVisibility(false);
		WidgetComponent->SetComponentTickEnabled(false);
	}

	OnFocusEnd.Broadcast(Interactor);

	UE_LOG(LogInteractable, Verbose, TEXT("InteractableManager: End focus on %s"), *GetOwner()->GetName());
}

int32 UInteractableManager::CompactFocusingInteractors()
{
	FocusingInteractors.RemoveAll([](const TWeakObjectPtr<AActor>& Weak)
	{
		return !Weak.IsValid();
	});
	return FocusingInteractors.Num();
}

EInteractionType UInteractableManager::GetInteractionType_Implementation() const
{
	return Config.InteractionType;
}

UInputAction* UInteractableManager::GetInputAction_Implementation() const
{
	return Config.InputAction;
}

FText UInteractableManager::GetInteractionText_Implementation() const
{
	return UInteractionFunctionLibrary::GetDisplayTextForConfig(Config);
}

FVector UInteractableManager::GetWidgetOffset_Implementation() const
{
	return WidgetOffset;
}

FInteractableHighlightStyle UInteractableManager::GetHighlightStyle_Implementation() const
{
	return UInteractionFunctionLibrary::MakeHighlightStyle(
		bEnableHighlight,
		HighlightStencilValue,
		HighlightColor,
		HighlightWidth,
		HighlightThreshold);
}

float UInteractableManager::GetTapHoldThreshold_Implementation() const
{
	return Config.InteractionType == EInteractionType::IT_Hold ? 0.0f : Config.TapHoldThreshold;
}

float UInteractableManager::GetHoldDuration_Implementation() const
{
	return Config.HoldDuration;
}

void UInteractableManager::OnHoldInteractionStart_Implementation(AActor* Interactor)
{
	if (UInteractableWidget* Widget = WidgetComponent ? Cast<UInteractableWidget>(WidgetComponent->GetWidget()) : nullptr)
	{
		FInteractableWidgetPresentationHelper::StartProgress(*Widget, EInteractionWidgetState::IWS_Holding);
	}

	SetProgressBarVisible(true);

	if (bAlwaysFaceCamera && Interactor)
	{
		UpdateWidgetRotationToFaceCamera(Interactor, 0.0f);
	}

	UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Hold start on %s"), *GetOwner()->GetName());
}

void UInteractableManager::OnHoldInteractionUpdate_Implementation(AActor* Interactor, float Progress)
{
	UpdateProgress(Progress, false);

	if (bAlwaysFaceCamera && CameraFacingUpdateRate <= 0.0f && Interactor)
	{
		UpdateWidgetRotationToFaceCamera(Interactor, 0.0f);
	}
}

void UInteractableManager::OnHoldInteractionComplete_Implementation(AActor* Interactor)
{
	if (UInteractableWidget* Widget = WidgetComponent ? Cast<UInteractableWidget>(WidgetComponent->GetWidget()) : nullptr)
	{
		FInteractableWidgetPresentationHelper::SetCompleted(*Widget);
	}

	OnHoldCompleted.Broadcast(Interactor);
	UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Hold completed on %s"), *GetOwner()->GetName());
}

void UInteractableManager::OnHoldInteractionCancelled_Implementation(AActor* Interactor)
{
	if (UInteractableWidget* Widget = WidgetComponent ? Cast<UInteractableWidget>(WidgetComponent->GetWidget()) : nullptr)
	{
		FInteractableWidgetPresentationHelper::SetCancelled(*Widget, true);
	}

	OnHoldCancelled.Broadcast(Interactor);
	UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Hold cancelled on %s"), *GetOwner()->GetName());
}

FText UInteractableManager::GetHoldInteractionText_Implementation() const
{
	return Config.HoldText;
}

int32 UInteractableManager::GetRequiredMashCount_Implementation() const
{
	return Config.RequiredMashCount;
}

float UInteractableManager::GetMashDecayRate_Implementation() const
{
	return Config.MashDecayRate;
}

void UInteractableManager::OnMashInteractionStart_Implementation(AActor* Interactor)
{
	if (UInteractableWidget* Widget = WidgetComponent ? Cast<UInteractableWidget>(WidgetComponent->GetWidget()) : nullptr)
	{
		FInteractableWidgetPresentationHelper::StartProgress(*Widget, EInteractionWidgetState::IWS_Mashing);
	}

	SetProgressBarVisible(true);
	OnMashProgress.Broadcast(Interactor, 0, Config.RequiredMashCount);

	if (bAlwaysFaceCamera && Interactor)
	{
		UpdateWidgetRotationToFaceCamera(Interactor, 0.0f);
	}

	UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Mash start on %s"), *GetOwner()->GetName());
}

void UInteractableManager::OnMashInteractionUpdate_Implementation(AActor* Interactor, int32 CurrentCount, int32 RequiredCount, float Progress)
{
	UpdateProgress(Progress, false);
	OnMashProgress.Broadcast(Interactor, CurrentCount, RequiredCount);

	if (bAlwaysFaceCamera && CameraFacingUpdateRate <= 0.0f && Interactor)
	{
		UpdateWidgetRotationToFaceCamera(Interactor, 0.0f);
	}

	UE_LOG(LogInteractable, Verbose, TEXT("InteractableManager: Mash progress %d/%d (%.1f%%)"),
		CurrentCount, RequiredCount, Progress * 100.0f);
}

void UInteractableManager::OnMashInteractionComplete_Implementation(AActor* Interactor)
{
	if (UInteractableWidget* Widget = WidgetComponent ? Cast<UInteractableWidget>(WidgetComponent->GetWidget()) : nullptr)
	{
		FInteractableWidgetPresentationHelper::SetCompleted(*Widget);
	}

	OnMashCompleted.Broadcast(Interactor);
	UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Mash completed on %s"), *GetOwner()->GetName());
}

void UInteractableManager::OnMashInteractionFailed_Implementation(AActor* Interactor)
{
	if (UInteractableWidget* Widget = WidgetComponent ? Cast<UInteractableWidget>(WidgetComponent->GetWidget()) : nullptr)
	{
		FInteractableWidgetPresentationHelper::SetCancelled(*Widget, false);
	}

	OnMashFailed.Broadcast(Interactor);
	UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Mash failed on %s"), *GetOwner()->GetName());
}

FText UInteractableManager::GetMashInteractionText_Implementation() const
{
	return Config.MashText;
}

void UInteractableManager::OnContinuousInteractionStart_Implementation(AActor* Interactor)
{
	SetProgressBarVisible(false);

	if (bAlwaysFaceCamera && Interactor)
	{
		UpdateWidgetRotationToFaceCamera(Interactor, 0.0f);
	}

	UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Continuous interaction started on %s"), *GetOwner()->GetName());
}

void UInteractableManager::OnContinuousInteractionUpdate_Implementation(AActor* Interactor, float HeldSeconds)
{
	if (bAlwaysFaceCamera && CameraFacingUpdateRate <= 0.0f && Interactor)
	{
		UpdateWidgetRotationToFaceCamera(Interactor, 0.0f);
	}

	UE_LOG(LogInteractable, Verbose, TEXT("InteractableManager: Continuous interaction update on %s (Held: %.2fs)"),
		*GetOwner()->GetName(), HeldSeconds);
}

void UInteractableManager::OnContinuousInteractionEnd_Implementation(AActor* Interactor)
{
	SetProgressBarVisible(false);
	UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Continuous interaction ended on %s"), *GetOwner()->GetName());
}

bool UInteractableManager::HasTooltip_Implementation() const
{
	return false;
}

UObject* UInteractableManager::GetTooltipData_Implementation() const
{
	return nullptr;
}

FVector UInteractableManager::GetTooltipWorldLocation_Implementation() const
{
	return GetOwner()->GetActorLocation() + WidgetOffset;
}

void UInteractableManager::UpdateProgress(float Progress, bool bIsDepleting)
{
	if (!WidgetComponent)
	{
		return;
	}

	if (UInteractableWidget* Widget = Cast<UInteractableWidget>(WidgetComponent->GetWidget()))
	{
		if (!UInteractionFunctionLibrary::SupportsProgressBar(Config.InteractionType))
		{
			Widget->SetProgressBarVisible(false);
			return;
		}

		const EInteractionWidgetState DesiredState =
			UInteractionFunctionLibrary::GetProgressWidgetState(Config.InteractionType);
		FInteractableWidgetPresentationHelper::UpdateProgress(*Widget, DesiredState, Progress);
	}
}

void UInteractableManager::SetProgressBarVisible(bool bVisible)
{
	if (!WidgetComponent)
	{
		return;
	}

	if (UInteractableWidget* Widget = Cast<UInteractableWidget>(WidgetComponent->GetWidget()))
	{
		const bool bShouldShow = bVisible && UInteractionFunctionLibrary::SupportsProgressBar(Config.InteractionType);

		if (!bShouldShow)
		{
			FInteractableWidgetPresentationHelper::SetIdle(*Widget, true);
			return;
		}

		Widget->SetProgressBarVisible(true);
	}
}

void UInteractableManager::SetCameraFacingEnabled(bool bEnabled)
{
	bAlwaysFaceCamera = bEnabled;

	if (bEnabled && CurrentInteractor && WidgetComponent && WidgetComponent->IsVisible())
	{
		UpdateWidgetRotationToFaceCamera(CurrentInteractor, 0.0f);

		if (CameraFacingUpdateRate > 0.0f)
		{
			StartCameraFacingUpdates();
		}
	}
	else
	{
		StopCameraFacingUpdates();
	}
}

void UInteractableManager::UpdateWidgetRotationToFaceCamera(AActor* Interactor, float DeltaTime)
{
	if (!WidgetComponent || !Interactor)
	{
		return;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	if (!UInteractionFunctionLibrary::GetInteractorView(Interactor, CameraLocation, CameraRotation))
	{
		return;
	}

	const FRotator NewRotation = UInteractionFunctionLibrary::CalculateCameraFacingRotation(
		WidgetComponent->GetComponentLocation(),
		CameraLocation,
		WidgetComponent->GetComponentRotation(),
		DeltaTime,
		RotationSmoothSpeed);

	WidgetComponent->SetWorldRotation(NewRotation);
}

void UInteractableManager::StartCameraFacingUpdates()
{
	if (!bAlwaysFaceCamera || CameraFacingUpdateRate <= 0.0f)
	{
		return;
	}

	StopCameraFacingUpdates();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CameraFacingTimerHandle,
			this,
			&UInteractableManager::UpdateCameraFacingTimer,
			CameraFacingUpdateRate,
			true
		);

		UE_LOG(LogInteractable, Verbose, TEXT("InteractableManager: Started camera-facing timer (Rate: %.3fs)"),
			CameraFacingUpdateRate);
	}
}

void UInteractableManager::StopCameraFacingUpdates()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CameraFacingTimerHandle);
	}
}

void UInteractableManager::UpdateCameraFacingTimer()
{
	if (CurrentInteractor && WidgetComponent && WidgetComponent->IsVisible())
	{
		UpdateWidgetRotationToFaceCamera(CurrentInteractor, CameraFacingUpdateRate);
	}
	else
	{
		StopCameraFacingUpdates();
	}
}

void UInteractableManager::UpdateWidgetText()
{
	if (!WidgetComponent)
	{
		return;
	}

	UInteractableWidget* Widget = Cast<UInteractableWidget>(WidgetComponent->GetWidget());
	if (!Widget)
	{
		return;
	}

	if (!Config.InputAction)
	{
		UE_LOG(LogInteractable, Error, TEXT("InteractableManager: InputAction not set on %s! Widget will not show key icon."),
			*GetOwner()->GetName());
	}

	Widget->SetInteractionData(Config.InputAction, UInteractionFunctionLibrary::GetDisplayTextForConfig(Config));
}

void UInteractableManager::ApplyHighlight(bool bHighlight)
{
	for (const TObjectPtr<UPrimitiveComponent>& MeshPtr : MeshesToHighlight)
	{
		UPrimitiveComponent* Mesh = MeshPtr.Get();
		if (!Mesh)
		{
			continue;
		}

		if (bHighlight)
		{
			Mesh->SetRenderCustomDepth(true);
			Mesh->SetCustomDepthStencilValue(HighlightStencilValue);
		}
		else
		{
			Mesh->SetRenderCustomDepth(false);
		}
	}
}

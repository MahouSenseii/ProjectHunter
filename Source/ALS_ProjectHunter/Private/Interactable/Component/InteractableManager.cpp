// Interactable/Component/InteractableManager.cpp

#include "Interactable/Component/InteractableManager.h"

#include "EngineUtils.h"
#include "Interactable/Widget/InteractableWidget.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Interactable/Library/InteractionStructLibrary.h"
DEFINE_LOG_CATEGORY(LogInteractable);

UInteractableManager::UInteractableManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractableManager::BeginPlay()
{
	Super::BeginPlay();

	// Auto-find meshes if none specified
	if (MeshesToHighlight.Num() == 0)
	{
		AutoFindMeshes();
	}

	// Create widget component
	if (bShowWidget && InteractionWidgetClass)
	{
		CreateWidgetComponent();
	}
	

}

void UInteractableManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up timer
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

	// Create widget component
	WidgetComponent = NewObject<UWidgetComponent>(Owner, UWidgetComponent::StaticClass(), TEXT("InteractionWidget"));
	if (!WidgetComponent)
	{
		UE_LOG(LogInteractable, Error, TEXT("InteractableManager: Failed to create WidgetComponent!"));
		return;
	}

	// Register component FIRST
	WidgetComponent->RegisterComponent();
	
	// ═══════════════════════════════════════════════════════════
	// CRITICAL QUALITY SETTINGS (Set BEFORE widget class!)
	// ═══════════════════════════════════════════════════════════
	
	// Set world space (we'll handle camera-facing manually)
	WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	
	// Geometry settings BEFORE widget creation
	WidgetComponent->SetGeometryMode(EWidgetGeometryMode::Plane);
	WidgetComponent->SetBlendMode(EWidgetBlendMode::Transparent);
	WidgetComponent->SetTwoSided(false);
	
	// Enable ticking
	WidgetComponent->SetTickMode(ETickMode::Enabled);
	WidgetComponent->SetWindowFocusable(false);
	
	// Set pivot BEFORE setting draw size
	WidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));
	
	// ═══════════════════════════════════════════════════════════
	// RESOLUTION SETTINGS
	// ═══════════════════════════════════════════════════════════
	
	if (bUseDesiredSize)
	{
		// METHOD 1: Use desired size (BEST for quality)
		WidgetComponent->SetDrawSize(WidgetDrawSize); // Physical world size
		WidgetComponent->SetDrawAtDesiredSize(true);  // Render at widget's native resolution
	}
	else
	{
		// METHOD 2: Manual high-resolution render target
		FVector2D HighResSize = WidgetDrawSize * ResolutionScale;
		WidgetComponent->SetDrawSize(HighResSize);
		WidgetComponent->SetDrawAtDesiredSize(false);
	}
	
	// ═══════════════════════════════════════════════════════════
	// SET WIDGET CLASS AND INITIALIZE
	// ═══════════════════════════════════════════════════════════
	
	// NOW set the widget class (after all quality settings)
	WidgetComponent->SetWidgetClass(InteractionWidgetClass);
	
	// Set position
	WidgetComponent->SetRelativeLocation(WidgetOffset);
	
	// Attach to root
	WidgetComponent->AttachToComponent(
		Owner->GetRootComponent(),
		FAttachmentTransformRules::KeepRelativeTransform
	);
	
	// CRITICAL: Initialize widget to apply all settings
	WidgetComponent->InitWidget();
	
	// Force redraw to ensure quality settings are applied
	WidgetComponent->RequestRedraw();
	
	// ═══════════════════════════════════════════════════════════
	// FINAL SETTINGS
	// ═══════════════════════════════════════════════════════════
	
	// Background
	WidgetComponent->SetBackgroundColor(FLinearColor::Transparent);
	
	// Hidden by default
	WidgetComponent->SetVisibility(false);

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

// ═══════════════════════════════════════════════════════════════════════
// INTERACTABLE INTERFACE - BASIC
// ═══════════════════════════════════════════════════════════════════════

void UInteractableManager::OnInteract_Implementation(AActor* Interactor)
{
	// For Tap or simple interactions
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
		// Hold/Mash interactions handled by their own functions
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
	// Store current interactor for camera-facing
	CurrentInteractor = Interactor;

	// Apply highlight
	if (bEnableHighlight)
	{
		ApplyHighlight(true);
	}

	// Show widget
	if (WidgetComponent)
	{
		WidgetComponent->SetVisibility(true);

		// Update widget with interaction-type-specific text
		if (UInteractableWidget* Widget = Cast<UInteractableWidget>(WidgetComponent->GetWidget()))
		{
			UpdateWidgetText();
			Widget->SetWidgetState(EInteractionWidgetState::IWS_Idle);
			Widget->SetProgressBarVisible(false);
			
			// CRITICAL: Force widget update to ensure crisp rendering
			WidgetComponent->RequestRedraw();
		}

		// Face camera immediately
		if (bAlwaysFaceCamera && Interactor)
		{
			UpdateWidgetRotationToFaceCamera(Interactor, 0.0f); // Instant snap on focus
			
			// Start continuous updates if update rate > 0
			if (CameraFacingUpdateRate > 0.0f)
			{
				StartCameraFacingUpdates();
			}
		}
	}

	// Broadcast event
	OnFocusBegin.Broadcast(Interactor);

	UE_LOG(LogInteractable, Verbose, TEXT("InteractableManager: Begin focus on %s (Type: %s)"), 
		*GetOwner()->GetName(), *UEnum::GetValueAsString(Config.InteractionType));
}

void UInteractableManager::OnEndFocus_Implementation(AActor* Interactor)
{
	// Clear current interactor
	CurrentInteractor = nullptr;

	// Stop camera-facing updates
	StopCameraFacingUpdates();

	// Remove highlight
	if (bEnableHighlight)
	{
		ApplyHighlight(false);
	}

	// Hide widget
	if (WidgetComponent)
	{
		if (UInteractableWidget* Widget = Cast<UInteractableWidget>(WidgetComponent->GetWidget()))
		{
			Widget->SetWidgetState(EInteractionWidgetState::IWS_Idle);
			Widget->SetProgressBarVisible(false);
			Widget->SetProgress(0.0f);
		}

		WidgetComponent->SetVisibility(false);
	}

	// Broadcast event
	OnFocusEnd.Broadcast(Interactor);

	UE_LOG(LogInteractable, Verbose, TEXT("InteractableManager: End focus on %s"), *GetOwner()->GetName());
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
	return GetDisplayTextForCurrentType();
}

FVector UInteractableManager::GetWidgetOffset_Implementation() const
{
	return WidgetOffset;
}

FInteractableHighlightStyle UInteractableManager::GetHighlightStyle_Implementation() const
{
	FInteractableHighlightStyle Style;
	Style.bEnableHighlight = bEnableHighlight;
	Style.StencilValue = HighlightStencilValue;
	Style.Color = HighlightColor;
	Style.OutlineWidth = HighlightWidth;
	Style.Threshold = HighlightThreshold;
	return Style;
}

// ═══════════════════════════════════════════════════════════════════════
// INTERACTABLE INTERFACE - HOLD INTERACTION
// ═══════════════════════════════════════════════════════════════════════

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
		Widget->SetWidgetState(EInteractionWidgetState::IWS_Holding);
		Widget->SetProgress(0.0f);
	}

	SetProgressBarVisible(true);
	
	// Update camera facing for progress bar visibility
	if (bAlwaysFaceCamera && Interactor)
	{
		UpdateWidgetRotationToFaceCamera(Interactor, 0.0f);
	}
	
	UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Hold start on %s"), *GetOwner()->GetName());
}

void UInteractableManager::OnHoldInteractionUpdate_Implementation(AActor* Interactor, float Progress)
{
	UpdateProgress(Progress, false);
	
	// Update camera facing during hold (only if no continuous timer)
	if (bAlwaysFaceCamera && CameraFacingUpdateRate <= 0.0f && Interactor)
	{
		UpdateWidgetRotationToFaceCamera(Interactor, 0.0f);
	}
}

void UInteractableManager::OnHoldInteractionComplete_Implementation(AActor* Interactor)
{
	if (UInteractableWidget* Widget = WidgetComponent ? Cast<UInteractableWidget>(WidgetComponent->GetWidget()) : nullptr)
	{
		Widget->SetWidgetState(EInteractionWidgetState::IWS_Completed);
		Widget->SetProgressBarVisible(true);
		Widget->SetProgress(1.0f);
	}

	OnHoldCompleted.Broadcast(Interactor);
	UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Hold completed on %s"), *GetOwner()->GetName());
}

void UInteractableManager::OnHoldInteractionCancelled_Implementation(AActor* Interactor)
{
	if (UInteractableWidget* Widget = WidgetComponent ? Cast<UInteractableWidget>(WidgetComponent->GetWidget()) : nullptr)
	{
		Widget->SetWidgetState(EInteractionWidgetState::IWS_Cancelled);
		Widget->SetProgressBarVisible(true);
		Widget->SetProgress(0.0f);
	}

	OnHoldCancelled.Broadcast(Interactor);
	UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Hold cancelled on %s"), *GetOwner()->GetName());
}

FText UInteractableManager::GetHoldInteractionText_Implementation() const
{
	return Config.HoldText;
}

// ═══════════════════════════════════════════════════════════════════════
// INTERACTABLE INTERFACE - MASH INTERACTION
// ═══════════════════════════════════════════════════════════════════════

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
		Widget->SetWidgetState(EInteractionWidgetState::IWS_Mashing);
		Widget->SetProgress(0.0f);
	}

	SetProgressBarVisible(true);
	OnMashProgress.Broadcast(Interactor, 0, Config.RequiredMashCount);
	
	// Update camera facing for progress bar visibility
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
	
	// Update camera facing during mash (only if no continuous timer)
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
		Widget->SetWidgetState(EInteractionWidgetState::IWS_Completed);
		Widget->SetProgressBarVisible(true);
		Widget->SetProgress(1.0f);
	}

	OnMashCompleted.Broadcast(Interactor);
	UE_LOG(LogInteractable, Log, TEXT("InteractableManager: Mash completed on %s"), *GetOwner()->GetName());
}

void UInteractableManager::OnMashInteractionFailed_Implementation(AActor* Interactor)
{
	if (UInteractableWidget* Widget = WidgetComponent ? Cast<UInteractableWidget>(WidgetComponent->GetWidget()) : nullptr)
	{
		Widget->SetWidgetState(EInteractionWidgetState::IWS_Cancelled);
		Widget->SetProgressBarVisible(true);
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

// ═══════════════════════════════════════════════════════════════════════
// INTERACTABLE INTERFACE - TOOLTIP
// ═══════════════════════════════════════════════════════════════════════

bool UInteractableManager::HasTooltip_Implementation() const
{
	// Can be overridden in Blueprint
	return false;
}

UObject* UInteractableManager::GetTooltipData_Implementation() const
{
	// Can be overridden in Blueprint
	return nullptr;
}

FVector UInteractableManager::GetTooltipWorldLocation_Implementation() const
{
	return GetOwner()->GetActorLocation() + WidgetOffset;
}

// ═══════════════════════════════════════════════════════════════════════
// PROGRESS BAR SUPPORT
// ═══════════════════════════════════════════════════════════════════════

void UInteractableManager::UpdateProgress(float Progress, bool bIsDepleting)
{
	if (!WidgetComponent)
	{
		return;
	}

	if (UInteractableWidget* Widget = Cast<UInteractableWidget>(WidgetComponent->GetWidget()))
	{
		if (!SupportsProgressBar())
		{
			Widget->SetProgressBarVisible(false);
			return;
		}

		const float ClampedProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
		const EInteractionWidgetState DesiredState =
			Config.InteractionType == EInteractionType::IT_Mash
				? EInteractionWidgetState::IWS_Mashing
				: EInteractionWidgetState::IWS_Holding;

		if (Widget->GetWidgetState() != DesiredState)
		{
			Widget->SetWidgetState(DesiredState);
		}

		Widget->SetProgress(ClampedProgress);
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
		const bool bShouldShow = bVisible && SupportsProgressBar();

		if (!bShouldShow)
		{
			Widget->SetWidgetState(EInteractionWidgetState::IWS_Idle);
			Widget->SetProgressBarVisible(false);
			Widget->SetProgress(0.0f);
			return;
		}

		Widget->SetProgressBarVisible(true);
	}
}

// ═══════════════════════════════════════════════════════════════════════
// BLUEPRINT CALLABLE
// ═══════════════════════════════════════════════════════════════════════

void UInteractableManager::SetCameraFacingEnabled(bool bEnabled)
{
	bAlwaysFaceCamera = bEnabled;
	
	if (bEnabled && CurrentInteractor && WidgetComponent && WidgetComponent->IsVisible())
	{
		// Face camera immediately
		UpdateWidgetRotationToFaceCamera(CurrentInteractor, 0.0f);
		
		// Start continuous updates if needed
		if (CameraFacingUpdateRate > 0.0f)
		{
			StartCameraFacingUpdates();
		}
	}
	else
	{
		// Stop updates
		StopCameraFacingUpdates();
	}
}

// ═══════════════════════════════════════════════════════════════════════
// CAMERA-FACING LOGIC (SINGLE RESPONSIBILITY: Widget rotation)
// ═══════════════════════════════════════════════════════════════════════

void UInteractableManager::UpdateWidgetRotationToFaceCamera(AActor* Interactor, float DeltaTime)
{
	if (!WidgetComponent || !Interactor)
	{
		return;
	}

	// Get camera location and rotation
	FVector CameraLocation;
	FRotator CameraRotation;
	if (!GetInteractorCamera(Interactor, CameraLocation, CameraRotation))
	{
		return;
	}

	// Calculate direction from widget to camera
	const FVector WidgetLocation = WidgetComponent->GetComponentLocation();
	const FVector DirectionToCamera = (CameraLocation - WidgetLocation).GetSafeNormal();

	// Calculate target rotation (face camera)
	FRotator TargetRotation = DirectionToCamera.Rotation();
	
	// Get current rotation
	FRotator CurrentRotation = WidgetComponent->GetComponentRotation();

	// Apply rotation (smooth or instant)
	FRotator NewRotation;
	
	if (RotationSmoothSpeed > 0.0f && DeltaTime > 0.0f)
	{
		// Smooth interpolation
		NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationSmoothSpeed);
	}
	else
	{
		// Instant snap
		NewRotation = TargetRotation;
	}

	// Set world rotation
	WidgetComponent->SetWorldRotation(NewRotation);
}

bool UInteractableManager::GetInteractorCamera(AActor* Interactor, FVector& OutCameraLocation, FRotator& OutCameraRotation) const
{
	if (!Interactor)
	{
		return false;
	}

	// ─────────────────────────────────────────────────────────────────────
	// METHOD 1: Try to get from player controller (most common)
	// ─────────────────────────────────────────────────────────────────────
	
	if (APlayerController* PC = Cast<APlayerController>(Interactor))
	{
		PC->GetPlayerViewPoint(OutCameraLocation, OutCameraRotation);
		return true;
	}

	// ─────────────────────────────────────────────────────────────────────
	// METHOD 2: Try to get from character's controller
	// ─────────────────────────────────────────────────────────────────────
	
	if (APawn* Pawn = Cast<APawn>(Interactor))
	{
		if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			PC->GetPlayerViewPoint(OutCameraLocation, OutCameraRotation);
			return true;
		}
	}

	// ─────────────────────────────────────────────────────────────────────
	// METHOD 3: Try to find camera component on interactor
	// ─────────────────────────────────────────────────────────────────────
	
	if (UCameraComponent* CameraComp = Interactor->FindComponentByClass<UCameraComponent>())
	{
		OutCameraLocation = CameraComp->GetComponentLocation();
		OutCameraRotation = CameraComp->GetComponentRotation();
		return true;
	}

	// ─────────────────────────────────────────────────────────────────────
	// METHOD 4: Fallback to actor location/rotation
	// ─────────────────────────────────────────────────────────────────────
	
	OutCameraLocation = Interactor->GetActorLocation();
	OutCameraRotation = Interactor->GetActorRotation();
	return true;
}

void UInteractableManager::StartCameraFacingUpdates()
{
	if (!bAlwaysFaceCamera || CameraFacingUpdateRate <= 0.0f)
	{
		return;
	}

	// Clear existing timer
	StopCameraFacingUpdates();

	// Start new timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CameraFacingTimerHandle,
			this,
			&UInteractableManager::UpdateCameraFacingTimer,
			CameraFacingUpdateRate,
			true // Looping
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
		// Stop timer if no longer needed
		StopCameraFacingUpdates();
	}
}

// ═══════════════════════════════════════════════════════════════════════
// HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════

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

	Widget->SetInteractionData(Config.InputAction, GetDisplayTextForCurrentType());
}

FText UInteractableManager::GetDisplayTextForCurrentType() const
{
	switch (Config.InteractionType)
	{
	case EInteractionType::IT_Tap:
		return Config.InteractionText;
			
	case EInteractionType::IT_Hold:
		return Config.HoldText;
			
	case EInteractionType::IT_Mash:
		return Config.MashText;
			
	case EInteractionType::IT_TapOrHold:
		// Show both options on separate lines
		return FText::Format(
			FText::FromString("{0}\n{1}"),
			Config.TapText,
			Config.HoldActionText
		);
			
	case EInteractionType::IT_Toggle:
	case EInteractionType::IT_Continuous:
	default:
		return Config.InteractionText;
	}
}

bool UInteractableManager::SupportsProgressBar() const
{
	return Config.InteractionType == EInteractionType::IT_Hold
		|| Config.InteractionType == EInteractionType::IT_TapOrHold
		|| Config.InteractionType == EInteractionType::IT_Mash;
}

// ═══════════════════════════════════════════════════════════════════════
// HIGHLIGHT
// ═══════════════════════════════════════════════════════════════════════

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



// Character/Component/Interaction/InteractionWidgetPresenter.cpp
// PH-4.3 — Widget + outline presentation extracted from UInteractionManager.

#include "Character/Component/Interaction/InteractionWidgetPresenter.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/ActorComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interactable/Interface/Interactable.h"
#include "Interactable/Widget/InteractableWidget.h"
#include "Interactable/Library/InteractionStructLibrary.h"
#include "Item/ItemInstance.h"
#include "Tower/Subsystem/GroundItemSubsystem.h"

DEFINE_LOG_CATEGORY(LogInteractionWidgetPresenter);

// ═══════════════════════════════════════════════════════════════════════
// CONSTRUCTOR
// ═══════════════════════════════════════════════════════════════════════

FInteractionWidgetPresenter::FInteractionWidgetPresenter()
{
	// All members default-constructed; Initialize() must be called before use.
}

// ═══════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════

void FInteractionWidgetPresenter::Initialize(UActorComponent* InOwnerComponent, UWorld* InWorld)
{
	OwnerComponent = InOwnerComponent;
	WorldContext   = InWorld;

	AActor* Owner = InOwnerComponent ? InOwnerComponent->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(Owner);
	if (!OwnerPawn)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC)
	{
		UE_LOG(LogInteractionWidgetPresenter, Warning,
			TEXT("FInteractionWidgetPresenter::Initialize — no PlayerController on '%s'; widget deferred."),
			*Owner->GetName());
		return;
	}

	// ── SCREEN-SPACE HUD WIDGET ──────────────────────────────────────────────
	// Overlay for hold-progress bars, cancellation feedback, and key prompts.
	// Shown for both actor interactables and ground items.
	if (InteractionWidgetClass)
	{
		InteractionWidget = CreateWidget<UInteractableWidget>(PC, InteractionWidgetClass);
		if (InteractionWidget)
		{
			// Alignment (0.5, 0.5) → SetPositionInViewport places the widget's
			// center at the given screen point (used by PositionWidgetAtGroundItem).
			InteractionWidget->AddToViewport(WidgetZOrder);
			InteractionWidget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
			InteractionWidget->Hide();
		}
		else
		{
			UE_LOG(LogInteractionWidgetPresenter, Error,
				TEXT("FInteractionWidgetPresenter::Initialize — failed to create screen HUD widget on '%s'"),
				*Owner->GetName());
		}
	}
	else
	{
		UE_LOG(LogInteractionWidgetPresenter, Warning,
			TEXT("FInteractionWidgetPresenter::Initialize — InteractionWidgetClass not set; screen HUD widget disabled."));
	}

	// ── WORLD-SPACE FLOATING WIDGET FOR GROUND ITEMS ─────────────────────────
	// Mirrors the UWidgetComponent on actor interactables (loot chests, portals).
	// Uses GroundItemWorldWidgetClass if assigned; falls back to InteractionWidgetClass.
	const TSubclassOf<UUserWidget> GroundWidgetClass =
		GroundItemWorldWidgetClass
			? TSubclassOf<UUserWidget>(GroundItemWorldWidgetClass)
			: TSubclassOf<UUserWidget>(InteractionWidgetClass);

	if (GroundWidgetClass)
	{
		GroundItemWorldWidget = NewObject<UWidgetComponent>(
			Owner,
			UWidgetComponent::StaticClass(),
			TEXT("GroundItemFloatingWidget"));

		if (GroundItemWorldWidget)
		{
			GroundItemWorldWidget->RegisterComponent();

			GroundItemWorldWidget->SetWidgetSpace(EWidgetSpace::World);
			GroundItemWorldWidget->SetGeometryMode(EWidgetGeometryMode::Plane);
			GroundItemWorldWidget->SetBlendMode(EWidgetBlendMode::Transparent);
			GroundItemWorldWidget->SetTwoSided(false);
			GroundItemWorldWidget->SetWindowFocusable(false);

			// Pivot at center so the widget is centered on its world position
			GroundItemWorldWidget->SetPivot(FVector2D(0.5f, 0.5f));
			GroundItemWorldWidget->SetDrawSize(GroundItemWidgetDrawSize);
			GroundItemWorldWidget->SetDrawAtDesiredSize(true);
			GroundItemWorldWidget->SetWidgetClass(GroundWidgetClass);

			// Attach to owner root; world location is overridden each tick
			GroundItemWorldWidget->AttachToComponent(
				Owner->GetRootComponent(),
				FAttachmentTransformRules::KeepRelativeTransform);

			GroundItemWorldWidget->InitWidget();
			GroundItemWorldWidget->SetVisibility(false);

			UE_LOG(LogInteractionWidgetPresenter, Log,
				TEXT("FInteractionWidgetPresenter: Ground item world widget created (Class: %s)"),
				*GroundWidgetClass->GetName());
		}
	}

	UE_LOG(LogInteractionWidgetPresenter, Log,
		TEXT("FInteractionWidgetPresenter: Initialized (HUD: %s | GroundItem: %s)"),
		InteractionWidgetClass     ? *InteractionWidgetClass->GetName()     : TEXT("none"),
		GroundItemWorldWidgetClass ? *GroundItemWorldWidgetClass->GetName() : TEXT("fallback"));
}

// ═══════════════════════════════════════════════════════════════════════
// WIDGET UPDATE
// ═══════════════════════════════════════════════════════════════════════

void FInteractionWidgetPresenter::UpdateForActorInteractable(const TScriptInterface<IInteractable>& Interactable)
{
	if (!InteractionWidget || !Interactable.GetInterface())
	{
		return;
	}

	InteractionWidget->SetInteractionData(
		IInteractable::Execute_GetInputAction(Interactable.GetObject()),
		IInteractable::Execute_GetInteractionText(Interactable.GetObject()));
	InteractionWidget->SetWidgetState(EInteractionWidgetState::IWS_Idle);
	InteractionWidget->Show();
}

void FInteractionWidgetPresenter::UpdateForGroundItem(int32 GroundItemID)
{
	if (GroundItemID == INDEX_NONE)
	{
		return;
	}

	// Build display text (shared by both widget types).
	FText Description = GroundItemDefaultText;
	if (UItemInstance* Item = GetGroundItemInstance(GroundItemID))
	{
		const FText ItemName = Item->GetDisplayName();
		if (!ItemName.IsEmpty())
		{
			Description = FText::Format(GroundItemNameFormat, ItemName);
		}
	}

	// ── PRIMARY: world-space floating widget above the item ──────────────────
	ShowGroundItemWorldWidget(GroundItemID);

	// ── SECONDARY: screen-space HUD widget ───────────────────────────────────
	if (!InteractionWidget)
	{
		return;
	}

	PositionWidgetAtGroundItem(GroundItemID);
	InteractionWidget->SetInteractionData(GroundItemActionInput, Description);
	InteractionWidget->SetWidgetState(EInteractionWidgetState::IWS_Idle);
	InteractionWidget->Show();
}

void FInteractionWidgetPresenter::PositionWidgetAtGroundItem(int32 GroundItemID)
{
	if (!InteractionWidget || GroundItemID == INDEX_NONE || !WorldContext)
	{
		return;
	}

	UGroundItemSubsystem* GroundSub = WorldContext->GetSubsystem<UGroundItemSubsystem>();
	if (!GroundSub)
	{
		return;
	}

	const TMap<int32, FVector>& Locations = GroundSub->GetInstanceLocations();
	const FVector* WorldLocPtr = Locations.Find(GroundItemID);
	if (!WorldLocPtr)
	{
		return;
	}

	// Lift the projection point above the item so the widget floats above it
	FVector WorldPos = *WorldLocPtr;
	WorldPos.Z += GroundItemWidgetHeightOffset;

	APlayerController* PC = GetOwnerPlayerController();
	if (!PC)
	{
		return;
	}

	FVector2D ScreenPos;
	if (!PC->ProjectWorldLocationToScreen(WorldPos, ScreenPos, /*bPlayerViewportRelative=*/false))
	{
		// Item is behind the camera — keep widget hidden at its current position
		return;
	}

	// Account for DPI scaling so the widget lands at the correct physical pixel
	const float DPIScale = UWidgetLayoutLibrary::GetViewportScale(OwnerComponent);
	if (DPIScale > KINDA_SMALL_NUMBER)
	{
		ScreenPos /= DPIScale;
	}

	InteractionWidget->SetPositionInViewport(ScreenPos, /*bRemoveDPIScale=*/false);
}

void FInteractionWidgetPresenter::ShowGroundItemWorldWidget(int32 GroundItemID)
{
	if (!GroundItemWorldWidget || GroundItemID == INDEX_NONE || !WorldContext)
	{
		return;
	}

	UGroundItemSubsystem* GroundSub = WorldContext->GetSubsystem<UGroundItemSubsystem>();
	if (!GroundSub)
	{
		return;
	}

	const TMap<int32, FVector>& Locations = GroundSub->GetInstanceLocations();
	const FVector* WorldLocPtr = Locations.Find(GroundItemID);
	if (!WorldLocPtr)
	{
		return;
	}

	// Position the widget component above the item in world space
	const FVector WidgetWorldPos = *WorldLocPtr + FVector(0.f, 0.f, GroundItemWidgetHeightOffset);
	GroundItemWorldWidget->SetWorldLocation(WidgetWorldPos);

	// Update the widget content
	if (UInteractableWidget* W = Cast<UInteractableWidget>(GroundItemWorldWidget->GetWidget()))
	{
		FText Description = GroundItemDefaultText;
		if (UItemInstance* Item = GetGroundItemInstance(GroundItemID))
		{
			const FText ItemName = Item->GetDisplayName();
			if (!ItemName.IsEmpty())
			{
				Description = FText::Format(GroundItemNameFormat, ItemName);
			}
		}

		W->SetInteractionData(GroundItemActionInput, Description);
		W->SetWidgetState(EInteractionWidgetState::IWS_Idle);
	}

	GroundItemWorldWidget->SetVisibility(true);
}

void FInteractionWidgetPresenter::HideGroundItemWorldWidget()
{
	if (GroundItemWorldWidget)
	{
		GroundItemWorldWidget->SetVisibility(false);
	}
}

void FInteractionWidgetPresenter::TickGroundItemWorldWidget(int32 GroundItemID)
{
	if (!GroundItemWorldWidget || !GroundItemWorldWidget->IsVisible()
		|| GroundItemID == INDEX_NONE || !WorldContext)
	{
		return;
	}

	UGroundItemSubsystem* GroundSub = WorldContext->GetSubsystem<UGroundItemSubsystem>();
	if (!GroundSub)
	{
		return;
	}

	const TMap<int32, FVector>& Locations = GroundSub->GetInstanceLocations();
	const FVector* LocPtr = Locations.Find(GroundItemID);
	if (!LocPtr)
	{
		return;
	}

	// Keep the widget anchored above the item as the player moves
	const FVector WidgetWorldPos = *LocPtr + FVector(0.f, 0.f, GroundItemWidgetHeightOffset);
	GroundItemWorldWidget->SetWorldLocation(WidgetWorldPos);

	// Face the widget toward the local player camera
	if (APlayerController* PC = GetOwnerPlayerController())
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);

		const FVector ToCamera = (CamLoc - WidgetWorldPos).GetSafeNormal();
		if (!ToCamera.IsNearlyZero())
		{
			GroundItemWorldWidget->SetWorldRotation(ToCamera.Rotation());
		}
	}
}

void FInteractionWidgetPresenter::HideAll()
{
	if (InteractionWidget)
	{
		InteractionWidget->Hide();
	}

	HideGroundItemWorldWidget();
}

void FInteractionWidgetPresenter::SetWidgetVisible(bool bVisible)
{
	if (!InteractionWidget)
	{
		return;
	}

	if (bVisible)
	{
		InteractionWidget->Show();
	}
	else
	{
		InteractionWidget->Hide();
	}
}

void FInteractionWidgetPresenter::SetHoldingState(float Progress, EManagedInteractionMode Mode)
{
	const EInteractionWidgetState WidgetState =
		(Mode == EManagedInteractionMode::ActorMash)
			? EInteractionWidgetState::IWS_Mashing
			: EInteractionWidgetState::IWS_Holding;

	// Screen HUD widget
	if (InteractionWidget)
	{
		if (!InteractionWidget->IsShown())
		{
			InteractionWidget->Show();
		}
		InteractionWidget->SetWidgetState(WidgetState);
		InteractionWidget->SetProgress(FMath::Clamp(Progress, 0.0f, 1.0f));
	}

	// World-space widget above the ground item
	if (Mode == EManagedInteractionMode::GroundTapOrHold && GroundItemWorldWidget)
	{
		if (UInteractableWidget* W = Cast<UInteractableWidget>(GroundItemWorldWidget->GetWidget()))
		{
			W->SetWidgetState(EInteractionWidgetState::IWS_Holding);
			W->SetProgress(FMath::Clamp(Progress, 0.0f, 1.0f));
		}
	}
}

void FInteractionWidgetPresenter::SetCompletedState()
{
	if (InteractionWidget)
	{
		if (!InteractionWidget->IsShown())
		{
			InteractionWidget->Show();
		}
		InteractionWidget->SetWidgetState(EInteractionWidgetState::IWS_Completed);
	}

	if (GroundItemWorldWidget)
	{
		if (UInteractableWidget* W = Cast<UInteractableWidget>(GroundItemWorldWidget->GetWidget()))
		{
			W->SetWidgetState(EInteractionWidgetState::IWS_Completed);
		}
	}
}

void FInteractionWidgetPresenter::SetCancelledState()
{
	if (InteractionWidget)
	{
		if (!InteractionWidget->IsShown())
		{
			InteractionWidget->Show();
		}
		InteractionWidget->SetWidgetState(EInteractionWidgetState::IWS_Cancelled);
	}

	if (GroundItemWorldWidget)
	{
		if (UInteractableWidget* W = Cast<UInteractableWidget>(GroundItemWorldWidget->GetWidget()))
		{
			W->SetWidgetState(EInteractionWidgetState::IWS_Cancelled);
		}
	}
}

bool FInteractionWidgetPresenter::IsHUDWidgetShown() const
{
	return InteractionWidget && InteractionWidget->IsShown();
}

// ═══════════════════════════════════════════════════════════════════════
// OUTLINE / HIGHLIGHT
// ═══════════════════════════════════════════════════════════════════════

bool FInteractionWidgetPresenter::InitializeOutlineMID()
{
	if (OutlineMID)
	{
		return true;
	}

	// Cache scan failure to avoid a repeated TActorIterator walk.
	// If the level has no PostProcessVolume, iterating every call is wasteful.
	if (!TargetPostProcessVolume && !bPostProcessSearchFailed)
	{
		if (!WorldContext)
		{
			return false;
		}

		for (TActorIterator<APostProcessVolume> It(WorldContext); It; ++It)
		{
			TargetPostProcessVolume = *It;
			if (TargetPostProcessVolume)
			{
				break;
			}
		}

		if (!TargetPostProcessVolume)
		{
			bPostProcessSearchFailed = true;
			UE_LOG(LogInteractionWidgetPresenter, Warning,
				TEXT("FInteractionWidgetPresenter: No PostProcessVolume found — outline disabled."));
			return false;
		}
	}

	if (!TargetPostProcessVolume)
	{
		return false;
	}

	if (!OutlineMaterial)
	{
		UE_LOG(LogInteractionWidgetPresenter, Warning,
			TEXT("FInteractionWidgetPresenter: OutlineMaterial is not assigned."));
		return false;
	}

	OutlineMID = UMaterialInstanceDynamic::Create(OutlineMaterial, OwnerComponent);
	if (!OutlineMID)
	{
		UE_LOG(LogInteractionWidgetPresenter, Warning,
			TEXT("FInteractionWidgetPresenter: Failed to create OutlineMID."));
		return false;
	}

	TargetPostProcessVolume->AddOrUpdateBlendable(OutlineMID, 1.0f);
	return true;
}

void FInteractionWidgetPresenter::ApplyHighlightStyle(const FInteractableHighlightStyle& Style)
{
	if (!InitializeOutlineMID())
	{
		return;
	}

	OutlineMID->SetVectorParameterValue(TEXT("Color"), Style.Color);
	OutlineMID->SetScalarParameterValue(TEXT("OutlineWidth"), Style.OutlineWidth);
	OutlineMID->SetScalarParameterValue(TEXT("Threshold"), Style.Threshold);
}

void FInteractionWidgetPresenter::ResetHighlightStyle()
{
	if (!InitializeOutlineMID())
	{
		return;
	}

	OutlineMID->SetVectorParameterValue(TEXT("Color"), PlayerHighlightColor);
	OutlineMID->SetScalarParameterValue(TEXT("OutlineWidth"), PlayerHighlightWidth);
	OutlineMID->SetScalarParameterValue(TEXT("Threshold"), PlayerHighlightThreshold);
}

// ═══════════════════════════════════════════════════════════════════════
// CLEANUP
// ═══════════════════════════════════════════════════════════════════════

void FInteractionWidgetPresenter::Shutdown()
{
	if (InteractionWidget)
	{
		InteractionWidget->RemoveFromParent();
		InteractionWidget = nullptr;
	}

	if (GroundItemWorldWidget)
	{
		GroundItemWorldWidget->DestroyComponent();
		GroundItemWorldWidget = nullptr;
	}
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL HELPERS
// ═══════════════════════════════════════════════════════════════════════

APlayerController* FInteractionWidgetPresenter::GetOwnerPlayerController() const
{
	if (!OwnerComponent)
	{
		return nullptr;
	}

	const APawn* OwnerPawn = Cast<APawn>(OwnerComponent->GetOwner());
	if (!OwnerPawn)
	{
		return nullptr;
	}

	return Cast<APlayerController>(OwnerPawn->GetController());
}

UItemInstance* FInteractionWidgetPresenter::GetGroundItemInstance(int32 GroundItemID) const
{
	if (GroundItemID == INDEX_NONE || !WorldContext)
	{
		return nullptr;
	}

	UGroundItemSubsystem* Subsystem = WorldContext->GetSubsystem<UGroundItemSubsystem>();
	return Subsystem ? Subsystem->GetItemByID(GroundItemID) : nullptr;
}

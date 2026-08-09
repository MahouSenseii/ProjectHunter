#include "Character/Components/Interaction/InteractionWidgetPresenter.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "UI/HUD/HunterHUD.h"
#include "Components/ActorComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interactable/Interface/Interactable.h"
#include "UI/Interaction/InteractableWidget.h"
#include "Interactable/Library/Structs/InteractionStructs.h"
#include "Item/ItemInstance.h"
#include "Tower/Subsystems/GroundItemSubsystem.h"

DEFINE_LOG_CATEGORY(LogInteractionWidgetPresenter);

FInteractionWidgetPresenter::FInteractionWidgetPresenter()
{
}

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
			TEXT("FInteractionWidgetPresenter::Initialize - no PlayerController on '%s'; widget deferred."),
			*Owner->GetName());
		return;
	}

	if (InteractionWidgetClass)
	{
		InteractionWidget = CreateWidget<UInteractableWidget>(PC, InteractionWidgetClass);
		if (InteractionWidget)
		{
			InteractionWidget->AddToViewport(WidgetZOrder);
			InteractionWidget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
			InteractionWidget->Hide();
		}
		else
		{
			UE_LOG(LogInteractionWidgetPresenter, Error,
				TEXT("FInteractionWidgetPresenter::Initialize - failed to create screen HUD widget on '%s'"),
				*Owner->GetName());
		}
	}
	else
	{
		UE_LOG(LogInteractionWidgetPresenter, Warning,
			TEXT("FInteractionWidgetPresenter::Initialize - InteractionWidgetClass not set; screen HUD widget disabled."));
	}

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

			GroundItemWorldWidget->SetPivot(FVector2D(0.5f, 0.5f));

			if (bUseGroundItemDesiredSize)
			{
				GroundItemWorldWidget->SetDrawSize(GroundItemWidgetDrawSize);
				GroundItemWorldWidget->SetDrawAtDesiredSize(true);
			}
			else
			{
				const float Scale = FMath::Clamp(GroundItemResolutionScale, 0.5f, 4.0f);
				GroundItemWorldWidget->SetDrawSize(GroundItemWidgetDrawSize * Scale);
				GroundItemWorldWidget->SetDrawAtDesiredSize(false);
				GroundItemWorldWidget->SetRelativeScale3D(FVector(1.0f / Scale));
			}

			GroundItemWorldWidget->SetWidgetClass(GroundWidgetClass);

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
		HideGroundItemWorldWidget();
		return;
	}

	if (PresentedGroundItemID == GroundItemID)
	{
		PositionWidgetAtGroundItem(GroundItemID);
		return;
	}

	PresentedGroundItemID = GroundItemID;

	FText Description = GroundItemDefaultText;
	UItemInstance* Item = GetGroundItemInstance(GroundItemID);
	if (Item)
	{
		const FText ItemName = Item->GetDisplayName();
		if (!ItemName.IsEmpty())
		{
			Description = FText::Format(GroundItemNameFormat, ItemName);
		}
	}

	ShowGroundItemWorldWidget(GroundItemID);
	ShowItemTooltipForGroundItem(GroundItemID);

	if (!InteractionWidget)
	{
		return;
	}

	PositionWidgetAtGroundItem(GroundItemID);
	ApplyGroundItemPromptData(*InteractionWidget, Description);
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
		return;
	}

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

	const FVector WidgetWorldPos = *WorldLocPtr + FVector(0.f, 0.f, GroundItemWidgetHeightOffset);
	GroundItemWorldWidget->SetWorldLocation(WidgetWorldPos);

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

		ApplyGroundItemPromptData(*W, Description);
		W->SetWidgetState(EInteractionWidgetState::IWS_Idle);
	}

	GroundItemWorldWidget->SetVisibility(true);
}

void FInteractionWidgetPresenter::HideGroundItemWorldWidget()
{
	PresentedGroundItemID = INDEX_NONE;

	if (GroundItemWorldWidget)
	{
		GroundItemWorldWidget->SetVisibility(false);
	}

	HideItemTooltip();
}

void FInteractionWidgetPresenter::HideItemTooltip()
{
	if (APlayerController* PC = GetOwnerPlayerController())
	{
		if (AHunterHUD* HunterHUD = Cast<AHunterHUD>(PC->GetHUD()))
		{
			HunterHUD->HideItemTooltip();
		}
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

	const FVector WidgetWorldPos = *LocPtr + FVector(0.f, 0.f, GroundItemWidgetHeightOffset);
	GroundItemWorldWidget->SetWorldLocation(WidgetWorldPos);

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

	if (InteractionWidget)
	{
		if (!InteractionWidget->IsShown())
		{
			InteractionWidget->Show();
		}
		InteractionWidget->SetWidgetState(WidgetState);
		InteractionWidget->SetProgress(FMath::Clamp(Progress, 0.0f, 1.0f));
	}

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

bool FInteractionWidgetPresenter::InitializeOutlineMID()
{
	if (OutlineMID)
	{
		return true;
	}

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
				TEXT("FInteractionWidgetPresenter: No PostProcessVolume found - outline disabled."));
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

void FInteractionWidgetPresenter::ApplyGroundItemPromptData(
	UInteractableWidget& Widget, const FText& Description) const
{
	if (GroundItemActionInput)
	{
		Widget.SetInteractionData(GroundItemActionInput, Description);
		return;
	}

	Widget.SetInteractionDataWithKey(GroundItemFallbackKey, Description);
}

void FInteractionWidgetPresenter::ShowItemTooltipForGroundItem(int32 GroundItemID)
{
	if (!bShowGroundItemTooltip)
	{
		HideItemTooltip();
		return;
	}

	APlayerController* PC = GetOwnerPlayerController();
	if (!PC)
	{
		return;
	}

	UItemInstance* Item = GetGroundItemInstance(GroundItemID);
	if (!Item)
	{
		HideItemTooltip();
		return;
	}

	AHunterHUD* HunterHUD = Cast<AHunterHUD>(PC->GetHUD());
	if (!HunterHUD || !WorldContext)
	{
		return;
	}

	UGroundItemSubsystem* GroundSub = WorldContext->GetSubsystem<UGroundItemSubsystem>();
	if (!GroundSub)
	{
		HideItemTooltip();
		return;
	}

	const TMap<int32, FVector>& Locations = GroundSub->GetInstanceLocations();
	const FVector* WorldLocPtr = Locations.Find(GroundItemID);
	if (!WorldLocPtr)
	{
		HideItemTooltip();
		return;
	}

	FVector WorldPos = *WorldLocPtr;
	WorldPos.Z += GroundItemWidgetHeightOffset;

	FVector2D ScreenPos;
	if (!PC->ProjectWorldLocationToScreen(WorldPos, ScreenPos, /*bPlayerViewportRelative=*/false))
	{
		HideItemTooltip();
		return;
	}

	const float DPIScale = UWidgetLayoutLibrary::GetViewportScale(OwnerComponent);
	if (DPIScale > KINDA_SMALL_NUMBER)
	{
		ScreenPos /= DPIScale;
	}

	HunterHUD->ShowItemTooltip(Item, ScreenPos + ItemTooltipScreenOffset);
}

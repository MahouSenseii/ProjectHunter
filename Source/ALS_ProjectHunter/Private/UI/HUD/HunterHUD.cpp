#include "UI/HUD/HunterHUD.h"

#include "UI/HUD/HunterMainHUDWidget.h"
#include "Character/PHBaseCharacter.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "GameFramework/PlayerController.h"
#include "UI/Menu/Camera/PHMenuCameraComponent.h"
#include "UI/Menu/Widgets/PHMenuRootWidget.h"
#include "TimerManager.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "UI/Interaction/ItemTooltipWidget.h"
#include "Tower/Subsystems/RunSubsystem.h"
#include "UI/HUD/PHFloorBannerWidget.h"
#include "UI/HUD/PHRunStatusWidget.h"
#include "Framework/GameModes/PHGameState.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogHunterHUD);

namespace
{
	const TCHAR* GetEndPlayReasonText(const EEndPlayReason::Type EndPlayReason)
	{
		switch (EndPlayReason)
		{
		case EEndPlayReason::Destroyed:
			return TEXT("Destroyed");
		case EEndPlayReason::LevelTransition:
			return TEXT("LevelTransition");
		case EEndPlayReason::EndPlayInEditor:
			return TEXT("EndPlayInEditor");
		case EEndPlayReason::RemovedFromWorld:
			return TEXT("RemovedFromWorld");
		case EEndPlayReason::Quit:
			return TEXT("Quit");
		default:
			return TEXT("Unknown");
		}
	}
}

AHunterHUD::AHunterHUD()
{
	MenuCamera = CreateDefaultSubobject<UPHMenuCameraComponent>(TEXT("MenuCamera"));
}

void AHunterHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* OwningPC = GetOwningPlayerController();
	if (ItemTooltipWidgetClass && OwningPC)
	{
		ItemTooltipWidget = CreateWidget<UItemTooltipWidget>(OwningPC, ItemTooltipWidgetClass);
		if (ItemTooltipWidget)
		{
			ItemTooltipWidget->AddToViewport(100);
			ItemTooltipWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	CreateMainHUDWidget();
	if (UWorld* World = GetWorld())
	{
		GameStateSetHandle = World->GameStateSetEvent.AddUObject(this, &AHunterHUD::BindRunGameState);
		BindRunGameState(World->GetGameState());
	}

	if (OwningPC)
	{
		OwningPC->OnPossessedPawnChanged.AddDynamic(this, &AHunterHUD::HandlePawnChanged);

		if (APHBaseCharacter* Character = Cast<APHBaseCharacter>(OwningPC->GetPawn()))
		{
			BindWidgetsToCharacter(Character);
		}

		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			const APlayerController* PC = GetOwningPlayerController();
			PH_LOG(LogHunterHUD, Log, "PostBeginPlay: HUD=%s OwnerPC=%s CurrentPCHUD=%s MainHUDWidget=%s IsInViewport=%s",
				*GetNameSafe(this),
				*GetNameSafe(PC),
				PC ? *GetNameSafe(PC->GetHUD()) : TEXT("None"),
				*GetNameSafe(MainHUDWidget),
				MainHUDWidget && MainHUDWidget->IsInViewport() ? TEXT("true") : TEXT("false"));
		}));
	}
}

void AHunterHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindRunGameState();
	if (UWorld* World = GetWorld())
	{
		World->GameStateSetEvent.Remove(GameStateSetHandle);
	}
	GameStateSetHandle.Reset();
	if (FloorBannerWidget)
	{
		FloorBannerWidget->HideBanner();
		FloorBannerWidget->RemoveFromParent();
		FloorBannerWidget = nullptr;
	}

	const APlayerController* PC = GetOwningPlayerController();
	PH_LOG(LogHunterHUD, Log, "EndPlay: HUD=%s Reason=%s OwnerPC=%s CurrentPCHUD=%s MainHUDWidget=%s IsInViewport=%s",
		*GetNameSafe(this),
		GetEndPlayReasonText(EndPlayReason),
		*GetNameSafe(PC),
		PC ? *GetNameSafe(PC->GetHUD()) : TEXT("None"),
		*GetNameSafe(MainHUDWidget),
		MainHUDWidget && MainHUDWidget->IsInViewport() ? TEXT("true") : TEXT("false"));

	if (APlayerController* OwningPC = GetOwningPlayerController())
	{
		OwningPC->OnPossessedPawnChanged.RemoveDynamic(this, &AHunterHUD::HandlePawnChanged);
	}

	if (MainHUDWidget)
	{
		MainHUDWidget->RemoveWidget();
		MainHUDWidget = nullptr;
	}

	if (MenuRootWidget)
	{
		MenuRootWidget->OnMenuPageChanged.RemoveDynamic(this, &AHunterHUD::HandleMenuPageChanged);
		MenuRootWidget->ReleaseCharacter();
		MenuRootWidget->RemoveFromParent();
		MenuRootWidget = nullptr;
	}

	if (ItemTooltipWidget)
	{
		ItemTooltipWidget->RemoveFromParent();
		ItemTooltipWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

// MENU

void AHunterHUD::ToggleMenu()
{
	if (IsMenuOpen())
	{
		CloseMenu();
	}
	else
	{
		OpenMenu(EMenuType::MT_Equipment);
	}
}

void AHunterHUD::OpenMenu(const EMenuType MenuType)
{
	if (!EnsureMenuRootWidget())
	{
		return;
	}

	MenuRootWidget->SetVisibility(ESlateVisibility::Visible);
	MenuRootWidget->OpenMenu(MenuType);
	ApplyMenuInputMode(true);

	if (MenuCamera)
	{
		// The root widget resolves MT_None to its configured default page, so
		// ask it what actually opened rather than trusting the argument.
		MenuCamera->ActivateMenuCamera(MenuRootWidget->GetActiveMenuType());
	}

	if (bHideMainHUDWhileMenuOpen && MainHUDWidget)
	{
		MainHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	UE_LOG(LogHunterHUD, Log, TEXT("OpenMenu: menu opened on page %d."),
		static_cast<int32>(MenuRootWidget->GetActiveMenuType()));
}

void AHunterHUD::CloseMenu()
{
	if (!IsMenuOpen())
	{
		return;
	}

	MenuRootWidget->SetVisibility(ESlateVisibility::Collapsed);
	ApplyMenuInputMode(false);

	if (MenuCamera)
	{
		MenuCamera->DeactivateMenuCamera();
	}

	if (bHideMainHUDWhileMenuOpen && MainHUDWidget)
	{
		MainHUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// A slot may have been hovered at the moment the menu closed.
	HideItemTooltip(EItemTooltipSource::ITS_None);

	UE_LOG(LogHunterHUD, Log, TEXT("CloseMenu: menu closed."));
}

bool AHunterHUD::IsMenuOpen() const
{
	return MenuRootWidget && MenuRootWidget->GetVisibility() == ESlateVisibility::Visible;
}

bool AHunterHUD::EnsureMenuRootWidget()
{
	if (MenuRootWidget)
	{
		return true;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return false;
	}

	if (!MenuRootWidgetClass)
	{
		PH_LOG_WARNING(LogHunterHUD,
			"EnsureMenuRootWidget: MenuRootWidgetClass is not set on %s. "
			"Assign a UPHMenuRootWidget Blueprint (for example WBP_SystemMenuRoot) in the HUD Blueprint defaults.",
			*GetNameSafe(this));
		return false;
	}

	MenuRootWidget = CreateWidget<UPHMenuRootWidget>(PC, MenuRootWidgetClass);
	if (!MenuRootWidget)
	{
		PH_LOG_WARNING(LogHunterHUD, "EnsureMenuRootWidget: CreateWidget failed for %s.",
			*GetNameSafe(MenuRootWidgetClass));
		return false;
	}

	// Created hidden; OpenMenu flips visibility. Kept alive (with cached pages)
	// for the lifetime of the HUD.
	MenuRootWidget->SetVisibility(ESlateVisibility::Collapsed);
	MenuRootWidget->AddToPlayerScreen(MenuZOrder);
	MenuRootWidget->OnMenuPageChanged.AddDynamic(this, &AHunterHUD::HandleMenuPageChanged);

	if (APHBaseCharacter* Character = Cast<APHBaseCharacter>(PC->GetPawn()))
	{
		MenuRootWidget->InitializeForCharacter(Character);
	}

	UE_LOG(LogHunterHUD, Log, TEXT("EnsureMenuRootWidget: created %s (Z=%d)."),
		*GetNameSafe(MenuRootWidget), MenuZOrder);

	return true;
}

void AHunterHUD::ApplyMenuInputMode(const bool bMenuOpen)
{
	if (!bManageInputMode)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	// Reference counted, so every true needs exactly one matching false.
	if (bBlockCharacterInputWhileMenuOpen && bMenuOpen != bCharacterInputBlocked)
	{
		PC->SetIgnoreLookInput(bMenuOpen);
		PC->SetIgnoreMoveInput(bMenuOpen);
		bCharacterInputBlocked = bMenuOpen;
	}
	else if (!bMenuOpen && bCharacterInputBlocked)
	{
		PC->SetIgnoreLookInput(false);
		PC->SetIgnoreMoveInput(false);
		bCharacterInputBlocked = false;
	}

	if (APawn* Pawn = PC->GetPawn())
	{
		if (bMenuOpen && bDisablePawnInputWhileMenuOpen && !bPawnInputDisabled)
		{
			Pawn->DisableInput(PC);
			bPawnInputDisabled = true;
		}
		else if (!bMenuOpen && bPawnInputDisabled)
		{
			Pawn->EnableInput(PC);
			bPawnInputDisabled = false;
		}
	}

	if (bMenuOpen)
	{
		if (bUseUIOnlyInputMode)
		{
			FInputModeUIOnly InputMode;
			if (MenuRootWidget)
			{
				InputMode.SetWidgetToFocus(MenuRootWidget->TakeWidget());
			}
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(InputMode);
		}
		else
		{
			FInputModeGameAndUI InputMode;
			if (MenuRootWidget)
			{
				InputMode.SetWidgetToFocus(MenuRootWidget->TakeWidget());
			}
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(InputMode);
		}

		PC->bShowMouseCursor = true;

		// SetWidgetToFocus is a request Slate can decline; asking the widget
		// directly is what makes the key handler actually receive anything.
		if (MenuRootWidget)
		{
			MenuRootWidget->SetKeyboardFocus();
		}
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}

void AHunterHUD::ShowItemTooltip(UItemInstance* Item, FVector2D ScreenPosition, const EItemTooltipSource Source)
{
	if (!CanShowItemTooltipFrom(Source))
	{
		return;
	}

	if (ItemTooltipWidget && Item)
	{
		ActiveItemTooltipSource = Source;

		ItemTooltipWidget->UpdateTooltip(Item);

		if (bPinItemTooltipToBottomRight)
		{
			// SetPositionInViewport resets viewport anchors, so apply the offset
			// before anchoring and aligning the tooltip to the bottom-right corner.
			ItemTooltipWidget->SetPositionInViewport(
				FVector2D(-ItemTooltipScreenPadding, -ItemTooltipScreenPadding),
				/*bRemoveDPIScale=*/false);
			ItemTooltipWidget->SetAnchorsInViewport(FAnchors(1.0f, 1.0f));
			ItemTooltipWidget->SetAlignmentInViewport(FVector2D(1.0f, 1.0f));
		}
		else
		{
			ItemTooltipWidget->SetPositionInViewport(ScreenPosition);
			ItemTooltipWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
		}

		ItemTooltipWidget->ShowAnimated();
	}
}

void AHunterHUD::ShowItemTooltipAtViewportPosition(
	UItemInstance* Item,
	const FVector2D ViewportPosition,
	const EItemTooltipSource Source)
{
	if (!ItemTooltipWidget || !Item || !CanShowItemTooltipFrom(Source))
	{
		return;
	}

	ActiveItemTooltipSource = Source;
	ItemTooltipWidget->UpdateTooltip(Item);

	// Anchor top-left so the clamp math below matches what the player sees.
	ItemTooltipWidget->SetAnchorsInViewport(FAnchors(0.0f, 0.0f));
	ItemTooltipWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
	ItemTooltipWidget->SetPositionInViewport(
		ClampTooltipToViewport(ViewportPosition),
		/*bRemoveDPIScale=*/false);

	ItemTooltipWidget->ShowAnimated();
}

void AHunterHUD::UpdateItemTooltipPosition(const FVector2D ViewportPosition, const EItemTooltipSource Source)
{
	if (!ItemTooltipWidget
		|| !ItemTooltipWidget->IsVisible()
		|| ActiveItemTooltipSource != Source)
	{
		return;
	}

	ItemTooltipWidget->SetPositionInViewport(
		ClampTooltipToViewport(ViewportPosition),
		/*bRemoveDPIScale=*/false);
}

FVector2D AHunterHUD::ClampTooltipToViewport(const FVector2D DesiredPosition) const
{
	if (!ItemTooltipWidget)
	{
		return DesiredPosition;
	}

	FVector2D Position = DesiredPosition + ItemTooltipCursorOffset;

	const float DPIScale = UWidgetLayoutLibrary::GetViewportScale(GetWorld());
	if (DPIScale <= 0.0f)
	{
		return Position;
	}

	// Both values are converted into the DPI-independent space the position uses.
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(GetWorld()) / DPIScale;

	// Desired size is zero until the widget has been laid out once.
	ItemTooltipWidget->ForceLayoutPrepass();
	const FVector2D TooltipSize = ItemTooltipWidget->GetDesiredSize();
	if (TooltipSize.IsNearlyZero())
	{
		return Position;
	}

	// Flip to the other side of the cursor before clamping, so the tooltip never
	// sits under the pointer near the right/bottom edges.
	if (Position.X + TooltipSize.X > ViewportSize.X)
	{
		Position.X = DesiredPosition.X - ItemTooltipCursorOffset.X - TooltipSize.X;
	}

	if (Position.Y + TooltipSize.Y > ViewportSize.Y)
	{
		Position.Y = DesiredPosition.Y - ItemTooltipCursorOffset.Y - TooltipSize.Y;
	}

	// Literals stay double: FVector2D components are doubles in UE5 and mixing in
	// float literals breaks FMath::Clamp/Max template deduction.
	Position.X = FMath::Clamp(Position.X, 0.0, FMath::Max(0.0, ViewportSize.X - TooltipSize.X));
	Position.Y = FMath::Clamp(Position.Y, 0.0, FMath::Max(0.0, ViewportSize.Y - TooltipSize.Y));

	return Position;
}

void AHunterHUD::HideItemTooltip(const EItemTooltipSource Source)
{
	// ITS_None is the force-hide. Otherwise a system may only close its own
	// tooltip - the interaction poll must not close the menu's.
	if (Source != EItemTooltipSource::ITS_None
		&& ActiveItemTooltipSource != EItemTooltipSource::ITS_None
		&& Source != ActiveItemTooltipSource)
	{
		return;
	}

	ActiveItemTooltipSource = EItemTooltipSource::ITS_None;

	if (ItemTooltipWidget)
	{
		ItemTooltipWidget->HideAnimated();
	}
}

bool AHunterHUD::CanShowItemTooltipFrom(const EItemTooltipSource Source) const
{
	// The menu owns the tooltip while it is open. The interaction system polls
	// on a timer and would otherwise stomp a slot tooltip the frame after it opens.
	if (Source == EItemTooltipSource::ITS_Interaction && IsMenuOpen())
	{
		return false;
	}

	return true;
}

void AHunterHUD::ShowMashProgressWidget(const FText& Text, int32 RequiredCount)
{
	BP_OnShowMashProgress(Text, RequiredCount);
}

void AHunterHUD::HideMashProgressWidget()
{
	BP_OnHideMashProgress();
}

void AHunterHUD::CreateMainHUDWidget()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	if (!MainHUDWidgetClass)
	{
		PH_LOG_WARNING(LogHunterHUD, "CreateMainHUDWidget: MainHUDWidgetClass was not set in BP.");
		return;
	}

	if (!MainHUDWidget)
	{
		MainHUDWidget = CreateWidget<UHunterMainHUDWidget>(PC, MainHUDWidgetClass);
	}

	if (MainHUDWidget)
	{
		MainHUDWidget->SetVisibility(ESlateVisibility::Visible);
		if (MainHUDWidget->AddToPlayerScreen(10))
		{
			PH_LOG(LogHunterHUD, Log, "CreateMainHUDWidget: Added MainHUDWidget=%s Class=%s PC=%s IsInViewport=%s",
				*GetNameSafe(MainHUDWidget),
				*GetNameSafe(MainHUDWidgetClass),
				*GetNameSafe(PC),
				MainHUDWidget->IsInViewport() ? TEXT("true") : TEXT("false"));
		}
		else
		{
			PH_LOG_WARNING(LogHunterHUD, "CreateMainHUDWidget: Failed to add MainHUDWidget '%s' to the owning player's screen.",
				*GetNameSafe(MainHUDWidget));
		}
	}
}

void AHunterHUD::BindWidgetsToCharacter(APHBaseCharacter* Character) const
{
	if (!MainHUDWidget)
	{
		PH_LOG_WARNING(LogHunterHUD, "BindWidgetsToCharacter: MainHUDWidget is null. Character=%s",
			*GetNameSafe(Character));
		return;
	}

	PH_LOG(LogHunterHUD, Log, "BindWidgetsToCharacter: MainHUDWidget=%s Character=%s CharacterClass=%s",
		*GetNameSafe(MainHUDWidget),
		*GetNameSafe(Character),
		Character ? *GetNameSafe(Character->GetClass()) : TEXT("None"));

	MainHUDWidget->BindToCharacter(Character);

	// Keep the menu (and all of its cached pages) bound to the same character.
	if (MenuRootWidget)
	{
		if (Character)
		{
			MenuRootWidget->InitializeForCharacter(Character);
		}
		else
		{
			MenuRootWidget->ReleaseCharacter();
		}
	}
}

void AHunterHUD::HandlePawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	APHBaseCharacter* NewCharacter = Cast<APHBaseCharacter>(NewPawn);
	PH_LOG(LogHunterHUD, Log, "HandlePawnChanged: OldPawn=%s NewPawn=%s NewPawnClass=%s NewCharacter=%s",
		*GetNameSafe(OldPawn),
		*GetNameSafe(NewPawn),
		NewPawn ? *GetNameSafe(NewPawn->GetClass()) : TEXT("None"),
		*GetNameSafe(NewCharacter));

	BindWidgetsToCharacter(NewCharacter);

	if (IsMenuOpen())
	{
		// bPawnInputDisabled refers to the pawn we just stopped possessing. A new
		// pawn arrives with its own input component live, so without clearing the
		// flag first the re-apply below is skipped and the player can act with the
		// menu still open.
		bPawnInputDisabled = false;
		ApplyMenuInputMode(true);
	}

	// A menu left open across a possession change is still framing the old body.
	if (MenuCamera && MenuCamera->IsMenuCameraActive())
	{
		MenuCamera->DeactivateMenuCamera();

		if (NewCharacter && IsMenuOpen() && MenuRootWidget)
		{
			MenuCamera->ActivateMenuCamera(MenuRootWidget->GetActiveMenuType());
		}
	}
}

void AHunterHUD::HandleMenuPageChanged(const EMenuType NewMenu, const EMenuType OldMenu)
{
	if (MenuCamera && IsMenuOpen())
	{
		MenuCamera->SetMenuPage(NewMenu);
	}
}

void AHunterHUD::HandleFloorAdvanced(const int32 NewFloor)
{
	BannerFloor = NewFloor;
	if (UPHFloorBannerWidget* EmbeddedBanner = MainHUDWidget ? MainHUDWidget->GetFloorBannerWidget() : nullptr)
	{
		FloorBannerEndsAt = 0.0;
		EmbeddedBanner->ShowFloor(NewFloor);
		return;
	}

	// The widget owns the announcement when one is authored. Created once and reused so a Blueprint
	// can run an entry animation without being rebuilt on every floor.
	if (FloorBannerWidgetClass)
	{
		if (!FloorBannerWidget)
		{
			if (APlayerController* OwningPC = GetOwningPlayerController())
			{
				FloorBannerWidget = CreateWidget<UPHFloorBannerWidget>(OwningPC, FloorBannerWidgetClass);
				if (FloorBannerWidget)
				{
					FloorBannerWidget->AddToPlayerScreen(20);
				}
			}
		}

		if (FloorBannerWidget)
		{
			FloorBannerWidget->ShowFloor(NewFloor);
			return;
		}
	}

	// Fallback: canvas text, so the climb stays testable before the widget is authored.
	FloorBannerEndsAt = GetWorld() ? GetWorld()->GetTimeSeconds() + FloorBannerSeconds : 0.0;
}

void AHunterHUD::UnbindRunGameState()
{
	if (APHGameState* GameState = PresentationGameState.Get())
	{
		GameState->OnReplicatedRunChanged.RemoveDynamic(this, &AHunterHUD::HandleRunSnapshot);
	}
	PresentationGameState.Reset();
}

void AHunterHUD::BindRunGameState(AGameStateBase* GameState)
{
	UnbindRunGameState();
	PresentationGameState = Cast<APHGameState>(GameState);
	if (APHGameState* HunterState = PresentationGameState.Get())
	{
		HunterState->OnReplicatedRunChanged.AddUniqueDynamic(this, &AHunterHUD::HandleRunSnapshot);
		// The HUD may appear after floor one was generated, or after a client joins.
		HandleRunSnapshot(HunterState->RunState, HunterState->RunSession);
	}
	else
	{
		HandleRunSnapshot(ERunState::Inactive, FRunSessionData());
	}
}

void AHunterHUD::HandleRunSnapshot(const ERunState State, FRunSessionData Session)
{
	if (UPHRunStatusWidget* Status = MainHUDWidget ? MainHUDWidget->GetRunStatusWidget() : nullptr)
	{
		Status->ApplyRunSnapshot(State, Session);
		Status->SetVisibility(bShowRunOverlay ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (State != ERunState::Active)
	{
		AnnouncedRunID.Invalidate();
		AnnouncedFloor = INDEX_NONE;
		FloorBannerEndsAt = 0.0;
		if (UPHFloorBannerWidget* EmbeddedBanner = MainHUDWidget ? MainHUDWidget->GetFloorBannerWidget() : nullptr)
		{
			EmbeddedBanner->HideBanner();
		}
		if (FloorBannerWidget)
		{
			FloorBannerWidget->HideBanner();
		}
		return;
	}
	const EFloorPhase Phase = Session.Floor.Phase;
	if (Phase == EFloorPhase::Generating || Phase == EFloorPhase::Transitioning)
	{
		FloorBannerEndsAt = 0.0;
		if (UPHFloorBannerWidget* EmbeddedBanner = MainHUDWidget ? MainHUDWidget->GetFloorBannerWidget() : nullptr)
		{
			EmbeddedBanner->HideBanner();
		}
		if (FloorBannerWidget)
		{
			FloorBannerWidget->HideBanner();
		}
	}
	const bool bReady = Phase == EFloorPhase::InProgress || Phase == EFloorPhase::ObjectiveComplete || Phase == EFloorPhase::RewardReady;
	if (bShowRunOverlay && bReady && Session.Floor.FloorNumber > 0 &&
		(AnnouncedRunID != Session.RunID || AnnouncedFloor != Session.Floor.FloorNumber))
	{
		AnnouncedRunID = Session.RunID;
		AnnouncedFloor = Session.Floor.FloorNumber;
		HandleFloorAdvanced(Session.Floor.FloorNumber);
	}
}

void AHunterHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!bShowRunOverlay || !Canvas)
	{
		return;
	}
	const bool bDrawLegacyStatus = !MainHUDWidget || !MainHUDWidget->GetRunStatusWidget();
	const bool bDrawLegacyBanner = GetWorld() && GetWorld()->GetTimeSeconds() < FloorBannerEndsAt;
	if (!bDrawLegacyStatus && !bDrawLegacyBanner)
	{
		return;
	}

	const UGameInstance* GameInstance = GetGameInstance();
	const URunSubsystem* Run = GameInstance ? GameInstance->GetSubsystem<URunSubsystem>() : nullptr;
	if (!Run || !Run->IsRunActive())
	{
		return;
	}

	const FRunFloorData Floor = Run->GetCurrentFloorData();
	const int32 Remaining = FMath::Max(0, Floor.ObjectiveTarget - Floor.ObjectiveProgress);

	UFont* Body = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* Large = GEngine ? GEngine->GetLargeFont() : nullptr;

	// Phase is on screen because a stalled floor is otherwise indistinguishable from a hard one:
	// if the last enemy fails to spawn, the counter simply stops and nothing says why.
	const TCHAR* PhaseText = TEXT("");
	switch (Floor.Phase)
	{
	case EFloorPhase::Generating:       PhaseText = TEXT("building"); break;
	case EFloorPhase::InProgress:       PhaseText = TEXT("clear the floor"); break;
	case EFloorPhase::ObjectiveComplete: PhaseText = TEXT("floor clear"); break;
	case EFloorPhase::RewardReady:      PhaseText = TEXT("exit open"); break;
	default: break;
	}

	// Keep legacy canvas diagnostics only for HUDs that have no authored status widget.
	if (bDrawLegacyStatus)
	{
		DrawText(FString::Printf(TEXT("FLOOR %d"), Floor.FloorNumber),
			FLinearColor(1.0f, 0.86f, 0.4f), 40.0f, 40.0f, Body, 1.4f);
		DrawText(FString::Printf(TEXT("Enemies remaining: %d / %d"), Remaining, Floor.ObjectiveTarget),
			Remaining > 0 ? FLinearColor::White : FLinearColor(0.4f, 1.0f, 0.4f), 40.0f, 78.0f, Body, 1.2f);
		DrawText(FString::Printf(TEXT("%s"), PhaseText),
			FLinearColor(0.7f, 0.8f, 1.0f), 40.0f, 108.0f, Body, 1.0f);
	}

	if (bDrawLegacyBanner)
	{
		const FString Banner = FString::Printf(TEXT("FLOOR %d"), BannerFloor);
		float Width = 0.0f;
		float Height = 0.0f;
		GetTextSize(Banner, Width, Height, Large, 2.0f);
		DrawText(Banner, FLinearColor(1.0f, 0.9f, 0.5f),
			(Canvas->SizeX - Width) * 0.5f, Canvas->SizeY * 0.22f, Large, 2.0f);
	}
}

void AHunterHUD::PHClearFloor()
{
	const UGameInstance* GameInstance = GetGameInstance();
	URunSubsystem* Run = GameInstance ? GameInstance->GetSubsystem<URunSubsystem>() : nullptr;
	if (!Run || !Run->IsRunActive())
	{
		UE_LOG(LogHunterHUD, Warning, TEXT("PHClearFloor: no run is active."));
		return;
	}

	const FRunFloorData Floor = Run->GetCurrentFloorData();
	UE_LOG(LogHunterHUD, Warning,
		TEXT("PHClearFloor: forcing floor %d complete at %d of %d enemies."),
		Floor.FloorNumber, Floor.ObjectiveProgress, Floor.ObjectiveTarget);
	Run->CompleteFloorObjective();
}

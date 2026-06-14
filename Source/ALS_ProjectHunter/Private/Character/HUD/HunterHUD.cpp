#include "Character/HUD/HunterHUD.h"

#include "Character/HUD/HunterMainHUDWidget.h"
#include "Character/PHBaseCharacter.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "GameFramework/PlayerController.h"
#include "Menu/Widgets/MenuRootWidget.h"
#include "TimerManager.h"
#include "Interactable/Widget/ItemTooltipWidget.h"

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

void AHunterHUD::BeginPlay()
{
	Super::BeginPlay();

	if (ItemTooltipWidgetClass && GetWorld())
	{
		ItemTooltipWidget = CreateWidget<UItemTooltipWidget>(GetWorld(), ItemTooltipWidgetClass);
		if (ItemTooltipWidget)
		{
			ItemTooltipWidget->AddToViewport(100);
			ItemTooltipWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	CreateMainHUDWidget();

	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->OnPossessedPawnChanged.AddDynamic(this, &AHunterHUD::HandlePawnChanged);

		if (APHBaseCharacter* Character = Cast<APHBaseCharacter>(PC->GetPawn()))
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

// ─────────────────────────────────────────────────────────────────────────────
// MENU
// ─────────────────────────────────────────────────────────────────────────────

void AHunterHUD::ToggleMenu()
{
	if (IsMenuOpen())
	{
		CloseMenu();
	}
	else
	{
		OpenMenu(EMenuType::MT_None);
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
			"Assign your WBP_MenuRoot in the HUD Blueprint defaults.",
			*GetNameSafe(this));
		return false;
	}

	MenuRootWidget = CreateWidget<UMenuRootWidget>(PC, MenuRootWidgetClass);
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

	if (APHBaseCharacter* Character = Cast<APHBaseCharacter>(PC->GetPawn()))
	{
		MenuRootWidget->InitializeForCharacter(Character);
	}

	UE_LOG(LogHunterHUD, Log, TEXT("EnsureMenuRootWidget: created %s (Z=%d)."),
		*GetNameSafe(MenuRootWidget), MenuZOrder);

	return true;
}

void AHunterHUD::ApplyMenuInputMode(const bool bMenuOpen) const
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

	if (bMenuOpen)
	{
		FInputModeGameAndUI InputMode;
		if (MenuRootWidget)
		{
			InputMode.SetWidgetToFocus(MenuRootWidget->TakeWidget());
		}
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}

void AHunterHUD::ShowItemTooltip(UItemInstance* Item, FVector2D ScreenPosition)
{
	if (ItemTooltipWidget && Item)
	{
		ItemTooltipWidget->UpdateTooltip(Item);
		ItemTooltipWidget->SetPositionInViewport(ScreenPosition);
		ItemTooltipWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void AHunterHUD::HideItemTooltip()
{
	if (ItemTooltipWidget)
	{
		ItemTooltipWidget->SetVisibility(ESlateVisibility::Hidden);
	}
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
}

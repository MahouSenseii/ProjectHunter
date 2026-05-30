#include "Character/HUD/HunterHUD.h"

#include "Character/HUD/HunterMainHUDWidget.h"
#include "Character/PHBaseCharacter.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "GameFramework/PlayerController.h"
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

	if (ItemTooltipWidget)
	{
		ItemTooltipWidget->RemoveFromParent();
		ItemTooltipWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
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

void AHunterHUD::ShowMashProgressWidget(const FText& Text, int32 INT32)
{
}

void AHunterHUD::HideMashProgressWidget()
{
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

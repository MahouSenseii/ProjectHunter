// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Widget.h"
#include "Components/OverlaySlot.h"
#include "Styling/SlateBrush.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UI/HUD/HunterHUDResourceWidget.h"
#include "UI/HUD/HunterMainHUDWidget.h"

namespace PHHUDWidgetInspectionTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	void LogTree(UWidgetTree* Tree, const FString& Prefix)
	{
		if (!Tree)
		{
			return;
		}

		TArray<UWidget*> Widgets;
		Tree->GetAllWidgets(Widgets);
		for (const UWidget* Widget : Widgets)
		{
			if (!Widget)
			{
				continue;
			}

			UE_LOG(LogTemp, Display, TEXT("PH HUD tree %s%s [%s]"), *Prefix, *Widget->GetName(),
				*Widget->GetClass()->GetPathName());
			if (const UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Widget->Slot))
			{
				UE_LOG(LogTemp, Display, TEXT("PH HUD overlay slot %s padding=(%.1f,%.1f,%.1f,%.1f) h=%d v=%d"),
					*Widget->GetName(), OverlaySlot->Padding.Left, OverlaySlot->Padding.Top,
					OverlaySlot->Padding.Right, OverlaySlot->Padding.Bottom,
					static_cast<int32>(OverlaySlot->HorizontalAlignment),
					static_cast<int32>(OverlaySlot->VerticalAlignment));
			}
			if (const UProgressBar* ProgressBar = Cast<UProgressBar>(Widget))
			{
				const FProgressBarStyle& Style = ProgressBar->GetWidgetStyle();
				UE_LOG(LogTemp, Display, TEXT("PH HUD progress %s percent=%.3f fill=%d fill_draw=%d bg_draw=%d fill_asset=%s bg_asset=%s"),
					*Widget->GetName(), ProgressBar->GetPercent(),
					static_cast<int32>(ProgressBar->GetBarFillType()),
					static_cast<int32>(Style.FillImage.DrawAs),
					static_cast<int32>(Style.BackgroundImage.DrawAs),
					*GetPathNameSafe(Style.FillImage.GetResourceObject()),
					*GetPathNameSafe(Style.BackgroundImage.GetResourceObject()));
			}
			if (const UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
			{
				UE_LOG(LogTemp, Display, TEXT("PH HUD user widget %s superclass [%s] resource=[%d] width=%.1f height=%.1f speed=%.1f"),
					*Widget->GetName(), *GetNameSafe(UserWidget->GetClass()->GetSuperClass()),
					Cast<UHunterHUDResourceWidget>(UserWidget)
						? static_cast<int32>(Cast<UHunterHUDResourceWidget>(UserWidget)->ResourceType)
						: -1,
					Cast<UHunterHUDResourceWidget>(UserWidget)
						? Cast<UHunterHUDResourceWidget>(UserWidget)->BarWidthOverride
						: 0.0f,
					Cast<UHunterHUDResourceWidget>(UserWidget)
						? Cast<UHunterHUDResourceWidget>(UserWidget)->BarHeightOverride
						: 0.0f,
					Cast<UHunterHUDResourceWidget>(UserWidget)
						? Cast<UHunterHUDResourceWidget>(UserWidget)->FillInterpSpeed
						: 0.0f);
			}
			if (const UUserWidget* ChildUserWidget = Cast<UUserWidget>(Widget))
			{
				LogTree(ChildUserWidget->WidgetTree, Prefix + TEXT("  "));
			}
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHHUDWidgetTreeInspectionTest,
	"ProjectHunter.HUD.PlayerWidgetTreeContainsExistingResourceWidgets",
	PHHUDWidgetInspectionTests::TestFlags)

bool FPHHUDWidgetTreeInspectionTest::RunTest(const FString&)
{
	using namespace PHHUDWidgetInspectionTests;

	FTestWorldWrapper TestWorld;
	if (!TestWorld.CreateTestWorld(EWorldType::GamePreview))
	{
		TestWorld.ForwardErrorMessages(this);
		return false;
	}

	UClass* WidgetClass = StaticLoadClass(
		UUserWidget::StaticClass(), nullptr,
		TEXT("/Game/ProjectHunter/UI/HUD/WBP_HunterHUD.WBP_HunterHUD_C"));
	if (!TestNotNull(TEXT("The existing player HUD class loads"), WidgetClass))
	{
		return false;
	}

	UHunterMainHUDWidget* HUD = CreateWidget<UHunterMainHUDWidget>(TestWorld.GetTestWorld(), WidgetClass);
	if (!TestNotNull(TEXT("The existing player HUD creates without replacing its class"), HUD))
	{
		return false;
	}

	// Exercise normal Slate construction, including each resource's authored appearance.
	HUD->TakeWidget();
	LogTree(HUD->WidgetTree, TEXT(""));

	const bool bHasHealth = HUD->GetHealthWidget() != nullptr;
	const bool bHasMana = HUD->GetManaWidget() != nullptr;
	const bool bHasStamina = HUD->GetStaminaWidget() != nullptr;
	UE_LOG(LogTemp, Display, TEXT("PH HUD bound resources: Health=%s Mana=%s Stamina=%s"),
		bHasHealth ? TEXT("yes") : TEXT("no"), bHasMana ? TEXT("yes") : TEXT("no"),
		bHasStamina ? TEXT("yes") : TEXT("no"));
	TestTrue(TEXT("The existing player HUD exposes its health resource widget"), bHasHealth);
	TestTrue(TEXT("The player HUD exposes the existing mana resource implementation"), bHasMana);
	TestTrue(TEXT("The existing player HUD exposes its stamina resource widget"), bHasStamina);

	return true;
}

#endif

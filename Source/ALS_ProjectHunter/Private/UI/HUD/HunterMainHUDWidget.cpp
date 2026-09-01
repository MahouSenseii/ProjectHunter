#include "UI/HUD/HunterMainHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Character/PHBaseCharacter.h"
#include "Components/TextBlock.h"
#include "Progression/Components/CharacterProgressionManager.h"
#include "UI/HUD/HunterHUDResourceWidget.h"
#include "UI/HUD/HunterHUD_XPWidget.h"
#include "UI/HUD/StatusEffect/StatusEffectHUDWidget.h"
#include "UI/HUD/HunterHUD.h"
#include "UI/HUD/PHRunStatusWidget.h"
#include "UI/HUD/PHFloorBannerWidget.h"

namespace
{
	FSlateBrush MakeRoundedBrush(const FLinearColor& Tint, const FLinearColor& Outline,
		const FVector4& CornerRadii, const float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Tint);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(
			CornerRadii, FSlateColor(Outline), FMath::Max(OutlineWidth, 0.0f));
		return Brush;
	}

	FPHHUDResourceVisualStyle MakeResourceStyle(
		const FLinearColor& FillColor,
		const FLinearColor& BackgroundColor,
		const FLinearColor& OutlineColor,
		const FVector4& CornerRadii,
		const float Width,
		const float Height,
		const float FillSpeed,
		const float LagDelay,
		const float LagSpeed)
	{
		FPHHUDResourceVisualStyle Style;
		Style.CurrentFillColor = FillColor;
		Style.FillInterpSpeed = FillSpeed;
		Style.DamageLagDelay = LagDelay;
		Style.DamageLagInterpSpeed = LagSpeed;
		Style.BarWidthOverride = Width;
		Style.BarHeightOverride = Height;
		Style.bApplyProgressBarImageStyle = true;
		Style.bUseLayeredBarBackgrounds = true;

		// Keep the fill brush white so SetColor remains the single tint source
		// for Current, DamageLag, and Reserved. The background and outline stay
		// resource-specific.
		Style.ProgressBarImageStyle.SetBackgroundImage(
			MakeRoundedBrush(BackgroundColor, OutlineColor, CornerRadii, 1.5f));
		Style.ProgressBarImageStyle.SetFillImage(
			MakeRoundedBrush(FLinearColor::White, OutlineColor, CornerRadii, 0.75f));
		return Style;
	}
}

UHunterMainHUDWidget::UHunterMainHUDWidget()
	: HealthResourceStyle(MakeResourceStyle(
		FLinearColor(0.92f, 0.96f, 0.95f, 1.0f),
		FLinearColor(0.035f, 0.060f, 0.090f, 0.90f),
		FLinearColor(0.38f, 0.55f, 0.65f, 0.95f),
		FVector4(1.0f, 1.0f, 1.0f, 1.0f),
		560.0f, 8.0f, 16.0f, 0.30f, 8.0f))
	, ManaResourceStyle(MakeResourceStyle(
		FLinearColor(0.05f, 0.72f, 1.0f, 1.0f),
		FLinearColor(0.008f, 0.025f, 0.055f, 0.88f),
		FLinearColor(0.20f, 0.62f, 0.82f, 0.90f),
		FVector4(1.0f, 1.0f, 1.0f, 1.0f),
		520.0f, 6.0f, 18.0f, 0.12f, 9.0f))
	, StaminaResourceStyle(MakeResourceStyle(
		FLinearColor(0.75f, 0.82f, 0.95f, 1.0f),
		FLinearColor(0.018f, 0.030f, 0.065f, 0.82f),
		FLinearColor(0.34f, 0.48f, 0.68f, 0.88f),
		FVector4(0.5f, 3.0f, 0.5f, 3.0f),
		360.0f, 3.0f, 20.0f, 0.08f, 11.0f))
{
}

void UHunterMainHUDWidget::BindToCharacter(APHBaseCharacter* Character)
{
	if (!Character)
	{
		UnbindPresentationSources();
		ClearPresentationText();
	}

	InitializeForCharacter(Character);
}

void UHunterMainHUDWidget::RemoveWidget()
{
	UnbindPresentationSources();
	ClearPresentationText();
	ReleaseCharacter();
	SetVisibility(ESlateVisibility::Collapsed);
	RemoveFromParent();
}

void UHunterMainHUDWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (!IsDesignTime())
	{
		RefreshPlayerLevelText();
		RefreshHealthValueText();
	}
}

UPHRunStatusWidget* UHunterMainHUDWidget::GetRunStatusWidget() const
{
	return RunStatusWidget.Get();
}

UPHFloorBannerWidget* UHunterMainHUDWidget::GetFloorBannerWidget() const
{
	return FloorBannerWidget.Get();
}

UHunterHUDResourceWidget* UHunterMainHUDWidget::GetHealthWidget() const
{
	if (HealthWidget)
	{
		return HealthWidget.Get();
	}

	if (HealthBar)
	{
		return HealthBar.Get();
	}

	if (WPB_HealthBar)
	{
		return WPB_HealthBar.Get();
	}

	if (Health)
	{
		return Health.Get();
	}

	return FindResourceWidgetByName(TEXT("Health"));
}

UHunterHUDResourceWidget* UHunterMainHUDWidget::GetStaminaWidget() const
{
	if (StaminaWidget)
	{
		return StaminaWidget.Get();
	}

	if (StaminaBar)
	{
		return StaminaBar.Get();
	}

	if (WPB_StaminaBar)
	{
		return WPB_StaminaBar.Get();
	}

	if (Stamina)
	{
		return Stamina.Get();
	}

	return FindResourceWidgetByName(TEXT("Stamina"));
}

UHunterHUDResourceWidget* UHunterMainHUDWidget::GetManaWidget() const
{
	if (ManaWidget)
	{
		return ManaWidget.Get();
	}

	if (Mana)
	{
		return Mana.Get();
	}

	return FindResourceWidgetByName(TEXT("Mana"));
}

void UHunterMainHUDWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	UnbindPresentationSources();
	ClearPresentationText();
	PresentationCharacter = Character;
	Character->OnDestroyed.AddUniqueDynamic(
		this, &UHunterMainHUDWidget::HandlePresentationCharacterDestroyed);

	if (UCharacterProgressionManager* Progression = Character->GetProgressionManager())
	{
		PresentationProgressionManager = Progression;
		Progression->OnProgressionChanged.AddUniqueDynamic(
			this, &UHunterMainHUDWidget::RefreshPlayerLevelText);
	}

	if (UHunterHUDResourceWidget* BoundHealthWidget = GetHealthWidget())
	{
		PresentationHealthWidget = BoundHealthWidget;
		HealthStateChangedHandle = BoundHealthWidget->OnResourceStateChanged().AddUObject(
			this, &UHunterMainHUDWidget::RefreshHealthValueText);
		BoundHealthWidget->InitializeForCharacter(Character);
	}

	if (UHunterHUDResourceWidget* BoundStaminaWidget = GetStaminaWidget())
	{
		BoundStaminaWidget->InitializeForCharacter(Character);
	}

	if (UHunterHUDResourceWidget* BoundManaWidget = GetManaWidget())
	{
		BoundManaWidget->InitializeForCharacter(Character);
	}
	else if (!bLoggedMissingManaWidget)
	{
		UE_LOG(LogHunterHUD, Warning,
			TEXT("MainHUDWidget '%s' has no Mana resource child. Add one existing "
				"WBP_BaseProgressBar instance named Mana and set ResourceType to Mana."),
			*GetNameSafe(this));
		bLoggedMissingManaWidget = true;
	}

	if (XPWidget)
	{
		XPWidget->InitializeForCharacter(Character);
	}

	if (StatusEffectWidget)
	{
		StatusEffectWidget->InitializeForCharacter(Character);
	}

	RefreshPlayerLevelText();
	RefreshHealthValueText();
}

void UHunterMainHUDWidget::NativeReleaseCharacter()
{
	UnbindPresentationSources();
	ClearPresentationText();

	if (UHunterHUDResourceWidget* BoundHealthWidget = GetHealthWidget())
	{
		BoundHealthWidget->ReleaseCharacter();
	}

	if (UHunterHUDResourceWidget* BoundStaminaWidget = GetStaminaWidget())
	{
		BoundStaminaWidget->ReleaseCharacter();
	}

	if (UHunterHUDResourceWidget* BoundManaWidget = GetManaWidget())
	{
		BoundManaWidget->ReleaseCharacter();
	}

	if (XPWidget)
	{
		XPWidget->ReleaseCharacter();
	}

	if (StatusEffectWidget)
	{
		StatusEffectWidget->ReleaseCharacter();
	}
}

void UHunterMainHUDWidget::NativeDestruct()
{
	// Base release may already see an invalid character during teardown.
	UnbindPresentationSources();
	ClearPresentationText();
	Super::NativeDestruct();
}

UHunterHUDResourceWidget* UHunterMainHUDWidget::FindResourceWidgetByName(const FName WidgetName) const
{
	if (!WidgetTree || WidgetName.IsNone())
	{
		return nullptr;
	}

	return Cast<UHunterHUDResourceWidget>(WidgetTree->FindWidget(WidgetName));
}

void UHunterMainHUDWidget::UnbindPresentationSources()
{
	if (UHunterHUDResourceWidget* ResourceWidget = PresentationHealthWidget.Get())
	{
		ResourceWidget->OnResourceStateChanged().Remove(HealthStateChangedHandle);
	}
	HealthStateChangedHandle.Reset();
	PresentationHealthWidget.Reset();

	if (UCharacterProgressionManager* Progression = PresentationProgressionManager.Get())
	{
		Progression->OnProgressionChanged.RemoveDynamic(
			this, &UHunterMainHUDWidget::RefreshPlayerLevelText);
	}
	PresentationProgressionManager.Reset();

	if (APHBaseCharacter* Character = PresentationCharacter.Get())
	{
		Character->OnDestroyed.RemoveDynamic(
			this, &UHunterMainHUDWidget::HandlePresentationCharacterDestroyed);
	}
	PresentationCharacter.Reset();
}

void UHunterMainHUDWidget::ClearPresentationText() const
{
	if (PlayerLevelText)
	{
		PlayerLevelText->SetText(FText::GetEmpty());
	}
	if (HealthValueText)
	{
		HealthValueText->SetText(FText::GetEmpty());
	}
}

void UHunterMainHUDWidget::RefreshHealthValueText()
{
	if (!HealthValueText)
	{
		return;
	}

	const UHunterHUDResourceWidget* ResourceWidget = PresentationHealthWidget.Get();
	if (!PresentationCharacter.IsValid() || !ResourceWidget ||
		ResourceWidget->GetBoundCharacter() != PresentationCharacter.Get() ||
		!ResourceWidget->HasResourceState())
	{
		HealthValueText->SetText(FText::GetEmpty());
		return;
	}

	const float Current = ResourceWidget->GetCurrentValue();
	const float Max = ResourceWidget->GetMaxValue();
	if (!FMath::IsFinite(Current) || !FMath::IsFinite(Max))
	{
		HealthValueText->SetText(FText::GetEmpty());
		return;
	}

	FNumberFormattingOptions NumberFormat;
	NumberFormat.UseGrouping = true;
	NumberFormat.MinimumFractionalDigits = 0;
	NumberFormat.MaximumFractionalDigits = 0;
	HealthValueText->SetText(FText::Format(
		NSLOCTEXT("ProjectHunterHUD", "HealthValue", "{0} / {1}"),
		FText::AsNumber(Current, &NumberFormat), FText::AsNumber(Max, &NumberFormat)));
}

void UHunterMainHUDWidget::RefreshPlayerLevelText()
{
	if (!PlayerLevelText)
	{
		return;
	}

	const APHBaseCharacter* Character = PresentationCharacter.Get();
	const UCharacterProgressionManager* Progression = PresentationProgressionManager.Get();
	if (!Character || !Progression || Character->GetProgressionManager() != Progression)
	{
		PlayerLevelText->SetText(FText::GetEmpty());
		return;
	}

	PlayerLevelText->SetText(FText::Format(
		NSLOCTEXT("ProjectHunterHUD", "PlayerLevel", "Level: {0}"),
		FText::AsNumber(Progression->Level)));
}

void UHunterMainHUDWidget::HandlePresentationCharacterDestroyed(AActor*)
{
	UnbindPresentationSources();
	ClearPresentationText();
}

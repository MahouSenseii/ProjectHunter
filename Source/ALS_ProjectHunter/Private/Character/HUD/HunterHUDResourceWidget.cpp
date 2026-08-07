#include "Character/HUD/HunterHUDResourceWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "Character/PHBaseCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	EProgressBarFillType::Type GetReservedFillType(const EProgressBarFillType::Type InFillType)
	{
		switch (InFillType)
		{
		case EProgressBarFillType::LeftToRight:
			return EProgressBarFillType::RightToLeft;
		case EProgressBarFillType::RightToLeft:
			return EProgressBarFillType::LeftToRight;
		case EProgressBarFillType::TopToBottom:
			return EProgressBarFillType::BottomToTop;
		case EProgressBarFillType::BottomToTop:
			return EProgressBarFillType::TopToBottom;
		case EProgressBarFillType::FillFromCenter:
		case EProgressBarFillType::FillFromCenterHorizontal:
		case EProgressBarFillType::FillFromCenterVertical:
		default:
			return InFillType;
		}
	}

	FLinearColor MakeDarkerBarColor(const FLinearColor& InColor, const float ColorScale, const float AlphaScale)
	{
		return FLinearColor(
			InColor.R * ColorScale,
			InColor.G * ColorScale,
			InColor.B * ColorScale,
			FMath::Clamp(InColor.A * AlphaScale, 0.f, 1.f));
	}

	void SetProgressBarPercent(UProgressBar* ProgressBar, const float Percent)
	{
		if (ProgressBar)
		{
			ProgressBar->SetPercent(FMath::Clamp(Percent, 0.f, 1.f));
		}
	}
}

void UHunterHUDResourceWidget::ResolveAttributesFromResourceType()
{
	// Only fill in attributes that haven't been manually overridden in BP defaults.
	// An attribute is considered "overridden" if it IsValid() already.

	switch (ResourceType)
	{
	case EHunterResourceType::Health:
		if (!CurrentAttribute.IsValid())  CurrentAttribute  = UHunterAttributeSet::GetHealthAttribute();
		if (!MaxAttribute.IsValid())      MaxAttribute      = UHunterAttributeSet::GetMaxEffectiveHealthAttribute();
		if (!ReservedAttribute.IsValid()) ReservedAttribute = UHunterAttributeSet::GetReservedHealthAttribute();
		break;

	case EHunterResourceType::Stamina:
		if (!CurrentAttribute.IsValid())  CurrentAttribute  = UHunterAttributeSet::GetStaminaAttribute();
		if (!MaxAttribute.IsValid())      MaxAttribute      = UHunterAttributeSet::GetMaxEffectiveStaminaAttribute();
		if (!ReservedAttribute.IsValid()) ReservedAttribute = UHunterAttributeSet::GetReservedStaminaAttribute();
		break;

	case EHunterResourceType::Mana:
		if (!CurrentAttribute.IsValid())  CurrentAttribute  = UHunterAttributeSet::GetManaAttribute();
		if (!MaxAttribute.IsValid())      MaxAttribute      = UHunterAttributeSet::GetMaxEffectiveManaAttribute();
		if (!ReservedAttribute.IsValid()) ReservedAttribute = UHunterAttributeSet::GetReservedManaAttribute();
		break;

	case EHunterResourceType::ArcaneShield:
		if (!CurrentAttribute.IsValid())  CurrentAttribute  = UHunterAttributeSet::GetArcaneShieldAttribute();
		if (!MaxAttribute.IsValid())      MaxAttribute      = UHunterAttributeSet::GetMaxEffectiveArcaneShieldAttribute();
		if (!ReservedAttribute.IsValid()) ReservedAttribute = UHunterAttributeSet::GetReservedArcaneShieldAttribute();
		break;
	}
}

void UHunterHUDResourceWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	ApplyConfiguredBarAppearance();
	UpdateResourceText();

	if (IsDesignTime() && bShowDesignerPreview)
	{
		ApplyDesignerPreview();
		return;
	}

	ApplyBarPercents();
}

void UHunterHUDResourceWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	ResolveAttributesFromResourceType();

	if (!TryBindToAbilitySystem(Character))
	{
		ScheduleBindingRetry();
	}
}

void UHunterHUDResourceWidget::NativeReleaseCharacter()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindingRetryTimerHandle);
		World->GetTimerManager().ClearTimer(NonPlayerHideTimerHandle);
	}

	UnbindAttributeDelegates();
}

bool UHunterHUDResourceWidget::TryBindToAbilitySystem(APHBaseCharacter* Character)
{
	if (!Character)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	const UHunterAttributeSet* LiveAttributeSet = ASC
		? ASC->GetSet<UHunterAttributeSet>()
		: nullptr;
	if (!ASC || !LiveAttributeSet)
	{
		return false;
	}

	auto ReadAttribute = [ASC](const FGameplayAttribute& Attribute, float& OutValue) -> bool
	{
		if (!Attribute.IsValid())
		{
			return true;
		}

		if (!ASC->HasAttributeSetForAttribute(Attribute))
		{
			return false;
		}

		bool bFound = false;
		OutValue = ASC->GetGameplayAttributeValue(Attribute, bFound);
		return bFound;
	};

	float NewCurrent = CachedCurrent;
	float NewMax = CachedMax;
	float NewReserved = CachedReserved;
	if (!ReadAttribute(CurrentAttribute, NewCurrent) ||
		!ReadAttribute(MaxAttribute, NewMax) ||
		!ReadAttribute(ReservedAttribute, NewReserved))
	{
		return false;
	}

	UnbindAttributeDelegates();
	BoundASC = ASC;

	CachedCurrent = FMath::Max(NewCurrent, 0.f);
	CachedMax = FMath::Max(NewMax, 1.f);
	CachedReserved = FMath::Max(NewReserved, 0.f);

	if (CurrentAttribute.IsValid())
	{
		CurrentHandle = ASC->GetGameplayAttributeValueChangeDelegate(CurrentAttribute)
			.AddUObject(this, &UHunterHUDResourceWidget::HandleCurrentChanged);
	}

	if (MaxAttribute.IsValid())
	{
		MaxHandle = ASC->GetGameplayAttributeValueChangeDelegate(MaxAttribute)
			.AddUObject(this, &UHunterHUDResourceWidget::HandleMaxChanged);
	}

	if (ReservedAttribute.IsValid())
	{
		ReservedHandle = ASC->GetGameplayAttributeValueChangeDelegate(ReservedAttribute)
			.AddUObject(this, &UHunterHUDResourceWidget::HandleReservedChanged);
	}

	bIsPlayerOwned = Character->IsPlayerControlled();

	// Snap immediately so the bound ProgressBars open at the character's state
	// instead of showing zero until the next tick.
	DisplayFillPercent = GetFillPercent();
	DamageLagPercent = DisplayFillPercent;
	DisplayReservedPercent = GetReservedPercent();
	DamageLagDelayRemaining = 0.f;
	bDisplayInitialized = true;
	BroadcastResourceState();
	ApplyConfiguredBarAppearance();
	ApplyBarPercents();
	UpdateManagedVisibility(0.f);
	return true;
}

void UHunterHUDResourceWidget::UnbindAttributeDelegates()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (CurrentHandle.IsValid())
	{
		if (ASC && CurrentAttribute.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(CurrentAttribute).Remove(CurrentHandle);
		}
		CurrentHandle.Reset();
	}

	if (MaxHandle.IsValid())
	{
		if (ASC && MaxAttribute.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(MaxAttribute).Remove(MaxHandle);
		}
		MaxHandle.Reset();
	}

	if (ReservedHandle.IsValid())
	{
		if (ASC && ReservedAttribute.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(ReservedAttribute).Remove(ReservedHandle);
		}
		ReservedHandle.Reset();
	}

	BoundASC.Reset();
}

void UHunterHUDResourceWidget::ScheduleBindingRetry()
{
	UWorld* World = GetWorld();
	if (!World || BindingRetryTimerHandle.IsValid())
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		BindingRetryTimerHandle,
		this,
		&UHunterHUDResourceWidget::HandleBindingRetry,
		0.1f,
		true);
}

void UHunterHUDResourceWidget::HandleBindingRetry()
{
	APHBaseCharacter* Character = BoundCharacter.Get();
	if (!Character)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(BindingRetryTimerHandle);
		}
		return;
	}

	if (TryBindToAbilitySystem(Character))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(BindingRetryTimerHandle);
		}
	}
}

float UHunterHUDResourceWidget::GetFillPercent() const
{
	return (CachedMax > 0.f) ? FMath::Clamp(CachedCurrent / CachedMax, 0.f, 1.f) : 0.f;
}

float UHunterHUDResourceWidget::GetReservedPercent() const
{
	return (CachedMax > 0.f) ? FMath::Clamp(CachedReserved / CachedMax, 0.f, 1.f) : 0.f;
}

FText UHunterHUDResourceWidget::GetResourceDisplayName() const
{
	const UEnum* ResourceEnum = StaticEnum<EHunterResourceType>();
	return ResourceEnum
		? ResourceEnum->GetDisplayNameTextByValue(static_cast<int64>(ResourceType))
		: FText::GetEmpty();
}

FText UHunterHUDResourceWidget::GetFormattedResourceValue() const
{
	return BuildFormattedResourceValue(CachedCurrent, CachedMax, CachedReserved);
}

float UHunterHUDResourceWidget::DamageLag()
{
	const UWorld* World = GetWorld();
	const float DeltaSeconds = World ? World->GetDeltaSeconds() : 0.f;
	const float UpdatedPercent = UpdateDamageLagPercent(DeltaSeconds, GetFillPercent());
	ApplyBarPercents();
	return UpdatedPercent;
}

void UHunterHUDResourceWidget::SetBarFillType(const TEnumAsByte<EProgressBarFillType::Type> InBarFillType)
{
	BarFillType = InBarFillType;

	const EProgressBarFillType::Type CurrentFillType = BarFillType.GetValue();
	const EProgressBarFillType::Type ReservedFillType = GetReservedFillType(CurrentFillType);

	if (Bar_Current)
	{
		Bar_Current->SetBarFillType(CurrentFillType);
	}

	if (Bar_DamageLag)
	{
		Bar_DamageLag->SetBarFillType(CurrentFillType);
	}

	if (Bar_Reserved)
	{
		Bar_Reserved->SetBarFillType(ReservedFillType);
	}
}

void UHunterHUDResourceWidget::SetColor(const FLinearColor InCurrentFillColor)
{
	CurrentFillColor = InCurrentFillColor;

	if (Bar_Current)
	{
		Bar_Current->SetFillColorAndOpacity(CurrentFillColor);
	}

	if (Bar_DamageLag)
	{
		Bar_DamageLag->SetFillColorAndOpacity(MakeDarkerBarColor(CurrentFillColor, 0.55f, 0.65f));
	}

	if (Bar_Reserved)
	{
		Bar_Reserved->SetFillColorAndOpacity(MakeDarkerBarColor(CurrentFillColor, 0.30f, 0.45f));
	}
}

void UHunterHUDResourceWidget::SetImage(const FProgressBarStyle& InProgressBarStyle)
{
	bApplyProgressBarImageStyle = true;
	ProgressBarImageStyle = InProgressBarStyle;
	ApplyProgressBarImages(ProgressBarImageStyle);
}

void UHunterHUDResourceWidget::SetSize(const float InWidthOverride, const float InHeightOverride)
{
	BarWidthOverride = FMath::Max(InWidthOverride, 0.f);
	BarHeightOverride = FMath::Max(InHeightOverride, 0.f);

	if (!BarSize)
	{
		return;
	}

	if (BarWidthOverride > 0.f)
	{
		BarSize->SetWidthOverride(BarWidthOverride);
	}
	else
	{
		BarSize->ClearWidthOverride();
	}

	if (BarHeightOverride > 0.f)
	{
		BarSize->SetHeightOverride(BarHeightOverride);
	}
	else
	{
		BarSize->ClearHeightOverride();
	}
}

void UHunterHUDResourceWidget::HandleCurrentChanged(const FOnAttributeChangeData& Data)
{
	const float OldValue    = CachedCurrent;
	CachedCurrent           = FMath::Max(Data.NewValue, 0.f);
	const float ChangeDelta = CachedCurrent - OldValue;
	OnCurrentValueChanged(CachedCurrent, ChangeDelta);
	BroadcastResourceState();
	UpdateManagedVisibility(ChangeDelta);
}

void UHunterHUDResourceWidget::HandleMaxChanged(const FOnAttributeChangeData& Data)
{
	CachedMax = FMath::Max(Data.NewValue, 1.f);
	OnMaxValueChanged(CachedMax);
	BroadcastResourceState();
}

void UHunterHUDResourceWidget::HandleReservedChanged(const FOnAttributeChangeData& Data)
{
	CachedReserved = FMath::Max(Data.NewValue, 0.f);
	OnReservedValueChanged(CachedReserved);
	BroadcastResourceState();
}

void UHunterHUDResourceWidget::BroadcastResourceState()
{
	UpdateResourceText();
	OnResourceUpdated(CachedCurrent, CachedMax, CachedReserved,
	                  GetFillPercent(), GetReservedPercent());
}

void UHunterHUDResourceWidget::ApplyConfiguredBarAppearance()
{
	SetBarFillType(BarFillType);
	SetColor(CurrentFillColor);
	SetSize(BarWidthOverride, BarHeightOverride);

	if (bApplyProgressBarImageStyle)
	{
		ApplyProgressBarImages(ProgressBarImageStyle);
	}
}

void UHunterHUDResourceWidget::ApplyProgressBarImages(const FProgressBarStyle& InProgressBarStyle)
{
	auto ApplyImages = [&InProgressBarStyle](UProgressBar* ProgressBar)
	{
		if (!ProgressBar)
		{
			return;
		}

		FProgressBarStyle UpdatedStyle = ProgressBar->GetWidgetStyle();
		UpdatedStyle.SetBackgroundImage(InProgressBarStyle.BackgroundImage);
		UpdatedStyle.SetFillImage(InProgressBarStyle.FillImage);
		ProgressBar->SetWidgetStyle(UpdatedStyle);
	};

	ApplyImages(Bar_DamageLag);
	ApplyImages(Bar_Current);
	ApplyImages(Bar_Reserved);
}

void UHunterHUDResourceWidget::ApplyDesignerPreview()
{
	DisplayFillPercent = FMath::Clamp(DesignerPreviewFillPercent, 0.f, 1.f);
	DamageLagPercent = FMath::Clamp(DesignerPreviewDamageLagPercent, 0.f, 1.f);
	DisplayReservedPercent = FMath::Clamp(DesignerPreviewReservedPercent, 0.f, 1.f);

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ApplyBarPercents();

	if (ResourceNameText)
	{
		ResourceNameText->SetText(GetResourceDisplayName());
	}

	if (ResourceValueText)
	{
		ResourceValueText->SetText(BuildFormattedResourceValue(
			DesignerPreviewFillPercent * 100.0f,
			100.0f,
			DesignerPreviewReservedPercent * 100.0f));
	}
}

void UHunterHUDResourceWidget::ApplyBarPercents() const
{
	SetProgressBarPercent(Bar_DamageLag, DamageLagPercent);
	SetProgressBarPercent(Bar_Current, DisplayFillPercent);
	SetProgressBarPercent(Bar_Reserved, DisplayReservedPercent);
}

void UHunterHUDResourceWidget::UpdateResourceText() const
{
	if (ResourceNameText)
	{
		ResourceNameText->SetText(GetResourceDisplayName());
	}

	if (ResourceValueText)
	{
		ResourceValueText->SetText(GetFormattedResourceValue());
	}
}

FText UHunterHUDResourceWidget::BuildFormattedResourceValue(
	const float Current,
	const float Max,
	const float Reserved) const
{
	if (Reserved > KINDA_SMALL_NUMBER)
	{
		return FText::Format(
			NSLOCTEXT("ProjectHunterResource", "ValueWithReserved", "{0} / {1} ({2} Reserved)"),
			FormatResourceNumber(Current),
			FormatResourceNumber(Max),
			FormatResourceNumber(Reserved));
	}

	return FText::Format(
		NSLOCTEXT("ProjectHunterResource", "Value", "{0} / {1}"),
		FormatResourceNumber(Current),
		FormatResourceNumber(Max));
}

FText UHunterHUDResourceWidget::FormatResourceNumber(const float Value) const
{
	FNumberFormattingOptions FormattingOptions;
	FormattingOptions.MinimumFractionalDigits = ResourceValueDecimalPlaces;
	FormattingOptions.MaximumFractionalDigits = ResourceValueDecimalPlaces;
	return FText::AsNumber(Value, &FormattingOptions);
}

float UHunterHUDResourceWidget::UpdateDamageLagPercent(const float InDeltaTime, const float TargetFill)
{
	DamageLagPercent = FMath::FInterpTo(
		DamageLagPercent,
		FMath::Clamp(TargetFill, 0.f, 1.f),
		InDeltaTime,
		DamageLagInterpSpeed);

	return DamageLagPercent;
}

void UHunterHUDResourceWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	// Tick exists only to smooth the displayed bar toward the real attribute value,
	// decoupling the visual from how often the attribute changes. It early-outs once
	// settled (below) so at-rest/hidden bars are effectively free - important since
	// many enemy bars can exist at once.
	Super::NativeTick(MyGeometry, InDeltaTime);

	const float TargetFill     = GetFillPercent();
	const float TargetReserved = GetReservedPercent();

	if (!bDisplayInitialized)
	{
		// Snap on the first tick after (re)init so bars open at the right value
		// rather than sweeping up from zero.
		DisplayFillPercent      = TargetFill;
		DamageLagPercent        = TargetFill;
		DisplayReservedPercent  = TargetReserved;
		DamageLagDelayRemaining = 0.f;
		bDisplayInitialized     = true;
		ApplyBarPercents();
		return;
	}

	// Nothing to interpolate once the displayed values match their targets.
	if (FMath::IsNearlyEqual(DisplayFillPercent, TargetFill, 0.001f) &&
		FMath::IsNearlyEqual(DisplayReservedPercent, TargetReserved, 0.001f) &&
		FMath::IsNearlyEqual(DamageLagPercent, DisplayFillPercent, 0.001f))
	{
		return;
	}

	DisplayFillPercent     = FMath::FInterpTo(DisplayFillPercent, TargetFill, InDeltaTime, FillInterpSpeed);
	DisplayReservedPercent = FMath::FInterpTo(DisplayReservedPercent, TargetReserved, InDeltaTime, FillInterpSpeed);

	// Damage lag: follow the fill up instantly on a gain, but trail behind on a
	// loss after a short delay so the amount lost stays readable for a beat.
	if (DisplayFillPercent >= DamageLagPercent)
	{
		DamageLagPercent        = DisplayFillPercent;
		DamageLagDelayRemaining = DamageLagDelay;
	}
	else
	{
		DamageLagDelayRemaining -= InDeltaTime;
		if (DamageLagDelayRemaining <= 0.f)
		{
			UpdateDamageLagPercent(InDeltaTime, DisplayFillPercent);
		}
	}

	ApplyBarPercents();
}

void UHunterHUDResourceWidget::UpdateManagedVisibility(float ChangeDelta)
{
	if (!bManageVisibility)
	{
		return;
	}

	if (bIsPlayerOwned)
	{
		// Player Stamina bar only matters while stamina is being spent, so it hides
		// once (nearly) full. Other player bars keep their BP-set visibility.
		if (ResourceType == EHunterResourceType::Stamina)
		{
			SetVisibility(GetFillPercent() >= StaminaHideWhenFullThreshold
				? ESlateVisibility::Hidden
				: ESlateVisibility::SelfHitTestInvisible);
		}
		return;
	}

	if (!bAutoHideNonPlayerBar)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(NonPlayerHideTimerHandle);
		}

		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		return;
	}

	// Non-player bar: hidden until Current is lost, then shown briefly.
	// Healing or no-op updates should not reveal the bar or refresh the hide timer.
	const float CurrentLossAmount = -ChangeDelta;
	if (CurrentLossAmount > KINDA_SMALL_NUMBER)
	{
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				NonPlayerHideTimerHandle, this,
				&UHunterHUDResourceWidget::HideBar, NonPlayerAutoHideDelay, false);
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (World->GetTimerManager().IsTimerActive(NonPlayerHideTimerHandle))
		{
			return;
		}
	}

	SetVisibility(ESlateVisibility::Hidden);
}

void UHunterHUDResourceWidget::HideBar()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(NonPlayerHideTimerHandle);
	}

	SetVisibility(ESlateVisibility::Hidden);
}

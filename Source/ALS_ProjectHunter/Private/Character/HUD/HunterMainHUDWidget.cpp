#include "Character/HUD/HunterMainHUDWidget.h"

#include "Character/HUD/HunterHUDResourceWidget.h"
#include "Character/HUD/HunterHUD_XPWidget.h"
#include "Character/HUD/StatusEffect/StatusEffectHUDWidget.h"

void UHunterMainHUDWidget::BindToCharacter(APHBaseCharacter* Character)
{
	InitializeForCharacter(Character);
}

void UHunterMainHUDWidget::RemoveWidget()
{
	ReleaseCharacter();
	SetVisibility(ESlateVisibility::Collapsed);
	RemoveFromParent();
}

UHunterHUDResourceWidget* UHunterMainHUDWidget::GetHealthWidget() const
{
	if (HealthWidget)
	{
		return HealthWidget.Get();
	}

	return HealthBar ? HealthBar.Get() : WPB_HealthBar.Get();
}

UHunterHUDResourceWidget* UHunterMainHUDWidget::GetStaminaWidget() const
{
	if (StaminaWidget)
	{
		return StaminaWidget.Get();
	}

	return StaminaBar ? StaminaBar.Get() : WPB_StaminaBar.Get();
}

void UHunterMainHUDWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	if (UHunterHUDResourceWidget* BoundHealthWidget = GetHealthWidget())
	{
		BoundHealthWidget->InitializeForCharacter(Character);
	}

	if (UHunterHUDResourceWidget* BoundStaminaWidget = GetStaminaWidget())
	{
		BoundStaminaWidget->InitializeForCharacter(Character);
	}

	if (ManaWidget)
	{
		ManaWidget->InitializeForCharacter(Character);
	}

	if (XPWidget)
	{
		XPWidget->InitializeForCharacter(Character);
	}

	if (StatusEffectWidget)
	{
		StatusEffectWidget->InitializeForCharacter(Character);
	}
}

void UHunterMainHUDWidget::NativeReleaseCharacter()
{
	if (UHunterHUDResourceWidget* BoundHealthWidget = GetHealthWidget())
	{
		BoundHealthWidget->ReleaseCharacter();
	}

	if (UHunterHUDResourceWidget* BoundStaminaWidget = GetStaminaWidget())
	{
		BoundStaminaWidget->ReleaseCharacter();
	}

	if (ManaWidget)
	{
		ManaWidget->ReleaseCharacter();
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

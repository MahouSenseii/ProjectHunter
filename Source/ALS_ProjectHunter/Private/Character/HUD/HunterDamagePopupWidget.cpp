#include "Character/HUD/HunterDamagePopupWidget.h"

#include "Combat/Library/FunctionLibraries/CombatFunctionLibrary.h"
#include "Components/WidgetComponent.h"

void UHunterDamagePopupWidget::InitializeDamagePopup(const FCombatDamagePopupData& InPopupData)
{
	PopupData = InPopupData;
	OnDamagePopupInitialized(PopupData);
}

FText UHunterDamagePopupWidget::GetDamageText() const
{
	return UCombatFunctionLibrary::FormatDamagePopupAmount(PopupData.TotalDamage);
}

void UHunterDamagePopupWidget::FinishDamagePopup()
{
	if (UWidgetComponent* WidgetComponent = OwningWorldWidgetComponent.Get())
	{
		OwningWorldWidgetComponent.Reset();
		WidgetComponent->DestroyComponent();
		return;
	}

	RemoveFromParent();
}

void UHunterDamagePopupWidget::SetOwningWorldWidgetComponent(UWidgetComponent* InWidgetComponent)
{
	OwningWorldWidgetComponent = InWidgetComponent;
}

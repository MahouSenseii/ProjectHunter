#include "Character/HUD/HunterHUDBaseWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Character/PHBaseCharacter.h"
#include "Components/Widget.h"

void UHunterHUDBaseWidget::InitializeForCharacter(APHBaseCharacter* Character)
{
	if (Character && BoundCharacter.Get() == Character)
	{
		return;
	}

	if (BoundCharacter.IsValid())
	{
		ReleaseCharacter();
	}

	if (!Character)
	{
		return;
	}

	BoundCharacter = Character;
	NativeInitializeForCharacter(Character);
	InitializeChildHUDWidgets(Character);
	OnCharacterBound(Character);
}

void UHunterHUDBaseWidget::ReleaseCharacter()
{
	if (!BoundCharacter.IsValid())
	{
		return;
	}

	OnCharacterReleased();
	NativeReleaseCharacter();
	ReleaseChildHUDWidgets();
	BoundCharacter.Reset();
}

void UHunterHUDBaseWidget::NativeDestruct()
{
	ReleaseCharacter();
	Super::NativeDestruct();
}

void UHunterHUDBaseWidget::InitializeChildHUDWidgets(APHBaseCharacter* Character)
{
	if (!WidgetTree || !Character)
	{
		return;
	}

	TArray<UWidget*> ChildWidgets;
	WidgetTree->GetAllWidgets(ChildWidgets);

	for (UWidget* ChildWidget : ChildWidgets)
	{
		UHunterHUDBaseWidget* ChildHUDWidget = Cast<UHunterHUDBaseWidget>(ChildWidget);
		if (ChildHUDWidget && ChildHUDWidget != this)
		{
			ChildHUDWidget->InitializeForCharacter(Character);
		}
	}
}

void UHunterHUDBaseWidget::ReleaseChildHUDWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	TArray<UWidget*> ChildWidgets;
	WidgetTree->GetAllWidgets(ChildWidgets);

	for (UWidget* ChildWidget : ChildWidgets)
	{
		UHunterHUDBaseWidget* ChildHUDWidget = Cast<UHunterHUDBaseWidget>(ChildWidget);
		if (ChildHUDWidget && ChildHUDWidget != this)
		{
			ChildHUDWidget->ReleaseCharacter();
		}
	}
}

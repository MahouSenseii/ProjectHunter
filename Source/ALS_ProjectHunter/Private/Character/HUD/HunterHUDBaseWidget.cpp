// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#include "Character/HUD/HunterHUDBaseWidget.h"
#include "Character/PHBaseCharacter.h"

void UHunterHUDBaseWidget::InitializeForCharacter(APHBaseCharacter* Character)
{
	// Release any previous binding cleanly before rebinding
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
	BoundCharacter.Reset();
}

void UHunterHUDBaseWidget::NativeDestruct()
{
	// Clean up any live delegate handles before the widget is garbage-collected
	ReleaseCharacter();
	Super::NativeDestruct();
}

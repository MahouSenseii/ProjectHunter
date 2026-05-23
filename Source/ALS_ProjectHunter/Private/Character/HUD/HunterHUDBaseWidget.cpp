#include "Character/HUD/HunterHUDBaseWidget.h"
#include "Character/PHBaseCharacter.h"

void UHunterHUDBaseWidget::InitializeForCharacter(APHBaseCharacter* Character)
{
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
	ReleaseCharacter();
	Super::NativeDestruct();
}

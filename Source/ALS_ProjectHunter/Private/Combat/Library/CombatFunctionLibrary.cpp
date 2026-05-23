#include "Combat/Library/CombatFunctionLibrary.h"
#include "Character/PHBaseCharacter.h"


float UCombatFunctionLibrary::GetHealthPercent(const APHBaseCharacter* Character)
{
	if (!Character )
	{
		return 0.f;
	}
	return Character->GetHealthPercent();
}

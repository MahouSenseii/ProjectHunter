// Combat/Library/CombatFunctionLibrary.cpp
#include "Combat/Library/CombatFunctionLibrary.h"
#include "Character/PHBaseCharacter.h"

bool UCombatFunctionLibrary::AreHostile(const APHBaseCharacter* Source, const APHBaseCharacter* Target)
{
	if (!Source || !Target)
	{
		return false;
	}
	return Source->IsHostile(Target);
}

bool UCombatFunctionLibrary::IsAlive(const APHBaseCharacter* Character)
{
	return Character && !Character->bIsDead;
}

float UCombatFunctionLibrary::GetHealthPercent(const APHBaseCharacter* Character)
{
	if (!Character || Character->bIsDead)
	{
		return 0.f;
	}
	return Character->GetHealthPercent();
}

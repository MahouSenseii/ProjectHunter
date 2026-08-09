#pragma once

#include "CoreMinimal.h"

class UAbilitySystemComponent;
class UCharacterProgressionManager;
class UHunterAttributeSet;

class ALS_PROJECTHUNTER_API FProgressionAbilityHelper
{
public:
	static UAbilitySystemComponent* GetAbilitySystemComponent(const UCharacterProgressionManager& Manager);
	static UHunterAttributeSet* GetAttributeSet(const UCharacterProgressionManager& Manager);
	static void TrySyncPlayerLevelAttribute(UAbilitySystemComponent* ASC, int32 Level);
};

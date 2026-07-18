#include "Progression/Library/FunctionLibraries/ProgressionStatFunctionLibrary.h"

#include "AbilitySystem/HunterAttributeSet.h"

FGameplayAttribute UProgressionStatFunctionLibrary::GetAttributeForStatName(const FName StatName)
{
	if (StatName == "Strength")     return UHunterAttributeSet::GetStrengthAttribute();
	if (StatName == "Intelligence") return UHunterAttributeSet::GetIntelligenceAttribute();
	if (StatName == "Dexterity")    return UHunterAttributeSet::GetDexterityAttribute();
	if (StatName == "Endurance")    return UHunterAttributeSet::GetEnduranceAttribute();
	if (StatName == "Affliction")   return UHunterAttributeSet::GetAfflictionAttribute();
	if (StatName == "Luck")         return UHunterAttributeSet::GetLuckAttribute();
	if (StatName == "Covenant")     return UHunterAttributeSet::GetCovenantAttribute();

	return FGameplayAttribute{};
}

#include "Item/Library/FunctionLibraries/ItemRequirementFunctionLibrary.h"

bool UItemRequirementFunctionLibrary::MeetsItemRequirements(
	const FItemStatRequirement& Requirements,
	int32 HunterLevel,
	int32 Strength,
	int32 Dexterity,
	int32 Intelligence,
	int32 Endurance,
	int32 Affliction,
	int32 Luck,
	int32 Covenant)
{
	return Requirements.MeetsRequirements(
		HunterLevel,
		Strength,
		Dexterity,
		Intelligence,
		Endurance,
		Affliction,
		Luck,
		Covenant);
}

int32 UItemRequirementFunctionLibrary::GetRequiredLevel(const FItemStatRequirement& Requirements)
{
	return Requirements.RequiredLevel;
}

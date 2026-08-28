// Item/Library/Structs/ItemRequirementStructs.cpp

#include "Item/Library/Structs/ItemRequirementStructs.h"

namespace
{
void AddRequirementCheck(
	FItemRequirementCheckResult& Result,
	const EItemRequirementType RequirementType,
	const float CurrentValue,
	const float RequiredValue)
{
	if (RequiredValue <= 0.0f)
	{
		return;
	}

	FItemRequirementStatus& Status = Result.Checks.AddDefaulted_GetRef();
	Status.RequirementType = RequirementType;
	Status.CurrentValue = CurrentValue;
	Status.RequiredValue = RequiredValue;
	Status.MissingValue = FMath::Max(RequiredValue - CurrentValue, 0.0f);
	Status.bMet = Status.MissingValue <= 0.0f;

	if (Status.bMet)
	{
		return;
	}

	FItemRequirementFailure& Failure = Result.Failures.AddDefaulted_GetRef();
	Failure.RequirementType = RequirementType;
	Failure.CurrentValue = CurrentValue;
	Failure.RequiredValue = RequiredValue;
	Failure.MissingValue = Status.MissingValue;
}
}

FItemRequirementCheckResult FItemStatRequirement::EvaluateRequirements(
	const float Level,
	const float Strength,
	const float Dexterity,
	const float Intelligence,
	const float Endurance,
	const float Affliction,
	const float Luck,
	const float Covenant) const
{
	FItemRequirementCheckResult Result;
	Result.bItemValid = true;
	Result.bStatsAvailable = true;

	AddRequirementCheck(Result, EItemRequirementType::Level, Level, RequiredLevel);
	AddRequirementCheck(Result, EItemRequirementType::Strength, Strength, RequiredStrength);
	AddRequirementCheck(Result, EItemRequirementType::Dexterity, Dexterity, RequiredDexterity);
	AddRequirementCheck(Result, EItemRequirementType::Intelligence, Intelligence, RequiredIntelligence);
	AddRequirementCheck(Result, EItemRequirementType::Endurance, Endurance, RequiredEndurance);
	AddRequirementCheck(Result, EItemRequirementType::Affliction, Affliction, RequiredAffliction);
	AddRequirementCheck(Result, EItemRequirementType::Luck, Luck, RequiredLuck);
	AddRequirementCheck(Result, EItemRequirementType::Covenant, Covenant, RequiredCovenant);

	Result.bMeetsRequirements = Result.Failures.IsEmpty();
	return Result;
}

bool FItemStatRequirement::MeetsRequirements(
	const int32 Level,
	const int32 Strength,
	const int32 Dexterity,
	const int32 Intelligence,
	const int32 Endurance,
	const int32 Affliction,
	const int32 Luck,
	const int32 Covenant) const
{
	return EvaluateRequirements(
		Level,
		Strength,
		Dexterity,
		Intelligence,
		Endurance,
		Affliction,
		Luck,
		Covenant).bMeetsRequirements;
}

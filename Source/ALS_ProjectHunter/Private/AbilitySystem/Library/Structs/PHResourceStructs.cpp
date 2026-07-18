#include "AbilitySystem/Library/Structs/PHResourceStructs.h"

FPHResourceReservationInput::FPHResourceReservationInput(
	const float InRawMaxValue,
	const float InFlatReservedValue,
	const float InPercentageReservedValue,
	const float InExistingReservedValue,
	const float InMaxReservedValue)
	: RawMaxValue(InRawMaxValue)
	, FlatReservedValue(InFlatReservedValue)
	, PercentageReservedValue(InPercentageReservedValue)
	, ExistingReservedValue(InExistingReservedValue)
	, MaxReservedValue(InMaxReservedValue)
{
}

FPHPrimaryDerivedResourceInput::FPHPrimaryDerivedResourceInput(
	const float InBaseMaxValue,
	const float InBasePrimaryBonus,
	const float InPrimaryValue,
	const float InPlayerLevel,
	const float InPerLevelBonus)
	: BaseMaxValue(InBaseMaxValue)
	, BasePrimaryBonus(InBasePrimaryBonus)
	, PrimaryValue(InPrimaryValue)
	, PlayerLevel(InPlayerLevel)
	, PerLevelBonus(InPerLevelBonus)
{
}

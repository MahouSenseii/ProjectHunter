// Item/Moveset/MovesetData.cpp
#include "Item/Moveset/MovesetData.h"
#include "Curves/CurveFloat.h"

int64 UMovesetData::GetXPRequiredForLevel(int32 TargetLevel) const
{
	if (TargetLevel < 2)
	{
		return 0;
	}

	if (XPCurve)
	{
		return static_cast<int64>(XPCurve->GetFloatValue(static_cast<float>(TargetLevel)));
	}

	// Default exponential curve: 500 * 2^(level-2)
	// Level 2  =    500 XP
	// Level 5  =   4 000 XP
	// Level 10 = 128 000 XP
	// Level 20 = ~131 M  XP
	return static_cast<int64>(500.0 * FMath::Pow(2.0, static_cast<double>(TargetLevel - 2)));
}

bool UMovesetData::IsCompatibleWithWeapon(EItemSubType WeaponSubType) const
{
	// Empty = accepts any weapon
	if (CompatibleWeaponTypes.Num() == 0)
	{
		return true;
	}
	return CompatibleWeaponTypes.Contains(WeaponSubType);
}

// Data/MonsterModifierData.cpp
#include "Data/MonsterModifierData.h"

float UMonsterSpawnConfig::GetEffectiveMagicChance(int32 AreaLevel, float MagicFind) const
{
	const float LevelBonus = FMath::Max(0, AreaLevel - 1) * MagicChancePerAreaLevel;
	const float MFBonus    = MagicFind * MagicFindChanceScalar;
	return FMath::Clamp(BaseMagicChance + LevelBonus + MFBonus, 0.0f, 1.0f);
}

float UMonsterSpawnConfig::GetEffectiveRareChance(int32 AreaLevel, float MagicFind) const
{
	const float LevelBonus = FMath::Max(0, AreaLevel - 1) * RareChancePerAreaLevel;
	const float MFBonus    = MagicFind * MagicFindChanceScalar * 0.5f; // Half MF weight for rares
	return FMath::Clamp(BaseRareChance + LevelBonus + MFBonus, 0.0f, 1.0f);
}

EMonsterTier UMonsterSpawnConfig::RollMonsterTier(int32 AreaLevel, float MagicFind) const
{
	const float Roll       = FMath::FRand();
	const float RareChance = GetEffectiveRareChance(AreaLevel, MagicFind);
	const float MagicChance = GetEffectiveMagicChance(AreaLevel, MagicFind);

	if (Roll < RareChance)
	{
		return EMonsterTier::MT_Rare;
	}
	if (Roll < MagicChance)
	{
		return EMonsterTier::MT_Magic;
	}
	return EMonsterTier::MT_Normal;
}

int32 UMonsterSpawnConfig::RollPackSize(EMonsterTier Tier) const
{
	switch (Tier)
	{
	case EMonsterTier::MT_Magic:
		return FMath::RandRange(MagicPackMin, MagicPackMax);
	case EMonsterTier::MT_Rare:
		return FMath::RandRange(RarePackMin, RarePackMax);
	default:
		return FMath::RandRange(NormalPackMin, NormalPackMax);
	}
}

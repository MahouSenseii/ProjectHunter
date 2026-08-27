#include "AI/Data/MonsterModifierData.h"

float UMonsterSpawnConfig::GetEffectiveMagicChance(int32 AreaLevel, float MagicFind) const
{
	const float LevelBonus = FMath::Max(0, AreaLevel - 1) * MagicChancePerAreaLevel;
	const float MFBonus    = MagicFind * MagicFindChanceScalar;
	return FMath::Clamp(BaseMagicChance + LevelBonus + MFBonus, 0.0f, 1.0f);
}

float UMonsterSpawnConfig::GetEffectiveRareChance(int32 AreaLevel, float MagicFind) const
{
	const float LevelBonus = FMath::Max(0, AreaLevel - 1) * RareChancePerAreaLevel;
	const float MFBonus    = MagicFind * MagicFindChanceScalar * 0.5f;
	return FMath::Clamp(BaseRareChance + LevelBonus + MFBonus, 0.0f, 1.0f);
}

namespace MonsterSpawnConfigPrivate
{
	EMonsterTier ResolveTierFromRoll(const float Roll, const float RareChance, const float MagicChance)
	{
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
}

EMonsterTier UMonsterSpawnConfig::RollMonsterTier(int32 AreaLevel, float MagicFind) const
{
	return MonsterSpawnConfigPrivate::ResolveTierFromRoll(
		FMath::FRand(),
		GetEffectiveRareChance(AreaLevel, MagicFind),
		GetEffectiveMagicChance(AreaLevel, MagicFind));
}

EMonsterTier UMonsterSpawnConfig::RollMonsterTierSeeded(
	int32 AreaLevel, float MagicFind, FRandomStream& Stream) const
{
	return MonsterSpawnConfigPrivate::ResolveTierFromRoll(
		Stream.FRand(),
		GetEffectiveRareChance(AreaLevel, MagicFind),
		GetEffectiveMagicChance(AreaLevel, MagicFind));
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

int32 UMonsterSpawnConfig::RollPackSizeSeeded(EMonsterTier Tier, FRandomStream& Stream) const
{
	switch (Tier)
	{
	case EMonsterTier::MT_Magic:
		return Stream.RandRange(MagicPackMin, MagicPackMax);
	case EMonsterTier::MT_Rare:
		return Stream.RandRange(RarePackMin, RarePackMax);
	default:
		return Stream.RandRange(NormalPackMin, NormalPackMax);
	}
}

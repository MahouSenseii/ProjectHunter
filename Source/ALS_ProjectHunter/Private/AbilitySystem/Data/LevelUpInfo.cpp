// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/LevelUpInfo.h"

ULevelUpInfo::ULevelUpInfo()
{
	LevelUpInformation.SetNum(101);

		for(int i = 0; i <= 100 ; i++)
		{
			const int32 ExpNeeded = BaseExpMultiplier * FMath::Pow(i, 2.0f);
			LevelUpInformation[i].LevelUpRequirement = ExpNeeded;
	
		}
}

FLevelUpResult ULevelUpInfo::TryLevelUp(const int32 XP, const int32 Level)
{
	FLevelUpResult Result;

	if (HasLeveledUp(XP, Level))
	{
		Result.bLeveledUp = true;
		Result.XPLeft = XP - LevelUpInformation[Level].LevelUpRequirement;
		Result.AttributePointsAwarded = LevelUpInformation[Level].AttributePointAward;
	}

	return Result;
}

int32 ULevelUpInfo::GetXpNeededForLevelUp(const int32 Level) const
{
	return LevelUpInformation[Level].LevelUpRequirement;
}

bool ULevelUpInfo::HasLeveledUp(const int32 XP, const int32 Level) const
{
	return XP >= GetXpNeededForLevelUp(Level);
}
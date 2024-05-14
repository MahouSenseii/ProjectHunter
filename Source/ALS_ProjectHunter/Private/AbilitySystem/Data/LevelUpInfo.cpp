// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/LevelUpInfo.h"

ULevelUpInfo::ULevelUpInfo()
{
	LevelUpInformation.SetNum(101);

		for(int i = 0; i <= 100 ; i++)
		{
			const int32 ExpNeeded = BaseExpMultiplier * FMath::Pow(i, 2.0f);
			LevelUpInformation[i].LevelUpRequirement = ExpNeeded;
			if(i != 0)
			{
				LevelUpInformation[i].AttributePointAward = 1;
			}
		}
}

int32 ULevelUpInfo::GetXpNeededForLevelUp(const int32 Level)
{
	return LevelUpInformation[Level].LevelUpRequirement;
}

int32 ULevelUpInfo::LevelUp(const int32 XP , int32 Level)
{
	if(LevelUpInformation[Level].LevelUpRequirement <= XP)
	{
		return XP - LevelUpInformation[Level].LevelUpRequirement;
	}
	return 0;
}



// Fill out your copyright notice in the Description page of Project Settings.


#include "Interfaces/CombatSubInterface.h"

// Add default functionality here for any ICombatSubInterface functions that are not pure virtual.

int32 ICombatSubInterface::GetPlayerLevel()
{
	return 1;
}

int32 ICombatSubInterface::GetCurrentXP()
{
	return 1;
}

int32 ICombatSubInterface::GetXPFNeededForNextLevel()
{
	return 1;			
}

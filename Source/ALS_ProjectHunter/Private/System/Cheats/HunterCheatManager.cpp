// Fill out your copyright notice in the Description page of Project Settings.


#include "System/Cheats/HunterCheatManager.h"

#include "System/Cheats/HunterCheatComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UHunterCheatManager::DiableStaminaDegen()
{
	if (UHunterCheatComponent* CheatComponent = GetHunterCheatComponent())
	{
		CheatComponent->DisableStaminaDrain();
	}
}

void UHunterCheatManager::RefillHealth()
{
	if (UHunterCheatComponent* CheatComponent = GetHunterCheatComponent())
	{
		CheatComponent->RefillHealth();
	}
}

void UHunterCheatManager::RefillStamina()
{
	if (UHunterCheatComponent* CheatComponent = GetHunterCheatComponent())
	{
		CheatComponent->RefillStamina();
	}
}

void UHunterCheatManager::ReserveHealth(float NewValue)
{
	if (UHunterCheatComponent* CheatComponent = GetHunterCheatComponent())
	{
		CheatComponent->ReserveHealth(NewValue);
	}
}

void UHunterCheatManager::ShowCheatList()
{
	if (UHunterCheatComponent* CheatComponent = GetHunterCheatComponent())
	{
		CheatComponent->ShowCheatList();
	}
}

UHunterCheatComponent* UHunterCheatManager::GetHunterCheatComponent() const
{
	APlayerController* PC = GetPlayerController();
	if (!PC)
	{
		return nullptr;
	}

	if (UHunterCheatComponent* CheatComponent =
		PC->FindComponentByClass<UHunterCheatComponent>())
	{
		return CheatComponent;
	}

	APawn* Pawn = PC->GetPawn();
	return Pawn ? Pawn->FindComponentByClass<UHunterCheatComponent>() : nullptr;
}

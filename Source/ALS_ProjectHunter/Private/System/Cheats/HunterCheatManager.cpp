#include "System/Cheats/HunterCheatManager.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "System/Cheats/HunterCheatComponent.h"

void UHunterCheatManager::DisableStaminaDrain()
{
	if (UHunterCheatComponent* CheatComponent = GetHunterCheatComponent())
	{
		CheatComponent->DisableStaminaDrain();
	}
}

void UHunterCheatManager::ReactivateStaminaDrain()
{
	if (UHunterCheatComponent* CheatComponent = GetHunterCheatComponent())
	{
		CheatComponent->ReactivateStaminaDrain();
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

void UHunterCheatManager::ReserveHealth(const float NewValue)
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
	APlayerController* PlayerController = GetPlayerController();
	if (!PlayerController)
	{
		return nullptr;
	}

	if (UHunterCheatComponent* CheatComponent = PlayerController->FindComponentByClass<UHunterCheatComponent>())
	{
		return CheatComponent;
	}

	APawn* Pawn = PlayerController->GetPawn();
	return Pawn ? Pawn->FindComponentByClass<UHunterCheatComponent>() : nullptr;
}

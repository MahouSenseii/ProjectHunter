// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "HunterCheatManager.generated.h"

class UHunterCheatComponent;
/**
 * @class UHunterCheatManager
 * @brief Custom cheat manager to provide useful cheat commands for gameplay testing and debugging.
 *
 * This class extends the base UCheatManager class and adds a variety of commands
 * that allow developers and testers to manipulate in-game variables such as health and stamina,
 * as well as provide debugging information.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterCheatManager : public UCheatManager
{
	GENERATED_BODY()
	
public:
	
	// Removes Stamina Degen will set to 0 
	UFUNCTION(exec)
	void DisableStaminaDrain();

	//Re-addes Stamina Degen
	UFUNCTION(exec)
	void ReactivateStaminaDrain();
	
	// Refill Health to max
	UFUNCTION(exec)
	void RefillHealth();
	
	// Refill Stamina to max
	UFUNCTION(exec)
	void RefillStamina();
	
	
	//Set Reserved amount of health
	UFUNCTION(exec)
	void ReserveHealth(float NewValue);
	
	// Prints all available Project Hunter cheat/debug commands.
	UFUNCTION(exec)
	void ShowCheatList();
	
private:
	UHunterCheatComponent* GetHunterCheatComponent() const;
};

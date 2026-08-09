#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "HunterCheatManager.generated.h"

class UHunterCheatComponent;

UCLASS()
class ALS_PROJECTHUNTER_API UHunterCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION(exec)
	void DisableStaminaDrain();

	UFUNCTION(exec)
	void ReactivateStaminaDrain();

	UFUNCTION(exec)
	void RefillHealth();

	UFUNCTION(exec)
	void RefillStamina();

	UFUNCTION(exec)
	void ReserveHealth(float NewValue);

	UFUNCTION(exec)
	void ShowCheatList();

private:
	UHunterCheatComponent* GetHunterCheatComponent() const;
};

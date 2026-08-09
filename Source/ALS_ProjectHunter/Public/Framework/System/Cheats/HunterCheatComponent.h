#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HunterCheatComponent.generated.h"

class UHunterAbilitySystemComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UHunterCheatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHunterCheatComponent();

	UFUNCTION(BlueprintCallable, Category = "Project Hunter|Cheats|Stats")
	void DisableStaminaDrain();

	UFUNCTION(BlueprintCallable, Category = "Project Hunter|Cheats|Stats")
	void ReactivateStaminaDrain();

	UFUNCTION(BlueprintCallable, Category = "Project Hunter|Cheats|Stats")
	void RefillHealth();

	UFUNCTION(BlueprintCallable, Category = "Project Hunter|Cheats|Stats")
	void RefillStamina();

	UFUNCTION(BlueprintCallable, Category = "Project Hunter|Cheats|Stats")
	void ReserveHealth(float NewValue);

	UFUNCTION(BlueprintCallable, Category = "Project Hunter|Cheats")
	void ShowCheatList();

private:
	UHunterAbilitySystemComponent* GetTargetASC() const;
};

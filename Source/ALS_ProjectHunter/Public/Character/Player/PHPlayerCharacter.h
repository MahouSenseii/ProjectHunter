// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "Character/PHBaseCharacter.h"
#include "PHPlayerCharacter.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API APHPlayerCharacter : public APHBaseCharacter
{
	GENERATED_BODY()

public:


	
	APHPlayerCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_PlayerState() override;

	/** Combat Interface */

	virtual int32 GetPlayerLevel() override;
	/** End Combat Interface */

	UFUNCTION(BlueprintCallable, Category = "Manager")
	UInventoryManager* GetInventoryManager(){ return InventoryManager;}
	
protected:
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Manager")
	TObjectPtr<UInventoryManager> InventoryManager;
private:

	virtual void InitAbilityActorInfo() override;

};

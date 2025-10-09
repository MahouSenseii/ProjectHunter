// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "Library/PHCharacterEnumLibrary.h"
#include "RegenEffect.generated.h"

/**
 * 
 */

UCLASS()
class ALS_PROJECTHUNTER_API URegenEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	URegenEffect();

	/** Which vital resource this effect regenerates */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen")
	EVitalRegenType RegenType;

protected:
	/** Configure the effect based on the regen type */
	virtual void PostInitProperties() override;

private:
	/** Setup modifiers for Health regeneration */
	void SetupHealthRegen();
    
	/** Setup modifiers for Mana regeneration */
	void SetupManaRegen();
    
	/** Setup modifiers for Stamina regeneration */
	void SetupStaminaRegen();
    
	/** Setup modifiers for Arcane Shield regeneration */
	void SetupArcaneShieldRegen();
	
};

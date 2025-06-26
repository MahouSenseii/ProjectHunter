// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/PHBaseCharacter.h"
#include "Components/ActorComponent.h"
#include "Item/WeaponItem.h"
#include "Library/PHCombatStructLibrary.h"
#include "CombatManager.generated.h"




UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALS_PROJECTHUNTER_API UCombatManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCombatManager();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;


	UFUNCTION()
	static bool RollForStatusEffect(const APHBaseCharacter* Attacker, EDamageTypes DamageType);

	/* ============================= */
	/* === Poise & Stagger System === */
	/* ============================= */

	

public:	
	
		
};

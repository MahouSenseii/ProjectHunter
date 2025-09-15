// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PHGameMode.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API APHGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	
	// Called when the game starts
	virtual void BeginPlay() override;

};

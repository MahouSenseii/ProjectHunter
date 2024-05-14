// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Library/PHCharacterStructLibrary.h"
#include "LevelUpInfo.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API ULevelUpInfo : public UDataAsset
{
	GENERATED_BODY()

public:

	ULevelUpInfo();
	
	UPROPERTY(EditDefaultsOnly)
	TArray<FPHLevelUpInfo> LevelUpInformation;

	UFUNCTION(BlueprintCallable)
	int32 GetXpNeededForLevelUp(const int32 Level);

	UFUNCTION()
	int32 LevelUp(const int32 XP, int32 Level);

private:

	UPROPERTY()
	float BaseExpMultiplier = 100.0f;
		
};

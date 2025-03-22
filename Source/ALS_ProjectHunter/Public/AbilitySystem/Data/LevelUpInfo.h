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
	int32 GetXpNeededForLevelUp(const int32 Level) const;

	UFUNCTION(BlueprintCallable)
	bool HasLeveledUp(const int32 XP, const int32 Level) const;

	UFUNCTION(BlueprintCallable)
	FLevelUpResult TryLevelUp(const int32 XP, const int32 Level);

private:

	UPROPERTY(EditDefaultsOnly)
	float BaseExpMultiplier = 100.0f;
		
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "PHAssetManager.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	
	static UPHAssetManager& Get();

protected:

	virtual void StartInitialLoading() override;
};

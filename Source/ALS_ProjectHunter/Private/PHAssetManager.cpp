// Fill out your copyright notice in the Description page of Project Settings.


#include "PHAssetManager.h"
#include "PHGameplayTags.h"

UPHAssetManager& UPHAssetManager::Get()
{
	check(GEngine);
	UPHAssetManager* PHAssetManager = 	Cast<UPHAssetManager>(GEngine->AssetManager);
	return  *PHAssetManager;
}

void UPHAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	FPHGameplayTags::InitializeNativeGameplayTags();
}

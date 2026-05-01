// Fill out your copyright notice in the Description page of Pt9 Settings.


#include "System/PHAssetManager.h"
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

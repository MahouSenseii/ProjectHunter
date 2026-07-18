#include "System/PHAssetManager.h"

#include "PHGameplayTags.h"

UPHAssetManager& UPHAssetManager::Get()
{
	check(GEngine);

	UPHAssetManager* PHAssetManager = Cast<UPHAssetManager>(GEngine->AssetManager);
	check(PHAssetManager);

	return *PHAssetManager;
}

void UPHAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	FPHGameplayTags::InitializeNativeGameplayTags();
}

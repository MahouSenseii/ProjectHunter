#include "Framework/System/PHAssetManager.h"

#include "Tags/PHGameplayTags.h"

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

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Tower/Library/Structs/StashSaveStructs.h"
#include "StashSaveGame.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API UStashTabSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName TabID;

	UPROPERTY()
	FIntPoint GridSize = FIntPoint(12, 12);

	UPROPERTY()
	TArray<FStashItemSaveData> Items;
};

UCLASS()
class ALS_PROJECTHUNTER_API UStashHandlesSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FStashTabHandleSaveData> Handles;
};

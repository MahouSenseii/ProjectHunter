#pragma once

#include "CoreMinimal.h"
#include "Tower/Library/Enums/StashEnumLibrary.h"
#include "StashSaveStructs.generated.h"

USTRUCT()
struct ALS_PROJECTHUNTER_API FStashItemSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	FIntPoint GridPosition = FIntPoint::ZeroValue;

	UPROPERTY()
	TArray<uint8> ItemBytes;

	UPROPERTY()
	FSoftClassPath ItemClassPath;
};

USTRUCT()
struct ALS_PROJECTHUNTER_API FStashTabHandleSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	FName TabID;

	UPROPERTY()
	FText TabName;

	UPROPERTY()
	EStashTabType TabType = EStashTabType::STT_Normal;

	UPROPERTY()
	int32 CachedItemCount = 0;

	UPROPERTY()
	FLinearColor AccentColor = FLinearColor::White;
};

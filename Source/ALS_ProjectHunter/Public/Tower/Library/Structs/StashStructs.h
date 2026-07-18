#pragma once

#include "CoreMinimal.h"
#include "Tower/Library/Enums/StashEnumLibrary.h"
#include "StashStructs.generated.h"

class UItemInstance;

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FStashItemEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	TObjectPtr<UItemInstance> Item;

	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	FIntPoint GridPosition = FIntPoint::ZeroValue;

	FStashItemEntry() = default;
	FStashItemEntry(UItemInstance* InItem, FIntPoint InPos)
		: Item(InItem), GridPosition(InPos)
	{}
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FStashTabData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stash")
	FName TabID;

	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	TArray<FStashItemEntry> Items;

	UPROPERTY(BlueprintReadOnly, Category = "Stash")
	FIntPoint GridSize = FIntPoint(12, 12);

	FStashTabData() = default;
	explicit FStashTabData(FName InID) : TabID(InID) {}
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FStashTabHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stash")
	FName TabID;

	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	FText TabName;

	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	EStashTabType TabType = EStashTabType::STT_Normal;

	UPROPERTY(BlueprintReadOnly, Category = "Stash")
	int32 CachedItemCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	FLinearColor AccentColor = FLinearColor::White;

	UPROPERTY(Transient)
	bool bIsLoaded = false;

	UPROPERTY(Transient)
	bool bIsDirty = false;

	FStashTabHandle() = default;
	FStashTabHandle(FName InID, FText InName, EStashTabType InType = EStashTabType::STT_Normal)
		: TabID(InID), TabName(InName), TabType(InType)
	{}
};

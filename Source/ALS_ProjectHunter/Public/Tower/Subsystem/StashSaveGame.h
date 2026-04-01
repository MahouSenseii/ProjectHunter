// Tower/Subsystem/StashSaveGame.h
//
// OPT-SAVE: Minimal save-game objects for the lazy-loading stash system.
// Each stash tab gets its own save slot; handles are saved in a separate slot.
// Item data is serialized as raw bytes via FObjectAndNameAsStringProxyArchive so
// UItemInstance UObjects (with their SaveGame UPROPERTYs) survive round-tripping.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Tower/Subsystem/StashSubsystem.h"
#include "StashSaveGame.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// Serialized form of a single stash item (grid pos + raw item bytes)
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT()
struct FStashItemSaveData
{
	GENERATED_BODY()

	/** Grid position within the tab */
	UPROPERTY()
	FIntPoint GridPosition = FIntPoint::ZeroValue;

	/** Raw bytes produced by serializing the UItemInstance */
	UPROPERTY()
	TArray<uint8> ItemBytes;

	/** Class path of the UItemInstance subclass (needed to reconstruct) */
	UPROPERTY()
	FSoftClassPath ItemClassPath;
};

// ─────────────────────────────────────────────────────────────────────────────
// Save object for a single stash tab
// ─────────────────────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
// Serialized form of a tab handle (lightweight metadata)
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT()
struct FStashTabHandleSaveData
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

// ─────────────────────────────────────────────────────────────────────────────
// Save object for all tab handles (loaded once on login)
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class ALS_PROJECTHUNTER_API UStashHandlesSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FStashTabHandleSaveData> Handles;
};

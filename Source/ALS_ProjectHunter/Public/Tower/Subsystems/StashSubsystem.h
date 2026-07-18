#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tower/Library/Enums/StashEnumLibrary.h"
#include "Tower/Library/Structs/StashStructs.h"
#include "StashSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogStashSubsystem, Log, All);

class UItemInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStashTabLoaded, FName, TabID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStashItemAdded,
	FName, TabID, UItemInstance*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStashItemRemoved,
	FName, TabID, UItemInstance*, Item);

UCLASS()
class ALS_PROJECTHUNTER_API UStashSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Stash")
	void LoadStashHandles(const FString& CharacterSlotName);

	UFUNCTION(BlueprintCallable, Category = "Stash")
	bool RequestTabData(int32 TabIndex);

	// C++ only because UHT cannot expose USTRUCT pointers.
	FStashTabData* GetLoadedTabData(int32 TabIndex);

	UFUNCTION(BlueprintPure, Category = "Stash")
	bool IsTabLoaded(int32 TabIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Stash")
	bool AddItemToTab(int32 TabIndex, UItemInstance* Item, FIntPoint GridPos);

	UFUNCTION(BlueprintCallable, Category = "Stash")
	bool AddItemToTabAutoPlace(int32 TabIndex, UItemInstance* Item);

	UFUNCTION(BlueprintCallable, Category = "Stash")
	UItemInstance* RemoveItemFromTab(int32 TabIndex, FIntPoint GridPos);

	UFUNCTION(BlueprintCallable, Category = "Stash")
	bool MoveItem(int32 FromTabIndex, FIntPoint FromPos, int32 ToTabIndex, FIntPoint ToPos);

	UFUNCTION(BlueprintCallable, Category = "Stash")
	void MarkTabDirty(int32 TabIndex);

	UFUNCTION(BlueprintCallable, Category = "Stash")
	void FlushDirtyTabs();

	UFUNCTION(BlueprintCallable, Category = "Stash")
	void UnloadCleanTabs();

	UFUNCTION(BlueprintCallable, Category = "Stash")
	int32 AddTab(const FText& Name, EStashTabType Type = EStashTabType::STT_Normal);

	UFUNCTION(BlueprintCallable, Category = "Stash")
	void RenameTab(int32 TabIndex, const FText& NewName);

	UFUNCTION(BlueprintPure, Category = "Stash")
	const TArray<FStashTabHandle>& GetTabHandles() const { return TabHandles; }

	UFUNCTION(BlueprintPure, Category = "Stash")
	int32 GetTabCount() const { return TabHandles.Num(); }

	UPROPERTY(BlueprintAssignable, Category = "Stash|Events")
	FOnStashTabLoaded OnStashTabLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Stash|Events")
	FOnStashItemAdded OnStashItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Stash|Events")
	FOnStashItemRemoved OnStashItemRemoved;

private:
	UPROPERTY()
	TArray<FStashTabHandle> TabHandles;

	UPROPERTY()
	TMap<FName, FStashTabData> LoadedTabs;

	UPROPERTY()
	FString ActiveSlotName;

	FString BuildTabSlotName(FName TabID) const;

	bool FindFreeGridPosition(const FStashTabData& Tab, FIntPoint& OutPos) const;

	void SaveTab(int32 TabIndex);

	void SaveHandles();

	bool LoadTab(int32 TabIndex);

	bool IsValidTabIndex(int32 TabIndex) const;
};

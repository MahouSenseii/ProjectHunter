// Copyright@2024 Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Character/PHBaseCharacter.h"
#include "Components/ActorComponent.h"
#include "Item/BaseItem.h"
#include "InventoryManager.generated.h"

class UInteractableWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGemsChanged);
DECLARE_LOG_CATEGORY_EXTERN(LogInventoryManager, Log, All);
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UInventoryManager : public UActorComponent
{
	GENERATED_BODY()

public:
	/* ============================= */
	/* ===      Constructor      === */
	/* ============================= */

	UInventoryManager();
	void GenerateID();

	/* ============================= */
	/* ===      Core Logic       === */
	/* ============================= */

	UFUNCTION(BlueprintCallable)
	APHBaseCharacter* SetPlayerCharacter();

	UFUNCTION(BlueprintCallable)
	APHBaseCharacter* GetOwnerCharacter() const;

	UFUNCTION(BlueprintCallable)
	FString GetID() { return InventoryID; }

	UFUNCTION(BlueprintCallable)
	int32 GetTileSize() { return TileSize; }

	UFUNCTION(BlueprintCallable, Category = "Base")
	void ResizeInventory();

	void ClearAllSlots();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Converters")
	int32 TileToIndex(FTile Tile);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Converters")
	void IndexToTile(int32 Index, FTile& Tile);

	UFUNCTION(BlueprintCallable, Category = "Converters")
	TMap<UBaseItem*, FTile> GetAllItems();

	UFUNCTION(BlueprintCallable, Category = "Remove")
	void RemoveItemInInventory(UBaseItem* Item);
	
	void ForEachOccupiedTile(UBaseItem* Item, const TFunctionRef<void(const FTile&)>& Func) const;

	UFUNCTION(BlueprintCallable, Category = "Remove")
	bool DropItemInInventory(UBaseItem* Item);

	UFUNCTION(BlueprintCallable, Category = "Checker")
	bool GetItemAtTile(int32 Index, UBaseItem*& RetrievedItem);

	UFUNCTION(BlueprintCallable, Category = "Checker")
	bool IsRoomAvailable(UBaseItem* Item, int32 TopLeftIndex);

	bool ForEachTile(UBaseItem* Item, int32 TopLeftIndex, const TFunction<bool(FTile)>& Func);

	UFUNCTION(BlueprintCallable, Category = "Checker")
	bool IsTileValid(FTile Tile) const;

	UFUNCTION(BlueprintCallable, Category = "Checker")
	bool ContainsItem(const UBaseItem* Item) const;

	UFUNCTION(BlueprintCallable, Category = "Add")
	bool TryToAddItemToInventory(UBaseItem* Item, bool CheckRotated);

	UFUNCTION(BlueprintCallable, Category = "Add")
	void AddItemAt(UBaseItem* Item, int32 TopLeftIndex);

	UFUNCTION(BlueprintCallable, Category = "Add")
	bool TryToAddItemToInventoryRotated(UBaseItem* Item);
	
	void RebuildTopLeftMap();

protected:
	virtual void BeginPlay() override;
	void InitializeInventoryManager();

	UFUNCTION()
	FVector GetSpawnLocation() const;


public:

	
	/* ============================= */
	/* ===       Helpers         === */
	/* ============================= */

	static bool AreItemsStackable(UBaseItem* A, UBaseItem* B);
	bool CanAcceptItemAt(UBaseItem* NewItem, int32 Index);
	UBaseItem* GetItemAt(int32 Index) const;

	TArray<FTile> GetOccupiedTilesForItem(UBaseItem* Item) const;
	void ValidateTileMap();

private:
	
	void SetItemInTiles(UBaseItem* Item, const FTile& TopLeft);
	void ClearItemFromTiles(UBaseItem* Item, const FTile& TopLeft);

	


public:
	/* ============================= */
	/* ===         Events         === */
	/* ============================= */

	UPROPERTY(BlueprintAssignable)
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable)
	FOnGemsChanged OnGemsChanged;

	/* ============================= */
	/* ===        Public Vars     === */
	/* ============================= */

	UPROPERTY(BlueprintReadOnly)
	TArray<UBaseItem*> InventoryList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid")
	int32 Rows = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid")
	int32 Columns = 14;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid")
	float TileSize = 20.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Checker")
	bool Generated = false;

	UPROPERTY(BlueprintReadWrite, Category = "Checker")
	FVector2D RarityMinMaxClamp;

	UPROPERTY(BlueprintReadWrite)
	TMap<EItemRarity, int32> RarityMap;

	UPROPERTY(BlueprintReadWrite)
	int32 LootAmountRoll = 0;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UInventoryManager> OtherInventory = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shop")
	UDataTable* SpawnableItems;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shop")
	UDataTable* MasterDropList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shop")
	float MinThreshold = .35f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shop")
	FVector2D MinMaxLootAmount;

	UPROPERTY(BlueprintReadOnly)
	APHBaseCharacter* OwnerCharacter;


private:
	/* ============================= */
	/* ===       Private Vars  ==== */
	/* ============================= */
	UPROPERTY()
	int32 Gems = 1000;

	UPROPERTY(Transient)
	TMap<FTile, UBaseItem*> TopLeftItemMap;
	
	FString InventoryID = "";
};
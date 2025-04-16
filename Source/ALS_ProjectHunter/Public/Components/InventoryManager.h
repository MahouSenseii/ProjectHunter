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
	
	static void ForEachOccupiedTile(const FTile& TopLeft, const FVector2D& Dimensions, const TFunctionRef<void(const FTile&)>& Func);

	UFUNCTION(BlueprintCallable, Category = "Remove")
	bool DropItemInInventory(UBaseItem* Item);

	UFUNCTION(BlueprintCallable, Category = "Checker")
	bool GetItemAtTile(int32 Index, UBaseItem*& RetrievedItem);

	UFUNCTION(BlueprintCallable, Category = "Checker")
	bool IsRoomAvailable(UBaseItem* Item, int32 TopLeftIndex);

	bool ForEachTile(UBaseItem* Item, int32 TopLeftIndex, const TFunction<void(FTile)>& Func);

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

	
	/* ============================= */
	/* ===        Shop           === */
	/* ============================= */

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RandomizeInventory();

	UFUNCTION(BlueprintCallable, Category = "Shop")
	int32 GetGeneratedItemAmount() const;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool SpawnItemByName(const FName ItemName, const UDataTable* DataTable);

	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintGetter, Category = "Shop")
	int32 GetGems() const { return Gems; }

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void AddGems(int32 InAmount) { Gems += InAmount; }

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void SubtractGems(int32 InAmount) { Gems -= InAmount; }

	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool HasEnoughGems(UBaseItem* Item) const;

	static int32 CalculateStackedItemValue(const FItemInformation& ItemData);
	static int32 CalculateValue(const FItemInformation& ItemData);
	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool ExecuteItemTrade(const TArray<UBaseItem*>& Items, UInventoryManager* Seller, UInventoryManager* Buyer, const TArray<int32>& OptionalTargetTileIndices, FString& OutMessage);

	static bool CanBuyerAffordItems(const TArray<UBaseItem*>& Items, UInventoryManager* Seller, int32& OutTotalCost, FString& OutMessage);
	static bool FindPlacementIndices(const TArray<UBaseItem*>& Items, UInventoryManager* Buyer, const TArray<int32>& OptionalTargetTileIndices, TArray<int32>& OutValidIndices, FString& OutMessage);
	static void FinalizeTrade(const TArray<UBaseItem*>& Items, UInventoryManager* Seller, UInventoryManager* Buyer, const TArray<int32>& ValidIndices, int32 TotalCost);

	
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
	int32 Colums = 14;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid")
	float TileSize = 15.0f;

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
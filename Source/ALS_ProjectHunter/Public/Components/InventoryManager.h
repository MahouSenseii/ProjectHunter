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

	/* ============================= */
	/* ===      Core Logic       === */
	/* ============================= */

	// Initialization
	UFUNCTION(BlueprintCallable)
	void InitializeInventoryManager();
	
	void GenerateID();

	// Character Management
	UFUNCTION(BlueprintCallable)
	APHBaseCharacter* SetPlayerCharacter();

	UFUNCTION(BlueprintCallable)
	APHBaseCharacter* GetOwnerCharacter() const { return OwnerCharacter; }

	// Inventory Properties
	UFUNCTION(BlueprintCallable)
	FString GetID() const { return InventoryID; }

	UFUNCTION(BlueprintCallable)
	int32 GetTileSize() const { return TileSize; }

	UFUNCTION(BlueprintCallable)
	int32 GetOccupiedSlotCount() const { return OccupiedSlotCount; }

	UFUNCTION(BlueprintCallable)
	int32 GetTotalSlotCount() const { return Rows * Columns; }

	// Inventory Management
	UFUNCTION(BlueprintCallable, Category = "Base")
	void ResizeInventory();

	void ClearAllSlots();

	// Coordinate Conversion
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Converters")
	int32 TileToIndex(const FTile& Tile) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Converters")
	FTile IndexToTile(int32 Index) const;

	// Item Queries
	UFUNCTION(BlueprintCallable, Category = "Converters")
	TMap<UBaseItem*, FTile> GetAllItems() const;

	UFUNCTION(BlueprintCallable, Category = "Checker")
	bool GetItemAtTile(int32 Index, UBaseItem*& RetrievedItem) const;

	UFUNCTION(BlueprintCallable, Category = "Checker")
	UBaseItem* GetItemAt(int32 Index) const;

	UFUNCTION(BlueprintCallable, Category = "Checker")
	bool ContainsItem(const UBaseItem* Item) const;

	UFUNCTION(BlueprintCallable, Category = "Checker")
	FTile GetItemTopLeft(const UBaseItem* Item) const;

	// Item Addition
	UFUNCTION(BlueprintCallable, Category = "Add")
	bool TryToAddItemToInventory(UBaseItem* Item, bool CheckRotated = true);

	UFUNCTION(BlueprintCallable, Category = "Add")
	bool TryToAddItemAt(UBaseItem* Item, int32 TopLeftIndex);

	UFUNCTION(BlueprintCallable, Category = "Add")
	bool TryToAddItemAtTile(UBaseItem* Item, const FTile& TopLeftTile);

	// Item Removal
	UFUNCTION(BlueprintCallable, Category = "Remove")
	bool RemoveItemFromInventory(UBaseItem* Item);
	
	UFUNCTION(BlueprintCallable, Category = "Remove")
	bool DropItemFromInventory(UBaseItem* Item);

	// Space Checking
	UFUNCTION(BlueprintCallable, Category = "Checker")
	bool IsRoomAvailable(const UBaseItem* Item, int32 TopLeftIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Checker")
	bool IsRoomAvailableAtTile(const UBaseItem* Item, const FTile& TopLeftTile) const;

	UFUNCTION(BlueprintCallable, Category = "Checker")
	bool IsTileValid(const FTile& Tile) const;
	

	// Helper Functions
	static bool AreItemsStackable( UBaseItem* A,  UBaseItem* B);
	static bool CanStackItems( UBaseItem* ExistingItem,  UBaseItem* NewItem);
	TArray<FTile> GetOccupiedTilesForItem( UBaseItem* Item) const;

	UFUNCTION(BlueprintCallable, Category="Stacking")
	bool CanStackItemInstances(class UItemInstanceObject* A, class UItemInstanceObject* B) const;

	UFUNCTION(BlueprintCallable, Category="Stacking")
	bool TryStackItemInstance(class UItemInstanceObject* ItemInstance);
	
	// Validation (Editor only)
#if WITH_EDITOR
	void ValidateInventoryIntegrity() const;
#endif

protected:
	virtual void BeginPlay() override;


	// Internal Helpers
	UFUNCTION()
	FVector GetDropLocation() const;

private:
	// Internal Item Management
	bool TryToAddItemRotated(UBaseItem* Item);
	bool TryPlaceItemAtFirstAvailable(UBaseItem* Item, bool CheckRotated);
	bool InternalAddItemAt(UBaseItem* Item, const FTile& TopLeftTile);
	void SetItemInTiles(UBaseItem* Item, const FTile& TopLeft);
	void ClearItemFromTiles(const UBaseItem* Item, const FTile& TopLeft);
	static bool ForEachTile(const UBaseItem* Item, const FTile& StartTile, const TFunctionRef<bool(const FTile&)>& Func);
	
	// Fast Lookup Management
	void RegisterItemInLookups(UBaseItem* Item, const FTile& TopLeft);
	void UnregisterItemFromLookups(UBaseItem* Item);
	void InvalidateCaches() const;


	// Stacking
	bool TryStackItem(UBaseItem* Item);
	void RebuildStackableCache();

public:
	/* ============================= */
	/* ===         Events        === */
	/* ============================= */

	UPROPERTY(BlueprintAssignable)
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable)
	FOnGemsChanged OnGemsChanged;

	/* ============================= */
	/* ===     Configuration     === */
	/* ============================= */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid")
	int32 Rows = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid")
	int32 Columns = 14;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid")
	float TileSize = 20.0f;

	// Drop Configuration
	UPROPERTY(EditDefaultsOnly, Category = "Inventory|Dropping")
	float ItemDropDistance = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory|Dropping")
	float ItemDropHeightOffset = 120.0f;

	// Shop/Generation
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shop")
	UDataTable* SpawnableItems;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shop")
	UDataTable* MasterDropList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shop")
	float MinThreshold = .35f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shop")
	FVector2D MinMaxLootAmount;

	// Runtime State
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

protected:
	UPROPERTY(BlueprintReadOnly)
	APHBaseCharacter* OwnerCharacter;

private:
	/* ============================= */
	/* ===    Private State      === */
	/* ============================= */
	
	// Core Inventory Data
	UPROPERTY(Transient)
	TArray<UBaseItem*> InventoryList;

	UPROPERTY(Transient)
	TMap<FTile, UBaseItem*> TopLeftItemMap;

	// Fast Lookup Tables
	mutable TMultiMap<FName, UBaseItem*> StackableItemsCache;
	mutable bool bStackableCacheDirty = true;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UBaseItem>, FTile> ItemToTopLeftMap;

	// Cached Data
	UPROPERTY(Transient)
	int32 OccupiedSlotCount = 0;

	mutable TMap<UBaseItem*, FTile> CachedAllItems;
	mutable bool bAllItemsCacheDirty = true;

	// Inventory Properties
	UPROPERTY()
	int32 Gems = 1000;

	FString InventoryID = "";
};
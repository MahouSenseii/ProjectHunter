// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/PHBaseCharacter.h"
#include "Components/ActorComponent.h"
#include "Item/BaseItem.h"
#include "InventoryManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGemsChanged);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALS_PROJECTHUNTER_API UInventoryManager : public UActorComponent
{
	GENERATED_BODY()

public:

	// Declare your Event Dispatcher
	// Events
	UPROPERTY(BlueprintAssignable)
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable)
	FOnGemsChanged OnGemsChanged;
	
	
	// Sets default values for this component's properties
	// Constructor
	UInventoryManager();
	void GenerateID();

	// Helper functions
	UFUNCTION(BlueprintCallable)
	APHBaseCharacter* SetPlayerCharacter();

	// Components and references
	UPROPERTY(BlueprintReadOnly)
	TArray<UBaseItem*> InventoryList;

	UFUNCTION(BlueprintCallable)
	APHBaseCharacter* GetOwnerCharacter() const;

	UFUNCTION(BlueprintCallable)
	FString GetID() { return InventoryID;}

	// Components and references
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid")
	int32 Rows = 7;

	/**
	 * 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid")
	int32 Colums = 14;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid")
	float TileSize = 30.0f;

	// State
	UPROPERTY(BlueprintReadWrite, Category = "Checker")
	bool IsDirty = false;

	UPROPERTY(BlueprintReadWrite, Category = "Checker")
	bool Generated = false;

	UPROPERTY(BlueprintReadWrite, Category = "Checker")
	FVector2D RarityMinMaxClamp;

	UPROPERTY(BlueprintReadWrite)
	TMap<EItemRarity, int32>RarityMap;

	UPROPERTY(BlueprintReadWrite)
	int32 LootAmountRoll = 0;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	void InitializeInventoryManager();


	UPROPERTY(BlueprintReadOnly)
	APHBaseCharacter* OwnerCharacter;

	UFUNCTION()
	FVector GetSpawnLocation() const;


public:
	

	// Conversion helpers
	UFUNCTION(BlueprintCallable, Category = "Base")
	void ResizeInventory();
	void ClearAllSlots();

	// Conversion helpers
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Converters")
	int32 TileToIndex(FTile Tile);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Converters")
	void IndexToTile(int32 Index, FTile& Tile);

	// Fetch data
	UFUNCTION(BlueprintCallable, Category = "Converters")
	TMap<UBaseItem*, FTile> GetAllItems();

	UFUNCTION(BlueprintCallable, Category = "Remove")
	void RemoveItemInInventory(UBaseItem* Item);

	UFUNCTION(BlueprintCallable, Category = "Remove")
	bool DropItemInInventory(UBaseItem* Item);

	UFUNCTION(BlueprintCallable, Category = "Checker")
	bool GetItemAtTile(int32 Index, UBaseItem*& RetrievedItem);

	UFUNCTION(BlueprintCallable, Category = "Checker")
	bool IsRoomAvailable(UBaseItem* Item, int32 TopLeftIndex);

	bool   ForEachTile(UBaseItem* Item, int32 TopLeftIndex, const TFunction<void(FTile)>& Func);

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
	
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RandomizeInventory();

	UFUNCTION(BlueprintCallable, Category = "Shop")
	int32 GetGeneratedItemAmount() const;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool SpawnItemByName(const FName ItemName, const UDataTable* DataTable);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shop")
	UDataTable* SpawnableItems;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shop")
	UDataTable* MasterDropList;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shop")
	float MinThreshold = .35;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shop")
	FVector2D MinMaxLootAmount;

	// Gems

	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintGetter, Category = "Shop")
	int32 GetGems() const { return Gems; }

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void AddGems(int32 InAmount) {  Gems =+ InAmount; };

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void SubtractGems(int32 InAmount) { Gems =- InAmount; };

	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool HasEnoughGems(UBaseItem* Item) const;

	static int32 CalculateStackedItemValue(const FItemInformation& ItemData);
	static int32 CalculateValue(const FItemInformation& ItemData);


private:

	
	UPROPERTY()
	int32 Gems = 1000;
	
	FString InventoryID = "";
};


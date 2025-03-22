// Copyright@2024 Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpawnableLootManager.generated.h"

class UPHAttributeSet;
class AItemPickup;
class UBoxComponent;
class UDataTable;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API USpawnableLootManager : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USpawnableLootManager();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	/** Generates the number of items that should drop, influenced by attributes */
	UFUNCTION(BlueprintCallable, Category = "Loot|Drop System")
	int32 GenerateDropAmount(UPHAttributeSet* AttributeSet);
	

	/** Gets a random spawn location within the designated spawn box */
	UFUNCTION(BlueprintCallable, Category = "Loot|Drop System")
	FTransform GetSpawnLocation() const;

	/** Retrieves the currently assigned SpawnBox */
	UFUNCTION(BlueprintGetter, Category = "Loot|Drop System")
	UBoxComponent* GetSpawnBox() const { return SpawnBox; }

	/** Assigns a new SpawnBox for determining item drop locations */
	UFUNCTION(BlueprintCallable, Category = "Loot|Drop System")
	void SetSpawnBox(UBoxComponent* InBox) { SpawnBox = InBox; }

	/** Spawns an item by its name from the given DataTable */
	UFUNCTION(BlueprintCallable, Category = "Loot|Drop System")
	void SpawnItemByName(const FName ItemName, UDataTable* DataTable, const FTransform SpawnTransform);

	/** Determines and spawns the appropriate loot based on the player's attributes */
	UFUNCTION(BlueprintCallable, Category = "Loot|Drop System")
	void GetSpawnItem(UPHAttributeSet* AttributeSet);

	/** Defines the minimum and maximum possible loot drops */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot|Drop Settings")
	FVector2D MinMaxLootAmount;

	/** Data table defining all potential spawnable items */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot|Drop Tables")
	UDataTable* SpawnableItems;

	/** Master data table containing all item definitions */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot|Drop Tables")
	UDataTable* MasterDropList;

	/** Minimum threshold probability for rolling the lowest amount of loot */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot|Drop Settings")
	float MinThreshold = 0.75f;

private:
	/** The class of pickup item to spawn */
	TSubclassOf<AItemPickup> PickUpClass;

	/** The designated spawn area for item drops */
	UPROPERTY()
	UBoxComponent* SpawnBox = nullptr;

	/** Tracks whether loot has already been claimed */
	UPROPERTY()
	bool bLooted = false;
};

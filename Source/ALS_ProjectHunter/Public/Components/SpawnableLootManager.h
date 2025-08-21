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

	UFUNCTION(BlueprintCallable, Category = "Loot|Control")
	void ResetLootStatus() { bLooted = false; }

	/** Spawns an item by its name from the given DataTable */
	UFUNCTION(BlueprintCallable, Category = "Loot|Drop System")
	void SpawnItemByName(const FName ItemName, UDataTable* DataTable, const FTransform SpawnTransform);

	/** Determines and spawns the appropriate loot based on the player's attributes */
	UFUNCTION(BlueprintCallable, Category = "Loot|Drop System")
	void GetSpawnItem(UPHAttributeSet* AttributeSet);

	/** Defines the minimum and maximum possible loot drops */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot|Drop Settings")
	FVector2D MinMaxLootAmount;

	/** Minimum threshold probability for rolling the lowest amount of loot */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot|Drop Settings")
	float MinThreshold = 0.75f;

	/** Data table defining all potential spawnable items */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot|Drop Tables")
	UDataTable* SpawnableItems;

	/** Master data table containing all item definitions */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot|Drop Tables")
	UDataTable* MasterDropList;

	/** Optional height offset above the ground hit location when spawning items */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Drop Settings")
	float GroundOffsetZ = 10.0f;

	/** Controls the diminishing returns curve for Luck. Higher = slower progression. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Luck")
	float LuckSoftCapConstant = 100.0f;

	/** Expected max Luck a player can achieve (used for scaling) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Luck")
	float MaxPlayerLuck = 100.0f;

	/** Base probability (0–1) that at least one item drops before quantity roll */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot|Drop Settings")
	float BaseDropChance = 0.45f;


private:
	/** The class of pickup item to spawn */
	UPROPERTY(EditAnywhere, Category = "Loot|Internal")
	TSubclassOf<AItemPickup> PickUpClass;

	/** The designated spawn area for item drops */
	UPROPERTY()
	UBoxComponent* SpawnBox = nullptr;

	/** Tracks whether loot has already been claimed */
	UPROPERTY(BlueprintReadOnly, Category = "Loot|Internal", meta = (AllowPrivateAccess = "true"))
	bool bLooted = false;
};

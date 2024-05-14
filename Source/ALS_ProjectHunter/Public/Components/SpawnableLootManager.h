// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpawnableLootManager.generated.h"

class UPHAttributeSet;
class AItemPickup;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
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

	UFUNCTION(BlueprintCallable, BlueprintCallable)
	int32 GenerateDropAmount(UPHAttributeSet* AttributeSet);

	UFUNCTION(BlueprintCallable, BlueprintCallable)
	FTransform GetSpawnLocation();

	UFUNCTION(BlueprintGetter, BlueprintCallable)
	UBoxComponent* GetSpawnBox() const { return SpawnBox; }

	UFUNCTION(BlueprintCallable)
	void SetSpawnBox(UBoxComponent* InBox) { SpawnBox = InBox; }

	UFUNCTION(BlueprintCallable)
	void SpawnItemByName(FName ItemName, UDataTable* DataTable, FTransform SpawnTransform);

	UFUNCTION(BlueprintCallable)
	void GetSpawnItem(UPHAttributeSet* AttributeSet);

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector2D MinMaxLootAmount;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot")
	UDataTable* SpawnableItems;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot")
	UDataTable* MasterDropList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot")
	float MinThreshold = .75;

private:

	TSubclassOf<AItemPickup> PickUpClass;

	UPROPERTY()
	UBoxComponent* SpawnBox;

	UPROPERTY()
	bool Looted = false;
};

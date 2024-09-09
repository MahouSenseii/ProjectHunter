// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Library/PHItemEnumLibrary.h"
#include "Item/EquippableItem.h"
#include "Components/ActorComponent.h"
#include "EquipmentManager.generated.h"

// Declare the delegate (if using non-standard types, make sure you've included the header files that define them)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentChanged);

/**
 * 
 */

class UBaseItem;
class AALSCharacter;
class ACharacterCapture;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALS_PROJECTHUNTER_API UEquipmentManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UEquipmentManager();

	UFUNCTION()
	void UpdateMesh(UBaseItem* Item, EEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable, Category = "Checker")
	bool CheckSlot(UBaseItem* Item);

	UFUNCTION()
	TArray<UBaseItem*> EquipmentCheck() const;

	UFUNCTION(BlueprintCallable)
	bool IsItemEquippable(UBaseItem* Item);

	UFUNCTION(BlueprintCallable)
	bool AddItemInSlotToInventory(UBaseItem* Item);

	UFUNCTION(BlueprintCallable)
	void TryToEquip(UBaseItem* Item, bool HasMesh, EEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable)
	void HandleHasMesh(UBaseItem* Item, EEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable)
	void HandleNoMesh(UBaseItem* Item, EEquipmentSlot Slot);


	UFUNCTION(BlueprintCallable)
	void RemoveEquippedItem(UBaseItem* Item, EEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable)
	void RemoveItemInSlot(EEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable)
	bool IsSocketEmpty(EEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable)
	void AttachItem(TSubclassOf<AEquippedObject> Class, UBaseItem* Item, EEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable)
	FName FindSlotName(EEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable)
	bool DropItem(UBaseItem* Item);

	UFUNCTION()
	UInventoryManager* GetInventoryManager() const;

	FVector GetGroundSpawnLocation() const;
	

protected:
	// Called when the game starts
	virtual void BeginPlay() override;


	//Variables
	
public:
	
	UPROPERTY(BlueprintAssignable)
	FOnEquipmentChanged OnEquipmentChanged;

	UPROPERTY(BlueprintReadOnly, Category = "Owner")
	TObjectPtr<AALSCharacter> OwnerCharacter;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Checker")
	TMap<EEquipmentSlot, UBaseItem*> EquipmentData;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Checker")
	TMap<EEquipmentSlot, AEquippedObject*> EquipmentItem;

	
protected:
	
};

// Copyright@2024 Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Library/PHItemEnumLibrary.h"
#include "Item/EquippableItem.h"
#include "Components/ActorComponent.h"
#include "EquipmentManager.generated.h"

// Delegate for broadcasting equipment changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentChanged);

class UBaseItem;
class AALSCharacter;
class ACharacterCapture;

/**
 *  Manages character equipment and inventory interactions.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UEquipmentManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	/** Constructor */
	UEquipmentManager();

	/* ============================= */
	/* === Equipment Slot Checking === */
	/* ============================= */
	
	/** Checks if a given slot is available for the item */
	UFUNCTION(BlueprintCallable, Category = "Checker")
	bool CheckSlot(UBaseItem* Item);


	UFUNCTION(BlueprintCallable, Category ="Checker")
	UEquippableItem* GetItemInSlot(EEquipmentSlot SlotToCheck);
	
	/** Returns a list of all currently equipped items */
	UFUNCTION()
	TArray<UBaseItem*> EquipmentCheck() const;

	/** Determines if an item can be equipped */
	UFUNCTION(BlueprintCallable)
	bool IsItemEquippable(UBaseItem* Item);

	/** Checks if an equipment slot is empty */
	UFUNCTION(BlueprintCallable)
	bool IsSocketEmpty(EEquipmentSlot Slot) const ;
	
	/* ============================= */
	/* === Equipment Handling === */
	/* ============================= */

	/** Attempts to equip an item in the designated slot */
	UFUNCTION(BlueprintCallable)
	void TryToEquip(UBaseItem* Item, bool HasMesh, EEquipmentSlot Slot);

	/** Handles equipping an item that has a mesh */
	UFUNCTION(BlueprintCallable)
	void HandleHasMesh(UBaseItem* Item, EEquipmentSlot Slot);

	/** Handles equipping an item without a mesh */
	UFUNCTION(BlueprintCallable)
	void HandleNoMesh(UBaseItem* Item, EEquipmentSlot Slot);

	/** Removes a specific equipped item */
	UFUNCTION(BlueprintCallable)
	void RemoveEquippedItem(UBaseItem* Item, EEquipmentSlot Slot);

	/** Removes whatever item is currently in the slot */
	UFUNCTION(BlueprintCallable)
	void RemoveItemInSlot(EEquipmentSlot Slot);

	/** Attaches an item to the character's socket */
	UFUNCTION(BlueprintCallable)
	void AttachItem(TSubclassOf<AEquippedObject> Class, UBaseItem* Item, EEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void ApplyAllEquipmentStatsToAttributes(APHBaseCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void ApplyItemStatBonuses(UEquippableItem* Item, APHBaseCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RemoveItemStatBonuses(UEquippableItem* Item, APHBaseCharacter* Character);

	//Not used but optional in case of later development 
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void ResetAllEquipmentBonuses(APHBaseCharacter* Character);


	/* ============================= */
	/* === Inventory & Dropping === */
	/* ============================= */

	/** Moves an equipped item back into the inventory */
	UFUNCTION(BlueprintCallable)
	bool AddItemInSlotToInventory(UBaseItem* Item);

	/** Drops the item from the equipment onto the ground */
	UFUNCTION(BlueprintCallable)
	bool DropItem(UBaseItem* Item);

	/** Gets the spawn location for dropped items */
	FVector GetGroundSpawnLocation() const;

	/** Retrieves the inventory manager */
	UFUNCTION()
	UInventoryManager* GetInventoryManager() const;

protected:
	/** Called when the game starts */
	virtual void BeginPlay() override;

	/* ============================= */
	/* === Variables === */
	/* ============================= */

public:
	/** Delegate for notifying when equipment changes */
	UPROPERTY(BlueprintAssignable)
	FOnEquipmentChanged OnEquipmentChanged;

	/** Reference to the character that owns this equipment manager */
	UPROPERTY(BlueprintReadOnly, Category = "Owner")
	TObjectPtr<AALSCharacter> OwnerCharacter;

	/** Mapping of equipped slots to items */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	TMap<EEquipmentSlot, UBaseItem*> EquipmentData;

	/** Mapping of equipped slots to objects */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Equipment")
	TMap<EEquipmentSlot, AEquippedObject*> EquipmentItem;


protected:

	UPROPERTY()
	TMap<const UEquippableItem*, FPassiveEffectHandleList> AppliedPassiveEffects;

	UPROPERTY()
	TMap<const UEquippableItem*, FAppliedStats> AppliedItemStats;
};

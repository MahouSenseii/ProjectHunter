#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Equipment/Library/Enums/EquipmentEnums.h"
#include "Equipment/Library/Structs/EquipmentStructs.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "EquipmentManager.generated.h"

class AEquippedItemRuntimeActor;
class FEquipmentHandSlotMutationHelper;
class FEquipmentMutationHelper;
class FEquipmentReplicationHelper;
class FEquipmentSlotResolver;
class FOutBunch;
class UActorChannel;
class UCharacterSystemCoordinatorComponent;
class UEquipmentPresentationComponent;
class UInventoryManager;
class UItemInstance;
struct FItemBase;
struct FReplicationFlags;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnEquipmentChanged,
	EEquipmentSlot, Slot,
	UItemInstance*, NewItem,
	UItemInstance*, OldItem);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UEquipmentManager : public UActorComponent
{
	GENERATED_BODY()

	friend class FEquipmentMutationHelper;
	friend class FEquipmentHandSlotMutationHelper;
	friend class FEquipmentReplicationHelper;
	friend class FEquipmentSlotResolver;

public:
	UEquipmentManager();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	UItemInstance* EquipItem(UItemInstance* Item, EEquipmentSlot Slot = EEquipmentSlot::ES_None, bool bSwapToBag = true);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	UItemInstance* UnequipItem(EEquipmentSlot Slot, bool bMoveToBag = true);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	UItemInstance* SwapEquipment(UItemInstance* Item, EEquipmentSlot Slot = EEquipmentSlot::ES_None);

	UFUNCTION(BlueprintPure, Category = "Equipment")
	UItemInstance* GetEquippedItem(EEquipmentSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsSlotOccupied(EEquipmentSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	TArray<UItemInstance*> GetAllEquippedItems() const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	EEquipmentSlot DetermineEquipmentSlot(UItemInstance* Item) const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool CanEquipToSlot(UItemInstance* Item, EEquipmentSlot Slot) const;

	/**
	 * Slot that backs what Slot shows. A two-handed weapon fills both hands, so
	 * while one is equipped either hand slot resolves to ES_TwoHand.
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	EEquipmentSlot ResolveOccupyingSlot(EEquipmentSlot Slot) const;

	bool TryEquipGroundPickupItem(UItemInstance* Item, EEquipmentSlot& OutEquippedSlot, bool bSwapToBag = true);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void UnequipAll(bool bMoveToBag = true);

	UFUNCTION(BlueprintCallable, Category = "Equipment|Debug", meta = (AutoCreateRefTerm = "BaseItemHandle"))
	UItemInstance* GiveWeapon(
		const FDataTableRowHandle& BaseItemHandle,
		int32 ItemLevel = 1,
		EItemRarity Rarity = EItemRarity::IR_GradeF,
		bool bGenerateAffixes = true);

	UFUNCTION(BlueprintPure, Category = "Equipment")
	AEquippedItemRuntimeActor* GetActiveRuntimeItemActor(EEquipmentSlot Slot) const;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedItems, BlueprintReadOnly, Category = "Equipment")
	TArray<FEquipmentSlotEntry> EquippedItemsArray;

	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FOnEquipmentChanged OnEquipmentChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Config")
	int32 MaxRingSlots = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Config")
	bool bAutoSlotSelection = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Config")
	bool bApplyStatsOnEquip = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Config")
	bool bAutoUpdateWeapons = true;

protected:
	UFUNCTION()
	void OnRep_EquippedItems();

	UItemInstance* EquipItemInternal(UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag,
	                                 bool bUseGroundPickupRules = false);
	EEquipmentSlot GetNextAvailableRingSlot() const;
	bool IsRingSlot(EEquipmentSlot Slot) const;

	void CacheComponents();
	void RebuildEquipmentMap();
	void AddEquipment(EEquipmentSlot Slot, UItemInstance* Item);
	void RemoveEquipment(EEquipmentSlot Slot);

	UPROPERTY(Transient)
	TObjectPtr<UInventoryManager> InventoryManager = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterSystemCoordinatorComponent> CharacterSystemCoordinator = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UEquipmentPresentationComponent> EquipmentPresentation = nullptr;

	UPROPERTY(Transient)
	TMap<EEquipmentSlot, UItemInstance*> EquippedItemsMap;

	UFUNCTION(Server, Reliable)
	void ServerEquipItem(UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag);

	UFUNCTION(Server, Reliable)
	void ServerUnequipItem(EEquipmentSlot Slot, bool bMoveToBag);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastEquipmentChanged(EEquipmentSlot Slot, UItemInstance* NewItem, UItemInstance* OldItem);
};

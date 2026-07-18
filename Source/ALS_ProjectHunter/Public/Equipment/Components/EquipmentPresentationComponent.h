#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Equipment/Library/Enums/EquipmentEnums.h"
#include "EquipmentPresentationComponent.generated.h"

class AEquippedItemRuntimeActor;
class UEquipmentManager;
class UItemInstance;
class USceneComponent;
class USkeletalMeshComponent;
struct FItemAttachmentRules;
struct FItemBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnEquipmentVisualUpdated,
	EEquipmentSlot, Slot,
	UItemInstance*, NewItem);

UCLASS(ClassGroup = (ProjectHunter), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UEquipmentPresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentPresentationComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleEquipmentChanged(EEquipmentSlot Slot, UItemInstance* NewItem);

	UFUNCTION(BlueprintCallable, Category = "ProjectHunter|Equipment|Presentation")
	void RefreshOverlayStateFromEquipment(const UEquipmentManager* EquipmentManager);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Presentation")
	AEquippedItemRuntimeActor* GetActiveRuntimeItemActor(EEquipmentSlot Slot) const;

	UPROPERTY(BlueprintAssignable, Category = "ProjectHunter|Equipment|Presentation")
	FOnEquipmentVisualUpdated OnWeaponUpdated;

protected:
	void AttachItemVisual(EEquipmentSlot Slot, UItemInstance* Item);
	void DetachItemVisual(EEquipmentSlot Slot);

	void SpawnWeaponActor(EEquipmentSlot Slot, UItemInstance* Item,
	                      const FItemBase* BaseData, FName SocketName);
	void SpawnWeaponMesh(EEquipmentSlot Slot, UItemInstance* Item,
	                     const FItemBase* BaseData, FName SocketName);

	static FName GetSocketContextForSlot(EEquipmentSlot Slot);
	static FName ResolveSocketForSlot(EEquipmentSlot Slot, const FItemBase* BaseData);
	static FAttachmentTransformRules ConvertAttachmentRules(const FItemAttachmentRules& ItemRules);

private:
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> CharacterMesh = nullptr;

	UPROPERTY(Transient)
	TMap<EEquipmentSlot, TObjectPtr<AEquippedItemRuntimeActor>> SpawnedActors;

	UPROPERTY(Transient)
	TMap<EEquipmentSlot, TObjectPtr<USceneComponent>> SpawnedMeshComponents;
};

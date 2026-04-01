#pragma once

#include "CoreMinimal.h"
#include "Combat/Library/CombatStruct.h"
#include "GameFramework/Actor.h"
#include "Item/Library/ItemEnums.h"
#include "EquippedItemRuntimeActor.generated.h"

class APawn;
class UCombatManager;
class UItemInstance;
class USceneComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogEquippedItemRuntimeActor, Log, All);

/**
 * Shared runtime host for equipped items that need active behavior.
 * Blueprint children should own presentation/timing, while this base class owns common runtime state.
 */
UCLASS(Blueprintable)
class ALS_PROJECTHUNTER_API AEquippedItemRuntimeActor : public AActor
{
	GENERATED_BODY()

public:
	AEquippedItemRuntimeActor();

	UFUNCTION(BlueprintCallable, Category = "Runtime Item")
	void InitializeFromItem(UItemInstance* Item, APawn* OwnerPawn, EEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable, Category = "Runtime Item")
	void OnEquipped();

	UFUNCTION(BlueprintCallable, Category = "Runtime Item")
	void OnUnequipped();

	UFUNCTION(BlueprintCallable, Category = "Runtime Item")
	bool BeginPrimaryAction();

	UFUNCTION(BlueprintCallable, Category = "Runtime Item")
	void EndPrimaryAction();

	UFUNCTION(BlueprintPure, Category = "Runtime Item")
	UItemInstance* GetItemInstance() const { return ItemInstance; }

	UFUNCTION(BlueprintPure, Category = "Runtime Item")
	APawn* GetOwningPawn() const { return OwningPawn; }

	UFUNCTION(BlueprintPure, Category = "Runtime Item")
	EEquipmentSlot GetEquippedSlot() const { return EquippedSlot; }

	UFUNCTION(BlueprintPure, Category = "Runtime Item")
	bool IsRuntimeItemEquipped() const { return bIsEquipped; }

	UFUNCTION(BlueprintPure, Category = "Runtime Item")
	bool IsPrimaryActionActive() const { return bPrimaryActionActive; }

	UFUNCTION(BlueprintPure, Category = "Runtime Item")
	bool CanBeginPrimaryAction() const;

	UFUNCTION(BlueprintPure, Category = "Runtime Item|Combat")
	bool HasAuthorityToApplyDamage() const;

	UFUNCTION(BlueprintPure, Category = "Runtime Item|Combat")
	AActor* GetCombatSourceActor() const;

	UFUNCTION(BlueprintPure, Category = "Runtime Item|Combat")
	UCombatManager* GetCombatManager() const { return CombatManager; }

	UFUNCTION(BlueprintCallable, Category = "Runtime Item|Combat")
	void ResetHitActorsForCurrentAction();

	UFUNCTION(BlueprintPure, Category = "Runtime Item|Combat")
	bool HasActorBeenHitThisAction(const AActor* TargetActor) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime Item|Combat")
	bool MarkActorHitThisAction(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "Runtime Item|Combat")
	bool ResolveHitOnActor(AActor* TargetActor, FCombatResolveResult& OutResult);

	UFUNCTION(BlueprintCallable, Category = "Runtime Item|Combat")
	int32 ResolveHitResults(const TArray<FHitResult>& HitResults, TArray<FCombatResolveResult>& OutResults);

	UFUNCTION(BlueprintCallable, Category = "Runtime Item|Combat")
	bool PerformSphereTrace(
		FVector Start,
		FVector End,
		float Radius,
		TArray<FHitResult>& OutHits,
		TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn,
		bool bIgnoreOwner = true) const;

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Runtime Item")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Runtime Item|Combat")
	TObjectPtr<UCombatManager> CombatManager;

	UPROPERTY(BlueprintReadOnly, Category = "Runtime Item")
	TObjectPtr<UItemInstance> ItemInstance = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Runtime Item")
	TObjectPtr<APawn> OwningPawn = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Runtime Item")
	EEquipmentSlot EquippedSlot = EEquipmentSlot::ES_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runtime Item|Combat", meta = (ClampMin = "0.0"))
	float PrimaryActionCooldown = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runtime Item|Combat")
	TEnumAsByte<ECollisionChannel> DefaultTraceChannel = ECC_Pawn;

	UPROPERTY(BlueprintReadOnly, Category = "Runtime Item")
	bool bIsEquipped = false;

	UPROPERTY(BlueprintReadOnly, Category = "Runtime Item")
	bool bPrimaryActionActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Runtime Item")
	float LastPrimaryActionEndTime = -1.0f;

	virtual void HandleItemInitialized();
	virtual void HandleEquipped();
	virtual void HandleUnequipped();
	virtual void HandlePrimaryActionStarted();
	virtual void HandlePrimaryActionEnded();

	UFUNCTION(BlueprintImplementableEvent, Category = "Runtime Item", meta = (DisplayName = "On Runtime Item Initialized"))
	void K2_OnRuntimeItemInitialized(UItemInstance* Item, APawn* OwnerPawn, EEquipmentSlot Slot);

	UFUNCTION(BlueprintImplementableEvent, Category = "Runtime Item", meta = (DisplayName = "On Equipped"))
	void K2_OnEquipped();

	UFUNCTION(BlueprintImplementableEvent, Category = "Runtime Item", meta = (DisplayName = "On Unequipped"))
	void K2_OnUnequipped();

	UFUNCTION(BlueprintImplementableEvent, Category = "Runtime Item", meta = (DisplayName = "On Primary Action Started"))
	void K2_OnPrimaryActionStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Runtime Item", meta = (DisplayName = "On Primary Action Ended"))
	void K2_OnPrimaryActionEnded();

private:
	TSet<TWeakObjectPtr<AActor>> HitActorsThisAction;
};

// Runtime actor used by the ProjectHunter equipment presentation system.
#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "Systems/Combat/Library/CombatStructs.h"
#include "GameFramework/Actor.h"
#include "Item/Library/ItemEnums.h"
#include "EquippedItemRuntimeActor.generated.h"

class APHBaseCharacter;
class APawn;
struct FPropertyChangedEvent;
class UCombatManager;
class UItemInstance;
class USceneComponent;
class USplineComponent;
class UStaticMesh;
class UStaticMeshComponent;
class USkeletalMesh;
class USkeletalMeshComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogEquippedItemRuntimeActor, Log, All);

/**
 * Runtime actor for equipped items.
 * Owns the visual mesh preview used by Blueprint children and the spline used
 * to author weapon damage traces.
 */
UCLASS(Blueprintable, BlueprintType)
class ALS_PROJECTHUNTER_API AEquippedItemRuntimeActor : public AActor
{
	GENERATED_BODY()

public:
	AEquippedItemRuntimeActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Called by EquipmentManager after spawning. Sets owner data for the lifetime of this actor. */
	UFUNCTION(BlueprintCallable, Category = "Runtime Item")
	void InitializeFromItem(UItemInstance* Item, APawn* OwnerPawn, EEquipmentSlot Slot);

	UFUNCTION(BlueprintPure, Category = "Runtime Item")
	UItemInstance* GetItemInstance() const { return ItemInstance; }

	UFUNCTION(BlueprintPure, Category = "Runtime Item")
	APHBaseCharacter* GetOwningPawn() const { return OwnerActor; }

	/** Resolves CombatManager from the owning pawn. Works unarmed too. */
	UFUNCTION(BlueprintPure, Category = "Runtime Item|Combat")
	UCombatManager* GetCombatManager() const;

	UFUNCTION(BlueprintPure, Category = "Runtime Item|Components")
	UStaticMeshComponent* GetStaticMeshComponent() const { return StaticMeshComponent; }

	UFUNCTION(BlueprintPure, Category = "Runtime Item|Components")
	USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComponent; }

	UFUNCTION(BlueprintPure, Category = "Runtime Item|Components")
	USplineComponent* GetDamageTrace() const { return DamageTrace; }

	UFUNCTION(BlueprintPure, Category = "Runtime Item|Preview")
	UStaticMesh* GetStaticMeshMesh() const { return StaticMesh; }

	UFUNCTION(BlueprintCallable, Category = "Runtime Item|Preview")
	void SetStaticMesh(UStaticMesh* NewMesh);

	UFUNCTION(BlueprintPure, Category = "Runtime Item|Preview")
	USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }

	UFUNCTION(BlueprintCallable, Category = "Runtime Item|Preview")
	void SetSkeletalMesh(USkeletalMesh* NewMesh);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Runtime Item|Preview")
	void RefreshVisualPreview();

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Runtime Item")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(BlueprintReadOnly, Category = "Runtime Item")
	TObjectPtr<UItemInstance> ItemInstance = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Runtime Item")
	TObjectPtr<APHBaseCharacter> OwnerActor = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Runtime Item|Components")
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Runtime Item|Components")
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Runtime Item|Components")
	TObjectPtr<USplineComponent> DamageTrace;

	/** Blueprint child preview mesh for static-mesh weapons/items. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Runtime Item|Preview")
	TObjectPtr<UStaticMesh> StaticMesh = nullptr;

	/** Blueprint child preview mesh for skeletal-mesh weapons/items. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Runtime Item|Preview")
	TObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;

	void ApplyConfiguredPreviewMeshes();
};

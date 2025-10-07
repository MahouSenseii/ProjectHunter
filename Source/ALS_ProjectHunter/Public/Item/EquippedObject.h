#pragma once

#include "CoreMinimal.h"
#include "Data/UItemDefinitionAsset.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IEquippable.h" 
#include "Library/PHItemStructLibrary.h"
#include "EquippedObject.generated.h"

// Forward declarations
class AALSBaseCharacter;
class UCombatManager;
class USceneComponent;
class USplineComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;

// === Delegate Declarations ===
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipped, AEquippedObject*, EquippedObject, AActor*, Owner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnequipped, AEquippedObject*, EquippedObject);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInstanceDataUpdated);

/**
 * Visual representation of an equipped item
 * Does NOT manage durability (that's for crafting only)
 * Receives pre-generated instance data, doesn't generate it
 */
UCLASS()
class ALS_PROJECTHUNTER_API AEquippedObject : public AActor, public IEquippable
{
    GENERATED_BODY()
    
public:    
    AEquippedObject();
    
    // === Initialization ===
    
    /** Initialize this equipped object with item data (called when equipping) */
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void InitializeFromItem(const UItemDefinitionAsset* InDefinition, const FItemInstanceData& InInstanceData);
    
    /** Update instance data (e.g., after crafting operations) */
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void UpdateInstanceData(const FItemInstanceData& NewInstanceData);
    
    // === IEquippable Interface Implementation ===
    virtual void OnEquipped_Implementation(AActor* NewOwner) override;
    virtual void OnUnequipped_Implementation() override;
    virtual bool CanBeEquippedBy_Implementation(AActor* Actor) const override;
    virtual FName GetAttachmentSocket_Implementation() const override;
    virtual UItemDefinitionAsset* GetEquippableItemInfo_Implementation() const override;
    virtual void ApplyStatsToOwner_Implementation(AActor* Owner) override;
    virtual void RemoveStatsFromOwner_Implementation(AActor* Owner) override;
    virtual EEquipmentSlot GetEquipmentSlot_Implementation() const override;
    
    // === Getters ===
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
    const UItemDefinitionAsset* GetItemDefinition() const { return ItemDefinition; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
    const FItemInstanceData& GetInstanceData() const { return InstanceData; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
    FText GetDisplayName() const;
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
    EItemRarity GetRarity() const;
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
    bool IsEquipped() const { return bIsEquipped; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
    bool IsInitialized() const { return bIsInitialized; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character")
    AALSBaseCharacter* GetOwningCharacter() const { return OwningCharacter; }
    
    // === Utility ===
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Converter")
    static UCombatManager* ConvertActorToCombatManager(const AActor* InActor);
    
    UFUNCTION(BlueprintCallable, Category = "Validation")
    bool IsValidForEquipping() const;
    
    // === Pooling Support ===
    
    UFUNCTION(BlueprintCallable, Category = "Pooling")
    void ResetForPool();
    
    UFUNCTION(BlueprintCallable, Category = "Pooling")
    void ReinitializeFromPool(const UItemDefinitionAsset* InDefinition, const FItemInstanceData& InInstanceData);
    
    // === Events/Delegates ===
    
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEquipped OnEquipped;
    
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnUnequipped OnUnequipped;
    
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnInstanceDataUpdated OnInstanceDataUpdated;

protected:
    virtual void BeginPlay() override;
    
    /** Setup the visual mesh from definition */
    void SetupMesh();
    
    /** Apply visual effects based on rarity */
    void UpdateVisualEffects();

public:
    // === Visual Components ===
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> DefaultScene;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> StaticMesh;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> SkeletalMesh;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
    TObjectPtr<USplineComponent> DamageSpline;
    
    // === Runtime Collision Tracking ===
    
    UPROPERTY(BlueprintReadWrite, Category = "Collision|Runtime")
    TArray<FVector> PrevTracePoints;
    
    UPROPERTY(BlueprintReadWrite, Category = "Collision|Runtime")
    TArray<AActor*> CurrentActorHit;

protected:
    
    UPROPERTY(BlueprintReadOnly, Category = "Item")
    const UItemDefinitionAsset* ItemDefinition;
    
    
    UPROPERTY(BlueprintReadOnly, Category = "Item")
    FItemInstanceData InstanceData;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Sockets")
    FName DefaultAttachmentSocket = "WeaponSocket";
    
private:
    UPROPERTY()
    TObjectPtr<AALSBaseCharacter> OwningCharacter;
    
    UPROPERTY()
    bool bIsEquipped = false;
    
    UPROPERTY()
    bool bIsInitialized = false;
    
    UPROPERTY()
    bool bIsFromPool = false;
    
    // Track applied effects for removal
    UPROPERTY()
    TArray<FActiveGameplayEffectHandle> AppliedEffectHandles;
};
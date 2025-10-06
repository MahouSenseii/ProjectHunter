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
class UTimelineComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UDataTable;

// === Delegate Declarations ===
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipped, AEquippedObject*, EquippedObject, AActor*, Owner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnequipped, AEquippedObject*, EquippedObject);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemInfoChanged,  UItemDefinitionAsset*&, OldInfo,  UItemDefinitionAsset*&, NewInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDurabilityChanged, float, NewDurability);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemBroken, AEquippedObject*, BrokenItem);

UCLASS()
class ALS_PROJECTHUNTER_API AEquippedObject : public AActor, public IEquippable
{
    GENERATED_BODY()
    
public:    
    AEquippedObject();
    
    // === IEquippable Interface Implementation ===
    virtual void OnEquipped_Implementation(AActor* NewOwner) override;
    virtual void OnUnequipped_Implementation() override;
    virtual bool CanBeEquippedBy_Implementation(AActor* Actor) const override;
    virtual FName GetAttachmentSocket_Implementation() const override;
    virtual  UItemDefinitionAsset* GetEquippableItemInfo_Implementation() const override;
    virtual void ApplyStatsToOwner_Implementation(AActor* Owner) override;
    virtual void RemoveStatsFromOwner_Implementation(AActor* Owner) override;
    virtual EEquipmentSlot GetEquipmentSlot_Implementation() const override;
    
    // === Getters with Validation ===
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
    UItemDefinitionAsset*& GetItemInfo()  { return ItemInfo; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
    const FPHItemStats& GetItemStats() const { return ItemInfo->ItemStats; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
    bool IsEquipped() const { return bIsEquipped; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
    bool IsBroken() const { return CurrentDurability <= 0.0f; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
    float GetDurability() const { return CurrentDurability; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
    float GetMaxDurability() const { return MaxDurability; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character")
    AALSBaseCharacter* GetOwningCharacter() const { return OwningCharacter; }
    
    // === Setters with Validation ===
    UFUNCTION(BlueprintCallable, Category = "Item", CallInEditor)
    bool SetItemInfo(UItemDefinitionAsset*& Info);
    void SetItemInfoRotated(bool bRotated);
    void SetMesh(UStaticMesh* Mesh) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Converter")
    static UCombatManager* ConvertActorToCombatManager(const AActor* InActor);

    UFUNCTION(BlueprintCallable, Category = "Character")
    bool SetOwningCharacter(AALSBaseCharacter* InOwner);
    
    UFUNCTION(BlueprintCallable, Category = "Item")
    void SetDurability(float NewDurability);
    
    UFUNCTION(BlueprintCallable, Category = "Item")
    void ModifyDurability(float DeltaDurability);
    
    UFUNCTION(BlueprintCallable, Category = "Mesh")
    bool SetMesh(UStaticMesh* Mesh);
    
    // === Validation Functions ===
    UFUNCTION(BlueprintCallable, Category = "Validation")
    bool ValidateItemInfo( UItemDefinitionAsset*& Info, FString& OutError) const;
    
    UFUNCTION(BlueprintCallable, Category = "Validation")
    bool IsValidForEquipping() const;
    
    // === Pooling Support ===
    UFUNCTION(BlueprintCallable, Category = "Pooling")
    void ResetForPool();
    
    UFUNCTION(BlueprintCallable, Category = "Pooling")
    void InitializeFromPool(UItemDefinitionAsset*& Info);
    
    // === Events/Delegates ===
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEquipped OnEquipped;
    
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnUnequipped OnUnequipped;
    
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnItemInfoChanged OnItemInfoChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDurabilityChanged OnDurabilityChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnItemBroken OnItemBroken;

protected:
    virtual void BeginPlay() override;
    virtual void PostInitializeComponents() override;
    
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
    // === Public Properties ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> DefaultScene;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> StaticMesh;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> SkeletalMesh;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
    TObjectPtr<USplineComponent> DamageSpline;
    
    // Runtime collision tracking
    UPROPERTY(BlueprintReadWrite, Category = "Collision|Runtime")
    TArray<FVector> PrevTracePoints;
    
    UPROPERTY(BlueprintReadWrite, Category = "Collision|Runtime")
    TArray<AActor*> CurrentActorHit;

protected:

    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Info")
    UItemDefinitionAsset* ItemInfo;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Durability")
    float MaxDurability = 100.0f;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Durability")
    float CurrentDurability = 100.0f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Sockets")
    FName DefaultAttachmentSocket = "WeaponSocket";
    
private:
    // === Private Properties ===
    UPROPERTY()
    TObjectPtr<AALSBaseCharacter> OwningCharacter;
    
    UPROPERTY()
    bool bIsEquipped = false;
    
    // For pooling
    UPROPERTY()
    bool bIsFromPool = false;
};
// ItemInstance.h - FIXED VERSION with proper UBaseItem inheritance
#pragma once

#include "CoreMinimal.h"
#include "Item/BaseItem.h"
#include "Library/PHItemStructLibrary.h"
#include "ItemInstance.generated.h"

// Forward declarations
class UItemDefinitionAsset;

// Enum to specify what type of change occurred
UENUM(BlueprintType)
enum class EItemChangeType : uint8
{
    Quantity        UMETA(DisplayName = "Quantity Changed"),
    Durability      UMETA(DisplayName = "Durability Changed"),
    Rotation        UMETA(DisplayName = "Rotation Changed"),
    Enchantments    UMETA(DisplayName = "Enchantments Changed"),
    Properties      UMETA(DisplayName = "Properties Changed"),
    Initialization  UMETA(DisplayName = "Item Initialized"),
    FullRefresh     UMETA(DisplayName = "Full Refresh")
};


class UItemInstanceObject;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnInstanceChanged,
    UItemInstanceObject*, Item,
    EItemChangeType, ChangeType
);


UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UItemInstanceObject : public UBaseItem  // ✅ Inherit from your UBaseItem
{
    GENERATED_BODY()

public:
    UItemInstanceObject();

    /* ============================= */
    /* ===   UBaseItem Overrides === */
    /* ============================= */

    // Override BaseItem virtual methods to return instance-specific data
    virtual int32 GetQuantity() const override;
    virtual bool AddQuantity(int32 InQty) override;
    virtual bool RemoveQuantity(int32 InQty) override;
    virtual void SetQuantity(int32 NewQty) override;
    virtual void SetRotated(bool bNewRotated) override;
    virtual FIntPoint GetDimensions() const override;
    virtual const FItemInformation& GetItemInfo() const override;
    virtual void SetItemInfo(const FItemInformation& NewItemInfo) override;
    virtual bool ValidateItemData() const override;

    // Override inline methods from BaseItem
    virtual bool IsRotated() const override { return GetItemInfo().ItemInfo.Rotated; }
    virtual bool IsStackable() const override { return GetItemInfo().ItemInfo.Stackable; }
    virtual int32 GetMaxStackSize() const override { return GetItemInfo().ItemInfo.MaxStackSize; }

    /* ============================= */
    /* ===      Core API         === */
    /* ============================= */

    // Rebuild the cached item info view from base definition and instance data
    UFUNCTION(BlueprintCallable, Category = "Item Instance")
    void RebuildItemInfoView() const;

    // Check if this instance is valid and usable
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Instance")
    bool IsValidInstance() const;

    // Initialize this instance from a base definition asset
    UFUNCTION(BlueprintCallable, Category = "Item Instance")
    void InitializeFromDefinition(UItemDefinitionAsset* Definition);

    /* ============================= */
    /* ===   Durability Control  === */
    /* ============================= */

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Instance|Durability")
    float GetCurrentDurability() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Instance|Durability")
    float GetMaxDurability() const;

    UFUNCTION(BlueprintCallable, Category = "Item Instance|Durability")
    bool SetDurability(float NewDurability);

    UFUNCTION(BlueprintCallable, Category = "Item Instance|Durability")
    bool DamageDurability(float DamageAmount);

    UFUNCTION(BlueprintCallable, Category = "Item Instance|Durability")
    bool RepairDurability(float RepairAmount);

    /* ============================= */
    /* ===    Rotation Control   === */
    /* ============================= */
    
    virtual void ToggleRotation() override;  // Override the BaseItem version

    /* ============================= */
    /* ===    Instance Data       === */
    /* ============================= */

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Instance|Instance")
    int32 GetItemLevel() const { return Instance.ItemLevel; }

    UFUNCTION(BlueprintCallable, Category = "Item Instance|Instance")
    void SetItemLevel(int32 NewLevel);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Instance|Instance")
    EItemRarity GetInstanceRarity() const { return Instance.Rarity; }

    UFUNCTION(BlueprintCallable, Category = "Item Instance|Instance")
    void SetInstanceRarity(EItemRarity NewRarity);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Instance|Instance")
    bool IsIdentified() const { return Instance.bIdentified; }

    UFUNCTION(BlueprintCallable, Category = "Item Instance|Instance")
    void SetIdentified(bool bNewIdentified);

    /* ============================= */
    /* ===      Events & Cache    === */
    /* ============================= */

    // Delegate for change notifications
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnInstanceChanged OnInstanceChanged;

    // Force a complete cache rebuild
    UFUNCTION(BlueprintCallable, Category = "Item Instance")
    void InvalidateCache();

    /* ============================= */
    /* ===      Data Access      === */
    /* ============================= */

    // Read-only access to the instance data
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Instance")
    const FItemInstance& GetInstanceData() const { return Instance; }

public:
    // Static/base definition (Data Asset)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Definition")
    TSoftObjectPtr<UItemDefinitionAsset> BaseDef;

    // Runtime rolled data (visible but not directly editable in editor)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
    FItemInstance Instance;

private:
    /* ============================= */
    /* ===      Cache            === */
    /* ============================= */

    // Back-compat cache: what the rest of your systems expect to read
    UPROPERTY(Transient)
    mutable FItemInformation ItemInfoView;

    UPROPERTY(Transient)
    mutable bool bCacheValid = false;

    /* ============================= */
    /* ===      Helpers          === */
    /* ============================= */

    // Apply level-based scaling to item properties
    void ApplyLevelScaling(FItemInformation& ItemInfo) const;

    // Notify change and invalidate cache
    void NotifyInstanceChanged(EItemChangeType ChangeType);

    // Validate that changes are within acceptable bounds
    bool ValidateQuantityChange(int32 NewQuantity) const;
    bool ValidateDurabilityChange(float NewDurability) const;
};
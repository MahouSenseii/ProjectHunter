// Item/ItemInstance.h
#pragma once

#include "CoreMinimal.h"
#include "Item/Library/ItemEnums.h"
#include "Item/Library/ItemStructs.h"
#include "GameplayEffectTypes.h"
#include "ItemInstance.generated.h"

// Forward declarations
class UAbilitySystemComponent;
class UStaticMesh;
class USkeletalMesh;
class UMaterialInstance;

/**
 * Runtime Item Instance 
 */
UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UItemInstance : public UObject
{
	GENERATED_BODY()

public:
	UItemInstance();

	// ═══════════════════════════════════════════════
	// STATIC DATA REFERENCE
	// ═══════════════════════════════════════════════
	
	/** Reference to base item data in DataTable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Item")
	FDataTableRowHandle BaseItemHandle;

	// ═══════════════════════════════════════════════
	// IDENTITY
	// ═══════════════════════════════════════════════
	
	/** Unique identifier for this item instance */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Item")
	FGuid UniqueID;

	/** Random seed for deterministic generation */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Item")
	int32 Seed;
	
	// ═══════════════════════════════════════════════
	// QUANTITY & WEIGHT 
	// ═══════════════════════════════════════════════
	
	/** Stack quantity (for stackable items) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Item")
	int32 Quantity = 1;

	/** Total weight (base weight × quantity) - Hunter manga weight limit */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	float TotalWeight = 0.0f;
	
	// ═══════════════════════════════════════════════
	// HUNTER MANGA PROPERTIES (Equipment)
	// ═══════════════════════════════════════════════
	
	/** Item level (1-100) - Affects affix tier rolls */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Item|Hunter")
	int32 ItemLevel = 1;
	
	/** Item rarity grade (F-SS) - Determines affix count */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Item|Hunter")
	EItemRarity Rarity = EItemRarity::IR_GradeF;

	/** Has item been identified? (unidentified items hide affixes) */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Item|Hunter")
	bool bIdentified = true;

	/** Generated display name (cached for performance) */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Item|Hunter")
	FText DisplayName;

	/** Whether name has been generated yet */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Item|Hunter")
	bool bHasNameBeenGenerated = false;

	// ═══════════════════════════════════════════════
	// AFFIXES (Equipment Only)
	// ═══════════════════════════════════════════════
	
	/** All item stats (implicits + generated affixes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Item|Affixes")
	FPHItemStats Stats;

	// ═══════════════════════════════════════════════
	// CONSUMABLE PROPERTIES
	// ═══════════════════════════════════════════════
	
	/** Remaining uses (for multi-use consumables like potions) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Item|Consumable")
	int32 RemainingUses = 1;

	/** Cooldown remaining in seconds */
	UPROPERTY(BlueprintReadWrite, Category = "Item|Consumable")
	float CooldownRemaining = 0.0f;

	/** Last use timestamp (for cooldown tracking) */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Item|Consumable")
	float LastUseTime = 0.0f;

	// ═══════════════════════════════════════════════
	// DURABILITY (Equipment)
	// ═══════════════════════════════════════════════
	
	/** Durability system (equipment only) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Item|Durability")
	FItemDurability Durability;

	// ═══════════════════════════════════════════════
	// CORRUPTION STATE (Negative Affixes)
	// ═══════════════════════════════════════════════

	/** Does this item have ANY corrupted (negative) affixes? */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Item|Corruption")
	bool bHasCorruptedAffixes = false;

	/** Total corruption points (sum of all negative affix rank points) */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Item|Corruption")
	int32 TotalCorruptionPoints = 0;

	/** Can this item still be modified? (corrupted items cannot be) */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Item|State")
	bool bCanBeModified = true;

	// ═══════════════════════════════════════════════
	// RUNE CRAFTING (Hunter Manga Style)
	// ═══════════════════════════════════════════════
	
	/** Rune crafting data (equipment only) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Item|Runes")
	FRuneCraftingData RuneCraftingData;

	// ═══════════════════════════════════════════════
	// QUEST ITEMS
	// ═══════════════════════════════════════════════
	
	/** Quest ID this item belongs to (if quest item) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Item|Quest")
	FName QuestID;

	/** Is this a key item? (cannot be dropped/sold) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Item|Quest")
	bool bIsKeyItem = false;

	// ═══════════════════════════════════════════════
	// ECONOMY
	// ═══════════════════════════════════════════════
	
	/** Can this item be traded with other hunters? */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Item|Economy")
	bool bIsTradeable = true;

	/** Is this item character-bound? (soulbound) */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Item|Economy")
	bool bIsSoulbound = false;

	/** Value modifier (from quality, affixes, etc.) */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Item|Economy")
	float ValueModifier = 0.0f;

	// ═══════════════════════════════════════════════
	// TRANSIENT (NOT SAVED)
	// ═══════════════════════════════════════════════
	
	/** Active Gameplay Effect handles (from affixes) */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Effects")
	TArray<FActiveGameplayEffectHandle> AppliedEffectHandles;

	/** Are affix effects currently active on character? */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Effects")
	bool bEffectsActive = false;

	/** Cached base data (performance optimization) */
	mutable FItemBase* CachedBaseData = nullptr;

	/** Is cached base data dirty? */
	UPROPERTY(Transient)
	mutable bool bCacheDirty = true;

	// ═══════════════════════════════════════════════
	// INITIALIZATION
	// ═══════════════════════════════════════════════
	
	/**
	 * Initialize item instance (NO corruption)
	 * Uses AffixGenerator internally for affix rolling
	 * 
	 * @param InBaseItemHandle - Row handle to FItemBase in DataTable
	 * @param InItemLevel - Item level (1-100) affects affix tier rolls
	 * @param InRarity - Item grade (F-SS) determines affix count
	 * @param bGenerateAffixes - Whether to roll affixes (only for equipment Grade E+)
	 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void Initialize(
		FDataTableRowHandle InBaseItemHandle,
		int32 InItemLevel = 1,
		EItemRarity InRarity = EItemRarity::IR_GradeF,
		bool bGenerateAffixes = true);

	/**
	 * Initialize item instance WITH CORRUPTION SUPPORT
	 * 
	 * @param InBaseItemHandle - Row handle to FItemBase in DataTable
	 * @param InItemLevel - Item level (1-100) affects affix tier rolls
	 * @param InRarity - Item grade (F-SS) determines affix count
	 * @param bGenerateAffixes - Whether to roll affixes (only for equipment Grade E+)
	 * @param CorruptionChance - Per-affix chance to be corrupted (0.0 - 1.0)
	 * @param bForceCorrupted - Force at least one corrupted affix
	 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void InitializeWithCorruption(
		FDataTableRowHandle InBaseItemHandle,
		int32 InItemLevel,
		EItemRarity InRarity,
		bool bGenerateAffixes,
		float CorruptionChance,
		bool bForceCorrupted);

	// ═══════════════════════════════════════════════
	// NAME GENERATION
	// ═══════════════════════════════════════════════
	
	/**
	 * Get display name (generates if not cached)
	 * Hunter Manga Style:
	 * - Grade F: "Iron Sword"
	 * - Grade D: "Flaming Iron Sword of Power"
	 * - Grade A: "Demon-Slaying Blade"
	 * - Grade SS: "[Shadow Monarch's Dagger]"
	 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	FText GetDisplayName();

	/** Force regenerate display name */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void RegenerateDisplayName();

	// ═══════════════════════════════════════════════
	// VISUAL GETTERS
	// ═══════════════════════════════════════════════
	
	/** Get mesh for ground display */
	UFUNCTION(BlueprintPure, Category = "Item|Visuals")
	UStaticMesh* GetGroundMesh() const;

	/** Get mesh when equipped */
	UFUNCTION(BlueprintPure, Category = "Item|Visuals")
	USkeletalMesh* GetEquippedMesh() const;

	/** Get inventory icon */
	UFUNCTION(BlueprintPure, Category = "Item|Visuals")
	UMaterialInstance* GetInventoryIcon() const;

	/** Get rarity color (Grade F-SS colors) */
	UFUNCTION(BlueprintPure, Category = "Item|Visuals")
	FLinearColor GetRarityColor() const;
	

	// ═══════════════════════════════════════════════
	// CONVENIENCE GETTERS (Cached)
	// ═══════════════════════════════════════════════
	
	/** Get base item name */
	UFUNCTION(BlueprintPure, Category = "Item|Base")
	FText GetBaseItemName() const;

	/** Get item type */
	UFUNCTION(BlueprintPure, Category = "Item|Base")
	EItemType GetItemType() const;

	/** Get item subtype */
	UFUNCTION(BlueprintPure, Category = "Item|Base")
	EItemSubType GetItemSubType() const;

	/** Get equipment slot */
	UFUNCTION(BlueprintPure, Category = "Item|Base")
	EEquipmentSlot GetEquipmentSlot() const;

	/** Get max stack size */
	UFUNCTION(BlueprintPure, Category = "Item|Base")
	int32 GetMaxStackSize() const;

	/** Get base weight */
	UFUNCTION(BlueprintPure, Category = "Item|Base")
	float GetBaseWeight() const;

	float GetTotalWeight() const { return TotalWeight; }

	/** Is this a two-handed weapon? */
	UFUNCTION(BlueprintPure, Category = "Item|Base")
	bool bIsTwoHanded() const;

	/** Update total weight based on quantity */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void UpdateTotalWeight();

	// ═══════════════════════════════════════════════
	// AFFIX OPERATIONS (Equipment)
	// ═══════════════════════════════════════════════
	
	/**
	 * Apply all affix effects to character via GAS
	 * @param ASC - Target's Ability System Component
	 */
	UFUNCTION(BlueprintCallable, Category = "Item|Affixes")
	void ApplyAffixesToCharacter(UAbilitySystemComponent* ASC);

	/**
	 * Remove all affix effects from character
	 * @param ASC - Target's Ability System Component
	 */
	UFUNCTION(BlueprintCallable, Category = "Item|Affixes")
	void RemoveAffixesFromCharacter(UAbilitySystemComponent* ASC);

	// ═══════════════════════════════════════════════
	// CONSUMABLE OPERATIONS
	// ═══════════════════════════════════════════════
	
	/**
	 * Use consumable (apply effects, reduce quantity/uses)
	 * @param Target - Target to apply consumable effects to
	 * @return True if successfully used
	 */
	UFUNCTION(BlueprintCallable, Category = "Item|Consumable")
	bool UseConsumable(AActor* Target);

	/**
	 * Check if consumable can be used
	 * @return True if can be used (not on cooldown, has uses remaining, etc.)
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Consumable")
	bool CanUseConsumable() const;

	/**
	 * Get cooldown progress (0.0 to 1.0)
	 * @return 1.0 if ready, 0.0 if just used
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Consumable")
	float GetCooldownProgress() const;

	// ═══════════════════════════════════════════════
	// SETTERS (For Loot System Integration)
	// ═══════════════════════════════════════════════

	/**
	 * Set seed before initialization (for deterministic generation)
	 * Must be called BEFORE Initialize()
	 * @param InSeed - Seed value (0 = generate random)
	 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetSeed(const int32 InSeed) { Seed = InSeed; }

	/**
	 * Set quantity (for stackable items)
	 * @param InQuantity - Stack count
	 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetQuantity(const int32 InQuantity) { Quantity = InQuantity; };

	/**
	 * Reduce remaining uses
	 * @param Amount - Amount to reduce
	 * @return True if item was consumed (0 uses remaining)
	 */
	UFUNCTION(BlueprintCallable, Category = "Item|Consumable")
	bool ReduceUses(int32 Amount = 1);

	// ═══════════════════════════════════════════════
	// IDENTIFICATION (Equipment)
	// ═══════════════════════════════════════════════
	
	/** Identify item (reveal all affixes) */
	UFUNCTION(BlueprintCallable, Category = "Item|Hunter")
	void Identify();

	/** Check if item is identified */
	UFUNCTION(BlueprintPure, Category = "Item|Hunter")
	bool IsIdentified() const { return bIdentified; }

	/** Check if has unidentified affixes */
	UFUNCTION(BlueprintPure, Category = "Item|Hunter")
	bool HasUnidentifiedAffixes() const;

	// ═══════════════════════════════════════════════
	// ITEM TYPE CHECKS
	// ═══════════════════════════════════════════════
	
	/** Is this equipment? (weapon, armor, accessory) */
	UFUNCTION(BlueprintPure, Category = "Item|Type")
	bool IsEquipment() const;

	/** Is this a consumable? (potion, food, scroll) */
	UFUNCTION(BlueprintPure, Category = "Item|Type")
	bool IsConsumable() const;

	/** Is this a material? (crafting component) */
	UFUNCTION(BlueprintPure, Category = "Item|Type")
	bool IsMaterial() const;

	/** Is this a quest item? (key, document) */
	UFUNCTION(BlueprintPure, Category = "Item|Type")
	bool IsQuestItem() const;

	/** Is this currency? (gold, tokens) */
	UFUNCTION(BlueprintPure, Category = "Item|Type")
	bool IsCurrency() const;

	/** Is this a key item? */
	UFUNCTION(BlueprintPure, Category = "Item|Type")
	bool IsKeyItem() const;

	// ═══════════════════════════════════════════════
	// CORRUPTION CHECKS
	// ═══════════════════════════════════════════════

	/** Check if item has any corrupted (negative) affixes */
	UFUNCTION(BlueprintPure, Category = "Item|Corruption")
	bool IsCorrupted() const { return bHasCorruptedAffixes; }

	/** Get corruption severity (absolute value of negative points) */
	UFUNCTION(BlueprintPure, Category = "Item|Corruption")
	int32 GetCorruptionSeverity() const { return FMath::Abs(TotalCorruptionPoints); }

	/** Check if item can still be modified (crafting, enchanting, etc.) */
	UFUNCTION(BlueprintPure, Category = "Item|State")
	bool CanBeModified() const { return bCanBeModified && !bHasCorruptedAffixes; }

	/** Calculate corruption state from current affixes (call after affix generation) */
	UFUNCTION(BlueprintCallable, Category = "Item|Corruption")
	void CalculateCorruptionState();

	/** Get all corrupted (negative) affixes on this item */
	UFUNCTION(BlueprintCallable, Category = "Item|Corruption")
	TArray<FPHAttributeData> GetCorruptedAffixes() const;

	// ═══════════════════════════════════════════════
	// ITEM STATE CHECKS
	// ═══════════════════════════════════════════════
	
	/** Can this item be equipped? */
	UFUNCTION(BlueprintPure, Category = "Item|State")
	bool CanBeEquipped() const;

	/** Can this item be traded? */
	UFUNCTION(BlueprintPure, Category = "Item|State")
	bool CanBeTraded() const { return bIsTradeable && !bIsSoulbound && !bIsKeyItem; }

	/** Is this item stackable? */
	UFUNCTION(BlueprintPure, Category = "Item|State")
	bool IsStackable() const;

	/** Can this item stack with another? */
	UFUNCTION(BlueprintPure, Category = "Item|State")
	bool CanStackWith(const UItemInstance* Other) const;

	/** Is this item broken? (0 durability) */
	UFUNCTION(BlueprintPure, Category = "Item|State")
	bool IsBroken() const { return Durability.IsBroken(); }

	/** Is this item consumed? (0 uses or quantity) */
	UFUNCTION(BlueprintPure, Category = "Item|State")
	bool IsConsumed() const;

	// ═══════════════════════════════════════════════
	// DURABILITY (Equipment)
	// ═══════════════════════════════════════════════
	
	/** Reduce durability by amount */
	UFUNCTION(BlueprintCallable, Category = "Item|Durability")
	void ReduceDurability(float Amount) { Durability.Reduce(Amount); }

	/** Repair item to full durability */
	UFUNCTION(BlueprintCallable, Category = "Item|Durability")
	void RepairToFull() { Durability.RepairFull(); }

	/** Get durability as percentage (0.0 to 1.0) */
	UFUNCTION(BlueprintPure, Category = "Item|Durability")
	float GetDurabilityPercent() const { return Durability.GetDurabilityPercent(); }

	// ═══════════════════════════════════════════════
	// STACKING (Consumables, Materials, Currency)
	// ═══════════════════════════════════════════════
	
	/**
	 * Add to stack
	 * @param Amount - Amount to add
	 * @return Overflow amount (if stack maxed)
	 */
	UFUNCTION(BlueprintCallable, Category = "Item|Stack")
	int32 AddToStack(int32 Amount);

	/**
	 * Remove from stack
	 * @param Amount - Amount to remove
	 * @return Amount actually removed
	 */
	UFUNCTION(BlueprintCallable, Category = "Item|Stack")
	int32 RemoveFromStack(int32 Amount);

	/**
	 * Split stack into new instance
	 * @param Amount - Amount to split off
	 * @return New item instance with split amount
	 */
	UFUNCTION(BlueprintCallable, Category = "Item|Stack")
	UItemInstance* SplitStack(int32 Amount);

	/**
	 * Get remaining stack space
	 * @return Available space in stack
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Stack")
	int32 GetRemainingStackSpace() const;

	// ═══════════════════════════════════════════════
	// VALUE & ECONOMY
	// ═══════════════════════════════════════════════
	
	/**
	 * Get calculated value (with affixes, grade, quantity, etc.)
	 * Hunter Manga: Higher grades worth exponentially more
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Economy")
	int32 GetCalculatedValue() const;

	/**
	 * Get sell value (percentage of total value)
	 * @param SellPercentage - Percentage to sell for (default 50%)
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Economy")
	int32 GetSellValue(float SellPercentage = 0.5f) const;

	// ═══════════════════════════════════════════════
	// BASE DATA ACCESS (Cached for Performance)
	// ═══════════════════════════════════════════════
	
	/**
	 * Get base item data from DataTable (cached)
	 * @return Pointer to FItemBase, nullptr if invalid
	 */
	FItemBase* GetBaseData() const;

	/**
	 * Get base item data (Blueprint version)
	 * @param OutBaseData - Output base data
	 * @return True if valid base data exists
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Base", meta = (DisplayName = "Get Base Data"))
	bool GetBaseDataBP(FItemBase& OutBaseData) const;

	/**
	 * Check if has valid base data
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Base")
	bool HasValidBaseData() const;

	/**
	 * Invalidate cached base data (call if DataTable changes)
	 */
	UFUNCTION(BlueprintCallable, Category = "Item|Base")
	void InvalidateBaseCache();

	// ═══════════════════════════════════════════════
	// SERIALIZATION HELPERS
	// ═══════════════════════════════════════════════
	
	/** Prepare item for save (cleanup transient data) */
	UFUNCTION(BlueprintCallable, Category = "Item|Serialization")
	void PrepareForSave();

	/** Post-load initialization (after deserialization) */
	UFUNCTION(BlueprintCallable, Category = "Item|Serialization")
	void PostLoadInitialize();

private:
	/** Generate rare/legendary name for high-grade items */
	FText GenerateRareName() const;

	/** Apply consumable effects to target */
	bool ApplyConsumableEffects(AActor* Target);

	/** Update cooldown timer */
	void UpdateCooldown(float DeltaTime);
};
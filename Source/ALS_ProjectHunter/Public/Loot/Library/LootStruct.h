// Loot/Library/LootStruct.h

#pragma once

#include "CoreMinimal.h"
#include "LootEnum.h"
#include "Engine/DataTable.h"
#include "Item/ItemInstance.h"
#include "Item/Library/ItemStructs.h"
#include "Item/Library/ItemEnums.h"
#include "LootStruct.generated.h"

// Forward declarations
class UItemInstance;

// ═══════════════════════════════════════════════════════════════════════
// LOOT DROP SETTINGS - Controls generation behavior
// ═══════════════════════════════════════════════════════════════════════

/**
 * FLootDropSettings - Configuration for loot generation
 * 
 * SINGLE RESPONSIBILITY: Define how loot should be generated
 */
USTRUCT(BlueprintType)
struct FLootDropSettings
{
	GENERATED_BODY()

	// ═══════════════════════════════════════════════
	// DROP COUNTS
	// ═══════════════════════════════════════════════

	/** Minimum number of drops */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops", meta = (ClampMin = "0"))
	int32 MinDrops = 1;

	/** Maximum number of drops */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops", meta = (ClampMin = "0"))
	int32 MaxDrops = 3;

	/** Multiplier for drop chance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops", meta = (ClampMin = "0.0"))
	float DropChanceMultiplier = 1.0f;

	/** Multiplier for quantity per drop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops", meta = (ClampMin = "0.0"))
	float QuantityMultiplier = 1.0f;

	// ═══════════════════════════════════════════════
	// RARITY
	// ═══════════════════════════════════════════════

	/** Base rarity for this drop source */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity")
	EDropRarity SourceRarity = EDropRarity::DR_Common;

	/** Bonus chance for higher rarity (0.0 - 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RarityBonusChance = 0.0f;

	/** Minimum item rarity that can drop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity")
	EItemRarity MinimumItemRarity = EItemRarity::IR_None;

	// ═══════════════════════════════════════════════
	// LEVEL
	// ═══════════════════════════════════════════════

	/** Source level (affects item level) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level", meta = (ClampMin = "1", ClampMax = "100"))
	int32 SourceLevel = 1;

	/** Level variance (+/- this value) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level", meta = (ClampMin = "0"))
	int32 LevelVariance = 2;

	// ═══════════════════════════════════════════════
	// CORRUPTION (Negative Affix System)
	// ═══════════════════════════════════════════════

	/** 
	 * Global multiplier for corruption chance 
	 * Applied to each entry's CorruptionChancePerAffix
	 * 0.0 = no corruption, 2.0 = double corruption chance
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corruption", meta = (ClampMin = "0.0"))
	float CorruptionChanceMultiplier = 1.0f;

	/** 
	 * Force all dropped items to have at least one corrupted (negative) affix 
	 * Useful for cursed chests, corrupted zones, etc.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corruption")
	bool bForceCorruptedDrops = false;

	/** Only drop corrupted items? (filters non-corruptible entries) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corruption")
	bool bOnlyCorruptedDrops = false;

	/** Exclude corrupted entries from pool? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corruption")
	bool bExcludeCorruptedEntries = false;

	// ═══════════════════════════════════════════════
	// PLAYER MODIFIERS (Runtime bonuses)
	// ═══════════════════════════════════════════════

	/** Player luck bonus (affects rare drops) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	float PlayerLuckBonus = 0.0f;

	/** Player magic find bonus (affects quantity) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	float PlayerMagicFindBonus = 0.0f;

	FLootDropSettings()
		: MinDrops(1)
		, MaxDrops(3)
		, DropChanceMultiplier(1.0f)
		, QuantityMultiplier(1.0f)
		, SourceRarity(EDropRarity::DR_Common)
		, RarityBonusChance(0.0f)
		, MinimumItemRarity(EItemRarity::IR_None)
		, SourceLevel(1)
		, LevelVariance(2)
		, CorruptionChanceMultiplier(1.0f)
		, bForceCorruptedDrops(false)
		, bOnlyCorruptedDrops(false)
		, bExcludeCorruptedEntries(false)
		, PlayerLuckBonus(0.0f)
		, PlayerMagicFindBonus(0.0f)
	{}
};

// ═══════════════════════════════════════════════════════════════════════
// LOOT SOURCE REGISTRY - Maps source IDs to loot tables
// ═══════════════════════════════════════════════════════════════════════

/**
 * FLootSourceEntry - DataTable row mapping a drop source to its loot table
 * 
 * SINGLE RESPONSIBILITY: Define the mapping from source → loot data
 * 
 * USAGE:
 *   Create DT_LootSourceRegistry with rows like:
 *   - "Goblin_Basic" → Category: NPC, Table: DT_GoblinLoot
 *   - "Chest_Common" → Category: Chest, Table: DT_CommonChestLoot
 *   - "Barrel_Dungeon" → Category: Breakable, Table: DT_BarrelLoot
 */
USTRUCT(BlueprintType)
struct FLootSourceEntry : public FTableRowBase
{
	GENERATED_BODY()

	// ═══════════════════════════════════════════════
	// IDENTIFICATION
	// ═══════════════════════════════════════════════

	/** Display name for this loot source */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
	FText DisplayName;

	/** Category for organization (NPC, Chest, Breakable, Boss, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
	ELootSourceType Category = ELootSourceType::LST_None;

	/** Source rarity (affects base drop quality) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
	EDropRarity SourceRarity = EDropRarity::DR_Common;

	/** Tags for filtering */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
	TArray<FName> Tags;

	// ═══════════════════════════════════════════════
	// LOOT TABLE REFERENCE
	// ═══════════════════════════════════════════════

	/** Reference to the loot table DataTable (lazy loaded) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TSoftObjectPtr<UDataTable> LootTable;

	/** Specific row name in the loot table (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	FName LootTableRowName;

	/** Default drop settings for this source */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	FLootDropSettings DefaultSettings;

	// ═══════════════════════════════════════════════
	// LEVEL & SCALING
	// ═══════════════════════════════════════════════

	/** Base level for this source */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level", meta = (ClampMin = "1", ClampMax = "100"))
	int32 BaseLevel = 1;

	// ═══════════════════════════════════════════════
	// CURRENCY & EXPERIENCE
	// ═══════════════════════════════════════════════

	/** Minimum currency to drop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency", meta = (ClampMin = "0"))
	int32 MinCurrency = 0;

	/** Maximum currency to drop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency", meta = (ClampMin = "0"))
	int32 MaxCurrency = 0;

	/** Experience reward */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience", meta = (ClampMin = "0"))
	int32 ExperienceReward = 0;

	// ═══════════════════════════════════════════════
	// FLAGS
	// ═══════════════════════════════════════════════

	/** Is this source enabled? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flags")
	bool bEnabled = true;

	/** Is this a boss source? (guaranteed drops) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flags")
	bool bIsBoss = false;

	/** Scale drops with player count? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flags")
	bool bScaleWithPlayerCount = false;

	// ═══════════════════════════════════════════════
	// VALIDATION
	// ═══════════════════════════════════════════════

	bool IsValid() const
	{
		return bEnabled && !LootTable.IsNull();
	}

	bool HasTag(FName Tag) const
	{
		return Tags.Contains(Tag);
	}

	FLootSourceEntry()
		: Category(ELootSourceType::LST_None)
		, SourceRarity(EDropRarity::DR_Common)
		, BaseLevel(1)
		, MinCurrency(0)
		, MaxCurrency(0)
		, ExperienceReward(0)
		, bEnabled(true)
		, bIsBoss(false)
		, bScaleWithPlayerCount(false)
	{}
};

// ═══════════════════════════════════════════════════════════════════════
// LOOT ENTRY - Single possible drop definition
// ═══════════════════════════════════════════════════════════════════════

/**
 * FLootEntry - Defines a single possible item drop
 * 
 * SINGLE RESPONSIBILITY: Define what CAN drop and probability
 * NOTE: Does NOT hold affixes - ItemInstance::Initialize() handles that
 */
USTRUCT(BlueprintType)
struct FLootEntry
{
	GENERATED_BODY()

	// ═══════════════════════════════════════════════
	// ITEM REFERENCE
	// ═══════════════════════════════════════════════

	/** Reference to item base in DataTable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FDataTableRowHandle ItemRowHandle;

	/** Use direct class reference instead of DataTable row? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSubclassOf<UItemInstance> ItemClass;

	// ═══════════════════════════════════════════════
	// DROP CHANCE & WEIGHT
	// ═══════════════════════════════════════════════

	/** Base chance to drop (0.0 - 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 1.0f;

	/** Weight for weighted random selection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chance", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	// ═══════════════════════════════════════════════
	// QUANTITY
	// ═══════════════════════════════════════════════

	/** Minimum quantity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quantity", meta = (ClampMin = "1"))
	int32 MinQuantity = 1;

	/** Maximum quantity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quantity", meta = (ClampMin = "1"))
	int32 MaxQuantity = 1;

	// ═══════════════════════════════════════════════
	// RARITY OVERRIDE
	// ═══════════════════════════════════════════════

	/** Override rarity (IR_None = use rolled rarity) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity")
	EItemRarity OverrideRarity = EItemRarity::IR_None;

	// ═══════════════════════════════════════════════
	// GENERATION FLAGS
	// ═══════════════════════════════════════════════

	/** Generate affixes for this item? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bGenerateAffixes = true;

	/** 
	 * Use source's level with variance for item level generation?
	 * TRUE = Use SourceLevel +/- LevelVariance from FLootDropSettings
	 * FALSE = Use MinItemLevel to MaxItemLevel from this entry
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bUseItemLevel = true;

	/** Entry-specific min item level (only used if bUseItemLevel = false) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (EditCondition = "!bUseItemLevel", ClampMin = "1", ClampMax = "100"))
	int32 MinItemLevel = 1;

	/** Entry-specific max item level (only used if bUseItemLevel = false) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (EditCondition = "!bUseItemLevel", ClampMin = "1", ClampMax = "100"))
	int32 MaxItemLevel = 100;

	// ═══════════════════════════════════════════════
	// CORRUPTION (Negative Affix System)
	// ═══════════════════════════════════════════════

	/** Is this entry pre-defined as corrupted? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corruption")
	bool bIsCorrupted = false;

	/** Corruption type (for pre-corrupted entries) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corruption", meta = (EditCondition = "bIsCorrupted"))
	ECorruptionType CorruptionType = ECorruptionType::CT_None;

	/** Can this entry roll corrupted (negative) affixes during generation? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corruption")
	bool bCanBeCorrupted = true;

	/** 
	 * Per-affix chance to be corrupted (negative affix) 
	 * Only applies if bCanBeCorrupted = true
	 * 0.0 = no corruption, 1.0 = all affixes corrupted
	 * This is multiplied by Settings.CorruptionChanceMultiplier
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corruption", meta = (EditCondition = "bCanBeCorrupted", ClampMin = "0.0", ClampMax = "1.0"))
	float CorruptionChancePerAffix = 0.0f;

	/** 
	 * Force at least one corrupted (negative) affix on items from this entry 
	 * Guarantees item will have IsCorrupted() = true
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corruption", meta = (EditCondition = "bCanBeCorrupted"))
	bool bForceOneCorruptedAffix = false;

	// ═══════════════════════════════════════════════
	// VALIDATION
	// ═══════════════════════════════════════════════

	/** Check if entry has valid item reference */
	FORCEINLINE bool IsValid() const
	{
		return ItemRowHandle.DataTable != nullptr || ItemClass != nullptr;
	}

	/** Get effective weight (accounts for drop chance) */
	FORCEINLINE float GetEffectiveWeight() const
	{
		return Weight * DropChance;
	}

	FLootEntry()
		: ItemClass(nullptr)
		, DropChance(1.0f)
		, Weight(1.0f)
		, MinQuantity(1)
		, MaxQuantity(1)
		, OverrideRarity(EItemRarity::IR_None)
		, bGenerateAffixes(true)
		, bUseItemLevel(true)
		, MinItemLevel(1)
		, MaxItemLevel(100)
		, bIsCorrupted(false)
		, CorruptionType(ECorruptionType::CT_None)
		, bCanBeCorrupted(true)
		, CorruptionChancePerAffix(0.0f)
		, bForceOneCorruptedAffix(false)
	{}
};

// ═══════════════════════════════════════════════════════════════════════
// LOOT TABLE - Collection of entries (DataTable row)
// ═══════════════════════════════════════════════════════════════════════

/**
 * FLootTable - DataTable row containing loot entries
 * 
 */
USTRUCT(BlueprintType)
struct FLootTable : public FTableRowBase
{
	GENERATED_BODY()

	// ═══════════════════════════════════════════════
	// ENTRIES
	// ═══════════════════════════════════════════════

	/** All possible drops in this table */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entries")
	TArray<FLootEntry> Entries;

	// ═══════════════════════════════════════════════
	// SELECTION SETTINGS
	// ═══════════════════════════════════════════════

	/** How to select items from the pool */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
	ELootSelectionMethod SelectionMethod = ELootSelectionMethod::LSM_Weighted;

	/** Allow same entry to drop multiple times? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
	bool bAllowDuplicates = true;

	/** Min selections (0 = use settings) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	int32 MinSelections = 0;

	/** Max selections (0 = use settings) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	int32 MaxSelections = 0;

	// ═══════════════════════════════════════════════
	// HELPER FUNCTIONS
	// ═══════════════════════════════════════════════

	/** Get valid entries only */
	TArray<FLootEntry> GetValidEntries() const
	{
		TArray<FLootEntry> Result;
		for (const FLootEntry& Entry : Entries)
		{
			if (Entry.IsValid())
			{
				Result.Add(Entry);
			}
		}
		return Result;
	}

	/** Get corrupted entries */
	TArray<FLootEntry> GetCorruptedEntries() const
	{
		TArray<FLootEntry> Result;
		for (const FLootEntry& Entry : Entries)
		{
			if (Entry.bIsCorrupted && Entry.IsValid())
			{
				Result.Add(Entry);
			}
		}
		return Result;
	}

	/** Get non-corrupted entries */
	TArray<FLootEntry> GetNormalEntries() const
	{
		TArray<FLootEntry> Result;
		for (const FLootEntry& Entry : Entries)
		{
			if (!Entry.bIsCorrupted && Entry.IsValid())
			{
				Result.Add(Entry);
			}
		}
		return Result;
	}

	/** Filter entries by corruption type */
	TArray<FLootEntry> GetEntriesByCorruptionType(ECorruptionType Type) const
	{
		TArray<FLootEntry> Result;
		for (const FLootEntry& Entry : Entries)
		{
			if (Entry.CorruptionType == Type && Entry.IsValid())
			{
				Result.Add(Entry);
			}
		}
		return Result;
	}

	/** Get entries that can generate corrupted affixes */
	TArray<FLootEntry> GetCorruptibleEntries() const
	{
		TArray<FLootEntry> Result;
		for (const FLootEntry& Entry : Entries)
		{
			if (Entry.bCanBeCorrupted && Entry.IsValid())
			{
				Result.Add(Entry);
			}
		}
		return Result;
	}

	float GetTotalWeight() const
	{
		float TotalWeight = 0.0f;
		for (const FLootEntry& Entry : Entries)
		{
			TotalWeight += Entry.GetEffectiveWeight();
		}
		return TotalWeight;
	}

	

	FLootTable()
		: SelectionMethod(ELootSelectionMethod::LSM_Weighted)
		, bAllowDuplicates(true)
		, MinSelections(0)
		, MaxSelections(0)
	{}
};

// ═══════════════════════════════════════════════════════════════════════
// LOOT RESULT - Output from loot generation
// ═══════════════════════════════════════════════════════════════════════

/**
 * FLootResult - Single generated item result
 * 
 * SINGLE RESPONSIBILITY: Hold generation result data
 */
USTRUCT(BlueprintType)
struct FLootResult
{
	GENERATED_BODY()

	/** Generated item instance */
	UPROPERTY(BlueprintReadOnly, Category = "Result")
	UItemInstance* Item = nullptr;

	/** Quantity of this item */
	UPROPERTY(BlueprintReadOnly, Category = "Result")
	int32 Quantity = 1;

	/** Source entry index (for debugging) */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 SourceEntryIndex = -1;

	/** Was this item corrupted during generation? */
	UPROPERTY(BlueprintReadOnly, Category = "Result")
	bool bWasCorrupted = false;

	FLootResult()
		: Item(nullptr)
		, Quantity(1)
		, SourceEntryIndex(-1)
		, bWasCorrupted(false)
	{}

	FLootResult(UItemInstance* InItem, int32 InQuantity, int32 InSourceIndex = -1, bool bCorrupted = false)
		: Item(InItem)
		, Quantity(InQuantity)
		, SourceEntryIndex(InSourceIndex)
		, bWasCorrupted(bCorrupted)
	{}

	bool IsValid() const { return Item != nullptr && Quantity > 0; }
};

/**
 * FLootResultBatch - Collection of generated results
 */
USTRUCT(BlueprintType)
struct FLootResultBatch
{
	GENERATED_BODY()

	/** All generated items */
	UPROPERTY(BlueprintReadOnly, Category = "Result")
	TArray<FLootResult> Results;

	/** Total items generated */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalItemCount = 0;

	/** Currency dropped */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 CurrencyDropped = 0;

	/** Experience reward */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 ExperienceReward = 0;

	/** Source type that generated this loot */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	ELootSourceType SourceType = ELootSourceType::LST_None;

	/** Source ID (row name from registry) */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FName SourceID;

	/** Seed used for generation (for replication) */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Seed = 0;

	void AddResult(const FLootResult& Result)
	{
		if (Result.IsValid())
		{
			Results.Add(Result);
			TotalItemCount += Result.Quantity;
		}
	}

	void Clear()
	{
		Results.Empty();
		TotalItemCount = 0;
		CurrencyDropped = 0;
		ExperienceReward = 0;
	}

	/** Get count of corrupted items in batch */
	int32 GetCorruptedItemCount() const
	{
		int32 Count = 0;
		for (const FLootResult& Result : Results)
		{
			if (Result.bWasCorrupted)
			{
				Count++;
			}
		}
		return Count;
	}

	/** Check if any items were corrupted */
	bool HasCorruptedItems() const
	{
		for (const FLootResult& Result : Results)
		{
			if (Result.bWasCorrupted)
			{
				return true;
			}
		}
		return false;
	}

	FLootResultBatch()
		: TotalItemCount(0)
		, CurrencyDropped(0)
		, ExperienceReward(0)
		, SourceType(ELootSourceType::LST_None)
		, Seed(0)
	{}
};

// ═══════════════════════════════════════════════════════════════════════
// LOOT REQUEST - Input for loot generation
// ═══════════════════════════════════════════════════════════════════════

/**
 * FLootRequest - All data needed to generate loot
 * 
 * SINGLE RESPONSIBILITY: Encapsulate generation request
 */
USTRUCT(BlueprintType)
struct FLootRequest
{
	GENERATED_BODY()

	/** Source ID (row name in DT_LootSourceRegistry) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Request")
	FName SourceID;

	/** Override settings (optional - uses source defaults if not set) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Request")
	FLootDropSettings OverrideSettings;

	/** Use override settings? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Request")
	bool bUseOverrideSettings = false;

	/** Player luck stat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Request")
	float PlayerLuck = 0.0f;

	/** Player magic find stat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Request")
	float PlayerMagicFind = 0.0f;

	/** Override level (0 = use source default) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Request", meta = (ClampMin = "0", ClampMax = "100"))
	int32 OverrideLevel = 0;

	/** Random seed (0 = generate) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Request")
	int32 Seed = 0;

	/** Number of players (for scaling) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Request", meta = (ClampMin = "1"))
	int32 PlayerCount = 1;

	FLootRequest()
		: bUseOverrideSettings(false)
		, PlayerLuck(0.0f)
		, PlayerMagicFind(0.0f)
		, OverrideLevel(0)
		, Seed(0)
		, PlayerCount(1)
	{}

	FLootRequest(FName InSourceID)
		: SourceID(InSourceID)
		, bUseOverrideSettings(false)
		, PlayerLuck(0.0f)
		, PlayerMagicFind(0.0f)
		, OverrideLevel(0)
		, Seed(0)
		, PlayerCount(1)
	{}
};

// ═══════════════════════════════════════════════════════════════════════
// SPAWN SETTINGS - How to spawn loot in world
// ═══════════════════════════════════════════════════════════════════════

/**
 * FLootSpawnSettings - Controls how loot is spawned in world
 */
USTRUCT(BlueprintType)
struct FLootSpawnSettings
{
	GENERATED_BODY()

	/** Base spawn location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	FVector SpawnLocation = FVector::ZeroVector;

	/** Scatter radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0.0"))
	float ScatterRadius = 100.0f;

	/** Height offset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	float HeightOffset = 50.0f;

	/** Random scatter vs organized circle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	bool bRandomScatter = true;

	/** Apply physics impulse to spawned items? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	bool bApplyPhysicsImpulse = false;

	/** Physics impulse strength */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (EditCondition = "bApplyPhysicsImpulse"))
	float ImpulseStrength = 500.0f;

	FLootSpawnSettings()
		: SpawnLocation(FVector::ZeroVector)
		, ScatterRadius(100.0f)
		, HeightOffset(50.0f)
		, bRandomScatter(true)
		, bApplyPhysicsImpulse(false)
		, ImpulseStrength(500.0f)
	{}
};
// Loot/Library/LootEnum.h
#pragma once

#include "CoreMinimal.h"
#include "LootEnum.generated.h"

/**
 * Drop rarity tier - affects loot quality and quantity
 * Used by NPCs, Chests, Breakables, etc.
 */
UENUM(BlueprintType)
enum class EDropRarity : uint8
{
	DR_Common       UMETA(DisplayName = "Common"),
	DR_Uncommon     UMETA(DisplayName = "Uncommon"),
	DR_Rare         UMETA(DisplayName = "Rare"),
	DR_Epic         UMETA(DisplayName = "Epic"),
	DR_Legendary    UMETA(DisplayName = "Legendary"),
	DR_Mythical     UMETA(DisplayName = "Mythical")
};

/**
 * Loot source type - what entity dropped this loot
 * Used for different generation rules and filtering
 */
UENUM(BlueprintType)
enum class ELootSourceType : uint8
{
	LST_None        UMETA(DisplayName = "None"),
	LST_NPC         UMETA(DisplayName = "NPC"),           // Enemies, shopkeepers, etc.
	LST_Chest       UMETA(DisplayName = "Chest"),         // Loot chests, treasure boxes
	LST_Breakable   UMETA(DisplayName = "Breakable"),     // Barrels, crates, pots
	LST_Boss        UMETA(DisplayName = "Boss"),          // Boss monsters (guaranteed drops)
	LST_Quest       UMETA(DisplayName = "Quest"),         // Quest rewards
	LST_Crafting    UMETA(DisplayName = "Crafting"),      // Crafting results
	LST_Shop        UMETA(DisplayName = "Shop")           // Shop inventory generation
};

/**
 * Loot selection method - how items are chosen from pool
 */
UENUM(BlueprintType)
enum class ELootSelectionMethod : uint8
{
	LSM_Weighted        UMETA(DisplayName = "Weighted Random"),     // Use weights
	LSM_Sequential      UMETA(DisplayName = "Sequential"),          // Roll each entry in order
	LSM_GuaranteedOne   UMETA(DisplayName = "Guaranteed One"),      // At least one item drops
	LSM_All             UMETA(DisplayName = "All")                  // All entries that pass chance
};

/**
 * Corruption type - for corrupted item filtering
 */
UENUM(BlueprintType)
enum class ECorruptionType : uint8
{
	CT_None         UMETA(DisplayName = "None"),          // Not corrupted
	CT_Minor        UMETA(DisplayName = "Minor"),         // Minor corruption
	CT_Major        UMETA(DisplayName = "Major"),         // Major corruption
	CT_Abyssal      UMETA(DisplayName = "Abyssal")        // Full corruption
};
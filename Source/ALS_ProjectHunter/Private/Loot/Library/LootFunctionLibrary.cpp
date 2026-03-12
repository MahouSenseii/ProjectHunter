// Loot/Library/LootFunctionLibrary.cpp

#include "Loot/Library/LootFunctionLibrary.h"
#include "Loot/Generation/LootGenerator.h"
#include "Engine/DataTable.h"

// ═══════════════════════════════════════════════════════════════════════
// RARITY UTILITIES
// ═══════════════════════════════════════════════════════════════════════

FText ULootFunctionLibrary::GetDropRarityDisplayName(EDropRarity Rarity)
{
	switch (Rarity)
	{
		case EDropRarity::DR_Common:     return FText::FromString(TEXT("Common"));
		case EDropRarity::DR_Uncommon:   return FText::FromString(TEXT("Uncommon"));
		case EDropRarity::DR_Rare:       return FText::FromString(TEXT("Rare"));
		case EDropRarity::DR_Epic:       return FText::FromString(TEXT("Epic"));
		case EDropRarity::DR_Legendary:  return FText::FromString(TEXT("Legendary"));
		case EDropRarity::DR_Mythical:   return FText::FromString(TEXT("Mythical"));
		default:                         return FText::FromString(TEXT("Unknown"));
	}
}

FLinearColor ULootFunctionLibrary::GetDropRarityColor(EDropRarity Rarity)
{
	switch (Rarity)
	{
		case EDropRarity::DR_Common:     return FLinearColor(0.8f, 0.8f, 0.8f);     // Gray
		case EDropRarity::DR_Uncommon:   return FLinearColor(0.2f, 0.8f, 0.2f);     // Green
		case EDropRarity::DR_Rare:       return FLinearColor(0.2f, 0.4f, 1.0f);     // Blue
		case EDropRarity::DR_Epic:       return FLinearColor(0.6f, 0.2f, 0.9f);     // Purple
		case EDropRarity::DR_Legendary:  return FLinearColor(1.0f, 0.6f, 0.0f);     // Orange
		case EDropRarity::DR_Mythical:   return FLinearColor(1.0f, 0.2f, 0.2f);     // Red
		default:                         return FLinearColor::White;
	}
}

EItemRarity ULootFunctionLibrary::DropRarityToItemRarity(EDropRarity DropRarity)
{
	switch (DropRarity)
	{
		case EDropRarity::DR_Common:     return EItemRarity::IR_GradeF;
		case EDropRarity::DR_Uncommon:   return EItemRarity::IR_GradeE;
		case EDropRarity::DR_Rare:       return EItemRarity::IR_GradeD;
		case EDropRarity::DR_Epic:       return EItemRarity::IR_GradeC;
		case EDropRarity::DR_Legendary:  return EItemRarity::IR_GradeB;
		case EDropRarity::DR_Mythical:   return EItemRarity::IR_GradeA;
		default:                         return EItemRarity::IR_None;
	}
}

float ULootFunctionLibrary::GetRarityMultiplier(EDropRarity Rarity)
{
	switch (Rarity)
	{
		case EDropRarity::DR_Common:     return 1.0f;
		case EDropRarity::DR_Uncommon:   return 1.5f;
		case EDropRarity::DR_Rare:       return 2.0f;
		case EDropRarity::DR_Epic:       return 2.5f;
		case EDropRarity::DR_Legendary:  return 3.5f;
		case EDropRarity::DR_Mythical:   return 5.0f;
		default:                         return 1.0f;
	}
}

// ═══════════════════════════════════════════════════════════════════════
// LOOT TABLE UTILITIES
// ═══════════════════════════════════════════════════════════════════════

bool ULootFunctionLibrary::IsValidLootTableHandle(const FDataTableRowHandle& Handle)
{
	return FLootGenerator::GetLootTableFromHandle(Handle) != nullptr;
}

float ULootFunctionLibrary::GetLootTableTotalWeight(const FDataTableRowHandle& Handle)
{
	const FLootTable* Table = FLootGenerator::GetLootTableFromHandle(Handle);
	
	if (!Table)
	{
		return 0.0f;
	}
	
	return Table->GetTotalWeight();
}

int32 ULootFunctionLibrary::GetLootTableEntryCount(const FDataTableRowHandle& Handle)
{
	const FLootTable* Table = FLootGenerator::GetLootTableFromHandle(Handle);
	
	if (!Table)
	{
		return 0;
	}
	
	return Table->Entries.Num();
}

int32 ULootFunctionLibrary::GetCorruptedEntryCount(const FDataTableRowHandle& Handle)
{
	const FLootTable* Table = FLootGenerator::GetLootTableFromHandle(Handle);
	
	if (!Table)
	{
		return 0;
	}
	
	return Table->GetCorruptedEntries().Num();
}

// ═══════════════════════════════════════════════════════════════════════
// LOOT ENTRY UTILITIES
// ═══════════════════════════════════════════════════════════════════════

float ULootFunctionLibrary::GetEntryDropPercentage(const FLootEntry& Entry, float TotalTableWeight)
{
	if (TotalTableWeight <= 0.0f)
	{
		return 0.0f;
	}
	
	float EffectiveWeight = Entry.GetEffectiveWeight();
	return (EffectiveWeight / TotalTableWeight) * 100.0f;
}

bool ULootFunctionLibrary::IsValidLootEntry(const FLootEntry& Entry)
{
	return Entry.IsValid();
}

// ═══════════════════════════════════════════════════════════════════════
// CORRUPTION UTILITIES
// ═══════════════════════════════════════════════════════════════════════

FText ULootFunctionLibrary::GetCorruptionTypeName(ECorruptionType Type)
{
	switch (Type)
	{
		case ECorruptionType::CT_None:    return FText::FromString(TEXT("None"));
		case ECorruptionType::CT_Minor:   return FText::FromString(TEXT("Minor Corruption"));
		case ECorruptionType::CT_Major:   return FText::FromString(TEXT("Major Corruption"));
		case ECorruptionType::CT_Abyssal: return FText::FromString(TEXT("Abyssal Corruption"));
		default:                          return FText::FromString(TEXT("Unknown"));
	}
}

FLinearColor ULootFunctionLibrary::GetCorruptionTypeColor(ECorruptionType Type)
{
	switch (Type)
	{
		case ECorruptionType::CT_None:    return FLinearColor::White;
		case ECorruptionType::CT_Minor:   return FLinearColor(0.6f, 0.3f, 0.6f);    // Light purple
		case ECorruptionType::CT_Major:   return FLinearColor(0.4f, 0.0f, 0.4f);    // Dark purple
		case ECorruptionType::CT_Abyssal: return FLinearColor(0.1f, 0.0f, 0.1f);    // Near black
		default:                          return FLinearColor::White;
	}
}

float ULootFunctionLibrary::GetCorruptionSeverity(ECorruptionType Type)
{
	switch (Type)
	{
		case ECorruptionType::CT_None:    return 0.0f;
		case ECorruptionType::CT_Minor:   return 0.25f;
		case ECorruptionType::CT_Major:   return 0.5f;
		case ECorruptionType::CT_Abyssal: return 1.0f;
		default:                          return 0.0f;
	}
}

// ═══════════════════════════════════════════════════════════════════════
// SOURCE TYPE UTILITIES
// ═══════════════════════════════════════════════════════════════════════

FText ULootFunctionLibrary::GetLootSourceTypeName(ELootSourceType Type)
{
	switch (Type)
	{
		case ELootSourceType::LST_None:      return FText::FromString(TEXT("None"));
		case ELootSourceType::LST_NPC:       return FText::FromString(TEXT("NPC"));
		case ELootSourceType::LST_Chest:     return FText::FromString(TEXT("Chest"));
		case ELootSourceType::LST_Breakable: return FText::FromString(TEXT("Breakable"));
		case ELootSourceType::LST_Boss:      return FText::FromString(TEXT("Boss"));
		case ELootSourceType::LST_Quest:     return FText::FromString(TEXT("Quest Reward"));
		case ELootSourceType::LST_Crafting:  return FText::FromString(TEXT("Crafting"));
		case ELootSourceType::LST_Shop:      return FText::FromString(TEXT("Shop"));
		default:                             return FText::FromString(TEXT("Unknown"));
	}
}

FLootDropSettings ULootFunctionLibrary::GetDefaultSettingsForSourceType(ELootSourceType Type)
{
	FLootDropSettings Settings;
	
	switch (Type)
	{
		case ELootSourceType::LST_NPC:
			Settings.MinDrops = 0;
			Settings.MaxDrops = 2;
			Settings.SourceRarity = EDropRarity::DR_Common;
			break;
			
		case ELootSourceType::LST_Chest:
			Settings.MinDrops = 1;
			Settings.MaxDrops = 4;
			Settings.SourceRarity = EDropRarity::DR_Uncommon;
			Settings.RarityBonusChance = 0.1f;
			break;
			
		case ELootSourceType::LST_Breakable:
			Settings.MinDrops = 0;
			Settings.MaxDrops = 1;
			Settings.SourceRarity = EDropRarity::DR_Common;
			Settings.DropChanceMultiplier = 0.3f;
			break;
			
		case ELootSourceType::LST_Boss:
			Settings.MinDrops = 2;
			Settings.MaxDrops = 5;
			Settings.SourceRarity = EDropRarity::DR_Rare;
			Settings.RarityBonusChance = 0.25f;
			break;
			
		case ELootSourceType::LST_Quest:
			Settings.MinDrops = 1;
			Settings.MaxDrops = 1;
			Settings.SourceRarity = EDropRarity::DR_Rare;
			Settings.DropChanceMultiplier = 1.0f; // Guaranteed
			break;
			
		case ELootSourceType::LST_Crafting:
			Settings.MinDrops = 1;
			Settings.MaxDrops = 1;
			Settings.SourceRarity = EDropRarity::DR_Uncommon;
			break;
			
		case ELootSourceType::LST_Shop:
			Settings.MinDrops = 5;
			Settings.MaxDrops = 10;
			Settings.SourceRarity = EDropRarity::DR_Uncommon;
			Settings.RarityBonusChance = 0.05f;
			break;
			
		default:
			break;
	}
	
	return Settings;
}

// ═══════════════════════════════════════════════════════════════════════
// MATH UTILITIES
// ═══════════════════════════════════════════════════════════════════════

float ULootFunctionLibrary::ApplyLuckToDropChance(float BaseChance, float Luck)
{
	// Luck formula: Each point adds diminishing returns bonus
	// 100 luck = ~20% more rare items
	float LuckBonus = Luck / (Luck + 500.0f);
	
	return FMath::Clamp(BaseChance * (1.0f + LuckBonus), 0.0f, 1.0f);
}

int32 ULootFunctionLibrary::ApplyMagicFindToQuantity(int32 BaseQuantity, float MagicFind)
{
	// Magic find formula: Linear bonus to quantity
	// 100 MF = +50% quantity
	float Multiplier = 1.0f + (MagicFind / 200.0f);
	
	return FMath::Max(1, FMath::RoundToInt(BaseQuantity * Multiplier));
}
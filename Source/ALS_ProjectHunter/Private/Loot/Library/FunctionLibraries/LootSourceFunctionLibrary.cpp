#include "Loot/Library/FunctionLibraries/LootSourceFunctionLibrary.h"

FText ULootSourceFunctionLibrary::GetLootSourceTypeName(const ELootSourceType Type)
{
	switch (Type)
	{
	case ELootSourceType::LST_None: return FText::FromString(TEXT("None"));
	case ELootSourceType::LST_NPC: return FText::FromString(TEXT("NPC"));
	case ELootSourceType::LST_Chest: return FText::FromString(TEXT("Chest"));
	case ELootSourceType::LST_Breakable: return FText::FromString(TEXT("Breakable"));
	case ELootSourceType::LST_Boss: return FText::FromString(TEXT("Boss"));
	case ELootSourceType::LST_Quest: return FText::FromString(TEXT("Quest Reward"));
	case ELootSourceType::LST_Crafting: return FText::FromString(TEXT("Crafting"));
	case ELootSourceType::LST_Shop: return FText::FromString(TEXT("Shop"));
	default: return FText::FromString(TEXT("Unknown"));
	}
}

FLootDropSettings ULootSourceFunctionLibrary::GetDefaultSettingsForSourceType(const ELootSourceType Type)
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
		Settings.DropChanceMultiplier = 1.0f;
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

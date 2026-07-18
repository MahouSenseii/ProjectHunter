#include "Loot/Library/FunctionLibraries/LootSettingsFunctionLibrary.h"

FLootDropSettings ULootSettingsFunctionLibrary::BuildSettingsFromRequest(const FLootSourceEntry& Source, const FLootRequest& Request)
{
	FLootDropSettings Settings = Source.DefaultSettings;

	Settings.SourceLevel = Source.BaseLevel;
	Settings.SourceRarity = Source.SourceRarity;

	if (Request.OverrideSettings.MinDrops > 0)
	{
		Settings.MinDrops = Request.OverrideSettings.MinDrops;
	}
	if (Request.OverrideSettings.MaxDrops > 0)
	{
		Settings.MaxDrops = Request.OverrideSettings.MaxDrops;
	}
	if (Request.OverrideSettings.DropChanceMultiplier != 1.0f)
	{
		Settings.DropChanceMultiplier = Request.OverrideSettings.DropChanceMultiplier;
	}

	return Settings;
}

FLootDropSettings ULootSettingsFunctionLibrary::ApplyGlobalDropChanceMultiplier(const FLootDropSettings& Settings, float GlobalDropChanceMultiplier)
{
	FLootDropSettings Modified = Settings;
	Modified.DropChanceMultiplier *= GlobalDropChanceMultiplier;
	return Modified;
}

FLootDropSettings ULootSettingsFunctionLibrary::ApplyPlayerDropModifiers(const FLootDropSettings& Settings, float Luck, float MagicFind)
{
	FLootDropSettings Modified = Settings;

	Modified.PlayerLuckBonus = Luck;
	Modified.RarityBonusChance += Luck * 0.005f;

	Modified.PlayerMagicFindBonus = MagicFind;
	Modified.QuantityMultiplier *= (1.0f + MagicFind * 0.01f);

	return Modified;
}

#include "Item/Library/FunctionLibraries/ItemNameFunctionLibrary.h"

#include "Item/Library/FunctionLibraries/ItemEnumFunctionLibrary.h"

FText UItemNameFunctionLibrary::GenerateItemName(
	const FPHItemStats& ItemStats,
	const FItemBase& ItemBase,
	EItemRarity Rarity)
{
	if (Rarity <= EItemRarity::IR_GradeF)
	{
		return ItemBase.ItemName;
	}

	if (Rarity == EItemRarity::IR_GradeSS || ItemBase.bIsUnique)
	{
		return FText::Format(FText::FromString("[{0}]"), ItemBase.ItemName);
	}

	if (Rarity >= EItemRarity::IR_GradeA)
	{
		return FText::Format(FText::FromString("[{0}]"), ItemBase.ItemName);
	}

	FText BestPrefixName;
	FText BestSuffixName;

	if (ItemStats.Prefixes.Num() > 0)
	{
		const FPHAttributeData* BestPrefix = nullptr;
		int32 HighestRank = -100;

		for (const FPHAttributeData& Prefix : ItemStats.Prefixes)
		{
			if (Prefix.bIsIdentified && !Prefix.AffixName.IsEmpty())
			{
				const int32 Rank = UItemEnumFunctionLibrary::GetRankPointsValue(Prefix.RankPoints);
				if (Rank > HighestRank)
				{
					HighestRank = Rank;
					BestPrefix = &Prefix;
				}
			}
		}

		if (BestPrefix)
		{
			BestPrefixName = BestPrefix->AffixName;
		}
	}

	if (ItemStats.Suffixes.Num() > 0)
	{
		const FPHAttributeData* BestSuffix = nullptr;
		int32 HighestRank = -100;

		for (const FPHAttributeData& Suffix : ItemStats.Suffixes)
		{
			if (Suffix.bIsIdentified && !Suffix.AffixName.IsEmpty())
			{
				const int32 Rank = UItemEnumFunctionLibrary::GetRankPointsValue(Suffix.RankPoints);
				if (Rank > HighestRank)
				{
					HighestRank = Rank;
					BestSuffix = &Suffix;
				}
			}
		}

		if (BestSuffix)
		{
			BestSuffixName = BestSuffix->AffixName;
		}
	}

	FString FullName;

	if (!BestPrefixName.IsEmpty() && !BestSuffixName.IsEmpty())
	{
		FullName = FString::Printf(TEXT("%s %s %s"),
			*BestPrefixName.ToString(),
			*ItemBase.ItemName.ToString(),
			*BestSuffixName.ToString());
	}
	else if (!BestPrefixName.IsEmpty())
	{
		FullName = FString::Printf(TEXT("%s %s"),
			*BestPrefixName.ToString(),
			*ItemBase.ItemName.ToString());
	}
	else if (!BestSuffixName.IsEmpty())
	{
		FullName = FString::Printf(TEXT("%s %s"),
			*ItemBase.ItemName.ToString(),
			*BestSuffixName.ToString());
	}
	else
	{
		FullName = ItemBase.ItemName.ToString();
	}

	return FText::FromString(FullName);
}

FText UItemNameFunctionLibrary::GenerateLegendaryName(int32 Seed)
{
	return FText::FromString("Legendary Item");
}

FText UItemNameFunctionLibrary::GetPrefixName(const FPHAttributeData& Affix)
{
	if (!Affix.AffixName.IsEmpty())
	{
		return Affix.AffixName;
	}

	const FString AttributeName = Affix.AttributeName.ToString();

	if (AttributeName.Contains("Fire")) return FText::FromString("Flaming");
	if (AttributeName.Contains("Ice")) return FText::FromString("Frozen");
	if (AttributeName.Contains("Lightning")) return FText::FromString("Shocking");
	if (AttributeName.Contains("Light")) return FText::FromString("Radiant");
	if (AttributeName.Contains("Corruption")) return FText::FromString("Cursed");
	if (AttributeName.Contains("Physical")) return FText::FromString("Heavy");
	if (AttributeName.Contains("Strength")) return FText::FromString("Mighty");
	if (AttributeName.Contains("Dexterity")) return FText::FromString("Swift");
	if (AttributeName.Contains("Intelligence")) return FText::FromString("Sage's");

	return FText::FromString("Enhanced");
}

FText UItemNameFunctionLibrary::GetSuffixName(const FPHAttributeData& Affix)
{
	if (!Affix.AffixName.IsEmpty())
	{
		return Affix.AffixName;
	}

	const FString AttributeName = Affix.AttributeName.ToString();

	if (AttributeName.Contains("Strength")) return FText::FromString("of the Bear");
	if (AttributeName.Contains("Dexterity")) return FText::FromString("of the Falcon");
	if (AttributeName.Contains("Intelligence")) return FText::FromString("of the Owl");
	if (AttributeName.Contains("Endurance")) return FText::FromString("of the Titan");
	if (AttributeName.Contains("Fire")) return FText::FromString("of Fire");
	if (AttributeName.Contains("Ice")) return FText::FromString("of Ice");
	if (AttributeName.Contains("Lightning")) return FText::FromString("of Lightning");
	if (AttributeName.Contains("Speed")) return FText::FromString("of Swiftness");
	if (AttributeName.Contains("Life")) return FText::FromString("of Life");
	if (AttributeName.Contains("Mana")) return FText::FromString("of Mana");

	return FText::FromString("of Power");
}

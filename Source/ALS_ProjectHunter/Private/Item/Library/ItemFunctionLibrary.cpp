// Item/Library/ItemFunctionLibrary.cpp

#include "Item/Library/ItemFunctionLibrary.h"
#include "Item/ItemInstance.h"

// ═══════════════════════════════════════════════
// RARITY & DISPLAY (Hunter Manga)
// ═══════════════════════════════════════════════

FLinearColor UItemFunctionLibrary::GetRarityColor(EItemRarity Rarity)
{
	return GetItemRarityColor(Rarity);
}

FText UItemFunctionLibrary::GetRarityDisplayName(EItemRarity Rarity)
{
	switch (Rarity)
	{
		case EItemRarity::IR_GradeF:  return FText::FromString("Grade F (Common)");
		case EItemRarity::IR_GradeE:  return FText::FromString("Grade E (Uncommon)");
		case EItemRarity::IR_GradeD:  return FText::FromString("Grade D (Rare)");
		case EItemRarity::IR_GradeC:  return FText::FromString("Grade C (Elite)");
		case EItemRarity::IR_GradeB:  return FText::FromString("Grade B (Named)");
		case EItemRarity::IR_GradeA:  return FText::FromString("Grade A (Legendary)");
		case EItemRarity::IR_GradeS:  return FText::FromString("Grade S (Mythic)");
		case EItemRarity::IR_GradeSS: return FText::FromString("Grade SS (EX-Rank)");
		case EItemRarity::IR_Unknown: return FText::FromString("Unknown");
		case EItemRarity::IR_Corrupted: return FText::FromString("Corrupted");
		default: return FText::FromString("None");
	}
}

FText UItemFunctionLibrary::GetAffixCountText(EItemRarity Rarity)
{
	int32 MinPre, MaxPre, MinSuf, MaxSuf;
	GetAffixCountByRarity(Rarity, MinPre, MaxPre, MinSuf, MaxSuf);
	
	int32 MinTotal = MinPre + MinSuf;
	int32 MaxTotal = MaxPre + MaxSuf;
	
	if (MinTotal == 0 && MaxTotal == 0)
	{
		return FText::FromString("No Affixes");
	}
	else if (MinTotal == MaxTotal)
	{
		return FText::Format(FText::FromString("{0} Affixes"), MaxTotal);
	}
	else
	{
		return FText::Format(FText::FromString("{0}-{1} Affixes"), MinTotal, MaxTotal);
	}
}

// ═══════════════════════════════════════════════
// AFFIX FORMATTING
// ═══════════════════════════════════════════════

FString UItemFunctionLibrary::FormatAffixValue(
	float Value,
	EAttributeDisplayFormat Format,
	FName AttributeName,
	float MinValue,
	float MaxValue,
	const FText& CustomText)
{
	FString FormattedValue;
	
	switch (Format)
	{
		case EAttributeDisplayFormat::ADF_Additive:
			FormattedValue = FString::Printf(TEXT("+%d to %s"), 
				FMath::RoundToInt(Value), *AttributeName.ToString());
			break;

		case EAttributeDisplayFormat::ADF_FlatNegative:
			FormattedValue = FString::Printf(TEXT("-%d to %s"), 
				FMath::RoundToInt(FMath::Abs(Value)), *AttributeName.ToString());
			break;

		case EAttributeDisplayFormat::ADF_Percent:
			FormattedValue = FString::Printf(TEXT("+%d%% %s"), 
				FMath::RoundToInt(Value), *AttributeName.ToString());
			break;

		case EAttributeDisplayFormat::ADF_MinMax:
			FormattedValue = FString::Printf(TEXT("Adds %d-%d %s"), 
				FMath::RoundToInt(MinValue), FMath::RoundToInt(MaxValue), *AttributeName.ToString());
			break;

		case EAttributeDisplayFormat::ADF_Increase:
			FormattedValue = FString::Printf(TEXT("%d%% increased %s"), 
				FMath::RoundToInt(Value), *AttributeName.ToString());
			break;

		case EAttributeDisplayFormat::ADF_More:
			FormattedValue = FString::Printf(TEXT("%d%% more %s"), 
				FMath::RoundToInt(Value), *AttributeName.ToString());
			break;

		case EAttributeDisplayFormat::ADF_Less:
			FormattedValue = FString::Printf(TEXT("%d%% less %s"), 
				FMath::RoundToInt(Value), *AttributeName.ToString());
			break;

		case EAttributeDisplayFormat::ADF_Chance:
			FormattedValue = FString::Printf(TEXT("%d%% chance to %s"), 
				FMath::RoundToInt(Value), *AttributeName.ToString());
			break;

		case EAttributeDisplayFormat::ADF_Duration:
			FormattedValue = FString::Printf(TEXT("%.1fs duration to %s"), 
				Value, *AttributeName.ToString());
			break;

		case EAttributeDisplayFormat::ADF_Cooldown:
			FormattedValue = FString::Printf(TEXT("%.1fs cooldown on %s"), 
				Value, *AttributeName.ToString());
			break;

		case EAttributeDisplayFormat::ADF_SkillGrant:
			FormattedValue = FString::Printf(TEXT("Grants [%s] Level %d"), 
				*AttributeName.ToString(), FMath::RoundToInt(Value));
			break;

		case EAttributeDisplayFormat::ADF_CustomText:
			FormattedValue = CustomText.IsEmpty() ? 
				FString::Printf(TEXT("%d %s"), FMath::RoundToInt(Value), *AttributeName.ToString()) :
				CustomText.ToString();
			break;

		default:
			FormattedValue = FString::Printf(TEXT("%d %s"), 
				FMath::RoundToInt(Value), *AttributeName.ToString());
			break;
	}

	return FormattedValue;
}

FString UItemFunctionLibrary::FormatAffixText(const FPHAttributeData& Affix)
{
	return FormatAffixValue(
		Affix.RolledStatValue,
		Affix.DisplayFormat,
		Affix.AttributeName,
		Affix.MinValue,
		Affix.MaxValue,
		Affix.DisplayText
	);
}

FString UItemFunctionLibrary::GetModifyTypeSymbol(EModifyType ModifyType)
{
	return ::GetModifyTypeSymbol(ModifyType);
}

// ═══════════════════════════════════════════════
// RANK POINTS / TIER FUNCTIONS
// ═══════════════════════════════════════════════

int32 UItemFunctionLibrary::GetRankPointsValue(ERankPoints Points)
{
	return ::GetRankPointsValue(Points);
}

FText UItemFunctionLibrary::GetTierName(ERankPoints Points)
{
	int32 Value = GetRankPointsValue(Points);
	
	if (Value < 0)
	{
		return FText::Format(FText::FromString("Cursed (Tier {0})"), FMath::Abs(Value));
	}
	else if (Value == 0)
	{
		return FText::FromString("No Bonus");
	}
	else if (Value >= 10)
	{
		return FText::FromString("Perfect (Tier 10)");
	}
	else
	{
		return FText::Format(FText::FromString("Tier {0}"), Value);
	}
}

bool UItemFunctionLibrary::CompareAffixRank(const FPHAttributeData& AffixA, const FPHAttributeData& AffixB)
{
	return GetRankPointsValue(AffixA.RankPoints) > GetRankPointsValue(AffixB.RankPoints);
}

// ═══════════════════════════════════════════════
// NAME GENERATION (Hunter Manga Style)
// ═══════════════════════════════════════════════

FText UItemFunctionLibrary::GenerateItemName(
	const FPHItemStats& ItemStats,
	const FItemBase& ItemBase,
	EItemRarity Rarity)
{
	// Grade F/E: Just base name
	if (Rarity <= EItemRarity::IR_GradeE)
	{
		return ItemBase.ItemName;
	}

	// Grade SS (EX-Rank): [Preset Unique Name]
	// Unique items use their base name as the preset unique name
	if (Rarity == EItemRarity::IR_GradeSS || ItemBase.bIsUnique)
	{
		return FText::Format(FText::FromString("[{0}]"), ItemBase.ItemName);
	}

	// Grade A/S (Legendary): Use GenerateLegendaryName or preset name
	// For now, treat as [Legendary Name]
	if (Rarity >= EItemRarity::IR_GradeA)
	{
		// Could use procedural legendary name or preset
		return FText::Format(FText::FromString("[{0}]"), ItemBase.ItemName);
	}

	// Grade D/C/B: Generate from affixes (PoE-style)
	// Format: "PrefixName BaseItemName SuffixName"
	// Example: "Dragon's Katana of the Fang"
	
	FText BestPrefixName;
	FText BestSuffixName;

	// Find highest-ranked prefix with a name
	if (ItemStats.Prefixes.Num() > 0)
	{
		const FPHAttributeData* BestPrefix = nullptr;
		int32 HighestRank = -100;

		for (const FPHAttributeData& Prefix : ItemStats.Prefixes)
		{
			if (!Prefix.AffixName.IsEmpty())
			{
				int32 Rank = GetRankPointsValue(Prefix.RankPoints);
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

	// Find highest-ranked suffix with a name
	if (ItemStats.Suffixes.Num() > 0)
	{
		const FPHAttributeData* BestSuffix = nullptr;
		int32 HighestRank = -100;

		for (const FPHAttributeData& Suffix : ItemStats.Suffixes)
		{
			if (!Suffix.AffixName.IsEmpty())
			{
				int32 Rank = GetRankPointsValue(Suffix.RankPoints);
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

	// Build name: "PrefixName BaseItemName SuffixName"
	FString FullName;
	
	if (!BestPrefixName.IsEmpty() && !BestSuffixName.IsEmpty())
	{
		// Both prefix and suffix: "Dragon's Katana of the Fang"
		FullName = FString::Printf(TEXT("%s %s %s"), 
			*BestPrefixName.ToString(), 
			*ItemBase.ItemName.ToString(), 
			*BestSuffixName.ToString());
	}
	else if (!BestPrefixName.IsEmpty())
	{
		// Prefix only: "Dragon's Katana"
		FullName = FString::Printf(TEXT("%s %s"), 
			*BestPrefixName.ToString(), 
			*ItemBase.ItemName.ToString());
	}
	else if (!BestSuffixName.IsEmpty())
	{
		// Suffix only: "Katana of the Fang"
		FullName = FString::Printf(TEXT("%s %s"), 
			*ItemBase.ItemName.ToString(), 
			*BestSuffixName.ToString());
	}
	else
	{
		// No affix names: Just base name
		FullName = ItemBase.ItemName.ToString();
	}

	return FText::FromString(FullName);
}

FText UItemFunctionLibrary::GenerateLegendaryName(int32 Seed)
{
	// TODO: Implement procedural legendary name generation
	// Hunter Manga style names like:
	// - "Demon's Fang"
	// - "Shadow Whisper"
	// - "Eternal Frost"
	// - "Dragon's Wrath"
	// - "Starfall"
	
	return FText::FromString("Legendary Item");
}

FText UItemFunctionLibrary::GetPrefixName(const FPHAttributeData& Affix)
{
	// Use the affix's preset AffixName if available
	if (!Affix.AffixName.IsEmpty())
	{
		return Affix.AffixName;
	}

	// Fallback: Generate from attribute name (for backwards compatibility)
	FString AttrName = Affix.AttributeName.ToString();
	
	// Simple mapping examples:
	if (AttrName.Contains("Fire")) return FText::FromString("Flaming");
	if (AttrName.Contains("Ice")) return FText::FromString("Frozen");
	if (AttrName.Contains("Lightning")) return FText::FromString("Shocking");
	if (AttrName.Contains("Light")) return FText::FromString("Radiant");
	if (AttrName.Contains("Corruption")) return FText::FromString("Cursed");
	if (AttrName.Contains("Physical")) return FText::FromString("Heavy");
	if (AttrName.Contains("Strength")) return FText::FromString("Mighty");
	if (AttrName.Contains("Dexterity")) return FText::FromString("Swift");
	if (AttrName.Contains("Intelligence")) return FText::FromString("Sage's");
	
	return FText::FromString("Enhanced");
}

FText UItemFunctionLibrary::GetSuffixName(const FPHAttributeData& Affix)
{
	// Use the affix's preset AffixName if available
	if (!Affix.AffixName.IsEmpty())
	{
		return Affix.AffixName;
	}

	// Fallback: Generate from attribute name (for backwards compatibility)
	FString AttrName = Affix.AttributeName.ToString();
	
	// Simple mapping examples:
	if (AttrName.Contains("Strength")) return FText::FromString("of the Bear");
	if (AttrName.Contains("Dexterity")) return FText::FromString("of the Falcon");
	if (AttrName.Contains("Intelligence")) return FText::FromString("of the Owl");
	if (AttrName.Contains("Endurance")) return FText::FromString("of the Titan");
	if (AttrName.Contains("Fire")) return FText::FromString("of Fire");
	if (AttrName.Contains("Ice")) return FText::FromString("of Ice");
	if (AttrName.Contains("Lightning")) return FText::FromString("of Lightning");
	if (AttrName.Contains("Speed")) return FText::FromString("of Swiftness");
	if (AttrName.Contains("Life")) return FText::FromString("of Life");
	if (AttrName.Contains("Mana")) return FText::FromString("of Mana");
	
	return FText::FromString("of Power");
}

// ═══════════════════════════════════════════════
// DAMAGE CALCULATION (PoE2 Style)
// ═══════════════════════════════════════════════

FDamageRange UItemFunctionLibrary::CalculateFinalDamage(
	FDamageRange BaseDamage,
	float FlatAdded,
	float IncreasedPercent,
	float MorePercent)
{
	// PoE2 Formula: Base → +Flat → ×(1 + Increased/100) → ×(1 + More/100)
	
	float FinalMin = BaseDamage.MinDamage + FlatAdded;
	float FinalMax = BaseDamage.MaxDamage + FlatAdded;
	
	// Apply Increased
	if (IncreasedPercent != 0.0f)
	{
		float IncreasedMult = 1.0f + (IncreasedPercent / 100.0f);
		FinalMin *= IncreasedMult;
		FinalMax *= IncreasedMult;
	}
	
	// Apply More
	if (MorePercent != 0.0f)
	{
		float MoreMult = 1.0f + (MorePercent / 100.0f);
		FinalMin *= MoreMult;
		FinalMax *= MoreMult;
	}
	
	return FDamageRange(FinalMin, FinalMax);
}

float UItemFunctionLibrary::CalculateDPS(FDamageRange DamageRange, float AttackSpeed)
{
	return DamageRange.GetAverage() * AttackSpeed;
}

FDamageRange UItemFunctionLibrary::CalculateCriticalDamage(
	FDamageRange BaseDamage,
	float CritMultiplier)
{
	return FDamageRange(
		BaseDamage.MinDamage * CritMultiplier,
		BaseDamage.MaxDamage * CritMultiplier
	);
}

// ═══════════════════════════════════════════════
// DEFENSE CALCULATION (Hunter Manga)
// ═══════════════════════════════════════════════

float UItemFunctionLibrary::CalculateFinalResistance(
	float BaseResistance,
	float FlatAdded,
	float IncreasedPercent)
{
	float FinalResist = BaseResistance + FlatAdded;
	
	if (IncreasedPercent != 0.0f)
	{
		FinalResist *= (1.0f + IncreasedPercent / 100.0f);
	}
	
	// Cap at 100%
	return FMath::Clamp(FinalResist, 0.0f, 100.0f);
}

float UItemFunctionLibrary::CalculateArmorReduction(float Armor, float IncomingDamage)
{
	// PoE2-style: Armor / (Armor + 10 × Damage)
	if (IncomingDamage <= 0.0f) return 1.0f;
	
	float Reduction = Armor / (Armor + 10.0f * IncomingDamage);
	return FMath::Clamp(Reduction, 0.0f, 0.9f); // Cap at 90%
}

// ═══════════════════════════════════════════════
// WEIGHT & INVENTORY (Hunter Manga)
// ═══════════════════════════════════════════════

float UItemFunctionLibrary::CalculateMaxWeightFromStrength(
	int32 Strength,
	float WeightPerStrength)
{
	return Strength * WeightPerStrength;
}

float UItemFunctionLibrary::GetOverweightPercentage(float CurrentWeight, float MaxWeight)
{
	if (MaxWeight <= 0.0f) return 0.0f;
	if (CurrentWeight <= MaxWeight) return 0.0f;
	
	return (CurrentWeight - MaxWeight) / MaxWeight;
}

// ═══════════════════════════════════════════════
// ITEM VALIDATION & REQUIREMENTS
// ═══════════════════════════════════════════════

bool UItemFunctionLibrary::MeetsItemRequirements(
	const FItemStatRequirement& Requirements,
	int32 HunterLevel,
	int32 Strength,
	int32 Dexterity,
	int32 Intelligence,
	int32 Endurance,
	int32 Affliction,
	int32 Luck,
	int32 Covenant)
{
	return Requirements.MeetsRequirements(
		HunterLevel,
		Strength,
		Dexterity,
		Intelligence,
		Endurance,
		Affliction,
		Luck,
		Covenant
	);
}

int32 UItemFunctionLibrary::GetRequiredLevel(const FItemStatRequirement& Requirements)
{
	return Requirements.RequiredLevel;
}

// ═══════════════════════════════════════════════
// AFFIX GENERATION HELPERS
// ═══════════════════════════════════════════════

void UItemFunctionLibrary::GetAffixCountByRarity(
	EItemRarity Rarity,
	int32& OutMinPrefixes,
	int32& OutMaxPrefixes,
	int32& OutMinSuffixes,
	int32& OutMaxSuffixes)
{
	// Hunter Manga / ORV style affix counts (F-SS Grade System)
	switch (Rarity)
	{
		case EItemRarity::IR_GradeF:  // Grade F (Common) - No affixes
			OutMinPrefixes = 0; OutMaxPrefixes = 0;
			OutMinSuffixes = 0; OutMaxSuffixes = 0;
			break;
			
		case EItemRarity::IR_GradeE:  // Grade E (Uncommon) - 1-2 affixes
			OutMinPrefixes = 0; OutMaxPrefixes = 1;
			OutMinSuffixes = 0; OutMaxSuffixes = 1;
			break;
			
		case EItemRarity::IR_GradeD:  // Grade D (Rare) - 2-3 affixes
			OutMinPrefixes = 1; OutMaxPrefixes = 2;
			OutMinSuffixes = 1; OutMaxSuffixes = 2;
			break;
			
		case EItemRarity::IR_GradeC:  // Grade C (Elite) - 3-4 affixes
			OutMinPrefixes = 1; OutMaxPrefixes = 2;
			OutMinSuffixes = 2; OutMaxSuffixes = 3;
			break;
			
		case EItemRarity::IR_GradeB:  // Grade B (Named) - 4-5 affixes
			OutMinPrefixes = 2; OutMaxPrefixes = 3;
			OutMinSuffixes = 2; OutMaxSuffixes = 3;
			break;
			
		case EItemRarity::IR_GradeA:  // Grade A (Legendary) - 5-6 affixes
			OutMinPrefixes = 2; OutMaxPrefixes = 3;
			OutMinSuffixes = 3; OutMaxSuffixes = 3;
			break;
			
		case EItemRarity::IR_GradeS:  // Grade S (Mythic) - 6 affixes (max)
			OutMinPrefixes = 3; OutMaxPrefixes = 3;
			OutMinSuffixes = 3; OutMaxSuffixes = 3;
			break;
			
		case EItemRarity::IR_GradeSS:  // Grade SS (EX-Rank) - Fixed unique affixes
			OutMinPrefixes = 0; OutMaxPrefixes = 0;
			OutMinSuffixes = 0; OutMaxSuffixes = 0;
			break;
			
		default:
			OutMinPrefixes = 0; OutMaxPrefixes = 0;
			OutMinSuffixes = 0; OutMaxSuffixes = 0;
			break;
	}
}

float UItemFunctionLibrary::GetRarityValueMultiplier(EItemRarity Rarity)
{
	switch (Rarity)
	{
		case EItemRarity::IR_GradeF:  return 1.0f;
		case EItemRarity::IR_GradeE:  return 1.5f;
		case EItemRarity::IR_GradeD:  return 2.5f;
		case EItemRarity::IR_GradeC:  return 5.0f;
		case EItemRarity::IR_GradeB:  return 10.0f;
		case EItemRarity::IR_GradeA:  return 25.0f;
		case EItemRarity::IR_GradeS:  return 100.0f;
		case EItemRarity::IR_GradeSS: return 1000.0f;  // EX-Rank!
		default: return 1.0f;
	}
}

// ═══════════════════════════════════════════════
// ITEM COMPARISON
// ═══════════════════════════════════════════════

int32 UItemFunctionLibrary::CompareItemDamage(const FItemBase& ItemA, const FItemBase& ItemB)
{
	float DamageA = ItemA.WeaponStats.MinPhysicalDamage + ItemA.WeaponStats.MaxPhysicalDamage;
	float DamageB = ItemB.WeaponStats.MinPhysicalDamage + ItemB.WeaponStats.MaxPhysicalDamage;
	
	if (DamageA < DamageB) return -1;
	if (DamageA > DamageB) return 1;
	return 0;
}

int32 UItemFunctionLibrary::CompareItemValue(const FItemBase& ItemA, const FItemBase& ItemB)
{
	if (ItemA.Value < ItemB.Value) return -1;
	if (ItemA.Value > ItemB.Value) return 1;
	return 0;
}

int32 UItemFunctionLibrary::CompareItemInstanceValue(const UItemInstance* ItemA, const UItemInstance* ItemB)
{
	if (!ItemA || !ItemB) return 0;
	
	int32 ValueA = ItemA->GetCalculatedValue();
	int32 ValueB = ItemB->GetCalculatedValue();
	
	if (ValueA < ValueB) return -1;
	if (ValueA > ValueB) return 1;
	return 0;
}

int32 UItemFunctionLibrary::CompareItemInstanceRarity(const UItemInstance* ItemA, const UItemInstance* ItemB)
{
	if (!ItemA || !ItemB) return 0;
	
	uint8 RarityA = static_cast<uint8>(ItemA->Rarity);
	uint8 RarityB = static_cast<uint8>(ItemB->Rarity);
	
	if (RarityA < RarityB) return -1;
	if (RarityA > RarityB) return 1;
	return 0;
}

int32 UItemFunctionLibrary::CompareItemInstanceWeight(const UItemInstance* ItemA, const UItemInstance* ItemB)
{
	if (!ItemA || !ItemB) return 0;
	
	float WeightA = ItemA->GetTotalWeight();
	float WeightB = ItemB->GetTotalWeight();
	
	if (WeightA < WeightB) return -1;
	if (WeightA > WeightB) return 1;
	return 0;
}

// ═══════════════════════════════════════════════
// UTILITY FUNCTIONS
// ═══════════════════════════════════════════════

EDefenseType UItemFunctionLibrary::DamageTypeToResistance(EDamageType DamageType)
{
	switch (DamageType)
	{
		case EDamageType::DT_Fire:       return EDefenseType::DFT_FireResistance;
		case EDamageType::DT_Ice:        return EDefenseType::DFT_IceResistance;
		case EDamageType::DT_Lightning:  return EDefenseType::DFT_LightningResistance;
		case EDamageType::DT_Light:      return EDefenseType::DFT_LightResistance;
		case EDamageType::DT_Corruption: return EDefenseType::DFT_CorruptionResistance;
		default: return EDefenseType::DFT_None;
	}
}

FText UItemFunctionLibrary::GetItemTypeName(EItemType ItemType)
{
	return UEnum::GetDisplayValueAsText(ItemType);
}

FText UItemFunctionLibrary::GetItemSubTypeName(EItemSubType SubType)
{
	return UEnum::GetDisplayValueAsText(SubType);
}
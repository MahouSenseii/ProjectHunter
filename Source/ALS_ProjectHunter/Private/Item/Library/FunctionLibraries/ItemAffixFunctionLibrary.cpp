#include "Item/Library/FunctionLibraries/ItemAffixFunctionLibrary.h"

#include "Item/Library/FunctionLibraries/ItemEnumFunctionLibrary.h"

FText UItemAffixFunctionLibrary::GetAffixCountText(EItemRarity Rarity)
{
	int32 MinPrefixes;
	int32 MaxPrefixes;
	int32 MinSuffixes;
	int32 MaxSuffixes;
	GetAffixCountByRarity(Rarity, MinPrefixes, MaxPrefixes, MinSuffixes, MaxSuffixes);

	const int32 MinTotal = MinPrefixes + MinSuffixes;
	const int32 MaxTotal = MaxPrefixes + MaxSuffixes;

	if (MinTotal == 0 && MaxTotal == 0)
	{
		return FText::FromString("No Affixes");
	}

	if (MinTotal == MaxTotal)
	{
		return FText::Format(FText::FromString("{0} Affixes"), MaxTotal);
	}

	return FText::Format(FText::FromString("{0}-{1} Affixes"), MinTotal, MaxTotal);
}

FString UItemAffixFunctionLibrary::FormatAffixValue(
	float Value,
	EAttributeDisplayFormat Format,
	FName AttributeName,
	float MinValue,
	float MaxValue,
	const FText& CustomText)
{
	switch (Format)
	{
		case EAttributeDisplayFormat::ADF_Additive:
			return FString::Printf(TEXT("+%d to %s"),
				FMath::RoundToInt(Value), *AttributeName.ToString());

		case EAttributeDisplayFormat::ADF_FlatNegative:
			return FString::Printf(TEXT("-%d to %s"),
				FMath::RoundToInt(FMath::Abs(Value)), *AttributeName.ToString());

		case EAttributeDisplayFormat::ADF_Percent:
			return FString::Printf(TEXT("+%d%% %s"),
				FMath::RoundToInt(Value), *AttributeName.ToString());

		case EAttributeDisplayFormat::ADF_MinMax:
			return FString::Printf(TEXT("Adds %d-%d %s"),
				FMath::RoundToInt(MinValue), FMath::RoundToInt(MaxValue), *AttributeName.ToString());

		case EAttributeDisplayFormat::ADF_Increase:
			return FString::Printf(TEXT("%d%% increased %s"),
				FMath::RoundToInt(Value), *AttributeName.ToString());

		case EAttributeDisplayFormat::ADF_More:
			return FString::Printf(TEXT("%d%% more %s"),
				FMath::RoundToInt(Value), *AttributeName.ToString());

		case EAttributeDisplayFormat::ADF_Less:
			return FString::Printf(TEXT("%d%% less %s"),
				FMath::RoundToInt(Value), *AttributeName.ToString());

		case EAttributeDisplayFormat::ADF_Chance:
			return FString::Printf(TEXT("%d%% chance to %s"),
				FMath::RoundToInt(Value), *AttributeName.ToString());

		case EAttributeDisplayFormat::ADF_Duration:
			return FString::Printf(TEXT("%.1fs duration to %s"),
				Value, *AttributeName.ToString());

		case EAttributeDisplayFormat::ADF_Cooldown:
			return FString::Printf(TEXT("%.1fs cooldown on %s"),
				Value, *AttributeName.ToString());

		case EAttributeDisplayFormat::ADF_SkillGrant:
			return FString::Printf(TEXT("Grants [%s] Level %d"),
				*AttributeName.ToString(), FMath::RoundToInt(Value));

		case EAttributeDisplayFormat::ADF_CustomText:
			return CustomText.IsEmpty()
				? FString::Printf(TEXT("%d %s"), FMath::RoundToInt(Value), *AttributeName.ToString())
				: CustomText.ToString();

		default:
			return FString::Printf(TEXT("%d %s"),
				FMath::RoundToInt(Value), *AttributeName.ToString());
	}
}

FString UItemAffixFunctionLibrary::FormatAffixText(const FPHAttributeData& Affix)
{
	FString Formatted;
	if (Affix.DisplayFormat == EAttributeDisplayFormat::ADF_MinMax)
	{
		Formatted = FString::Printf(
			TEXT("Adds %d-%d %s"),
			FMath::RoundToInt(Affix.RolledStatValue),
			FMath::RoundToInt(Affix.RolledSecondaryStatValue),
			*Affix.AttributeName.ToString());
	}
	else
	{
		Formatted = FormatAffixValue(
			Affix.RolledStatValue,
			Affix.DisplayFormat,
			Affix.AttributeName,
			Affix.MinValue,
			Affix.MaxValue,
			Affix.DisplayText);
	}

	if (!Affix.ConditionDescription.IsEmpty())
	{
		Formatted += FString::Printf(TEXT(" (%s)"), *Affix.ConditionDescription.ToString());
	}
	return Formatted;
}

FString UItemAffixFunctionLibrary::GetModifyTypeSymbol(EModifyType ModifyType)
{
	return UItemEnumFunctionLibrary::GetModifyTypeSymbol(ModifyType);
}

int32 UItemAffixFunctionLibrary::GetRankPointsValue(ERankPoints Points)
{
	return UItemEnumFunctionLibrary::GetRankPointsValue(Points);
}

FText UItemAffixFunctionLibrary::GetTierName(ERankPoints Points)
{
	const int32 Value = GetRankPointsValue(Points);

	if (Value < 0)
	{
		return FText::Format(FText::FromString("Cursed (Tier {0})"), FMath::Abs(Value));
	}

	if (Value == 0)
	{
		return FText::FromString("No Bonus");
	}

	if (Value >= 10)
	{
		return FText::FromString("Perfect (Tier 10)");
	}

	return FText::Format(FText::FromString("Tier {0}"), Value);
}

bool UItemAffixFunctionLibrary::CompareAffixRank(const FPHAttributeData& AffixA, const FPHAttributeData& AffixB)
{
	return GetRankPointsValue(AffixA.RankPoints) > GetRankPointsValue(AffixB.RankPoints);
}

void UItemAffixFunctionLibrary::GetAffixCountByRarity(
	EItemRarity Rarity,
	int32& OutMinPrefixes,
	int32& OutMaxPrefixes,
	int32& OutMinSuffixes,
	int32& OutMaxSuffixes)
{
	switch (Rarity)
	{
		case EItemRarity::IR_GradeF:
			OutMinPrefixes = 0; OutMaxPrefixes = 0;
			OutMinSuffixes = 0; OutMaxSuffixes = 0;
			break;

		case EItemRarity::IR_GradeE:
			OutMinPrefixes = 0; OutMaxPrefixes = 1;
			OutMinSuffixes = 0; OutMaxSuffixes = 1;
			break;

		// Match FAffixGenerator so UI does not advertise impossible affix counts.
		case EItemRarity::IR_GradeD:
			OutMinPrefixes = 1; OutMaxPrefixes = 1;
			OutMinSuffixes = 0; OutMaxSuffixes = 1;
			break;

		case EItemRarity::IR_GradeC:
			OutMinPrefixes = 1; OutMaxPrefixes = 2;
			OutMinSuffixes = 1; OutMaxSuffixes = 1;
			break;

		case EItemRarity::IR_GradeB:
			OutMinPrefixes = 1; OutMaxPrefixes = 2;
			OutMinSuffixes = 1; OutMaxSuffixes = 2;
			break;

		case EItemRarity::IR_GradeA:
			OutMinPrefixes = 2; OutMaxPrefixes = 3;
			OutMinSuffixes = 2; OutMaxSuffixes = 2;
			break;

		case EItemRarity::IR_GradeS:
			OutMinPrefixes = 2; OutMaxPrefixes = 3;
			OutMinSuffixes = 2; OutMaxSuffixes = 3;
			break;

		case EItemRarity::IR_GradeSS:
			OutMinPrefixes = 3; OutMaxPrefixes = 3;
			OutMinSuffixes = 3; OutMaxSuffixes = 3;
			break;

		default:
			OutMinPrefixes = 0; OutMaxPrefixes = 0;
			OutMinSuffixes = 0; OutMaxSuffixes = 0;
			break;
	}
}

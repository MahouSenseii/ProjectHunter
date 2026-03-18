#include "Data/BaseStatsData.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AttributeSet.h"
#include "UObject/Field.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogBaseStatsData, Log, All);

namespace BaseStatsDataPrivate
{
	static constexpr TCHAR FallbackCategoryName[] = TEXT("Uncategorized");

	struct FReflectedStatDefinition
	{
		FName StatName = NAME_None;
		FText DisplayName;
		FString RawCategory;
		FName Category = NAME_None;
		FName MainCategory = NAME_None;
		FName SubCategory = NAME_None;
		int32 SortOrder = 0;
		FText Tooltip;
		FName IconName = NAME_None;
		EHunterStatType StatType = EHunterStatType::Neutral;
	};

	static bool IsGameplayAttributeDataProperty(const FProperty* Property)
	{
		const FStructProperty* StructProp = CastField<FStructProperty>(Property);
		return StructProp && StructProp->Struct == TBaseStructure<FGameplayAttributeData>::Get();
	}

	static bool ContainsToken(const FString& Source, const TCHAR* Token)
	{
		return Source.Contains(Token, ESearchCase::IgnoreCase);
	}

	static FString NormalizeCategoryToken(const FString& InToken)
	{
		FString TrimmedToken = InToken;
		TrimmedToken.TrimStartAndEndInline();

		if (TrimmedToken.IsEmpty())
		{
			return FString();
		}

		TArray<FString> Words;
		TrimmedToken.ParseIntoArrayWS(Words);
		if (Words.Num() == 0)
		{
			return FString();
		}

		return FString::Join(Words, TEXT(" "));
	}

	static FString BuildNormalizedCategoryPath(const FString& MainCategory, const FString& SubCategory)
	{
		return SubCategory.IsEmpty()
			? MainCategory
			: FString::Printf(TEXT("%s|%s"), *MainCategory, *SubCategory);
	}

	static FParsedStatCategory ParseCategoryPathImpl(const FString& InCategoryString)
	{
		FParsedStatCategory ParsedCategory;
		ParsedCategory.RawCategory = InCategoryString;
		ParsedCategory.RawCategory.TrimStartAndEndInline();

		// This is the one canonical split path used by runtime, debug, and editor code.
		// We keep the trimmed raw string, then normalize the logical "Parent|Child" path.
		TArray<FString> RawSegments;
		ParsedCategory.RawCategory.ParseIntoArray(RawSegments, TEXT("|"), false);

		TArray<FString> NormalizedSegments;
		NormalizedSegments.Reserve(RawSegments.Num());

		for (const FString& RawSegment : RawSegments)
		{
			const FString NormalizedSegment = NormalizeCategoryToken(RawSegment);
			if (!NormalizedSegment.IsEmpty())
			{
				NormalizedSegments.Add(NormalizedSegment);
			}
		}

		const FString MainCategory = NormalizedSegments.Num() > 0
			? NormalizedSegments[0]
			: FString(FallbackCategoryName);
		const FString SubCategory = NormalizedSegments.Num() > 1
			? NormalizedSegments[1]
			: FString();

		ParsedCategory.MainCategory = FName(*MainCategory);
		ParsedCategory.SubCategory = SubCategory.IsEmpty() ? NAME_None : FName(*SubCategory);
		ParsedCategory.NormalizedCategory = FName(*BuildNormalizedCategoryPath(MainCategory, SubCategory));

		return ParsedCategory;
	}

	static int32 GetMainCategoryPriority(FName CategoryName)
	{
		const FParsedStatCategory ParsedCategory = ParseCategoryPathImpl(CategoryName.ToString());
		const FString MainCategory = ParsedCategory.MainCategory.ToString();

		if (ContainsToken(MainCategory, TEXT("Primary")))
		{
			return 0;
		}

		if (MainCategory.Equals(TEXT("Vital"), ESearchCase::IgnoreCase) ||
			MainCategory.Equals(TEXT("Vitals"), ESearchCase::IgnoreCase))
		{
			return 10;
		}

		if (ContainsToken(MainCategory, TEXT("Offense")) || ContainsToken(MainCategory, TEXT("Combat")))
		{
			return 20;
		}

		if (MainCategory.Equals(TEXT("Defense"), ESearchCase::IgnoreCase) ||
			MainCategory.Equals(TEXT("Defence"), ESearchCase::IgnoreCase))
		{
			return 30;
		}

		if (ContainsToken(MainCategory, TEXT("Secondary")))
		{
			return 40;
		}

		if (ContainsToken(MainCategory, TEXT("Movement")))
		{
			return 50;
		}

		if (ContainsToken(MainCategory, TEXT("Utility")))
		{
			return 60;
		}

		if (ContainsToken(MainCategory, TEXT("Loot")))
		{
			return 70;
		}

		if (MainCategory.Equals(TEXT("Experience"), ESearchCase::IgnoreCase) ||
			MainCategory.Equals(TEXT("XP"), ESearchCase::IgnoreCase))
		{
			return 80;
		}

		if (ContainsToken(MainCategory, TEXT("Special")))
		{
			return 90;
		}

		if (ParsedCategory.MainCategory == FName(FallbackCategoryName))
		{
			return 1000;
		}

		return 500;
	}

	static EHunterStatType GetStatTypeForParsedCategory(const FParsedStatCategory& ParsedCategory)
	{
		const FString MainCategory = ParsedCategory.MainCategory.ToString();
		const FString SubCategory = ParsedCategory.SubCategory.ToString();

		if (ContainsToken(MainCategory, TEXT("Primary")))
		{
			return EHunterStatType::Primary;
		}

		if (MainCategory.Equals(TEXT("Vital"), ESearchCase::IgnoreCase) ||
			MainCategory.Equals(TEXT("Vitals"), ESearchCase::IgnoreCase))
		{
			return EHunterStatType::Vital;
		}

		if (ContainsToken(MainCategory, TEXT("Offense")) || ContainsToken(MainCategory, TEXT("Combat")))
		{
			return EHunterStatType::Offense;
		}

		if (MainCategory.Equals(TEXT("Defense"), ESearchCase::IgnoreCase) ||
			MainCategory.Equals(TEXT("Defence"), ESearchCase::IgnoreCase))
		{
			return EHunterStatType::Defense;
		}

		if (ContainsToken(MainCategory, TEXT("Secondary")))
		{
			if (ContainsToken(SubCategory, TEXT("Resist")) ||
				ContainsToken(SubCategory, TEXT("Reflect")) ||
				ContainsToken(SubCategory, TEXT("Armour")) ||
				ContainsToken(SubCategory, TEXT("Armor")) ||
				ContainsToken(SubCategory, TEXT("Block")))
			{
				return EHunterStatType::Defense;
			}

			if (ContainsToken(SubCategory, TEXT("Damage")) ||
				ContainsToken(SubCategory, TEXT("Offensive")) ||
				ContainsToken(SubCategory, TEXT("Conversion")) ||
				ContainsToken(SubCategory, TEXT("Ailment")) ||
				ContainsToken(SubCategory, TEXT("Duration")) ||
				ContainsToken(SubCategory, TEXT("Piercing")))
			{
				return EHunterStatType::Offense;
			}
		}

		if (ContainsToken(MainCategory, TEXT("Movement")) ||
			ContainsToken(MainCategory, TEXT("Utility")) ||
			ContainsToken(MainCategory, TEXT("Loot")) ||
			MainCategory.Equals(TEXT("Experience"), ESearchCase::IgnoreCase) ||
			MainCategory.Equals(TEXT("XP"), ESearchCase::IgnoreCase))
		{
			return EHunterStatType::Utility;
		}

		if (ContainsToken(MainCategory, TEXT("Special")))
		{
			return EHunterStatType::Special;
		}

		return EHunterStatType::Neutral;
	}

	static EHunterStatType ParseStatType(const FString& StatTypeString)
	{
		if (StatTypeString.IsEmpty())
		{
			return EHunterStatType::Neutral;
		}

		if (ContainsToken(StatTypeString, TEXT("Primary")))
		{
			return EHunterStatType::Primary;
		}

		if (ContainsToken(StatTypeString, TEXT("Offense")))
		{
			return EHunterStatType::Offense;
		}

		if (ContainsToken(StatTypeString, TEXT("Defense")))
		{
			return EHunterStatType::Defense;
		}

		if (ContainsToken(StatTypeString, TEXT("Resource")))
		{
			return EHunterStatType::Resource;
		}

		if (ContainsToken(StatTypeString, TEXT("Utility")))
		{
			return EHunterStatType::Utility;
		}

		if (ContainsToken(StatTypeString, TEXT("Special")))
		{
			return EHunterStatType::Special;
		}

		return EHunterStatType::Neutral;
	}

	static FString GetRawCategoryFromProperty(const FProperty* Property)
	{
		if (!Property)
		{
			return FString();
		}

		const FString StatCategory = Property->GetMetaData(TEXT("StatCategory"));
		if (!StatCategory.IsEmpty())
		{
			FString RawCategory = StatCategory;
			RawCategory.TrimStartAndEndInline();
			return RawCategory;
		}

		const FString PropertyCategory = Property->GetMetaData(TEXT("Category"));
		if (!PropertyCategory.IsEmpty())
		{
			FString RawCategory = PropertyCategory;
			RawCategory.TrimStartAndEndInline();
			return RawCategory;
		}

		return FString();
	}

	static int32 GetSortOrderFromProperty(const FProperty* Property)
	{
		if (!Property)
		{
			return 0;
		}

		const FString SortOrderString = Property->GetMetaData(TEXT("SortOrder"));
		return SortOrderString.IsEmpty() ? 0 : FCString::Atoi(*SortOrderString);
	}

	static FText GetTooltipFromProperty(const FProperty* Property)
	{
		if (!Property)
		{
			return FText::GetEmpty();
		}

		const FString TooltipString = Property->GetMetaData(TEXT("StatTooltip"));
		if (!TooltipString.IsEmpty())
		{
			return FText::FromString(TooltipString);
		}

		return Property->GetToolTipText();
	}

	static FName GetIconNameFromProperty(const FProperty* Property)
	{
		if (!Property)
		{
			return NAME_None;
		}

		const FString IconString = Property->GetMetaData(TEXT("StatIcon"));
		return IconString.IsEmpty() ? NAME_None : FName(*IconString);
	}

	static EHunterStatType GetStatTypeFromProperty(const FProperty* Property, const FParsedStatCategory& ParsedCategory)
	{
		if (!Property)
		{
			return EHunterStatType::Neutral;
		}

		const FString StatTypeString = Property->GetMetaData(TEXT("StatType"));
		if (!StatTypeString.IsEmpty())
		{
			const EHunterStatType ParsedStatType = ParseStatType(StatTypeString);
			const bool bIsVitalCategory =
				ParsedCategory.MainCategory.IsEqual(TEXT("Vital"), ENameCase::IgnoreCase) ||
				ParsedCategory.MainCategory.IsEqual(TEXT("Vitals"), ENameCase::IgnoreCase);

			if (bIsVitalCategory && ParsedStatType == EHunterStatType::Resource)
			{
				return EHunterStatType::Vital;
			}

			return ParsedStatType;
		}

		return GetStatTypeForParsedCategory(ParsedCategory);
	}

	static void ApplyParsedCategory(FStatInitializationEntry& Entry, const FParsedStatCategory& ParsedCategory)
	{
		Entry.RawCategory = ParsedCategory.RawCategory;
		Entry.Category = ParsedCategory.NormalizedCategory;
		Entry.MainCategory = ParsedCategory.MainCategory;
		Entry.SubCategory = ParsedCategory.SubCategory;
	}

	static void NormalizeEntryCategory(FStatInitializationEntry& Entry)
	{
		const FString CategorySource = !Entry.RawCategory.IsEmpty()
			? Entry.RawCategory
			: Entry.Category.ToString();
		const FParsedStatCategory ParsedCategory = ParseCategoryPathImpl(CategorySource);
		ApplyParsedCategory(Entry, ParsedCategory);

		const bool bIsVitalCategory =
			ParsedCategory.MainCategory.IsEqual(TEXT("Vital"), ENameCase::IgnoreCase) ||
			ParsedCategory.MainCategory.IsEqual(TEXT("Vitals"), ENameCase::IgnoreCase);

		if (bIsVitalCategory && Entry.StatType == EHunterStatType::Resource)
		{
			Entry.StatType = EHunterStatType::Vital;
		}
	}

	static TArray<FReflectedStatDefinition> GatherAttributeSetDefinitions(const UClass* AttributeSetClass)
	{
		TArray<FReflectedStatDefinition> Results;

		if (!AttributeSetClass)
		{
			UE_LOG(LogBaseStatsData, Warning, TEXT("GatherAttributeSetDefinitions: AttributeSetClass is null"));
			return Results;
		}

		if (!AttributeSetClass->IsChildOf(UAttributeSet::StaticClass()))
		{
			UE_LOG(LogBaseStatsData, Warning, TEXT("GatherAttributeSetDefinitions: %s is not an AttributeSet"), *GetNameSafe(AttributeSetClass));
			return Results;
		}

		for (TFieldIterator<FProperty> It(AttributeSetClass, EFieldIterationFlags::IncludeSuper); It; ++It)
		{
			FProperty* Property = *It;
			if (!Property || !IsGameplayAttributeDataProperty(Property) || Property->HasMetaData(TEXT("HideInStatsData")))
			{
				continue;
			}

			const FString RawCategory = GetRawCategoryFromProperty(Property);
			const FParsedStatCategory ParsedCategory = ParseCategoryPathImpl(RawCategory);

			FReflectedStatDefinition Definition;
			Definition.StatName = Property->GetFName();
			Definition.DisplayName = Property->GetDisplayNameText();
			Definition.RawCategory = ParsedCategory.RawCategory;
			Definition.Category = ParsedCategory.NormalizedCategory;
			Definition.MainCategory = ParsedCategory.MainCategory;
			Definition.SubCategory = ParsedCategory.SubCategory;
			Definition.SortOrder = GetSortOrderFromProperty(Property);
			Definition.Tooltip = GetTooltipFromProperty(Property);
			Definition.IconName = GetIconNameFromProperty(Property);
			Definition.StatType = GetStatTypeFromProperty(Property, ParsedCategory);

			UE_LOG(
				LogBaseStatsData,
				Verbose,
				TEXT("GatherAttributeSetDefinitions: Stat=%s RawCategory='%s' Main='%s' Sub='%s' Normalized='%s'"),
				*Definition.StatName.ToString(),
				*Definition.RawCategory,
				*Definition.MainCategory.ToString(),
				*Definition.SubCategory.ToString(),
				*Definition.Category.ToString());

			Results.Add(MoveTemp(Definition));
		}

		Results.Sort([](const FReflectedStatDefinition& A, const FReflectedStatDefinition& B)
		{
			const int32 MainPriorityA = GetMainCategoryPriority(A.MainCategory);
			const int32 MainPriorityB = GetMainCategoryPriority(B.MainCategory);
			if (MainPriorityA != MainPriorityB)
			{
				return MainPriorityA < MainPriorityB;
			}

			const int32 MainCompare = A.MainCategory.ToString().Compare(B.MainCategory.ToString(), ESearchCase::IgnoreCase);
			if (MainCompare != 0)
			{
				return MainCompare < 0;
			}

			const FString SubCategoryA = A.SubCategory.ToString();
			const FString SubCategoryB = B.SubCategory.ToString();
			const bool bHasSubCategoryA = !SubCategoryA.IsEmpty();
			const bool bHasSubCategoryB = !SubCategoryB.IsEmpty();
			if (bHasSubCategoryA != bHasSubCategoryB)
			{
				return !bHasSubCategoryA;
			}

			const int32 SubCompare = SubCategoryA.Compare(SubCategoryB, ESearchCase::IgnoreCase);
			if (SubCompare != 0)
			{
				return SubCompare < 0;
			}

			if (A.SortOrder != B.SortOrder)
			{
				return A.SortOrder < B.SortOrder;
			}

			const int32 DisplayCompare = A.DisplayName.ToString().Compare(B.DisplayName.ToString(), ESearchCase::IgnoreCase);
			if (DisplayCompare != 0)
			{
				return DisplayCompare < 0;
			}

			return A.StatName.ToString().Compare(B.StatName.ToString(), ESearchCase::IgnoreCase) < 0;
		});

		return Results;
	}

	static void ApplyStarterOverride(TArray<FStatInitializationEntry>& Entries, FName StatName, float Value)
	{
		if (FStatInitializationEntry* Entry = Entries.FindByPredicate(
			[&StatName](const FStatInitializationEntry& Existing)
			{
				return Existing.StatName == StatName;
			}))
		{
			Entry->bOverrideValue = true;
			Entry->BaseValue = Value;
		}
	}
}

UBaseStatsData::UBaseStatsData()
{
}

TSubclassOf<UAttributeSet> UBaseStatsData::ResolveSourceAttributeSetClass(const UBaseStatsData* Data)
{
	if (Data && Data->SourceAttributeSetClass)
	{
		return Data->SourceAttributeSetClass;
	}

	return UHunterAttributeSet::StaticClass();
}

void UBaseStatsData::GatherStatDefinitionsFromAttributeSet(TSubclassOf<UAttributeSet> AttributeSetClass, TArray<FStatInitializationEntry>& OutDefinitions)
{
	OutDefinitions.Reset();

	const TArray<BaseStatsDataPrivate::FReflectedStatDefinition> ReflectedDefinitions =
		BaseStatsDataPrivate::GatherAttributeSetDefinitions(AttributeSetClass.Get());

	OutDefinitions.Reserve(ReflectedDefinitions.Num());

	for (const BaseStatsDataPrivate::FReflectedStatDefinition& Definition : ReflectedDefinitions)
	{
		FStatInitializationEntry Entry;
		Entry.StatName = Definition.StatName;
		Entry.DisplayName = Definition.DisplayName;
		Entry.RawCategory = Definition.RawCategory;
		Entry.Category = Definition.Category;
		Entry.MainCategory = Definition.MainCategory;
		Entry.SubCategory = Definition.SubCategory;
		Entry.SortOrder = Definition.SortOrder;
		Entry.Tooltip = Definition.Tooltip;
		Entry.IconName = Definition.IconName;
		Entry.StatType = Definition.StatType;

		OutDefinitions.Add(MoveTemp(Entry));
	}
}

FParsedStatCategory UBaseStatsData::ParseCategoryPath(const FString& CategoryString)
{
	return BaseStatsDataPrivate::ParseCategoryPathImpl(CategoryString);
}

FParsedStatCategory UBaseStatsData::ParseCategoryPath(const FName& CategoryName)
{
	return BaseStatsDataPrivate::ParseCategoryPathImpl(CategoryName.ToString());
}

FName UBaseStatsData::NormalizeCategoryName(const FName& CategoryName)
{
	return ParseCategoryPath(CategoryName).NormalizedCategory;
}

int32 UBaseStatsData::GetCategorySortPriority(const FName& CategoryName)
{
	return BaseStatsDataPrivate::GetMainCategoryPriority(CategoryName);
}

FName UBaseStatsData::CallNormalizeCategoryName(const FName& CategoryName)
{
	return ParseCategoryPath(CategoryName).NormalizedCategory;
}

FLinearColor UBaseStatsData::GetStatTypeColor(EHunterStatType StatType)
{
	switch (StatType)
	{
	case EHunterStatType::Primary:
		return FLinearColor(0.96f, 0.81f, 0.35f, 1.0f);
	case EHunterStatType::Vital:
		return FLinearColor(0.0f, 0.81f, 0.0f, 1.0f);
	case EHunterStatType::Offense:
		return FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
	case EHunterStatType::Defense:
		return FLinearColor(0.39f, 0.70f, 0.92f, 1.0f);
	case EHunterStatType::Resource:
		return FLinearColor(0.34f, 0.78f, 0.56f, 1.0f);
	case EHunterStatType::Utility:
		return FLinearColor(0.66f, 0.66f, 0.77f, 1.0f);
	case EHunterStatType::Special:
		return FLinearColor(0.77f, 0.58f, 0.92f, 1.0f);
	default:
		return FLinearColor(0.82f, 0.82f, 0.82f, 1.0f);
	}
}

TMap<FName, float> UBaseStatsData::GetAllStatsAsMap() const
{
	TMap<FName, float> Result;

	for (const FStatInitializationEntry& Entry : BaseAttributes)
	{
		if (!Entry.IsValid() || !Entry.HasRuntimeValue())
		{
			continue;
		}

		Result.Add(Entry.StatName, Entry.BaseValue);
	}

	return Result;
}

TMap<FName, float> UBaseStatsData::GetStatsByCategory(FName CategoryName) const
{
	TMap<FName, float> Result;
	const FName NormalizedCategory = NormalizeCategoryName(CategoryName);

	for (const FStatInitializationEntry& Entry : BaseAttributes)
	{
		if (!Entry.IsValid() || !Entry.HasRuntimeValue())
		{
			continue;
		}

		if (NormalizeCategoryName(Entry.Category) == NormalizedCategory)
		{
			Result.Add(Entry.StatName, Entry.BaseValue);
		}
	}

	return Result;
}

TArray<FName> UBaseStatsData::GetSupportedCategories() const
{
	TSet<FName> UniqueCategories;

	for (const FStatInitializationEntry& Entry : BaseAttributes)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		UniqueCategories.Add(NormalizeCategoryName(Entry.Category));
	}

	TArray<FName> Result = UniqueCategories.Array();
	Result.Sort([](const FName& A, const FName& B)
	{
		const int32 PriorityA = UBaseStatsData::GetCategorySortPriority(A);
		const int32 PriorityB = UBaseStatsData::GetCategorySortPriority(B);
		if (PriorityA != PriorityB)
		{
			return PriorityA < PriorityB;
		}

		const FParsedStatCategory ParsedA = UBaseStatsData::ParseCategoryPath(A);
		const FParsedStatCategory ParsedB = UBaseStatsData::ParseCategoryPath(B);

		const int32 MainCompare = ParsedA.MainCategory.ToString().Compare(ParsedB.MainCategory.ToString(), ESearchCase::IgnoreCase);
		if (MainCompare != 0)
		{
			return MainCompare < 0;
		}

		const FString SubCategoryA = ParsedA.SubCategory.ToString();
		const FString SubCategoryB = ParsedB.SubCategory.ToString();
		const bool bHasSubCategoryA = !SubCategoryA.IsEmpty();
		const bool bHasSubCategoryB = !SubCategoryB.IsEmpty();
		if (bHasSubCategoryA != bHasSubCategoryB)
		{
			return !bHasSubCategoryA;
		}

		return SubCategoryA.Compare(SubCategoryB, ESearchCase::IgnoreCase) < 0;
	});

	return Result;
}

TArray<FName> UBaseStatsData::GetSupportedStatNames() const
{
	TArray<FName> Result;
	Result.Reserve(BaseAttributes.Num());

	for (const FStatInitializationEntry& Entry : BaseAttributes)
	{
		if (Entry.IsValid())
		{
			Result.Add(Entry.StatName);
		}
	}

	Result.Sort([](const FName& A, const FName& B)
	{
		return A.ToString().Compare(B.ToString(), ESearchCase::IgnoreCase) < 0;
	});

	return Result;
}

TArray<FStatInitializationEntry> UBaseStatsData::GetStatEntriesByCategory(FName CategoryName) const
{
	TArray<FStatInitializationEntry> Result;
	const FName NormalizedCategory = NormalizeCategoryName(CategoryName);

	for (const FStatInitializationEntry& Entry : BaseAttributes)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		if (NormalizeCategoryName(Entry.Category) == NormalizedCategory)
		{
			Result.Add(Entry);
		}
	}

	Result.Sort([](const FStatInitializationEntry& A, const FStatInitializationEntry& B)
	{
		if (A.SortOrder != B.SortOrder)
		{
			return A.SortOrder < B.SortOrder;
		}

		const int32 DisplayCompare = A.DisplayName.ToString().Compare(B.DisplayName.ToString(), ESearchCase::IgnoreCase);
		if (DisplayCompare != 0)
		{
			return DisplayCompare < 0;
		}

		return A.StatName.ToString().Compare(B.StatName.ToString(), ESearchCase::IgnoreCase) < 0;
	});

	return Result;
}

bool UBaseStatsData::GetStatValue(FName AttributeName, float& OutValue) const
{
	for (const FStatInitializationEntry& Entry : BaseAttributes)
	{
		if (Entry.StatName == AttributeName && Entry.HasRuntimeValue())
		{
			OutValue = Entry.BaseValue;
			return true;
		}
	}

	return false;
}

bool UBaseStatsData::HasAttribute(FName AttributeName) const
{
	return BaseAttributes.ContainsByPredicate(
		[&AttributeName](const FStatInitializationEntry& Entry)
		{
			return Entry.StatName == AttributeName;
		});
}

bool UBaseStatsData::HasCategory(FName CategoryName) const
{
	const FName NormalizedCategory = NormalizeCategoryName(CategoryName);

	return BaseAttributes.ContainsByPredicate(
		[&NormalizedCategory](const FStatInitializationEntry& Entry)
		{
			return UBaseStatsData::NormalizeCategoryName(Entry.Category) == NormalizedCategory;
		});
}

void UBaseStatsData::SortStatsByCategoryThenName()
{
	BaseAttributes.Sort([](const FStatInitializationEntry& A, const FStatInitializationEntry& B)
	{
		const FParsedStatCategory CategoryA = UBaseStatsData::ParseCategoryPath(A.Category);
		const FParsedStatCategory CategoryB = UBaseStatsData::ParseCategoryPath(B.Category);

		const int32 PriorityA = UBaseStatsData::GetCategorySortPriority(CategoryA.MainCategory);
		const int32 PriorityB = UBaseStatsData::GetCategorySortPriority(CategoryB.MainCategory);
		if (PriorityA != PriorityB)
		{
			return PriorityA < PriorityB;
		}

		const int32 MainCompare = CategoryA.MainCategory.ToString().Compare(CategoryB.MainCategory.ToString(), ESearchCase::IgnoreCase);
		if (MainCompare != 0)
		{
			return MainCompare < 0;
		}

		const FString SubCategoryA = CategoryA.SubCategory.ToString();
		const FString SubCategoryB = CategoryB.SubCategory.ToString();
		const bool bHasSubCategoryA = !SubCategoryA.IsEmpty();
		const bool bHasSubCategoryB = !SubCategoryB.IsEmpty();
		if (bHasSubCategoryA != bHasSubCategoryB)
		{
			return !bHasSubCategoryA;
		}

		const int32 SubCompare = SubCategoryA.Compare(SubCategoryB, ESearchCase::IgnoreCase);
		if (SubCompare != 0)
		{
			return SubCompare < 0;
		}

		if (A.SortOrder != B.SortOrder)
		{
			return A.SortOrder < B.SortOrder;
		}

		const int32 DisplayCompare = A.DisplayName.ToString().Compare(B.DisplayName.ToString(), ESearchCase::IgnoreCase);
		if (DisplayCompare != 0)
		{
			return DisplayCompare < 0;
		}

		return A.StatName.ToString().Compare(B.StatName.ToString(), ESearchCase::IgnoreCase) < 0;
	});
}

void UBaseStatsData::RefreshCategoriesFromDefinitions()
{
	RefreshFromAttributeSetDefinition();
}

void UBaseStatsData::RefreshFromAttributeSetDefinition()
{
	const TSubclassOf<UAttributeSet> ResolvedAttributeSetClass = ResolveSourceAttributeSetClass(this);
	if (!SourceAttributeSetClass)
	{
		UE_LOG(
			LogBaseStatsData,
			Warning,
			TEXT("RefreshFromAttributeSetDefinition: SourceAttributeSetClass is null on %s, defaulting to %s"),
			*GetName(),
			*GetNameSafe(ResolvedAttributeSetClass));
	}

	TArray<FStatInitializationEntry> ReflectedDefinitions;
	GatherStatDefinitionsFromAttributeSet(ResolvedAttributeSetClass, ReflectedDefinitions);

	TMap<FName, FStatInitializationEntry> ExistingEntries;
	ExistingEntries.Reserve(BaseAttributes.Num());

	for (const FStatInitializationEntry& Entry : BaseAttributes)
	{
		if (Entry.IsValid())
		{
			ExistingEntries.Add(Entry.StatName, Entry);
		}
	}

	TArray<FStatInitializationEntry> RebuiltEntries;
	RebuiltEntries.Reserve(ReflectedDefinitions.Num());

	for (const FStatInitializationEntry& ReflectedEntry : ReflectedDefinitions)
	{
		if (const FStatInitializationEntry* Existing = ExistingEntries.Find(ReflectedEntry.StatName))
		{
			FStatInitializationEntry NewEntry = ReflectedEntry;
			NewEntry.bOverrideValue = Existing->bOverrideValue;
			NewEntry.BaseValue = Existing->BaseValue;
			RebuiltEntries.Add(MoveTemp(NewEntry));
			continue;
		}

		RebuiltEntries.Add(ReflectedEntry);
	}

	BaseAttributes = MoveTemp(RebuiltEntries);
	SortStatsByCategoryThenName();
}

void UBaseStatsData::SortStats()
{
	SortStatsByCategoryThenName();
}

void UBaseStatsData::ValidateStats()
{
	TSet<FName> SeenNames;
	bool bFoundDuplicate = false;

	for (FStatInitializationEntry& Entry : BaseAttributes)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		BaseStatsDataPrivate::NormalizeEntryCategory(Entry);

		if (SeenNames.Contains(Entry.StatName))
		{
			bFoundDuplicate = true;
			UE_LOG(LogBaseStatsData, Warning, TEXT("ValidateStats: Duplicate stat name '%s' found in %s"), *Entry.StatName.ToString(), *GetName());
		}
		else
		{
			SeenNames.Add(Entry.StatName);
		}
	}

	if (!bFoundDuplicate)
	{
		UE_LOG(LogBaseStatsData, Verbose, TEXT("ValidateStats: No duplicate stats found in %s"), *GetName());
	}

	SortStatsByCategoryThenName();
}

void UBaseStatsData::StartStats()
{
	RefreshFromAttributeSetDefinition();

	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Strength"), 10.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Intelligence"), 10.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Dexterity"), 10.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Endurance"), 10.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Affliction"), 10.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Luck"), 10.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Covenant"), 10.0f);

	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxHealth"), 100.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxMana"), 100.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxStamina"), 100.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxArcaneShield"), 0.0f);

	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Health"), 100.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Mana"), 100.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Stamina"), 100.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("ArcaneShield"), 0.0f);

	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("HealthRegenRate"), 1.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("HealthRegenAmount"), 1.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("ManaRegenRate"), 1.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("ManaRegenAmount"), 1.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("StaminaRegenRate"), 1.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("StaminaRegenAmount"), 1.0f);

	SortStatsByCategoryThenName();
}

bool UBaseStatsData::IsInlineAttributeStat(FName StatName)
{
	return false;
}

void UBaseStatsData::PostLoad()
{
	Super::PostLoad();

	if (SourceAttributeSetClass)
	{
		RefreshFromAttributeSetDefinition();
	}
	else
	{
		ValidateStats();
	}
}

#if WITH_EDITOR
void UBaseStatsData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName ChangedPropertyName = PropertyChangedEvent.GetPropertyName();

	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UBaseStatsData, SourceAttributeSetClass))
	{
		RefreshFromAttributeSetDefinition();
	}
	else if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UBaseStatsData, BaseAttributes))
	{
		ValidateStats();
	}
}
#endif

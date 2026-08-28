#include "Stats/Data/BaseStatsData.h"

#include "GameplayEffect.h"

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

#if WITH_METADATA
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
#endif

		return FString();
	}

	static int32 GetSortOrderFromProperty(const FProperty* Property)
	{
		if (!Property)
		{
			return 0;
		}

#if WITH_METADATA
		const FString SortOrderString = Property->GetMetaData(TEXT("SortOrder"));
		return SortOrderString.IsEmpty() ? 0 : FCString::Atoi(*SortOrderString);
#else
		return 0;
#endif
	}

	static FText GetTooltipFromProperty(const FProperty* Property)
	{
		if (!Property)
		{
			return FText::GetEmpty();
		}

#if WITH_METADATA
		const FString TooltipString = Property->GetMetaData(TEXT("StatTooltip"));
		if (!TooltipString.IsEmpty())
		{
			return FText::FromString(TooltipString);
		}
#endif

#if WITH_EDITORONLY_DATA
		return Property->GetToolTipText();
#else
		return FText::GetEmpty();
#endif
	}

	static FName GetIconNameFromProperty(const FProperty* Property)
	{
		if (!Property)
		{
			return NAME_None;
		}

#if WITH_METADATA
		const FString IconString = Property->GetMetaData(TEXT("StatIcon"));
		return IconString.IsEmpty() ? NAME_None : FName(*IconString);
#else
		return NAME_None;
#endif
	}

	static EHunterStatType GetStatTypeFromProperty(const FProperty* Property, const FParsedStatCategory& ParsedCategory)
	{
		if (!Property)
		{
			return EHunterStatType::Neutral;
		}

#if WITH_METADATA
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
#endif

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
			if (!Property || !IsGameplayAttributeDataProperty(Property))
			{
				continue;
			}

#if WITH_METADATA
			if (Property->HasMetaData(TEXT("HideInStatsData")))
			{
				continue;
			}
#endif

			const FString RawCategory = GetRawCategoryFromProperty(Property);
			const FParsedStatCategory ParsedCategory = ParseCategoryPathImpl(RawCategory);

			FReflectedStatDefinition Definition;
			Definition.StatName = Property->GetFName();
#if WITH_EDITORONLY_DATA
			Definition.DisplayName = Property->GetDisplayNameText();
#else
			// Cooked builds use serialized BaseAttributes for presentation metadata.
			// This fallback keeps attribute-name resolution available without it.
			Definition.DisplayName = FText::FromString(
				FName::NameToDisplayString(Definition.StatName.ToString(), false));
#endif
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

	struct FRequiredStatDefault
	{
		const TCHAR* StatName;
		float Value;
	};

	static constexpr FRequiredStatDefault RequiredNonZeroDefaults[] =
	{

		// Regeneration and drain are Rate * Amount, so a zero rate silently
		// disables the resource entirely no matter what the amount says. These
		// were previously forced to 1 by an initialization effect; they are data
		// now, which means they have to be defaulted here instead.
		{TEXT("HealthRegenRate"), 1.0f},
		{TEXT("ManaRegenRate"), 1.0f},
		{TEXT("StaminaRegenRate"), 1.0f},
		{TEXT("ArcaneShieldRegenRate"), 1.0f},
		{TEXT("StaminaDegenRate"), 1.0f},
		// Multiplicitive in HunterGE_DerivedPrimaryVitals: a zero base can never
		// be scaled up, so stamina would never drain. The playable value lives in
		// the baseline; this only guarantees it is never zero.
		{TEXT("StaminaDegenAmount"), 1.0f},

		{TEXT("XPGainMultiplier"), 1.0f},
		{TEXT("XPPenalty"), 1.0f},
		{TEXT("BlockStaminaCostMultiplier"), 1.0f},
		{TEXT("BlockAngle"), 120.0f},
		{TEXT("BlockPhysicalMultiplier"), 1.0f},
		{TEXT("BlockElementalMultiplier"), 1.0f},
		{TEXT("BlockCorruptionMultiplier"), 1.0f},
		{TEXT("GlobalMoreDamage"), 1.0f},
		{TEXT("PhysicalMoreDamage"), 1.0f},
		{TEXT("ElementalMoreDamage"), 1.0f},
		{TEXT("FireMoreDamage"), 1.0f},
		{TEXT("IceMoreDamage"), 1.0f},
		{TEXT("LightningMoreDamage"), 1.0f},
		{TEXT("LightMoreDamage"), 1.0f},
		{TEXT("CorruptionMoreDamage"), 1.0f},
		{TEXT("GlobalDamageTakenMultiplier"), 1.0f},
		{TEXT("PhysicalDamageTakenMultiplier"), 1.0f},
		{TEXT("ElementalDamageTakenMultiplier"), 1.0f},
		{TEXT("FireDamageTakenMultiplier"), 1.0f},
		{TEXT("IceDamageTakenMultiplier"), 1.0f},
		{TEXT("LightningDamageTakenMultiplier"), 1.0f},
		{TEXT("LightDamageTakenMultiplier"), 1.0f},
		{TEXT("CorruptionDamageTakenMultiplier"), 1.0f},
		{TEXT("CritMultiplier"), 1.5f},
		{TEXT("SpellsCritMultiplier"), 1.0f},
		{TEXT("MaxLifeLeechRatePercent"), 20.0f},
		{TEXT("MaxManaLeechRatePercent"), 20.0f},
		{TEXT("MaxStaminaLeechRatePercent"), 20.0f},
		{TEXT("ArcaneShieldRechargeDelay"), 2.0f},
		{TEXT("ArcaneShieldRechargeRate"), 20.0f},
		{TEXT("CorruptionShieldDamageMultiplier"), 2.0f},
	};

	static void ApplyRequiredNonZeroDefaults(TArray<FStatInitializationEntry>& Entries)
	{
		for (const FRequiredStatDefault& Default : RequiredNonZeroDefaults)
		{
			ApplyStarterOverride(Entries, Default.StatName, Default.Value);
		}
	}

	static bool HasZeroedLegacyCombatDefaults(const TArray<FStatInitializationEntry>& Entries)
	{
		auto IsAuthoredZero = [&Entries](const FName StatName)
		{
			const FStatInitializationEntry* Entry = Entries.FindByPredicate(
				[StatName](const FStatInitializationEntry& Candidate)
				{
					return Candidate.StatName == StatName;
				});
			return Entry && Entry->bOverrideValue && FMath::IsNearlyZero(Entry->BaseValue);
		};

		// These three unrelated neutral ratios being authored as zero is the
		// signature produced by the old reflected-row setup, not a normal build.
		return IsAuthoredZero(TEXT("GlobalMoreDamage"))
			&& IsAuthoredZero(TEXT("GlobalDamageTakenMultiplier"))
			&& IsAuthoredZero(TEXT("CritMultiplier"));
	}

	static bool MigrateSpellCritMultiplierToNeutral(TArray<FStatInitializationEntry>& Entries)
	{
		FStatInitializationEntry* Entry = Entries.FindByPredicate(
			[](const FStatInitializationEntry& Candidate)
			{
				return Candidate.StatName == TEXT("SpellsCritMultiplier");
			});
		if (!Entry || (Entry->bOverrideValue && FMath::IsNearlyEqual(Entry->BaseValue, 1.f)))
		{
			return false;
		}

		Entry->BaseValue = 1.f;
		Entry->bOverrideValue = true;
		return true;
	}
}

UBaseStatsData::UBaseStatsData()
	: bSkipInitializationEffectsThatModifyAuthoredStats(true)
	, StatsSchemaVersion(0)
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
	BaseStatsDataPrivate::ApplyRequiredNonZeroDefaults(BaseAttributes);
	StatsSchemaVersion = CurrentStatsSchemaVersion;

	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("PlayerLevel"),       1.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("XPGainMultiplier"),  1.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("XPPenalty"),         1.0f);

	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Strength"),     10.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Intelligence"), 10.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Dexterity"),    10.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Endurance"),    10.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Affliction"),    5.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Luck"),          5.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Covenant"),      5.0f);

	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Health"),       100.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Mana"),         100.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("Stamina"),      100.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("ArcaneShield"),   0.0f);

	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxHealth"),      100.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxMana"),        100.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxStamina"),     100.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxArcaneShield"),  0.0f);

	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("HealthRegenRate"),      1.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("HealthRegenAmount"),    5.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxHealthRegenRate"),   5.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxHealthRegenAmount"), 20.0f);

	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("FlatReservedHealth"),       0.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("PercentageReservedHealth"), 0.0f);

	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("StaminaRegenRate"),      1.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("StaminaRegenAmount"),    8.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxStaminaRegenRate"),   5.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxStaminaRegenAmount"), 25.0f);

	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("StaminaDegenRate"),   0.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("StaminaDegenAmount"), 0.0f);

	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("FlatReservedStamina"),       0.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("PercentageReservedStamina"), 0.0f);

	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("ManaRegenRate"),      1.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("ManaRegenAmount"),    1.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxManaRegenRate"),   5.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxManaRegenAmount"), 20.0f);

	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("FlatReservedMana"),       0.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("PercentageReservedMana"), 0.0f);

	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("ArcaneShieldRegenRate"),         1.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("ArcaneShieldRegenAmount"),       3.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxArcaneShieldRegenRate"),      5.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxArcaneShieldRegenAmount"),   15.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("FlatReservedArcaneShield"),      0.0f);
	BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("PercentageReservedArcaneShield"), 0.0f);

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

	if (StatsSchemaVersion < CurrentStatsSchemaVersion)
	{
		bool bMigratedDefaults = false;
		if (BaseStatsDataPrivate::HasZeroedLegacyCombatDefaults(BaseAttributes))
		{
			BaseStatsDataPrivate::ApplyRequiredNonZeroDefaults(BaseAttributes);
			bMigratedDefaults = true;
			UE_LOG(
				LogBaseStatsData,
				Warning,
				TEXT("Migrated legacy zero-valued neutral combat multipliers in %s. Save the asset to persist schema version %d."),
				*GetPathName(),
				CurrentStatsSchemaVersion);
		}

		if (StatsSchemaVersion < 2
			&& BaseStatsDataPrivate::MigrateSpellCritMultiplierToNeutral(BaseAttributes))
		{
			bMigratedDefaults = true;
			UE_LOG(
				LogBaseStatsData,
				Warning,
				TEXT("Migrated SpellsCritMultiplier to neutral 1.0 in %s. Save the asset to persist schema version %d."),
				*GetPathName(),
				CurrentStatsSchemaVersion);
		}

		if (StatsSchemaVersion < 3)
		{
			BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxLifeLeechRatePercent"), 20.0f);
			BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxManaLeechRatePercent"), 20.0f);
			BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("MaxStaminaLeechRatePercent"), 20.0f);
			BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("ArcaneShieldRechargeDelay"), 2.0f);
			BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("ArcaneShieldRechargeRate"), 20.0f);
			BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, TEXT("CorruptionShieldDamageMultiplier"), 2.0f);
			bMigratedDefaults = true;
			UE_LOG(
				LogBaseStatsData,
				Warning,
				TEXT("Added leech-rate and Arcane Shield defaults to %s. Save the asset to persist schema version %d."),
				*GetPathName(),
				CurrentStatsSchemaVersion);
		}

#if WITH_EDITOR
		if (bMigratedDefaults)
		{
			MarkPackageDirty();
		}
#endif

		StatsSchemaVersion = CurrentStatsSchemaVersion;
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

#if WITH_EDITOR

namespace BaseStatsDataEditorPrivate
{
	/**
	 * Matches FStatsInitializer's own modifier walk exactly - if the two ever
	 * disagree, the button would clear the wrong rows and the effect would stay
	 * blocked with no indication why.
	 */
	FName GetModifierAttributeName(const FGameplayModifierInfo& Modifier)
	{
		if (!Modifier.Attribute.IsValid())
		{
			return NAME_None;
		}

		if (const FProperty* AttributeProperty = Modifier.Attribute.GetUProperty())
		{
			return AttributeProperty->GetFName();
		}

		const FString AttributeName = Modifier.Attribute.GetName();
		return AttributeName.IsEmpty() ? NAME_None : FName(*AttributeName);
	}

	void GatherEffectAttributes(const TSubclassOf<UGameplayEffect>& EffectClass, TSet<FName>& OutNames)
	{
		if (!EffectClass)
		{
			return;
		}

		const UGameplayEffect* EffectCDO = EffectClass->GetDefaultObject<UGameplayEffect>();
		if (!EffectCDO)
		{
			return;
		}

		for (const FGameplayModifierInfo& Modifier : EffectCDO->Modifiers)
		{
			// Override only: an Additive modifier composes with the authored base
			// rather than replacing it, so that base must stay authored.
			if (Modifier.ModifierOp != EGameplayModOp::Override)
			{
				continue;
			}

			const FName AttributeName = GetModifierAttributeName(Modifier);
			if (AttributeName != NAME_None)
			{
				OutNames.Add(AttributeName);
			}
		}
	}
}

void UBaseStatsData::ClearOverridesDrivenByInitializationEffects()
{
	TSet<FName> DrivenAttributes;
	for (const TSubclassOf<UGameplayEffect>& EffectClass : InitializationEffects)
	{
		BaseStatsDataEditorPrivate::GatherEffectAttributes(EffectClass, DrivenAttributes);
	}

	if (DrivenAttributes.IsEmpty())
	{
		UE_LOG(LogBaseStatsData, Warning,
			TEXT("ClearOverridesDrivenByInitializationEffects: %s has no InitializationEffects with attribute "
			     "modifiers, so there is nothing a derived effect would drive."),
			*GetName());
		return;
	}

	Modify();

	TArray<FName> Cleared;
	for (FStatInitializationEntry& Entry : BaseAttributes)
	{
		if (Entry.bOverrideValue && DrivenAttributes.Contains(Entry.StatName))
		{
			Entry.bOverrideValue = false;
			Cleared.Add(Entry.StatName);
		}
	}

	Cleared.Sort(FNameLexicalLess());

	UE_LOG(LogBaseStatsData, Log,
		TEXT("ClearOverridesDrivenByInitializationEffects: %s - %d attribute(s) are effect-driven, cleared %d "
		     "override(s): %s. Save the asset to persist."),
		*GetName(), DrivenAttributes.Num(), Cleared.Num(),
		Cleared.IsEmpty()
			? TEXT("none")
			: *FString::JoinBy(Cleared, TEXT(", "), [](const FName& N) { return N.ToString(); }));

	if (!Cleared.IsEmpty())
	{
		MarkPackageDirty();
	// Without this the details panel keeps showing the pre-click list.
	PostEditChange();
	}
}

namespace BaseStatsDataEditorPrivate
{
	struct FBaselineStat
	{
		const TCHAR* StatName;
		float Value;
	};

	/**
	 * The rows that are genuinely authored data. Everything else is either
	 * derived by an effect, computed by the attribute set, or reflection noise.
	 */
	static constexpr FBaselineStat BaselineStats[] =
	{
		{TEXT("PlayerLevel"),       0.0f},
		{TEXT("XPGainMultiplier"),  1.0f},
		{TEXT("XPPenalty"),         1.0f},

		// Characters start with nothing invested. Every primary contributes
		// additively, so zero here means the vitals equal their authored base.
		{TEXT("Strength"),          0.0f},
		{TEXT("Intelligence"),      0.0f},
		{TEXT("Dexterity"),         0.0f},
		{TEXT("Endurance"),         0.0f},
		{TEXT("Affliction"),        0.0f},
		{TEXT("Luck"),              0.0f},
		{TEXT("Covenant"),          0.0f},

		{TEXT("MaxHealth"),       100.0f},
		{TEXT("MaxMana"),         100.0f},
		{TEXT("MaxStamina"),      100.0f},
		{TEXT("MaxArcaneShield"),   0.0f},

		// Rate * Amount: both halves have to be non-zero or the resource looks
		// broken even though every rate is set correctly.
		{TEXT("HealthRegenAmount"),        1.0f},
		{TEXT("ManaRegenAmount"),          1.0f},
		{TEXT("StaminaRegenAmount"),       5.0f},
		// Must exceed StaminaRegenAmount, or a sprinting character nets stamina.
		{TEXT("StaminaDegenAmount"),      20.0f},
		{TEXT("ArcaneShieldRegenAmount"),  0.0f},

		{TEXT("Health"),          100.0f},
		{TEXT("Mana"),            100.0f},
		{TEXT("Stamina"),         100.0f},
	};

	/**
	 * Always recomputed by UHunterAttributeSet::Update*DerivedAttributes, so an
	 * authored value here is overwritten before anyone sees it.
	 */
	static const TSet<FName>& GetComputedOutputs()
	{
		static const TSet<FName> Outputs = {
			TEXT("MaxEffectiveHealth"),
			TEXT("MaxEffectiveMana"),
			TEXT("MaxEffectiveStamina"),
			TEXT("MaxEffectiveArcaneShield"),
		};
		return Outputs;
	}
}

bool UBaseStatsData::GetBaselineValueForStat(const FName StatName, float& OutValue)
{
	for (const BaseStatsDataEditorPrivate::FBaselineStat& Baseline : BaseStatsDataEditorPrivate::BaselineStats)
	{
		if (StatName == FName(Baseline.StatName))
		{
			OutValue = Baseline.Value;
			return true;
		}
	}

	for (const BaseStatsDataPrivate::FRequiredStatDefault& Default : BaseStatsDataPrivate::RequiredNonZeroDefaults)
	{
		if (StatName == FName(Default.StatName))
		{
			OutValue = Default.Value;
			return true;
		}
	}

	return false;
}

void UBaseStatsData::ResetToBaseline()
{
	Modify();

	TSet<FName> EffectDriven;
	for (const TSubclassOf<UGameplayEffect>& EffectClass : InitializationEffects)
	{
		BaseStatsDataEditorPrivate::GatherEffectAttributes(EffectClass, EffectDriven);
	}

	int32 ClearedCount = 0;
	for (FStatInitializationEntry& Entry : BaseAttributes)
	{
		if (Entry.bOverrideValue)
		{
			Entry.bOverrideValue = false;
			++ClearedCount;
		}
	}

	// Neutral multipliers first: a zero here silently breaks combat maths, and
	// they are never driven by an effect.
	BaseStatsDataPrivate::ApplyRequiredNonZeroDefaults(BaseAttributes);

	TArray<FName> Skipped;
	int32 AuthoredCount = 0;
	for (const BaseStatsDataEditorPrivate::FBaselineStat& Baseline : BaseStatsDataEditorPrivate::BaselineStats)
	{
		const FName StatName(Baseline.StatName);

		if (EffectDriven.Contains(StatName)
			|| BaseStatsDataEditorPrivate::GetComputedOutputs().Contains(StatName))
		{
			Skipped.Add(StatName);
			continue;
		}

		BaseStatsDataPrivate::ApplyStarterOverride(BaseAttributes, StatName, Baseline.Value);
		++AuthoredCount;
	}

	// The multipliers above are counted by walking the result rather than
	// guessing, so the number reported matches what the panel will show.
	int32 FinalAuthored = 0;
	for (const FStatInitializationEntry& Entry : BaseAttributes)
	{
		FinalAuthored += Entry.bOverrideValue ? 1 : 0;
	}

	UE_LOG(LogBaseStatsData, Log,
		TEXT("ResetToBaseline: %s - cleared %d override(s), re-authored %d baseline row(s), "
		     "%d row(s) authored in total. Save the asset to persist."),
		*GetName(), ClearedCount, AuthoredCount, FinalAuthored);

	if (!Skipped.IsEmpty())
	{
		Skipped.Sort(FNameLexicalLess());
		UE_LOG(LogBaseStatsData, Log,
			TEXT("ResetToBaseline: left %d baseline row(s) unauthored because an initialization effect or the "
			     "attribute set already drives them: %s"),
			Skipped.Num(),
			*FString::JoinBy(Skipped, TEXT(", "), [](const FName& N) { return N.ToString(); }));
	}

	MarkPackageDirty();
	// Without this the details panel keeps showing the pre-click list.
	PostEditChange();
}

void UBaseStatsData::LogInitializationEffectConflicts()
{
	const TMap<FName, float> AuthoredStats = GetAllStatsAsMap();

	UE_LOG(LogBaseStatsData, Log,
		TEXT("LogInitializationEffectConflicts: %s has %d authored row(s) of %d total, and %d "
		     "InitializationEffect(s)."),
		*GetName(), AuthoredStats.Num(), BaseAttributes.Num(), InitializationEffects.Num());

	for (const TSubclassOf<UGameplayEffect>& EffectClass : InitializationEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		TSet<FName> DrivenAttributes;
		BaseStatsDataEditorPrivate::GatherEffectAttributes(EffectClass, DrivenAttributes);

		TArray<FName> Conflicts;
		for (const FName& Name : DrivenAttributes)
		{
			if (AuthoredStats.Contains(Name))
			{
				Conflicts.Add(Name);
			}
		}
		Conflicts.Sort(FNameLexicalLess());

		if (Conflicts.IsEmpty())
		{
			UE_LOG(LogBaseStatsData, Log, TEXT("  %s: will APPLY (no authored conflicts)."),
				*GetNameSafe(EffectClass));
		}
		else
		{
			UE_LOG(LogBaseStatsData, Warning,
				TEXT("  %s: will be SKIPPED - blocked by %d authored row(s): %s"),
				*GetNameSafe(EffectClass), Conflicts.Num(),
				*FString::JoinBy(Conflicts, TEXT(", "), [](const FName& N) { return N.ToString(); }));
		}
	}
}

#endif

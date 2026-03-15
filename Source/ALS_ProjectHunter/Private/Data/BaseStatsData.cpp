#include "Data/BaseStatsData.h"

#include "Data/StatDefinitions.h"
#include "UObject/UnrealType.h"

namespace BaseStatsDataPrivate
{
	struct FQuickSetupStatBinding
	{
		FName StatName;
		float UBaseStatsData::* ValueMember = nullptr;
	};

	const TArray<FQuickSetupStatBinding>& GetQuickSetupBindings()
	{
		static const TArray<FQuickSetupStatBinding> Bindings = {
			{TEXT("Strength"), &UBaseStatsData::Strength},
			{TEXT("Intelligence"), &UBaseStatsData::Intelligence},
			{TEXT("Dexterity"), &UBaseStatsData::Dexterity},
			{TEXT("Endurance"), &UBaseStatsData::Endurance},
			{TEXT("Affliction"), &UBaseStatsData::Affliction},
			{TEXT("Luck"), &UBaseStatsData::Luck},
			{TEXT("Covenant"), &UBaseStatsData::Covenant},
			{TEXT("MaxHealth"), &UBaseStatsData::MaxHealth},
			{TEXT("MaxMana"), &UBaseStatsData::MaxMana},
			{TEXT("MaxStamina"), &UBaseStatsData::MaxStamina},
			{TEXT("MaxArcaneShield"), &UBaseStatsData::MaxArcaneShield},
			{TEXT("MinPhysicalDamage"), &UBaseStatsData::MinPhysicalDamage},
			{TEXT("MaxPhysicalDamage"), &UBaseStatsData::MaxPhysicalDamage},
			{TEXT("CritChance"), &UBaseStatsData::CritChance},
			{TEXT("CritMultiplier"), &UBaseStatsData::CritMultiplier},
			{TEXT("AttackSpeed"), &UBaseStatsData::AttackSpeed},
			{TEXT("CastSpeed"), &UBaseStatsData::CastSpeed},
			{TEXT("Armour"), &UBaseStatsData::Armour},
			{TEXT("MovementSpeed"), &UBaseStatsData::MovementSpeed}
		};

		return Bindings;
	}

	const TMap<FName, FName>& GetLegacyAttributeNameOverrides()
	{
		// Future legacy enum-to-stat redirects belong here.
		static const TMap<FName, FName> Overrides = {
			{TEXT("CooldownReduction"), TEXT("Cooldown")}
		};

		return Overrides;
	}

	const TMap<FName, FName>& GetLegacyEnumNameOverrides()
	{
		// Future stat-to-legacy-enum redirects belong here.
		static const TMap<FName, FName> Overrides = {
			{TEXT("Cooldown"), TEXT("CooldownReduction")}
		};

		return Overrides;
	}

	FName ResolveLegacyAttributeName(EHunterAttribute Attribute)
	{
		const UEnum* AttributeEnum = StaticEnum<EHunterAttribute>();
		if (!AttributeEnum)
		{
			return NAME_None;
		}

		const FString EnumNameString = AttributeEnum->GetNameStringByValue(static_cast<int64>(Attribute));
		if (EnumNameString.IsEmpty())
		{
			return NAME_None;
		}

		FName ResolvedName(*EnumNameString);
		if (const FName* OverrideName = GetLegacyAttributeNameOverrides().Find(ResolvedName))
		{
			ResolvedName = *OverrideName;
		}

		return FHunterStatDefinitions::HasStatDefinition(ResolvedName) ? ResolvedName : NAME_None;
	}

	bool TryResolveLegacyEnum(FName StatName, EHunterAttribute& OutAttribute)
	{
		const UEnum* AttributeEnum = StaticEnum<EHunterAttribute>();
		if (!AttributeEnum)
		{
			return false;
		}

		FName LegacyEnumName = StatName;
		if (const FName* OverrideName = GetLegacyEnumNameOverrides().Find(StatName))
		{
			LegacyEnumName = *OverrideName;
		}

		const int64 EnumValue = AttributeEnum->GetValueByNameString(LegacyEnumName.ToString());
		if (EnumValue == INDEX_NONE)
		{
			return false;
		}

		OutAttribute = static_cast<EHunterAttribute>(EnumValue);
		return true;
	}

	void AddQuickSetupStatsToMap(const UBaseStatsData* StatsData, TMap<FName, float>& OutStats)
	{
		if (!StatsData)
		{
			return;
		}

		for (const FQuickSetupStatBinding& Binding : GetQuickSetupBindings())
		{
			if (!Binding.ValueMember || !FHunterStatDefinitions::HasStatDefinition(Binding.StatName))
			{
				continue;
			}

			const float Value = StatsData->*(Binding.ValueMember);
			if (!FMath::IsNearlyZero(Value))
			{
				OutStats.Add(Binding.StatName, Value);
			}
		}
	}

	bool TryGetQuickSetupStatValue(const UBaseStatsData* StatsData, FName StatName, float& OutValue)
	{
		if (!StatsData || !FHunterStatDefinitions::HasStatDefinition(StatName))
		{
			return false;
		}

		for (const FQuickSetupStatBinding& Binding : GetQuickSetupBindings())
		{
			if (Binding.ValueMember && Binding.StatName == StatName)
			{
				OutValue = StatsData->*(Binding.ValueMember);
				return !FMath::IsNearlyZero(OutValue);
			}
		}

		return false;
	}

	const FStatInitializationEntry* FindEntryByName(const TArray<FStatInitializationEntry>& Entries, FName StatName)
	{
		return Entries.FindByPredicate([&StatName](const FStatInitializationEntry& Entry)
		{
			return Entry.StatName == StatName;
		});
	}

	bool CompareEntries(const FStatInitializationEntry& Left, const FStatInitializationEntry& Right)
	{
		const int32 LeftSortIndex = FHunterStatDefinitions::GetSortIndex(Left.StatName);
		const int32 RightSortIndex = FHunterStatDefinitions::GetSortIndex(Right.StatName);
		if (LeftSortIndex != RightSortIndex)
		{
			return LeftSortIndex < RightSortIndex;
		}

		const int32 LeftCategoryPriority = FHunterStatDefinitions::GetCategorySortPriority(Left.Category);
		const int32 RightCategoryPriority = FHunterStatDefinitions::GetCategorySortPriority(Right.Category);
		if (LeftCategoryPriority != RightCategoryPriority)
		{
			return LeftCategoryPriority < RightCategoryPriority;
		}

		const int32 DisplayComparison = Left.DisplayName.ToString().Compare(Right.DisplayName.ToString(), ESearchCase::IgnoreCase);
		if (DisplayComparison != 0)
		{
			return DisplayComparison < 0;
		}

		return Left.StatName.ToString().Compare(Right.StatName.ToString(), ESearchCase::IgnoreCase) < 0;
	}
}

FName FHunterAttributeHelper::GetAttributeName(EHunterAttribute Attribute)
{
	return BaseStatsDataPrivate::ResolveLegacyAttributeName(Attribute);
}

FStatInitializationEntry::FStatInitializationEntry()
	: Attribute(EHunterAttribute::Strength)
	, StatName(NAME_None)
	, Category(NAME_None)
	, bOverrideValue(false)
	, BaseValue(0.0f)
{
}

FName FStatInitializationEntry::GetAttributeName() const
{
	return StatName != NAME_None ? StatName : FHunterAttributeHelper::GetAttributeName(Attribute);
}

bool FStatInitializationEntry::HasRuntimeValue() const
{
	return bOverrideValue || !FMath::IsNearlyZero(BaseValue);
}

void FStatInitializationEntry::ApplyDefinition(const FHunterStatDefinition& Definition)
{
	StatName = Definition.StatName;
	DisplayName = Definition.DisplayName;
	Category = Definition.Category;

	EHunterAttribute ResolvedAttribute = Attribute;
	if (BaseStatsDataPrivate::TryResolveLegacyEnum(Definition.StatName, ResolvedAttribute))
	{
		Attribute = ResolvedAttribute;
	}
}

UBaseStatsData::UBaseStatsData()
{
}

TMap<FName, float> UBaseStatsData::GetAllStatsAsMap() const
{
	TMap<FName, float> StatsMap;
	BaseStatsDataPrivate::AddQuickSetupStatsToMap(this, StatsMap);

	for (const FStatInitializationEntry& Entry : BaseAttributes)
	{
		if (Entry.StatName == NAME_None || !Entry.HasRuntimeValue() || !FHunterStatDefinitions::HasStatDefinition(Entry.StatName))
		{
			continue;
		}

		StatsMap.Add(Entry.StatName, Entry.BaseValue);
	}

	return StatsMap;
}

TMap<FName, float> UBaseStatsData::GetStatsByCategory(FName CategoryName) const
{
	TMap<FName, float> CategoryStats;
	const TMap<FName, float> AllStats = GetAllStatsAsMap();

	for (const TPair<FName, float>& StatPair : AllStats)
	{
		const FHunterStatDefinition* Definition = FHunterStatDefinitions::GetStatDefinition(StatPair.Key);
		if (Definition && Definition->Category == CategoryName)
		{
			CategoryStats.Add(StatPair.Key, StatPair.Value);
		}
	}

	return CategoryStats;
}

TArray<FName> UBaseStatsData::GetSupportedCategories() const
{
	return FHunterStatDefinitions::GetAllCategories();
}

TArray<FName> UBaseStatsData::GetSupportedStatNames() const
{
	return FHunterStatDefinitions::GetAllStatNames();
}

TArray<FStatInitializationEntry> UBaseStatsData::GetStatEntriesByCategory(FName CategoryName) const
{
	TArray<FStatInitializationEntry> MatchingEntries;

	for (const FStatInitializationEntry& Entry : BaseAttributes)
	{
		if (Entry.Category == CategoryName)
		{
			MatchingEntries.Add(Entry);
		}
	}

	MatchingEntries.Sort([](const FStatInitializationEntry& Left, const FStatInitializationEntry& Right)
	{
		return BaseStatsDataPrivate::CompareEntries(Left, Right);
	});

	return MatchingEntries;
}

bool UBaseStatsData::GetStatValue(FName AttributeName, float& OutValue) const
{
	if (const FStatInitializationEntry* Entry = BaseStatsDataPrivate::FindEntryByName(BaseAttributes, AttributeName))
	{
		if (Entry->HasRuntimeValue())
		{
			OutValue = Entry->BaseValue;
			return true;
		}
	}

	if (BaseStatsDataPrivate::TryGetQuickSetupStatValue(this, AttributeName, OutValue))
	{
		return true;
	}

	if (const FStatInitializationEntry* Entry = BaseStatsDataPrivate::FindEntryByName(BaseAttributes, AttributeName))
	{
		OutValue = Entry->BaseValue;
		return false;
	}

	OutValue = 0.0f;
	return false;
}

bool UBaseStatsData::HasAttribute(FName AttributeName) const
{
	float Value = 0.0f;
	return GetStatValue(AttributeName, Value);
}

bool UBaseStatsData::HasCategory(FName CategoryName) const
{
	return FHunterStatDefinitions::GetAllCategories().Contains(CategoryName);
}

void UBaseStatsData::SortStatsByCategoryThenName()
{
	BaseAttributes.Sort([](const FStatInitializationEntry& Left, const FStatInitializationEntry& Right)
	{
		return BaseStatsDataPrivate::CompareEntries(Left, Right);
	});
}

void UBaseStatsData::RefreshCategoriesFromDefinitions()
{
	const TArray<FHunterStatDefinition>& Definitions = FHunterStatDefinitions::GetAllStatDefinitions();
	if (Definitions.Num() == 0)
	{
		BaseAttributes.Reset();
		return;
	}

	const bool bTreatExistingEntriesAsOverrides = BaseAttributes.Num() > 0 && BaseAttributes.Num() < Definitions.Num();

	TMap<FName, FStatInitializationEntry> ExistingEntriesByName;
	ExistingEntriesByName.Reserve(BaseAttributes.Num());

	for (const FStatInitializationEntry& Entry : BaseAttributes)
	{
		FStatInitializationEntry NormalizedEntry = Entry;
		const FName EffectiveStatName = Entry.GetAttributeName();
		if (EffectiveStatName == NAME_None)
		{
			continue;
		}

		NormalizedEntry.StatName = EffectiveStatName;
		if (bTreatExistingEntriesAsOverrides)
		{
			NormalizedEntry.bOverrideValue = true;
		}

		ExistingEntriesByName.Add(EffectiveStatName, MoveTemp(NormalizedEntry));
	}

	BaseAttributes.Reset(Definitions.Num());
	BaseAttributes.Reserve(Definitions.Num());

	for (const FHunterStatDefinition& Definition : Definitions)
	{
		FStatInitializationEntry Entry;
		if (const FStatInitializationEntry* ExistingEntry = ExistingEntriesByName.Find(Definition.StatName))
		{
			Entry = *ExistingEntry;
		}

		Entry.ApplyDefinition(Definition);
		BaseAttributes.Add(MoveTemp(Entry));
	}
}

void UBaseStatsData::RefreshFromAttributeSetDefinition()
{
#if WITH_EDITOR
	Modify();
#endif
	RefreshCategoriesFromDefinitions();
	SortStatsByCategoryThenName();
}

void UBaseStatsData::SortStats()
{
#if WITH_EDITOR
	Modify();
#endif
	SortStatsByCategoryThenName();
}

void UBaseStatsData::ValidateStats()
{
#if WITH_EDITOR
	Modify();
#endif
	RefreshCategoriesFromDefinitions();
	SortStatsByCategoryThenName();
}

void UBaseStatsData::PostLoad()
{
	Super::PostLoad();
	RefreshCategoriesFromDefinitions();
	SortStatsByCategoryThenName();
}

#if WITH_EDITOR
void UBaseStatsData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	RefreshCategoriesFromDefinitions();
	SortStatsByCategoryThenName();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

#include "Data/StatDefinitions.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AttributeSet.h"
#include "UObject/FieldIterator.h"
#include "UObject/UnrealType.h"

namespace HunterStatDefinitionsPrivate
{
	const FName CategoryVitals(TEXT("Vitals"));
	const FName CategoryPrimary(TEXT("Primary"));
	const FName CategorySecondary(TEXT("Secondary"));
	const FName CategoryCombat(TEXT("Combat"));
	const FName CategoryDefense(TEXT("Defense"));
	const FName CategoryUtility(TEXT("Utility"));
	const FName CategoryLoot(TEXT("Loot"));
	const FName CategoryResistances(TEXT("Resistances"));
	const FName CategoryMovement(TEXT("Movement"));
	const FName CategoryResources(TEXT("Resources"));
	const FName CategoryRegeneration(TEXT("Regeneration"));
	const FName CategoryCustom(TEXT("Custom"));

	struct FCompiledHunterStatDefinition
	{
		FHunterStatDefinition Definition;
		FGameplayAttribute Attribute;
	};

	struct FHunterStatDefinitionCache
	{
		bool bInitialized = false;
		TArray<FCompiledHunterStatDefinition> CompiledDefinitions;
		TArray<FHunterStatDefinition> PublicDefinitions;
		TMap<FName, int32> DefinitionIndices;
		TMap<FName, FGameplayAttribute> AttributesByName;
		TArray<FName> Categories;
	};

	bool ContainsInsensitive(const FString& Source, const TCHAR* Token)
	{
		return Source.Contains(Token, ESearchCase::IgnoreCase);
	}

	const TMap<FName, FName>& GetExactCategoryOverrides()
	{
		// Future exact-name category overrides belong here.
		static const TMap<FName, FName> Overrides = {
			{TEXT("Health"), CategoryVitals},
			{TEXT("MaxHealth"), CategoryVitals},
			{TEXT("MaxEffectiveHealth"), CategoryVitals},
			{TEXT("ReservedHealth"), CategoryVitals},
			{TEXT("MaxReservedHealth"), CategoryVitals},
			{TEXT("FlatReservedHealth"), CategoryVitals},
			{TEXT("PercentageReservedHealth"), CategoryVitals},
			{TEXT("Mana"), CategoryResources},
			{TEXT("MaxMana"), CategoryResources},
			{TEXT("MaxEffectiveMana"), CategoryResources},
			{TEXT("ReservedMana"), CategoryResources},
			{TEXT("MaxReservedMana"), CategoryResources},
			{TEXT("FlatReservedMana"), CategoryResources},
			{TEXT("PercentageReservedMana"), CategoryResources},
			{TEXT("Stamina"), CategoryResources},
			{TEXT("MaxStamina"), CategoryResources},
			{TEXT("MaxEffectiveStamina"), CategoryResources},
			{TEXT("ReservedStamina"), CategoryResources},
			{TEXT("MaxReservedStamina"), CategoryResources},
			{TEXT("FlatReservedStamina"), CategoryResources},
			{TEXT("PercentageReservedStamina"), CategoryResources},
			{TEXT("ArcaneShield"), CategoryResources},
			{TEXT("MaxArcaneShield"), CategoryResources},
			{TEXT("MaxEffectiveArcaneShield"), CategoryResources},
			{TEXT("ReservedArcaneShield"), CategoryResources},
			{TEXT("MaxReservedArcaneShield"), CategoryResources},
			{TEXT("FlatReservedArcaneShield"), CategoryResources},
			{TEXT("PercentageReservedArcaneShield"), CategoryResources},
			{TEXT("MovementSpeed"), CategoryMovement},
			{TEXT("Gems"), CategoryLoot},
			{TEXT("GlobalXPGain"), CategoryLoot},
			{TEXT("LocalXPGain"), CategoryLoot},
			{TEXT("XPGainMultiplier"), CategoryLoot},
			{TEXT("XPPenalty"), CategoryLoot},
			{TEXT("GlobalDamages"), CategoryCombat},
			{TEXT("ElementalDamage"), CategoryCombat},
			{TEXT("DamageOverTime"), CategoryCombat},
			{TEXT("MeleeDamage"), CategoryCombat},
			{TEXT("SpellDamage"), CategoryCombat},
			{TEXT("RangedDamage"), CategoryCombat},
			{TEXT("DamageBonusWhileAtFullHP"), CategoryCombat},
			{TEXT("DamageBonusWhileAtLowHP"), CategoryCombat},
			{TEXT("GlobalDefenses"), CategoryDefense},
			{TEXT("Armour"), CategoryDefense},
			{TEXT("ArmourFlatBonus"), CategoryDefense},
			{TEXT("ArmourPercentBonus"), CategoryDefense},
			{TEXT("BlockStrength"), CategoryDefense},
			{TEXT("Poise"), CategoryDefense},
			{TEXT("PoiseResistance"), CategoryDefense},
			{TEXT("StunRecovery"), CategoryDefense},
			{TEXT("Cooldown"), CategoryUtility},
			{TEXT("AuraEffect"), CategoryUtility},
			{TEXT("AuraRadius"), CategoryUtility},
			{TEXT("LifeLeech"), CategoryUtility},
			{TEXT("ManaLeech"), CategoryUtility},
			{TEXT("LifeOnHit"), CategoryUtility},
			{TEXT("ManaOnHit"), CategoryUtility},
			{TEXT("StaminaOnHit"), CategoryUtility},
			{TEXT("ManaCostChanges"), CategoryUtility},
			{TEXT("HealthCostChanges"), CategoryUtility},
			{TEXT("StaminaCostChanges"), CategoryUtility},
			{TEXT("Weight"), CategoryUtility},
			{TEXT("ComboCounter"), CategoryUtility},
			{TEXT("CombatAlignment"), CategoryUtility},
			{TEXT("CombatStatus"), CategoryUtility}
		};

		return Overrides;
	}

	const TMap<FName, FString>& GetDisplayNameOverrides()
	{
		// Future exact display-name overrides belong here.
		static const TMap<FName, FString> Overrides = {
			{TEXT("GlobalDamages"), TEXT("Global Damages")},
			{TEXT("GlobalDefenses"), TEXT("Global Defenses")}
		};

		return Overrides;
	}

	const TMap<FName, int32>& GetExplicitSortOverrides()
	{
		// Future explicit per-stat sort overrides belong here.
		static const TMap<FName, int32> Overrides;
		return Overrides;
	}

	FHunterStatDefinitionCache& GetMutableCache()
	{
		static FHunterStatDefinitionCache Cache;
		return Cache;
	}

	bool CompareDefinitions(const FHunterStatDefinition& Left, const FHunterStatDefinition& Right)
	{
		const int32 LeftCategoryPriority = FHunterStatDefinitions::GetCategorySortPriority(Left.Category);
		const int32 RightCategoryPriority = FHunterStatDefinitions::GetCategorySortPriority(Right.Category);
		if (LeftCategoryPriority != RightCategoryPriority)
		{
			return LeftCategoryPriority < RightCategoryPriority;
		}

		if (Left.SortOrder != Right.SortOrder)
		{
			return Left.SortOrder < Right.SortOrder;
		}

		const FString LeftDisplay = Left.DisplayName.ToString();
		const FString RightDisplay = Right.DisplayName.ToString();
		const int32 DisplayComparison = LeftDisplay.Compare(RightDisplay, ESearchCase::IgnoreCase);
		if (DisplayComparison != 0)
		{
			return DisplayComparison < 0;
		}

		return Left.StatName.ToString().Compare(Right.StatName.ToString(), ESearchCase::IgnoreCase) < 0;
	}

	void RebuildCache()
	{
		FHunterStatDefinitionCache& Cache = GetMutableCache();
		if (Cache.bInitialized)
		{
			return;
		}

		Cache.CompiledDefinitions.Reset();
		Cache.PublicDefinitions.Reset();
		Cache.DefinitionIndices.Reset();
		Cache.AttributesByName.Reset();
		Cache.Categories.Reset();

		for (TFieldIterator<FProperty> PropertyIt(UHunterAttributeSet::StaticClass(), EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
		{
			const FProperty* Property = *PropertyIt;
			const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
			if (!StructProperty || StructProperty->Struct != FGameplayAttributeData::StaticStruct())
			{
				continue;
			}

			const FString MetadataDisplayName = Property->HasMetaData(TEXT("DisplayName"))
				? Property->GetMetaData(TEXT("DisplayName")).TrimStartAndEnd()
				: FString();
			const FString MetadataCategory = Property->HasMetaData(TEXT("Category"))
				? Property->GetMetaData(TEXT("Category")).TrimStartAndEnd()
				: FString();

			FHunterStatDefinition Definition;
			Definition.StatName = Property->GetFName();
			Definition.DisplayName = FHunterStatDefinitions::BuildDefaultDisplayName(Definition.StatName, MetadataDisplayName);
			Definition.Category = FHunterStatDefinitions::BuildDefaultCategoryForStat(Definition.StatName, MetadataCategory);
			Definition.SortOrder = FHunterStatDefinitions::GetStatSortOrder(Definition.StatName);
			Definition.DebugColor = FHunterStatDefinitions::BuildDefaultDebugColor(Definition.Category);

			FCompiledHunterStatDefinition CompiledDefinition;
			CompiledDefinition.Definition = MoveTemp(Definition);
			CompiledDefinition.Attribute = FGameplayAttribute(const_cast<FProperty*>(Property));
			Cache.CompiledDefinitions.Add(MoveTemp(CompiledDefinition));
		}

		Cache.CompiledDefinitions.Sort([](const FCompiledHunterStatDefinition& Left, const FCompiledHunterStatDefinition& Right)
		{
			return CompareDefinitions(Left.Definition, Right.Definition);
		});

		Cache.PublicDefinitions.Reserve(Cache.CompiledDefinitions.Num());
		Cache.DefinitionIndices.Reserve(Cache.CompiledDefinitions.Num());
		Cache.AttributesByName.Reserve(Cache.CompiledDefinitions.Num());

		for (int32 DefinitionIndex = 0; DefinitionIndex < Cache.CompiledDefinitions.Num(); ++DefinitionIndex)
		{
			const FCompiledHunterStatDefinition& CompiledDefinition = Cache.CompiledDefinitions[DefinitionIndex];
			Cache.PublicDefinitions.Add(CompiledDefinition.Definition);
			Cache.DefinitionIndices.Add(CompiledDefinition.Definition.StatName, DefinitionIndex);
			Cache.AttributesByName.Add(CompiledDefinition.Definition.StatName, CompiledDefinition.Attribute);

			if (!Cache.Categories.Contains(CompiledDefinition.Definition.Category))
			{
				Cache.Categories.Add(CompiledDefinition.Definition.Category);
			}
		}

		Cache.bInitialized = true;
	}
}

FHunterStatDefinition::FHunterStatDefinition()
	: StatName(NAME_None)
	, Category(NAME_None)
	, SortOrder(0)
	, DebugColor(FLinearColor::White)
{
}

const TArray<FHunterStatDefinition>& FHunterStatDefinitions::GetAllStatDefinitions()
{
	HunterStatDefinitionsPrivate::RebuildCache();
	return HunterStatDefinitionsPrivate::GetMutableCache().PublicDefinitions;
}

void FHunterStatDefinitions::GatherAttributeDefinitionsFromAttributeSet(TArray<FHunterStatDefinition>& OutDefinitions)
{
	OutDefinitions = GetAllStatDefinitions();
}

const FHunterStatDefinition* FHunterStatDefinitions::GetStatDefinition(FName StatName)
{
	HunterStatDefinitionsPrivate::RebuildCache();

	const HunterStatDefinitionsPrivate::FHunterStatDefinitionCache& Cache = HunterStatDefinitionsPrivate::GetMutableCache();
	if (const int32* DefinitionIndex = Cache.DefinitionIndices.Find(StatName))
	{
		return Cache.PublicDefinitions.IsValidIndex(*DefinitionIndex) ? &Cache.PublicDefinitions[*DefinitionIndex] : nullptr;
	}

	return nullptr;
}

bool FHunterStatDefinitions::HasStatDefinition(FName StatName)
{
	return GetStatDefinition(StatName) != nullptr;
}

TArray<FName> FHunterStatDefinitions::GetAllStatNames()
{
	const TArray<FHunterStatDefinition>& Definitions = GetAllStatDefinitions();

	TArray<FName> StatNames;
	StatNames.Reserve(Definitions.Num());

	for (const FHunterStatDefinition& Definition : Definitions)
	{
		StatNames.Add(Definition.StatName);
	}

	return StatNames;
}

TArray<FName> FHunterStatDefinitions::GetAllCategories()
{
	HunterStatDefinitionsPrivate::RebuildCache();
	return HunterStatDefinitionsPrivate::GetMutableCache().Categories;
}

bool FHunterStatDefinitions::TryGetGameplayAttribute(FName StatName, FGameplayAttribute& OutAttribute)
{
	HunterStatDefinitionsPrivate::RebuildCache();

	const HunterStatDefinitionsPrivate::FHunterStatDefinitionCache& Cache = HunterStatDefinitionsPrivate::GetMutableCache();
	if (const FGameplayAttribute* FoundAttribute = Cache.AttributesByName.Find(StatName))
	{
		OutAttribute = *FoundAttribute;
		return true;
	}

	OutAttribute = FGameplayAttribute();
	return false;
}

FName FHunterStatDefinitions::BuildDefaultCategoryForStat(FName StatName, const FString& SourceCategory)
{
	const TMap<FName, FName>& ExactOverrides = HunterStatDefinitionsPrivate::GetExactCategoryOverrides();
	if (const FName* ExactCategory = ExactOverrides.Find(StatName))
	{
		return *ExactCategory;
	}

	const FString StatNameString = StatName.ToString();

	if (HunterStatDefinitionsPrivate::ContainsInsensitive(SourceCategory, TEXT("Primary Attribute")))
	{
		return HunterStatDefinitionsPrivate::CategoryPrimary;
	}

	if (HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Regen")) || HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Degen")))
	{
		return HunterStatDefinitionsPrivate::CategoryRegeneration;
	}

	if (HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Resistance")) || HunterStatDefinitionsPrivate::ContainsInsensitive(SourceCategory, TEXT("Resistance")))
	{
		return HunterStatDefinitionsPrivate::CategoryResistances;
	}

	if (HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Movement")) || HunterStatDefinitionsPrivate::ContainsInsensitive(SourceCategory, TEXT("Movement")))
	{
		return HunterStatDefinitionsPrivate::CategoryMovement;
	}

	if (HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Find")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("XP")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Experience")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(SourceCategory, TEXT("Experience")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Gems")))
	{
		return HunterStatDefinitionsPrivate::CategoryLoot;
	}

	if (HunterStatDefinitionsPrivate::ContainsInsensitive(SourceCategory, TEXT("Vital Attribute|Health")) || HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Health")))
	{
		return HunterStatDefinitionsPrivate::CategoryVitals;
	}

	if (HunterStatDefinitionsPrivate::ContainsInsensitive(SourceCategory, TEXT("Vital Attribute|Mana")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(SourceCategory, TEXT("Vital Attribute|Stamina")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(SourceCategory, TEXT("Vital Attribute|Arcane Shield")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Mana")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Stamina")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("ArcaneShield")))
	{
		return HunterStatDefinitionsPrivate::CategoryResources;
	}

	if (HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Armour")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Defense")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Block")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Reflect")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Poise")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("StunRecovery")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(SourceCategory, TEXT("Defensive")))
	{
		return HunterStatDefinitionsPrivate::CategoryDefense;
	}

	if (HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Damage")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Crit")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Attack")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Cast")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Projectile")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Spell")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Area")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Piercing")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("ChanceTo")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Duration")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Chain")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Fork")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(SourceCategory, TEXT("Damage")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(SourceCategory, TEXT("Offensive")))
	{
		return HunterStatDefinitionsPrivate::CategoryCombat;
	}

	if (HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Cooldown")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Cost")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Leech")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("OnHit")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Aura")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Weight")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(StatNameString, TEXT("Combo")) ||
		HunterStatDefinitionsPrivate::ContainsInsensitive(SourceCategory, TEXT("Vital Attribute|Misc")))
	{
		return HunterStatDefinitionsPrivate::CategoryUtility;
	}

	if (HunterStatDefinitionsPrivate::ContainsInsensitive(SourceCategory, TEXT("Secondary Attribute")))
	{
		return HunterStatDefinitionsPrivate::CategorySecondary;
	}

	return HunterStatDefinitionsPrivate::CategoryCustom;
}

FText FHunterStatDefinitions::BuildDefaultDisplayName(FName StatName, const FString& ExplicitDisplayName)
{
	const TMap<FName, FString>& ExactOverrides = HunterStatDefinitionsPrivate::GetDisplayNameOverrides();
	if (const FString* ExplicitOverride = ExactOverrides.Find(StatName))
	{
		return FText::FromString(*ExplicitOverride);
	}

	FString DisplayString = ExplicitDisplayName.TrimStartAndEnd();
	const FString StatNameString = StatName.ToString();

	if (DisplayString.IsEmpty() || DisplayString.Equals(StatNameString, ESearchCase::IgnoreCase))
	{
		DisplayString = FName::NameToDisplayString(StatNameString, false);
	}

	return FText::FromString(DisplayString);
}

FLinearColor FHunterStatDefinitions::BuildDefaultDebugColor(FName Category)
{
	// Future per-category or per-stat debug color overrides belong here.
	if (Category == HunterStatDefinitionsPrivate::CategoryVitals)
	{
		return FLinearColor(0.25f, 0.86f, 0.50f, 1.0f);
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryResources)
	{
		return FLinearColor(0.30f, 0.55f, 1.0f, 1.0f);
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryRegeneration)
	{
		return FLinearColor(0.35f, 0.95f, 0.80f, 1.0f);
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryPrimary)
	{
		return FLinearColor(1.0f, 0.87f, 0.35f, 1.0f);
	}

	if (Category == HunterStatDefinitionsPrivate::CategorySecondary)
	{
		return FLinearColor(0.58f, 0.78f, 1.0f, 1.0f);
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryCombat)
	{
		return FLinearColor(1.0f, 0.45f, 0.25f, 1.0f);
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryDefense)
	{
		return FLinearColor(0.40f, 0.82f, 1.0f, 1.0f);
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryResistances)
	{
		return FLinearColor(0.45f, 0.95f, 0.95f, 1.0f);
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryMovement)
	{
		return FLinearColor(0.90f, 0.90f, 0.55f, 1.0f);
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryUtility)
	{
		return FLinearColor(0.78f, 0.78f, 0.78f, 1.0f);
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryLoot)
	{
		return FLinearColor(1.0f, 0.82f, 0.30f, 1.0f);
	}

	return FLinearColor::White;
}

int32 FHunterStatDefinitions::GetCategorySortPriority(FName Category)
{
	if (Category == HunterStatDefinitionsPrivate::CategoryVitals)
	{
		return 0;
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryResources)
	{
		return 1;
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryRegeneration)
	{
		return 2;
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryPrimary)
	{
		return 3;
	}

	if (Category == HunterStatDefinitionsPrivate::CategorySecondary)
	{
		return 4;
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryCombat)
	{
		return 5;
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryDefense)
	{
		return 6;
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryResistances)
	{
		return 7;
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryMovement)
	{
		return 8;
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryUtility)
	{
		return 9;
	}

	if (Category == HunterStatDefinitionsPrivate::CategoryLoot)
	{
		return 10;
	}

	return 11;
}

int32 FHunterStatDefinitions::GetStatSortOrder(FName StatName)
{
	const TMap<FName, int32>& Overrides = HunterStatDefinitionsPrivate::GetExplicitSortOverrides();
	if (const int32* ExplicitOrder = Overrides.Find(StatName))
	{
		return *ExplicitOrder;
	}

	return 0;
}

int32 FHunterStatDefinitions::GetSortIndex(FName StatName)
{
	HunterStatDefinitionsPrivate::RebuildCache();

	const HunterStatDefinitionsPrivate::FHunterStatDefinitionCache& Cache = HunterStatDefinitionsPrivate::GetMutableCache();
	if (const int32* DefinitionIndex = Cache.DefinitionIndices.Find(StatName))
	{
		return *DefinitionIndex;
	}

	return MAX_int32;
}

void FHunterStatDefinitions::SortDefinitions(TArray<FHunterStatDefinition>& InOutDefinitions)
{
	InOutDefinitions.Sort([](const FHunterStatDefinition& Left, const FHunterStatDefinition& Right)
	{
		return HunterStatDefinitionsPrivate::CompareDefinitions(Left, Right);
	});
}

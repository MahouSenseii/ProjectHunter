#include "Character/Component/StatsDebug.h"

#include "Character/Component/StatsManager.h"
#include "Data/BaseStatsData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"

#include <cfloat>

DEFINE_LOG_CATEGORY(LogStatsDebugManager);

namespace StatsDebugPrivate
{
	enum class EStatDebugBucket : uint8
	{
		Vital,
		Resources,
		Regeneration,
		Primary,
		Secondary,
		Combat,
		Defense,
		Resistances,
		Movement,
		Utility,
		Loot,
		Special,
		Custom
	};

	const FColor CategoryHeaderColor(155, 155, 165);
	const FColor MissingAssetColor(150, 150, 150);
	const FColor OverrideDisabledColor(125, 125, 125);
	const FColor UnresolvedColor(255, 170, 72);
	const FColor MissingLiveAttributeColor(255, 220, 120);
	const FColor DivergentValueColor(255, 235, 135);

	FString BuildSearchText(const FStatDebugEntry& Entry)
	{
		return FString::Printf(
			TEXT("%s %s %s %s %s"),
			*Entry.StatName.ToString(),
			*Entry.DisplayName.ToString(),
			*Entry.Category.ToString(),
			*Entry.Tooltip.ToString(),
			*Entry.StatType.ToString());
	}

	FColor DimColor(const FColor& InColor, float Factor = 0.65f)
	{
		const FLinearColor LinearColor = FLinearColor(InColor) * Factor;
		return LinearColor.ToFColor(true);
	}

	bool ContainsToken(const FString& Source, const TCHAR* Token)
	{
		return Source.Contains(Token, ESearchCase::IgnoreCase);
	}

	bool IsRegenLikeStat(FName StatName)
	{
		const FString StatNameString = StatName.ToString();
		return ContainsToken(StatNameString, TEXT("Regen")) || ContainsToken(StatNameString, TEXT("Degen"));
	}

	const FStatInitializationEntry* FindDefinition(const TMap<FName, const FStatInitializationEntry*>& DefinitionsByName, FName StatName)
	{
		const FStatInitializationEntry* const* FoundDefinition = DefinitionsByName.Find(StatName);
		return FoundDefinition ? *FoundDefinition : nullptr;
	}

	const FStatInitializationEntry* FindAuthoredEntry(const UBaseStatsData* StatsData, FName StatName)
	{
		if (!StatsData)
		{
			return nullptr;
		}

		return StatsData->GetBaseAttributes().FindByPredicate([StatName](const FStatInitializationEntry& Entry)
		{
			return Entry.StatName == StatName;
		});
	}

	FString BuildAuthoredText(const UBaseStatsData* StatsData, const FStatInitializationEntry* AuthoredEntry)
	{
		if (!StatsData)
		{
			return TEXT("Authored N/A (no asset)");
		}

		if (!AuthoredEntry)
		{
			return TEXT("Authored Missing");
		}

		if (!AuthoredEntry->bOverrideValue)
		{
			return TEXT("Override Off");
		}

		return FString::Printf(TEXT("Authored %.2f"), AuthoredEntry->BaseValue);
	}
	FString BuildMainCategoryHeader(FName MainCategory)
	{
		return FString::Printf(TEXT("== %s =="), *MainCategory.ToString());
	}

	FString BuildSubCategoryHeader(FName SubCategory)
	{
		return FString::Printf(TEXT("-- %s --"), *SubCategory.ToString());
	}

	FString BuildLineLabel(const FStatInitializationEntry* Definition, const FStatDebugEntry& Entry)
	{
		const FText DisplayName = (Definition && !Definition->DisplayName.IsEmpty()) ? Definition->DisplayName : Entry.DisplayName;
		return DisplayName.IsEmpty()
			? FName::NameToDisplayString(Entry.StatName.ToString(), false)
			: DisplayName.ToString();
	}

	void WriteLinesToLog(const UStatsManager* StatsManager, const TArray<FString>& Lines)
	{
		const FString OwnerName = (StatsManager && StatsManager->GetOwner()) ? StatsManager->GetOwner()->GetName() : TEXT("UnknownOwner");

		for (const FString& Line : Lines)
		{
			UE_LOG(LogStatsDebugManager, Log, TEXT("[%s] %s"), *OwnerName, *Line);
		}
	}

	EStatDebugBucket ClassifyBucket(const FParsedStatCategory& ParsedCategory, FName StatName)
	{
		if (IsRegenLikeStat(StatName))
		{
			return EStatDebugBucket::Regeneration;
		}

		const FString MainCategory = ParsedCategory.MainCategory.ToString();
		const FString SubCategory = ParsedCategory.SubCategory.ToString();

		if (ContainsToken(MainCategory, TEXT("Primary")))
		{
			return EStatDebugBucket::Primary;
		}

		if (MainCategory.Equals(TEXT("Vital"), ESearchCase::IgnoreCase) ||
			MainCategory.Equals(TEXT("Vitals"), ESearchCase::IgnoreCase))
		{
			if (ContainsToken(SubCategory, TEXT("Misc")) || ContainsToken(SubCategory, TEXT("Utility")))
			{
				return EStatDebugBucket::Utility;
			}

			return EStatDebugBucket::Vital;
		}

		if (ContainsToken(MainCategory, TEXT("Offense")) || ContainsToken(MainCategory, TEXT("Combat")))
		{
			return EStatDebugBucket::Combat;
		}

		if (MainCategory.Equals(TEXT("Defense"), ESearchCase::IgnoreCase) ||
			MainCategory.Equals(TEXT("Defence"), ESearchCase::IgnoreCase))
		{
			return EStatDebugBucket::Defense;
		}

		if (ContainsToken(MainCategory, TEXT("Secondary")))
		{
			if (ContainsToken(SubCategory, TEXT("Resist")))
			{
				return EStatDebugBucket::Resistances;
			}

			if (ContainsToken(SubCategory, TEXT("Reflect")))
			{
				return EStatDebugBucket::Defense;
			}

			if (ContainsToken(SubCategory, TEXT("Damage")) ||
				ContainsToken(SubCategory, TEXT("Offensive")) ||
				ContainsToken(SubCategory, TEXT("Conversion")) ||
				ContainsToken(SubCategory, TEXT("Ailment")) ||
				ContainsToken(SubCategory, TEXT("Duration")) ||
				ContainsToken(SubCategory, TEXT("Piercing")))
			{
				return EStatDebugBucket::Combat;
			}

			return EStatDebugBucket::Secondary;
		}

		if (ContainsToken(MainCategory, TEXT("Movement")))
		{
			return EStatDebugBucket::Movement;
		}

		if (ContainsToken(MainCategory, TEXT("Utility")))
		{
			return EStatDebugBucket::Utility;
		}

		if (ContainsToken(MainCategory, TEXT("Loot")))
		{
			return EStatDebugBucket::Loot;
		}

		if (ContainsToken(MainCategory, TEXT("Special")))
		{
			return EStatDebugBucket::Special;
		}

		return EStatDebugBucket::Custom;
	}
}

FStatDebugEntry::FStatDebugEntry()
	: StatName(NAME_None)
	, Category(NAME_None)
	, SortOrder(0)
	, IconName(NAME_None)
	, StatType(NAME_None)
	, bEnabled(true)
	, DisplayColor(FColor::White)
{
}

FStatsDebugManager::FStatsDebugManager()
	: bEnableDebug(false)
	, bDrawToScreen(true)
	, bLogToOutput(false)
	, DebugRefreshRate(0.25f)
	, BaseMessageKey(50000)
	, bShowVitals(true)
	, bShowResources(true)
	, bShowRegeneration(true)
	, bShowPrimary(true)
	, bShowSecondary(true)
	, bShowCombat(true)
	, bShowDefense(true)
	, bShowResistances(true)
	, bShowMovement(true)
	, bShowUtility(true)
	, bShowLoot(true)
	, bShowSpecial(true)
	, bShowCustom(true)
	, bEntriesSynchronized(false)
	, LastScreenUpdateTimeSeconds(-DBL_MAX)
	, LastLogUpdateTimeSeconds(-DBL_MAX)
	, LastDrawnLineCount(0)
{
}

void FStatsDebugManager::InitializeDefaults()
{
	bEnableDebug = false;
	bDrawToScreen = true;
	bLogToOutput = false;
	DebugRefreshRate = 0.25f;
	BaseMessageKey = 50000;
	FilterString.Reset();
	bShowVitals = true;
	bShowResources = true;
	bShowRegeneration = true;
	bShowPrimary = true;
	bShowSecondary = true;
	bShowCombat = true;
	bShowDefense = true;
	bShowResistances = true;
	bShowMovement = true;
	bShowUtility = true;
	bShowLoot = true;
	bShowSpecial = true;
	bShowCustom = true;
	StatEntries.Reset();
	bEntriesSynchronized = false;
	LastScreenUpdateTimeSeconds = -DBL_MAX;
	LastLogUpdateTimeSeconds = -DBL_MAX;
	LastDrawnLineCount = 0;
	CachedDisplayLines.Reset();
	CachedLineColors.Reset();
}

void FStatsDebugManager::RegisterStat(const FStatDebugEntry& Entry)
{
	if (FStatDebugEntry* ExistingEntry = StatEntries.FindByPredicate([&Entry](const FStatDebugEntry& Existing)
	{
		return Existing.StatName == Entry.StatName;
	}))
	{
		ExistingEntry->DisplayName = Entry.DisplayName;
		ExistingEntry->Category = Entry.Category;
		ExistingEntry->SortOrder = Entry.SortOrder;
		ExistingEntry->Tooltip = Entry.Tooltip;
		ExistingEntry->IconName = Entry.IconName;
		ExistingEntry->StatType = Entry.StatType;
		ExistingEntry->DisplayColor = Entry.DisplayColor;
		ExistingEntry->bEnabled = Entry.bEnabled;
		return;
	}

	StatEntries.Add(Entry);
}

void FStatsDebugManager::RegisterStats(UStatsManager* StatsManager)
{
	if (!StatsManager)
	{
		return;
	}

	TArray<FStatInitializationEntry> Definitions;
	StatsManager->GatherStatDefinitions(Definitions);
	if (Definitions.Num() == 0)
	{
		bEntriesSynchronized = false;
		return;
	}

	bool bNeedsSync = !bEntriesSynchronized || StatEntries.Num() != Definitions.Num();
	if (!bNeedsSync)
	{
		TMap<FName, const FStatInitializationEntry*> DefinitionsByName;
		DefinitionsByName.Reserve(Definitions.Num());
		for (const FStatInitializationEntry& Definition : Definitions)
		{
			DefinitionsByName.Add(Definition.StatName, &Definition);
		}

		for (const FStatDebugEntry& ExistingEntry : StatEntries)
		{
			const FStatInitializationEntry* Definition = DefinitionsByName.FindRef(ExistingEntry.StatName);
			if (!Definition)
			{
				bNeedsSync = true;
				break;
			}

			const FName ExpectedStatType(*StaticEnum<EHunterStatType>()->GetNameStringByValue(static_cast<int64>(Definition->StatType)));

			if (ExistingEntry.Category != Definition->Category ||
				ExistingEntry.SortOrder != Definition->SortOrder ||
				ExistingEntry.DisplayName.ToString() != Definition->DisplayName.ToString() ||
				ExistingEntry.Tooltip.ToString() != Definition->Tooltip.ToString() ||
				ExistingEntry.IconName != Definition->IconName ||
				ExistingEntry.StatType != ExpectedStatType)
			{
				bNeedsSync = true;
				break;
			}
		}
	}

	if (!bNeedsSync)
	{
		return;
	}

	TMap<FName, FStatDebugEntry> ExistingEntries;
	ExistingEntries.Reserve(StatEntries.Num());
	for (const FStatDebugEntry& Entry : StatEntries)
	{
		ExistingEntries.Add(Entry.StatName, Entry);
	}

	StatEntries.Reset(Definitions.Num());
	StatEntries.Reserve(Definitions.Num());

	for (const FStatInitializationEntry& Definition : Definitions)
	{
		FStatDebugEntry NewEntry;
		NewEntry.StatName = Definition.StatName;
		NewEntry.DisplayName = Definition.DisplayName;
		NewEntry.Category = Definition.Category;
		NewEntry.SortOrder = Definition.SortOrder;
		NewEntry.Tooltip = Definition.Tooltip;
		NewEntry.IconName = Definition.IconName;
		NewEntry.StatType = FName(*StaticEnum<EHunterStatType>()->GetNameStringByValue(static_cast<int64>(Definition.StatType)));
		NewEntry.bEnabled = true;
		NewEntry.DisplayColor = UBaseStatsData::GetStatTypeColor(Definition.StatType).ToFColor(true);

		if (const FStatDebugEntry* ExistingEntry = ExistingEntries.Find(Definition.StatName))
		{
			NewEntry.bEnabled = ExistingEntry->bEnabled;
			NewEntry.DisplayColor = ExistingEntry->DisplayColor;
		}

		StatEntries.Add(MoveTemp(NewEntry));
	}

	bEntriesSynchronized = true;
}

bool FStatsDebugManager::IsStatEnabled(const FStatDebugEntry& Entry) const
{
	if (!Entry.bEnabled)
	{
		return false;
	}

	if (StatsDebugPrivate::IsRegenLikeStat(Entry.StatName))
	{
		if (!bShowRegeneration)
		{
			return false;
		}
	}
	else if (!IsCategoryEnabled(Entry.Category))
	{
		return false;
	}

	const FString ActiveFilter = FilterString.TrimStartAndEnd();
	return ActiveFilter.IsEmpty() || StatsDebugPrivate::BuildSearchText(Entry).Contains(ActiveFilter, ESearchCase::IgnoreCase);
}

bool FStatsDebugManager::IsCategoryEnabled(FName Category) const
{
	switch (StatsDebugPrivate::ClassifyBucket(UBaseStatsData::ParseCategoryPath(Category), NAME_None))
	{
	case StatsDebugPrivate::EStatDebugBucket::Vital:
		return bShowVitals;
	case StatsDebugPrivate::EStatDebugBucket::Resources:
		return bShowResources;
	case StatsDebugPrivate::EStatDebugBucket::Regeneration:
		return bShowRegeneration;
	case StatsDebugPrivate::EStatDebugBucket::Primary:
		return bShowPrimary;
	case StatsDebugPrivate::EStatDebugBucket::Secondary:
		return bShowSecondary;
	case StatsDebugPrivate::EStatDebugBucket::Combat:
		return bShowCombat;
	case StatsDebugPrivate::EStatDebugBucket::Defense:
		return bShowDefense;
	case StatsDebugPrivate::EStatDebugBucket::Resistances:
		return bShowResistances;
	case StatsDebugPrivate::EStatDebugBucket::Movement:
		return bShowMovement;
	case StatsDebugPrivate::EStatDebugBucket::Utility:
		return bShowUtility;
	case StatsDebugPrivate::EStatDebugBucket::Loot:
		return bShowLoot;
	case StatsDebugPrivate::EStatDebugBucket::Special:
		return bShowSpecial;
	default:
		return bShowCustom;
	}
}

void FStatsDebugManager::BuildDisplayLines(UStatsManager* StatsManager, TArray<FString>& OutLines, TArray<FColor>& OutColors)
{
	OutLines.Reset();
	OutColors.Reset();

	if (!StatsManager)
	{
		return;
	}

	RegisterStats(StatsManager);

	TArray<FStatInitializationEntry> Definitions;
	StatsManager->GatherStatDefinitions(Definitions);

	TMap<FName, const FStatInitializationEntry*> DefinitionsByName;
	DefinitionsByName.Reserve(Definitions.Num());
	for (const FStatInitializationEntry& Definition : Definitions)
	{
		DefinitionsByName.Add(Definition.StatName, &Definition);
	}

	const UBaseStatsData* StatsData = StatsManager->GetStatsDataAsset();

	OutLines.Reserve(StatEntries.Num() * 2);
	OutColors.Reserve(StatEntries.Num() * 2);

	FName ActiveMainCategory = NAME_None;
	FName ActiveSubCategory = NAME_None;

	for (const FStatDebugEntry& Entry : StatEntries)
	{
		if (!IsStatEnabled(Entry))
		{
			continue;
		}

		const FStatInitializationEntry* Definition = StatsDebugPrivate::FindDefinition(DefinitionsByName, Entry.StatName);
		const FParsedStatCategory ParsedCategory = UBaseStatsData::ParseCategoryPath(Definition ? Definition->Category : Entry.Category);
		const StatsDebugPrivate::EStatDebugBucket Bucket = StatsDebugPrivate::ClassifyBucket(ParsedCategory, Entry.StatName);

		if (Bucket == StatsDebugPrivate::EStatDebugBucket::Custom && !WarnedCustomBucketStats.Contains(Entry.StatName))
		{
			WarnedCustomBucketStats.Add(Entry.StatName);
			UE_LOG(
				LogStatsDebugManager,
				Warning,
				TEXT("StatsDebug: Stat '%s' fell into the Custom bucket for category '%s'. Enable Custom to view it or update bucket classification."),
				*Entry.StatName.ToString(),
				*(Definition ? Definition->Category.ToString() : Entry.Category.ToString()));
		}

		// Debug output groups on the parsed parent/subcategory path so "Vital|Health"
		// shows as nested headers instead of collapsing into a single flattened bucket.
		if (ParsedCategory.MainCategory != ActiveMainCategory)
		{
			OutLines.Add(StatsDebugPrivate::BuildMainCategoryHeader(ParsedCategory.MainCategory));
			OutColors.Add(StatsDebugPrivate::CategoryHeaderColor);
			ActiveMainCategory = ParsedCategory.MainCategory;
			ActiveSubCategory = NAME_None;
		}

		if (ParsedCategory.SubCategory != NAME_None && ParsedCategory.SubCategory != ActiveSubCategory)
		{
			OutLines.Add(StatsDebugPrivate::BuildSubCategoryHeader(ParsedCategory.SubCategory));
			OutColors.Add(StatsDebugPrivate::CategoryHeaderColor);
			ActiveSubCategory = ParsedCategory.SubCategory;
		}
		else if (ParsedCategory.SubCategory == NAME_None)
		{
			ActiveSubCategory = NAME_None;
		}

		const FStatInitializationEntry* AuthoredEntry = StatsDebugPrivate::FindAuthoredEntry(StatsData, Entry.StatName);

		FGameplayAttribute Attribute;
		FStatInitializationEntry ResolvedDefinition;
		const bool bResolvedAttribute = StatsManager->ResolveAttributeByName(Entry.StatName, Attribute, &ResolvedDefinition) && Attribute.IsValid();
		const bool bHasLiveAttribute = bResolvedAttribute && StatsManager->HasLiveAttribute(Attribute);
		const float LiveValue = bHasLiveAttribute ? StatsManager->GetAttributeValue(Attribute) : 0.0f;
		const FString AuthoredText = StatsDebugPrivate::BuildAuthoredText(StatsData, AuthoredEntry);
		const FString Label = StatsDebugPrivate::BuildLineLabel(Definition ? Definition : &ResolvedDefinition, Entry);
		

		FString LiveText;
		FString DeltaText;
		FColor RowColor = Entry.DisplayColor;

		if (!bResolvedAttribute)
		{
			LiveText = FString::Printf(TEXT("UNRESOLVED in %s"), *GetNameSafe(StatsManager->GetSourceAttributeSetClass()));
			RowColor = StatsDebugPrivate::UnresolvedColor;
		}
		else if (!bHasLiveAttribute)
		{
			LiveText = TEXT("Resolved, no live AttributeSet instance");
			RowColor = StatsDebugPrivate::MissingLiveAttributeColor;
		}
		else
		{
			LiveText = FString::Printf(TEXT("Live %.2f"), LiveValue);

			if (AuthoredEntry && AuthoredEntry->bOverrideValue)
			{
				DeltaText = FString::Printf(TEXT(" | Delta %+0.2f"), LiveValue - AuthoredEntry->BaseValue);
				if (!FMath::IsNearlyEqual(LiveValue, AuthoredEntry->BaseValue, 0.01f))
				{
					RowColor = StatsDebugPrivate::DivergentValueColor;
				}
			}
			else if (AuthoredEntry && !AuthoredEntry->bOverrideValue)
			{
				RowColor = StatsDebugPrivate::DimColor(RowColor);
			}
			else if (!StatsData || !AuthoredEntry)
			{
				RowColor = StatsDebugPrivate::MissingAssetColor;
			}
		}

		if (AuthoredEntry && !AuthoredEntry->bOverrideValue)
		{
			RowColor = StatsDebugPrivate::OverrideDisabledColor;
		}

		OutLines.Add(FString::Printf(TEXT("%s | %s | %s%s"), *Label, *LiveText, *AuthoredText, *DeltaText));
		OutColors.Add(RowColor);
	}

	if (OutLines.Num() == 0)
	{
		const FString ActiveFilter = FilterString.TrimStartAndEnd();
		if (ActiveFilter.IsEmpty())
		{
			OutLines.Add(TEXT("No reflected stats matched the active debug categories."));
		}
		else
		{
			OutLines.Add(FString::Printf(TEXT("No reflected stats matched filter '%s'."), *ActiveFilter));
		}

		OutColors.Add(FColor(180, 180, 180));
	}
}

void FStatsDebugManager::DrawDebug(UStatsManager* StatsManager, UObject* WorldContext)
{
	if (!bEnableDebug)
	{
		ClearDrawnMessages();
		LastScreenUpdateTimeSeconds = -DBL_MAX;
		LastLogUpdateTimeSeconds = -DBL_MAX;
		return;
	}

	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const double CurrentTimeSeconds = World ? static_cast<double>(World->GetTimeSeconds()) : FPlatformTime::Seconds();
	const bool bShouldDrawNow = bDrawToScreen && GEngine && ShouldRefresh(CurrentTimeSeconds, LastScreenUpdateTimeSeconds);
	const bool bShouldLogNow = bLogToOutput && ShouldRefresh(CurrentTimeSeconds, LastLogUpdateTimeSeconds);

	if (!bShouldDrawNow && !bShouldLogNow)
	{
		if (!bDrawToScreen)
		{
			ClearDrawnMessages();
			LastScreenUpdateTimeSeconds = -DBL_MAX;
		}

		return;
	}

	BuildDisplayLines(StatsManager, CachedDisplayLines, CachedLineColors);

	if (bShouldDrawNow && GEngine)
	{
		const float MessageDuration = FMath::Max(DebugRefreshRate * 1.15f, 0.10f);

		for (int32 LineIndex = 0; LineIndex < CachedDisplayLines.Num(); ++LineIndex)
		{
			const uint64 MessageKey = static_cast<uint64>(BaseMessageKey + LineIndex);
			const FColor LineColor = CachedLineColors.IsValidIndex(LineIndex) ? CachedLineColors[LineIndex] : FColor::White;
			GEngine->AddOnScreenDebugMessage(MessageKey, MessageDuration, LineColor, CachedDisplayLines[LineIndex], false);
		}

		for (int32 LineIndex = CachedDisplayLines.Num(); LineIndex < LastDrawnLineCount; ++LineIndex)
		{
			const uint64 MessageKey = static_cast<uint64>(BaseMessageKey + LineIndex);
			GEngine->RemoveOnScreenDebugMessage(MessageKey);
		}

		LastDrawnLineCount = CachedDisplayLines.Num();
	}
	else if (!bDrawToScreen)
	{
		ClearDrawnMessages();
		LastScreenUpdateTimeSeconds = -DBL_MAX;
	}

	if (bShouldLogNow)
	{
		StatsDebugPrivate::WriteLinesToLog(StatsManager, CachedDisplayLines);
	}
}

void FStatsDebugManager::LogDebug(UStatsManager* StatsManager)
{
	if (!bEnableDebug || !bLogToOutput || !StatsManager)
	{
		return;
	}

	const UWorld* World = StatsManager ? StatsManager->GetWorld() : nullptr;
	const double CurrentTimeSeconds = World ? static_cast<double>(World->GetTimeSeconds()) : FPlatformTime::Seconds();
	if (!ShouldRefresh(CurrentTimeSeconds, LastLogUpdateTimeSeconds))
	{
		return;
	}

	BuildDisplayLines(StatsManager, CachedDisplayLines, CachedLineColors);
	StatsDebugPrivate::WriteLinesToLog(StatsManager, CachedDisplayLines);
}

bool FStatsDebugManager::ShouldRefresh(double CurrentTimeSeconds, double& LastExecutionTimeSeconds)
{
	const double EffectiveRefreshRate = FMath::Max(0.05, static_cast<double>(DebugRefreshRate));
	if ((CurrentTimeSeconds - LastExecutionTimeSeconds) < EffectiveRefreshRate)
	{
		return false;
	}

	LastExecutionTimeSeconds = CurrentTimeSeconds;
	return true;
}

void FStatsDebugManager::ClearDrawnMessages()
{
	if (!GEngine || LastDrawnLineCount <= 0)
	{
		return;
	}

	for (int32 LineIndex = 0; LineIndex < LastDrawnLineCount; ++LineIndex)
	{
		const uint64 MessageKey = static_cast<uint64>(BaseMessageKey + LineIndex);
		GEngine->RemoveOnScreenDebugMessage(MessageKey);
	}

	LastDrawnLineCount = 0;
}

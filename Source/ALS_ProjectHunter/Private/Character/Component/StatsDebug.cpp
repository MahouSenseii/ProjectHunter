#include "Character/Component/StatsDebug.h"

#include "AttributeSet.h"
#include "Character/Component/StatsManager.h"
#include "Data/StatDefinitions.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"

DEFINE_LOG_CATEGORY(LogStatsDebugManager);

namespace StatsDebugPrivate
{
	FString BuildSearchText(const FStatDebugEntry& Entry)
	{
		return FString::Printf(TEXT("%s %s %s"), *Entry.StatName.ToString(), *Entry.DisplayName.ToString(), *Entry.Category.ToString());
	}

	float ReadAttributeValue(const UStatsManager* StatsManager, FName StatName)
	{
		if (!StatsManager)
		{
			return 0.0f;
		}

		FGameplayAttribute Attribute;
		return FHunterStatDefinitions::TryGetGameplayAttribute(StatName, Attribute)
			? StatsManager->GetAttributeValue(Attribute)
			: 0.0f;
	}

	void WriteLinesToLog(const UStatsManager* StatsManager, const TArray<FString>& Lines)
	{
		const FString OwnerName = (StatsManager && StatsManager->GetOwner()) ? StatsManager->GetOwner()->GetName() : TEXT("UnknownOwner");

		for (const FString& Line : Lines)
		{
			UE_LOG(LogStatsDebugManager, Log, TEXT("[%s] %s"), *OwnerName, *Line);
		}
	}
}

FStatDebugEntry::FStatDebugEntry()
	: StatName(NAME_None)
	, Category(NAME_None)
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
	bShowCustom = true;
	StatEntries.Reset();
	bEntriesSynchronized = false;
	LastScreenUpdateTimeSeconds = -DBL_MAX;
	LastLogUpdateTimeSeconds = -DBL_MAX;
	LastDrawnLineCount = 0;
	CachedDisplayLines.Reset();
	CachedLineColors.Reset();
}

void FStatsDebugManager::RegisterStat(const FName StatName, const FText& DisplayName, const FName Category, const FColor& DisplayColor, bool bEnabled)
{
	if (FStatDebugEntry* ExistingEntry = StatEntries.FindByPredicate([&StatName](const FStatDebugEntry& Entry)
	{
		return Entry.StatName == StatName;
	}))
	{
		ExistingEntry->DisplayName = DisplayName;
		ExistingEntry->Category = Category;
		ExistingEntry->DisplayColor = DisplayColor;
		ExistingEntry->bEnabled = bEnabled;
		return;
	}

	FStatDebugEntry NewEntry;
	NewEntry.StatName = StatName;
	NewEntry.DisplayName = DisplayName;
	NewEntry.Category = Category;
	NewEntry.DisplayColor = DisplayColor;
	NewEntry.bEnabled = bEnabled;
	StatEntries.Add(MoveTemp(NewEntry));
}

void FStatsDebugManager::RegisterStats()
{
	const TArray<FHunterStatDefinition>& Definitions = FHunterStatDefinitions::GetAllStatDefinitions();

	bool bNeedsSync = !bEntriesSynchronized || StatEntries.Num() != Definitions.Num();
	if (!bNeedsSync)
	{
		for (const FHunterStatDefinition& Definition : Definitions)
		{
			const FStatDebugEntry* ExistingEntry = StatEntries.FindByPredicate([&Definition](const FStatDebugEntry& Entry)
			{
				return Entry.StatName == Definition.StatName;
			});

			if (!ExistingEntry ||
				ExistingEntry->Category != Definition.Category ||
				ExistingEntry->DisplayName.ToString() != Definition.DisplayName.ToString())
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

	for (const FHunterStatDefinition& Definition : Definitions)
	{
		FStatDebugEntry NewEntry;
		NewEntry.StatName = Definition.StatName;
		NewEntry.DisplayName = Definition.DisplayName;
		NewEntry.Category = Definition.Category;
		NewEntry.bEnabled = true;
		NewEntry.DisplayColor = Definition.DebugColor.ToFColor(true);

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
	if (!Entry.bEnabled || !IsCategoryEnabled(Entry.Category))
	{
		return false;
	}

	const FString ActiveFilter = FilterString.TrimStartAndEnd();
	return ActiveFilter.IsEmpty() || StatsDebugPrivate::BuildSearchText(Entry).Contains(ActiveFilter, ESearchCase::IgnoreCase);
}

bool FStatsDebugManager::IsCategoryEnabled(FName Category) const
{
	if (Category == TEXT("Vitals"))
	{
		return bShowVitals;
	}

	if (Category == TEXT("Resources"))
	{
		return bShowResources;
	}

	if (Category == TEXT("Regeneration"))
	{
		return bShowRegeneration;
	}

	if (Category == TEXT("Primary"))
	{
		return bShowPrimary;
	}

	if (Category == TEXT("Secondary"))
	{
		return bShowSecondary;
	}

	if (Category == TEXT("Combat"))
	{
		return bShowCombat;
	}

	if (Category == TEXT("Defense"))
	{
		return bShowDefense;
	}

	if (Category == TEXT("Resistances"))
	{
		return bShowResistances;
	}

	if (Category == TEXT("Movement"))
	{
		return bShowMovement;
	}

	if (Category == TEXT("Utility"))
	{
		return bShowUtility;
	}

	if (Category == TEXT("Loot"))
	{
		return bShowLoot;
	}

	return bShowCustom;
}

void FStatsDebugManager::BuildDisplayLines(UStatsManager* StatsManager, TArray<FString>& OutLines)
{
	OutLines.Reset();
	CachedLineColors.Reset();

	if (!StatsManager)
	{
		return;
	}

	RegisterStats();

	OutLines.Reserve(StatEntries.Num());
	CachedLineColors.Reserve(StatEntries.Num());

	for (const FStatDebugEntry& Entry : StatEntries)
	{
		if (!IsStatEnabled(Entry))
		{
			continue;
		}

		const FHunterStatDefinition* Definition = FHunterStatDefinitions::GetStatDefinition(Entry.StatName);
		if (!Definition)
		{
			continue;
		}

		const float Value = StatsDebugPrivate::ReadAttributeValue(StatsManager, Entry.StatName);
		OutLines.Add(FString::Printf(TEXT("%s: %.2f"), *Definition->DisplayName.ToString(), Value));
		CachedLineColors.Add(Entry.DisplayColor);
	}

	if (OutLines.Num() == 0)
	{
		const FString ActiveFilter = FilterString.TrimStartAndEnd();
		if (ActiveFilter.IsEmpty())
		{
			OutLines.Add(TEXT("No enabled stats matched the current debug categories."));
		}
		else
		{
			OutLines.Add(FString::Printf(TEXT("No stats matched filter '%s'."), *ActiveFilter));
		}

		CachedLineColors.Add(FColor(180, 180, 180));
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

	BuildDisplayLines(StatsManager, CachedDisplayLines);

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

	const double CurrentTimeSeconds = FPlatformTime::Seconds();
	if (!ShouldRefresh(CurrentTimeSeconds, LastLogUpdateTimeSeconds))
	{
		return;
	}

	BuildDisplayLines(StatsManager, CachedDisplayLines);
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

#include "Stats/Debug/StatsDebugManager.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Stats/Components/StatsManager.h"
#include "Stats/Data/BaseStatsData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "HAL/PlatformTime.h"
#include "Stats/Library/Enums/StatsEnumLibrary.h"

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
			return TEXT("Authored | N/A (no asset)");
		}

		if (!AuthoredEntry)
		{
			return TEXT("Authored | Missing");
		}

		if (!AuthoredEntry->bOverrideValue)
		{
			return TEXT("Override | Off");
		}

		return FString::Printf(TEXT("Authored | %.2f"), AuthoredEntry->BaseValue);
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

	const UAbilitySystemComponent* ResolveDebugAbilitySystemComponent(const UStatsManager* StatsManager)
	{
		const AActor* Owner = StatsManager ? StatsManager->GetOwner() : nullptr;
		if (!Owner)
		{
			return nullptr;
		}

		if (const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Owner))
		{
			if (const UAbilitySystemComponent* ASC = AbilitySystemInterface->GetAbilitySystemComponent())
			{
				return ASC;
			}
		}

		return Owner->FindComponentByClass<UAbilitySystemComponent>();
	}

	FString BuildOwnerLine(const UStatsManager* StatsManager)
	{
		const AActor* Owner = StatsManager ? StatsManager->GetOwner() : nullptr;
		const UAbilitySystemComponent* ASC = ResolveDebugAbilitySystemComponent(StatsManager);
		return FString::Printf(TEXT("StatsDebug Owner: %s | ASC: %s"), *GetNameSafe(Owner), *GetNameSafe(ASC));
	}

	uint64 BuildScreenMessageKeyBase(const UStatsManager* StatsManager, const int32 BaseMessageKey)
	{
		const UObject* KeyObject = nullptr;
		if (StatsManager)
		{
			KeyObject = StatsManager->GetOwner() ? static_cast<const UObject*>(StatsManager->GetOwner()) : StatsManager;
		}

		const uint64 SafeBaseMessageKey = static_cast<uint64>(FMath::Max(0, BaseMessageKey));
		const uint64 OwnerOffset = KeyObject ? static_cast<uint64>(KeyObject->GetUniqueID()) * 1000ull : 0ull;
		return SafeBaseMessageKey + OwnerOffset;
	}

	bool ShouldDrawToScreen(const UStatsManager* StatsManager)
	{
		const AActor* Owner = StatsManager ? StatsManager->GetOwner() : nullptr;
		if (!Owner || Owner->GetNetMode() == NM_DedicatedServer)
		{
			return false;
		}

		// Blueprint assets can retain an older added component after the native
		// StatsManager is introduced. Only the owner's canonical manager presents.
		if (Owner->FindComponentByClass<UStatsManager>() != StatsManager)
		{
			return false;
		}

		// Replicated pawns inherit the same debug settings. Without this gate every
		// server/remote copy writes another panel into GEngine's global message list.
		const APawn* OwnerPawn = Cast<APawn>(Owner);
		return !OwnerPawn || OwnerPawn->IsLocallyControlled();
	}

	FGameplayAttribute ResolveCoreAttribute(const EHunterAttribute AttributeType)
	{
		switch (AttributeType)
		{
		case EHunterAttribute::Health:
			return UHunterAttributeSet::GetHealthAttribute();
		case EHunterAttribute::MaxEffectiveHealth:
			return UHunterAttributeSet::GetMaxEffectiveHealthAttribute();
		case EHunterAttribute::Stamina:
			return UHunterAttributeSet::GetStaminaAttribute();
		case EHunterAttribute::MaxEffectiveStamina:
			return UHunterAttributeSet::GetMaxEffectiveStaminaAttribute();
		case EHunterAttribute::Mana:
			return UHunterAttributeSet::GetManaAttribute();
		case EHunterAttribute::MaxEffectiveMana:
			return UHunterAttributeSet::GetMaxEffectiveManaAttribute();
		case EHunterAttribute::ArcaneShield:
			return UHunterAttributeSet::GetArcaneShieldAttribute();
		case EHunterAttribute::MaxEffectiveArcaneShield:
			return UHunterAttributeSet::GetMaxEffectiveArcaneShieldAttribute();
		case EHunterAttribute::ReservedHealth:
			return UHunterAttributeSet::GetReservedHealthAttribute();
		case EHunterAttribute::ReservedStamina:
			return UHunterAttributeSet::GetReservedStaminaAttribute();
		case EHunterAttribute::ReservedMana:
			return UHunterAttributeSet::GetReservedManaAttribute();
		case EHunterAttribute::ReservedArcaneShield:
			return UHunterAttributeSet::GetReservedArcaneShieldAttribute();
		default:
			return FGameplayAttribute();
		}
	}

	float GetCoreLiveAttributeValue(UStatsManager* StatsManager, const EHunterAttribute AttributeType)
	{
		if (!StatsManager)
		{
			return 0.f;
		}

		const FGameplayAttribute Attribute = ResolveCoreAttribute(AttributeType);
		if (Attribute.IsValid() && StatsManager->HasLiveAttribute(Attribute))
		{
			return StatsManager->GetAttributeValue(Attribute);
		}

		return StatsManager->GetAttributeByType(AttributeType);
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

		if (ContainsToken(MainCategory, TEXT("Experience")))
		{
			return EStatDebugBucket::Secondary;
		}

		if (ContainsToken(MainCategory, TEXT("Progression")))
		{
			return EStatDebugBucket::Primary;
		}

		if (ContainsToken(MainCategory, TEXT("Resource")))
		{
			return EStatDebugBucket::Resources;
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
	, bShowFullDetails(false)
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
	, LastLogUpdateTimeSeconds(-DBL_MAX)
	, LastDrawnLineCount(0)
	, LastScreenMessageKeyBase(0)
{
}

void FStatsDebugManager::InitializeDefaults()
{
	bEnableDebug = false;
	bDrawToScreen = true;
	bLogToOutput = false;
	DebugRefreshRate = 0.25f;
	BaseMessageKey = 50000;
	bShowFullDetails = false;
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
	LastLogUpdateTimeSeconds = -DBL_MAX;
	LastDrawnLineCount = 0;
	LastScreenMessageKeyBase = 0;
	CachedDisplayLines.Reset();
	CachedLineColors.Reset();
	CachedLiveValues.Reset();
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
	const FString ActiveFilter = FilterString.TrimStartAndEnd();

	auto MatchesFilter = [&ActiveFilter](const FString& SearchText)
	{
		return ActiveFilter.IsEmpty() || SearchText.Contains(ActiveFilter, ESearchCase::IgnoreCase);
	};

	auto AddNoMatchLine = [&]()
	{
		const FString LActiveFilter = FilterString.TrimStartAndEnd();
		if (LActiveFilter.IsEmpty())
		{
			OutLines.Add(TEXT("No reflected stats matched the active debug categories."));
		}
		else
		{
			OutLines.Add(FString::Printf(TEXT("No reflected stats matched filter '%s'."), *LActiveFilter));
		}

		OutColors.Add(FColor(180, 180, 180));
	};

	auto AddCoreResourceLine = [&](
		const TCHAR* Label,
		EHunterAttribute CurrentAttribute,
		EHunterAttribute MaxAttribute,
		EHunterAttribute ReservedAttribute,
		const FColor& RowColor)
	{
		const FString LabelString(Label);
		if (!MatchesFilter(FString::Printf(TEXT("%s ASC Live Resource Vitals"), *LabelString)))
		{
			return;
		}

		const FName CoreCategory(TEXT("ASC Live Resources"));
		if (ActiveMainCategory != CoreCategory)
		{
			OutLines.Add(StatsDebugPrivate::BuildMainCategoryHeader(CoreCategory));
			OutColors.Add(StatsDebugPrivate::CategoryHeaderColor);
			ActiveMainCategory = CoreCategory;
			ActiveSubCategory = NAME_None;
		}

		const float CurrentValue = StatsDebugPrivate::GetCoreLiveAttributeValue(StatsManager, CurrentAttribute);
		const float MaxValue = StatsDebugPrivate::GetCoreLiveAttributeValue(StatsManager, MaxAttribute);
		const float ReservedValue = StatsDebugPrivate::GetCoreLiveAttributeValue(StatsManager, ReservedAttribute);
		const float Percent = MaxValue > KINDA_SMALL_NUMBER
			? FMath::Clamp(CurrentValue / MaxValue, 0.0f, 1.0f) * 100.0f
			: 0.0f;

		if (ReservedValue > KINDA_SMALL_NUMBER)
		{
			const float TotalMaxValue = FMath::Max(MaxValue + ReservedValue, ReservedValue);
			const float ReservedPercent = TotalMaxValue > KINDA_SMALL_NUMBER
				? FMath::Clamp(ReservedValue / TotalMaxValue, 0.0f, 1.0f) * 100.0f
				: 0.0f;
			OutLines.Add(FString::Printf(
				TEXT("%s | ASC Live | %.2f / %.2f | Reserved %.2f (%0.0f%%) | %0.0f%%"),
				*LabelString,
				CurrentValue,
				MaxValue,
				ReservedValue,
				ReservedPercent,
				Percent));
		}
		else
		{
			OutLines.Add(FString::Printf(
				TEXT("%s | ASC Live | %.2f / %.2f | %0.0f%%"),
				*LabelString,
				CurrentValue,
				MaxValue,
				Percent));
		}
		OutColors.Add(RowColor);
	};

	const FString OwnerLine = StatsDebugPrivate::BuildOwnerLine(StatsManager);
	if (MatchesFilter(OwnerLine))
	{
		OutLines.Add(OwnerLine);
		OutColors.Add(StatsDebugPrivate::CategoryHeaderColor);
	}

	if (bShowVitals || bShowResources)
	{
		AddCoreResourceLine(TEXT("Health"), EHunterAttribute::Health, EHunterAttribute::MaxEffectiveHealth, EHunterAttribute::ReservedHealth, FColor(90, 220, 110));
		AddCoreResourceLine(TEXT("Stamina"), EHunterAttribute::Stamina, EHunterAttribute::MaxEffectiveStamina, EHunterAttribute::ReservedStamina, FColor(255, 210, 90));
		AddCoreResourceLine(TEXT("Mana"), EHunterAttribute::Mana, EHunterAttribute::MaxEffectiveMana, EHunterAttribute::ReservedMana, FColor(100, 170, 255));
		AddCoreResourceLine(TEXT("Arcane Shield"), EHunterAttribute::ArcaneShield, EHunterAttribute::MaxEffectiveArcaneShield, EHunterAttribute::ReservedArcaneShield, FColor(170, 130, 255));
	}

	if (!bShowFullDetails)
	{
		if (OutLines.Num() == 0)
		{
			AddNoMatchLine();
		}
		return;
	}

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
			LiveText = FString::Printf(TEXT("Live | UNRESOLVED in %s"), *GetNameSafe(StatsManager->GetSourceAttributeSetClass()));
			RowColor = StatsDebugPrivate::UnresolvedColor;
		}
		else if (!bHasLiveAttribute)
		{
			LiveText = TEXT("Live | Resolved, no live AttributeSet instance");
			RowColor = StatsDebugPrivate::MissingLiveAttributeColor;
		}
		else
		{
			LiveText = FString::Printf(TEXT("Live | %.2f"), LiveValue);

			if (AuthoredEntry && AuthoredEntry->bOverrideValue)
			{
				DeltaText = FString::Printf(TEXT(" | Delta | %+0.2f"), LiveValue - AuthoredEntry->BaseValue);
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
		AddNoMatchLine();
	}
}

void FStatsDebugManager::DrawDebug(UStatsManager* StatsManager, UObject* WorldContext)
{
	if (!bEnableDebug)
	{
		ClearDrawnMessages();
		LastLogUpdateTimeSeconds = -DBL_MAX;
		CachedLiveValues.Reset();
		return;
	}

	const bool bCanDrawToScreen = bDrawToScreen && StatsDebugPrivate::ShouldDrawToScreen(StatsManager);
	const bool bValuesChanged = CheckForValueChanges(StatsManager);
	const bool bNeedsScreenRedraw = bCanDrawToScreen && LastDrawnLineCount == 0;
	const bool bShouldBuildDisplayLines = (bCanDrawToScreen || bLogToOutput)
	                                   && (bValuesChanged || CachedDisplayLines.IsEmpty() || bNeedsScreenRedraw);

	TArray<FString> OldLines;
	if (bShouldBuildDisplayLines)
	{
		Swap(OldLines, CachedDisplayLines);
		BuildDisplayLines(StatsManager, CachedDisplayLines, CachedLineColors);
	}

	if (bCanDrawToScreen && GEngine)
	{
		if (bShouldBuildDisplayLines)
		{
			constexpr float PersistentDuration = 3600.f;
			const uint64 ScreenMessageKeyBase = StatsDebugPrivate::BuildScreenMessageKeyBase(StatsManager, BaseMessageKey);
			if (LastDrawnLineCount > 0 && LastScreenMessageKeyBase != ScreenMessageKeyBase)
			{
				ClearScreenMessages();
			}

			LastScreenMessageKeyBase = ScreenMessageKeyBase;

			for (int32 LineIndex = 0; LineIndex < CachedDisplayLines.Num(); ++LineIndex)
			{
				const bool bLineChanged = LineIndex >= LastDrawnLineCount
				                       || !OldLines.IsValidIndex(LineIndex)
				                       || OldLines[LineIndex] != CachedDisplayLines[LineIndex];
				if (!bLineChanged)
				{
					continue;
				}

				const uint64 MessageKey = ScreenMessageKeyBase + static_cast<uint64>(LineIndex);
				const FColor LineColor  = CachedLineColors.IsValidIndex(LineIndex)
				                        ? CachedLineColors[LineIndex]
				                        : FColor::White;
				GEngine->AddOnScreenDebugMessage(MessageKey, PersistentDuration, LineColor, CachedDisplayLines[LineIndex], false);
			}

			for (int32 LineIndex = CachedDisplayLines.Num(); LineIndex < LastDrawnLineCount; ++LineIndex)
			{
				GEngine->RemoveOnScreenDebugMessage(ScreenMessageKeyBase + static_cast<uint64>(LineIndex));
			}

			LastDrawnLineCount = CachedDisplayLines.Num();
		}
	}
	else
	{
		ClearScreenMessages();
	}

	if (bLogToOutput && bShouldBuildDisplayLines)
	{
		const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		const double CurrentTimeSeconds = World ? static_cast<double>(World->GetTimeSeconds()) : FPlatformTime::Seconds();
		if (ShouldRefresh(CurrentTimeSeconds, LastLogUpdateTimeSeconds))
		{
			StatsDebugPrivate::WriteLinesToLog(StatsManager, CachedDisplayLines);
		}
	}
}

void FStatsDebugManager::LogDebug(UStatsManager* StatsManager)
{
	if (!bEnableDebug || !bLogToOutput || !StatsManager)
	{
		return;
	}

	if (!CheckForValueChanges(StatsManager))
	{
		return;
	}

	const UWorld* World = StatsManager->GetWorld();
	const double CurrentTimeSeconds = World ? static_cast<double>(World->GetTimeSeconds()) : FPlatformTime::Seconds();
	if (!ShouldRefresh(CurrentTimeSeconds, LastLogUpdateTimeSeconds))
	{
		return;
	}

	BuildDisplayLines(StatsManager, CachedDisplayLines, CachedLineColors);
	StatsDebugPrivate::WriteLinesToLog(StatsManager, CachedDisplayLines);
}

bool FStatsDebugManager::CheckForValueChanges(UStatsManager* StatsManager)
{
	if (!StatsManager)
	{
		return false;
	}

	RegisterStats(StatsManager);

	bool bAnyChanged = false;

	auto TrackLiveValue = [this, &bAnyChanged](FName CacheKey, float CurrentValue)
	{
		float* PreviousValue = CachedLiveValues.Find(CacheKey);
		if (!PreviousValue)
		{
			CachedLiveValues.Add(CacheKey, CurrentValue);
			bAnyChanged = true;
		}
		else if (!FMath::IsNearlyEqual(CurrentValue, *PreviousValue, KINDA_SMALL_NUMBER))
		{
			*PreviousValue = CurrentValue;
			bAnyChanged = true;
		}
	};

	TrackLiveValue(TEXT("ASC.Health"), StatsDebugPrivate::GetCoreLiveAttributeValue(StatsManager, EHunterAttribute::Health));
	TrackLiveValue(TEXT("ASC.MaxEffectiveHealth"), StatsDebugPrivate::GetCoreLiveAttributeValue(StatsManager, EHunterAttribute::MaxEffectiveHealth));
	TrackLiveValue(TEXT("ASC.ReservedHealth"), StatsDebugPrivate::GetCoreLiveAttributeValue(StatsManager, EHunterAttribute::ReservedHealth));
	TrackLiveValue(TEXT("ASC.Stamina"), StatsDebugPrivate::GetCoreLiveAttributeValue(StatsManager, EHunterAttribute::Stamina));
	TrackLiveValue(TEXT("ASC.MaxEffectiveStamina"), StatsDebugPrivate::GetCoreLiveAttributeValue(StatsManager, EHunterAttribute::MaxEffectiveStamina));
	TrackLiveValue(TEXT("ASC.ReservedStamina"), StatsDebugPrivate::GetCoreLiveAttributeValue(StatsManager, EHunterAttribute::ReservedStamina));
	TrackLiveValue(TEXT("ASC.Mana"), StatsDebugPrivate::GetCoreLiveAttributeValue(StatsManager, EHunterAttribute::Mana));
	TrackLiveValue(TEXT("ASC.MaxEffectiveMana"), StatsDebugPrivate::GetCoreLiveAttributeValue(StatsManager, EHunterAttribute::MaxEffectiveMana));
	TrackLiveValue(TEXT("ASC.ReservedMana"), StatsDebugPrivate::GetCoreLiveAttributeValue(StatsManager, EHunterAttribute::ReservedMana));
	TrackLiveValue(TEXT("ASC.ArcaneShield"), StatsDebugPrivate::GetCoreLiveAttributeValue(StatsManager, EHunterAttribute::ArcaneShield));
	TrackLiveValue(TEXT("ASC.MaxEffectiveArcaneShield"), StatsDebugPrivate::GetCoreLiveAttributeValue(StatsManager, EHunterAttribute::MaxEffectiveArcaneShield));
	TrackLiveValue(TEXT("ASC.ReservedArcaneShield"), StatsDebugPrivate::GetCoreLiveAttributeValue(StatsManager, EHunterAttribute::ReservedArcaneShield));
	TrackLiveValue(TEXT("Debug.ShowFullDetails"), bShowFullDetails ? 1.0f : 0.0f);

	if (!bShowFullDetails)
	{
		return bAnyChanged;
	}

	for (const FStatDebugEntry& Entry : StatEntries)
	{
		if (!IsStatEnabled(Entry))
		{
			continue;
		}

		FGameplayAttribute Attribute;
		FStatInitializationEntry ResolvedDefinition;
		if (!StatsManager->ResolveAttributeByName(Entry.StatName, Attribute, &ResolvedDefinition)
			|| !Attribute.IsValid()
			|| !StatsManager->HasLiveAttribute(Attribute))
		{
			continue;
		}

		const float CurrentValue = StatsManager->GetAttributeValue(Attribute);
		float* PreviousValue = CachedLiveValues.Find(Entry.StatName);

		if (!PreviousValue)
		{
			CachedLiveValues.Add(Entry.StatName, CurrentValue);
			bAnyChanged = true;
		}
		else if (!FMath::IsNearlyEqual(CurrentValue, *PreviousValue, KINDA_SMALL_NUMBER))
		{
			*PreviousValue = CurrentValue;
			bAnyChanged = true;
		}
	}

	return bAnyChanged;
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

void FStatsDebugManager::ClearScreenMessages()
{
	if (GEngine && LastDrawnLineCount > 0)
	{
		const uint64 ScreenMessageKeyBase = LastScreenMessageKeyBase != 0
			? LastScreenMessageKeyBase
			: StatsDebugPrivate::BuildScreenMessageKeyBase(nullptr, BaseMessageKey);

		for (int32 LineIndex = 0; LineIndex < LastDrawnLineCount; ++LineIndex)
		{
			GEngine->RemoveOnScreenDebugMessage(ScreenMessageKeyBase + static_cast<uint64>(LineIndex));
		}
	}

	LastDrawnLineCount = 0;
	LastScreenMessageKeyBase = 0;
}

void FStatsDebugManager::ClearDrawnMessages()
{
	ClearScreenMessages();
	CachedDisplayLines.Reset();
	CachedLineColors.Reset();
	CachedLiveValues.Reset();
}

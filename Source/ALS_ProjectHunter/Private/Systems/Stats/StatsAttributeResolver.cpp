#include "Systems/Stats/StatsAttributeResolver.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Data/BaseStatsData.h"
#include "Systems/Stats/Components/StatsManager.h"

UHunterAttributeSet* FStatsAttributeResolver::GetAttributeSet(const UStatsManager& Manager)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent(Manager);
	if (!ASC)
	{
		return nullptr;
	}

	const UAttributeSet* LiveAttributeSet = GetLiveSourceAttributeSet(Manager, ASC, GetSourceAttributeSetClass(Manager).Get(), true);
	UHunterAttributeSet* HunterAttributeSet = Cast<UHunterAttributeSet>(const_cast<UAttributeSet*>(LiveAttributeSet));
	if (!HunterAttributeSet && LiveAttributeSet)
	{
		Manager.LogWarningOnce(
			FString::Printf(TEXT("LiveSetWrongType:%s"), *GetNameSafe(LiveAttributeSet->GetClass())),
			FString::Printf(
				TEXT("StatsManager: Live AttributeSet is not a UHunterAttributeSet. Actor=%s ASC=%s SourceAttributeSetClass=%s LiveAttributeSetClass=%s"),
				*GetNameSafe(Manager.GetOwner()),
				*GetNameSafe(ASC),
				*GetNameSafe(GetSourceAttributeSetClass(Manager)),
				*GetNameSafe(LiveAttributeSet->GetClass())));
	}

	Manager.CachedAttributeSet = HunterAttributeSet;
	return HunterAttributeSet;
}

UAbilitySystemComponent* FStatsAttributeResolver::GetAbilitySystemComponent(const UStatsManager& Manager)
{
	AActor* Owner = Manager.GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	UAbilitySystemComponent* ResolvedASC = nullptr;
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
	{
		ResolvedASC = ASI->GetAbilitySystemComponent();
	}

	if (!ResolvedASC)
	{
		ResolvedASC = Owner->FindComponentByClass<UAbilitySystemComponent>();
	}

	Manager.CachedASC = ResolvedASC;
	return ResolvedASC;
}

const UAttributeSet* FStatsAttributeResolver::GetLiveSourceAttributeSet(const UStatsManager& Manager, UAbilitySystemComponent* ASC, const UClass* DesiredClass, bool bLogIfMissing, FName AttributeName)
{
	const UClass* DesiredClassPtr = DesiredClass ? DesiredClass : UHunterAttributeSet::StaticClass();
	if (!ASC)
	{
		if (bLogIfMissing)
		{
			Manager.LogWarningOnce(
				FString::Printf(TEXT("MissingASC:%s"), *AttributeName.ToString()),
				FString::Printf(
					TEXT("StatsManager: Missing ASC while resolving live AttributeSet on actor=%s SourceAttributeSetClass=%s Attribute=%s"),
					*GetNameSafe(Manager.GetOwner()),
					*GetNameSafe(DesiredClassPtr),
					AttributeName.IsNone() ? TEXT("None") : *AttributeName.ToString()));
		}

		return nullptr;
	}

	const UAttributeSet* LiveAttributeSet = nullptr;
	for (const UAttributeSet* Candidate : ASC->GetSpawnedAttributes())
	{
		if (Candidate && Candidate->IsA(DesiredClassPtr))
		{
			LiveAttributeSet = Candidate;
			break;
		}
	}

	if (!LiveAttributeSet && DesiredClassPtr->IsChildOf(UHunterAttributeSet::StaticClass()))
	{
		const UAttributeSet* HunterAttributeSet = ASC->GetSet<UHunterAttributeSet>();
		if (HunterAttributeSet && HunterAttributeSet->IsA(DesiredClassPtr))
		{
			LiveAttributeSet = HunterAttributeSet;
		}
	}

	if (!LiveAttributeSet && bLogIfMissing)
	{
		Manager.LogWarningOnce(
			FString::Printf(TEXT("MissingLiveAttributeSet:%s:%s"), *GetNameSafe(DesiredClassPtr), *AttributeName.ToString()),
			FString::Printf(
				TEXT("StatsManager: No live AttributeSet instance is registered on ASC. Actor=%s ASC=%s SourceAttributeSetClass=%s Attribute=%s SpawnedAttributeCount=%d"),
				*GetNameSafe(Manager.GetOwner()),
				*GetNameSafe(ASC),
				*GetNameSafe(DesiredClassPtr),
				AttributeName.IsNone() ? TEXT("None") : *AttributeName.ToString(),
				ASC->GetSpawnedAttributes().Num()));
	}

	return LiveAttributeSet;
}

bool FStatsAttributeResolver::HasExpectedLiveAttributeSet(const UStatsManager& Manager, bool bLogIfMissing, FName AttributeName)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent(Manager);
	return GetLiveSourceAttributeSet(Manager, ASC, GetSourceAttributeSetClass(Manager).Get(), bLogIfMissing, AttributeName) != nullptr;
}

void FStatsAttributeResolver::RefreshCachedAbilitySystemState(const UStatsManager& Manager, const TCHAR* Context)
{
	UAbilitySystemComponent* ASC = nullptr;
	if (AActor* Owner = Manager.GetOwner())
	{
		if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
		{
			ASC = ASI->GetAbilitySystemComponent();
		}

		if (!ASC)
		{
			ASC = Owner->FindComponentByClass<UAbilitySystemComponent>();
		}
	}

	const UAttributeSet* LiveAttributeSet = GetLiveSourceAttributeSet(Manager, ASC, GetSourceAttributeSetClass(Manager).Get(), false);

	Manager.CachedASC = ASC;
	Manager.CachedAttributeSet = Cast<UHunterAttributeSet>(const_cast<UAttributeSet*>(LiveAttributeSet));

	Manager.LogAbilitySystemState(Context, ASC, LiveAttributeSet);

	if (!ASC)
	{
		Manager.LogWarningOnce(
			FString::Printf(TEXT("MissingASCState:%s"), Context),
			FString::Printf(
				TEXT("StatsManager[%s]: No ASC found on actor=%s"),
				Context,
				*GetNameSafe(Manager.GetOwner())));
	}
	else if (!LiveAttributeSet)
	{
		Manager.LogWarningOnce(
			FString::Printf(TEXT("MissingLiveSetState:%s"), Context),
			FString::Printf(
				TEXT("StatsManager[%s]: ASC=%s exists, but SourceAttributeSetClass=%s has no live registered AttributeSet on actor=%s"),
				Context,
				*GetNameSafe(ASC),
				*GetNameSafe(GetSourceAttributeSetClass(Manager)),
				*GetNameSafe(Manager.GetOwner())));
	}
}

bool FStatsAttributeResolver::ResolveAttributeByName(const UStatsManager& Manager, FName AttributeName, FGameplayAttribute& OutAttribute, FStatInitializationEntry* OutDefinition)
{
	OutAttribute = FGameplayAttribute();
	if (OutDefinition)
	{
		*OutDefinition = FStatInitializationEntry();
	}

	const TSubclassOf<UAttributeSet> AttributeSetClass = GetSourceAttributeSetClass(Manager);
	if (!AttributeSetClass)
	{
		Manager.LogWarningOnce(
			FString::Printf(TEXT("ResolveMissingSourceClass:%s"), *AttributeName.ToString()),
			FString::Printf(
				TEXT("ResolveAttributeByName: Missing SourceAttributeSetClass for actor=%s while resolving attribute=%s"),
				*GetNameSafe(Manager.GetOwner()),
				*AttributeName.ToString()));
		return false;
	}

	if (AttributeSetClass->IsChildOf(UHunterAttributeSet::StaticClass()))
	{
		OutAttribute = UHunterAttributeSet::FindAttributeByName(AttributeName);
	}
	else
	{
		Manager.LogWarningOnce(
			FString::Printf(TEXT("ResolveUnsupportedSourceClass:%s:%s"), *GetNameSafe(AttributeSetClass), *AttributeName.ToString()),
			FString::Printf(
				TEXT("ResolveAttributeByName: SourceAttributeSetClass=%s is not supported by UHunterAttributeSet::FindAttributeByName for actor=%s attribute=%s"),
				*GetNameSafe(AttributeSetClass),
				*GetNameSafe(Manager.GetOwner()),
				*AttributeName.ToString()));
	}

	if (!OutAttribute.IsValid())
	{
		Manager.LogWarningOnce(
			FString::Printf(TEXT("ResolveFailed:%s:%s"), *GetNameSafe(AttributeSetClass), *AttributeName.ToString()),
			FString::Printf(
				TEXT("ResolveAttributeByName: Failed to resolve attribute=%s for actor=%s SourceAttributeSetClass=%s"),
				*AttributeName.ToString(),
				*GetNameSafe(Manager.GetOwner()),
				*GetNameSafe(AttributeSetClass)));
		return false;
	}

	if (OutDefinition && Manager.StatsData)
	{
		const TArray<FStatInitializationEntry>& Definitions = Manager.StatsData->GetBaseAttributes();
		if (const FStatInitializationEntry* Found = Definitions.FindByPredicate([&](const FStatInitializationEntry& Entry)
		{
			return Entry.StatName == AttributeName;
		}))
		{
			*OutDefinition = *Found;
		}
	}

	if (OutDefinition && !OutDefinition->IsValid())
	{
		TArray<FStatInitializationEntry> ReflectedDefinitions;
		UBaseStatsData::GatherStatDefinitionsFromAttributeSet(AttributeSetClass, ReflectedDefinitions);
		if (const FStatInitializationEntry* Found = ReflectedDefinitions.FindByPredicate([&AttributeName](const FStatInitializationEntry& Entry)
		{
			return Entry.StatName == AttributeName;
		}))
		{
			*OutDefinition = *Found;
		}
	}

	UE_LOG(
		LogStatsManager,
		VeryVerbose,
		TEXT("ResolveAttributeByName: Resolved attribute=%s for actor=%s SourceAttributeSetClass=%s"),
		*AttributeName.ToString(),
		*GetNameSafe(Manager.GetOwner()),
		*GetNameSafe(AttributeSetClass));

	return true;
}

float FStatsAttributeResolver::GetAttributeValue(const UStatsManager& Manager, const FGameplayAttribute& Attribute)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent(Manager);
	if (!ASC || !Attribute.IsValid())
	{
		return 0.0f;
	}

	if (!HasLiveAttribute(Manager, Attribute))
	{
		return 0.0f;
	}

	return ASC->GetNumericAttribute(Attribute);
}

bool FStatsAttributeResolver::HasLiveAttribute(const UStatsManager& Manager, const FGameplayAttribute& Attribute)
{
	const FString AttributeName = Attribute.IsValid() ? Attribute.GetName() : TEXT("Invalid");
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent(Manager);
	if (!ASC)
	{
		Manager.LogWarningOnce(
			FString::Printf(TEXT("HasLiveAttributeMissingASC:%s"), *AttributeName),
			FString::Printf(
				TEXT("HasLiveAttribute: No ASC for actor=%s while checking attribute=%s SourceAttributeSetClass=%s"),
				*GetNameSafe(Manager.GetOwner()),
				*AttributeName,
				*GetNameSafe(GetSourceAttributeSetClass(Manager))));
		return false;
	}

	if (!Attribute.IsValid())
	{
		Manager.LogWarningOnce(
			TEXT("HasLiveAttributeInvalidAttribute"),
			FString::Printf(TEXT("HasLiveAttribute: Invalid gameplay attribute supplied for actor=%s"), *GetNameSafe(Manager.GetOwner())));
		return false;
	}

	const UClass* AttributeSetClass = Attribute.GetAttributeSetClass();
	const UAttributeSet* LiveAttributeSet = GetLiveSourceAttributeSet(
		Manager,
		ASC,
		AttributeSetClass ? AttributeSetClass : GetSourceAttributeSetClass(Manager).Get(),
		true,
		FName(*AttributeName));
	const bool bHasLiveAttribute = LiveAttributeSet && ASC->HasAttributeSetForAttribute(Attribute);
	if (!bHasLiveAttribute)
	{
		Manager.LogWarningOnce(
			FString::Printf(TEXT("HasLiveAttributeMissingLiveSet:%s"), *AttributeName),
			FString::Printf(
				TEXT("HasLiveAttribute: Attribute=%s has no live backing set. Actor=%s ASC=%s SourceAttributeSetClass=%s AttributeSetClass=%s LiveAttributeSet=%s"),
				*AttributeName,
				*GetNameSafe(Manager.GetOwner()),
				*GetNameSafe(ASC),
				*GetNameSafe(GetSourceAttributeSetClass(Manager)),
				*GetNameSafe(AttributeSetClass),
				*GetNameSafe(LiveAttributeSet)));
	}
	else
	{
		UE_LOG(
			LogStatsManager,
			VeryVerbose,
			TEXT("HasLiveAttribute: Attribute=%s is live for actor=%s ASC=%s LiveAttributeSet=%s"),
			*AttributeName,
			*GetNameSafe(Manager.GetOwner()),
			*GetNameSafe(ASC),
			*GetNameSafe(LiveAttributeSet));
	}

	return bHasLiveAttribute;
}

TSubclassOf<UAttributeSet> FStatsAttributeResolver::GetSourceAttributeSetClass(const UStatsManager& Manager)
{
	if (Manager.StatsData)
	{
		return UBaseStatsData::ResolveSourceAttributeSetClass(Manager.StatsData);
	}

	if (Manager.CachedAttributeSet)
	{
		return Manager.CachedAttributeSet->GetClass();
	}

	return UHunterAttributeSet::StaticClass();
}

void FStatsAttributeResolver::GatherStatDefinitions(const UStatsManager& Manager, TArray<FStatInitializationEntry>& OutDefinitions)
{
	OutDefinitions.Reset();

	if (Manager.StatsData && Manager.StatsData->GetBaseAttributes().Num() > 0)
	{
		OutDefinitions = Manager.StatsData->GetBaseAttributes();
		return;
	}

	UBaseStatsData::GatherStatDefinitionsFromAttributeSet(GetSourceAttributeSetClass(Manager), OutDefinitions);
}

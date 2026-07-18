#pragma once

#include "CoreMinimal.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UHunterAttributeSet;
class UStatsManager;
struct FGameplayAttribute;
struct FStatInitializationEntry;

class ALS_PROJECTHUNTER_API FStatsAttributeResolver
{
public:
	static UHunterAttributeSet* GetAttributeSet(const UStatsManager& Manager);
	static UAbilitySystemComponent* GetAbilitySystemComponent(const UStatsManager& Manager);
	static const UAttributeSet* GetLiveSourceAttributeSet(const UStatsManager& Manager, UAbilitySystemComponent* ASC, const UClass* DesiredClass, bool bLogIfMissing, FName AttributeName = NAME_None);
	static bool HasExpectedLiveAttributeSet(const UStatsManager& Manager, bool bLogIfMissing, FName AttributeName = NAME_None);
	static void RefreshCachedAbilitySystemState(const UStatsManager& Manager, const TCHAR* Context);
	static bool ResolveAttributeByName(const UStatsManager& Manager, FName AttributeName, FGameplayAttribute& OutAttribute, FStatInitializationEntry* OutDefinition = nullptr);
	static float GetAttributeValue(const UStatsManager& Manager, const FGameplayAttribute& Attribute);
	static bool HasLiveAttribute(const UStatsManager& Manager, const FGameplayAttribute& Attribute);
	static TSubclassOf<UAttributeSet> GetSourceAttributeSetClass(const UStatsManager& Manager);
	static void GatherStatDefinitions(const UStatsManager& Manager, TArray<FStatInitializationEntry>& OutDefinitions);
};

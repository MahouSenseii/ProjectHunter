#pragma once

#include "CoreMinimal.h"

class UGameplayEffect;
class UItemInstance;
class UStatsManager;
struct FGameplayAttribute;
struct FGameplayEffectSpecHandle;
struct FPHAttributeData;

class ALS_PROJECTHUNTER_API FEquipmentStatsApplier
{
public:
	static void ApplyEquipmentStats(UStatsManager& Manager, UItemInstance* Item);
	static void RemoveEquipmentStats(UStatsManager& Manager, UItemInstance* Item);
	static void RefreshEquipmentStats(UStatsManager& Manager);
	static void HandleEquipmentChanged(UStatsManager& Manager, UItemInstance* NewItem, UItemInstance* OldItem);
	static FGameplayEffectSpecHandle CreateEquipmentEffect(UStatsManager& Manager, UItemInstance* Item, const TArray<FPHAttributeData>& Stats);
	static bool ApplyStatModifier(UGameplayEffect* Effect, const FPHAttributeData& Stat, const FGameplayAttribute& Attribute);
};

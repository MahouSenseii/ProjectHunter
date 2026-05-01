#pragma once

#include "CoreMinimal.h"

class UBaseStatsData;
class UStatsManager;

class ALS_PROJECTHUNTER_API FStatsInitializer
{
public:
	static bool TryInitializeConfiguredStats(UStatsManager& Manager, const TCHAR* Context);
	static void InitializeFromDataAsset(UStatsManager& Manager, UBaseStatsData* InStatsData);
	static void InitializeFromMap(const UStatsManager& Manager, const TMap<FName, float>& StatsMap);
	static void SetStatValue(const UStatsManager& Manager, FName AttributeName, float Value);
	static bool SetNumericAttributeByName(const UStatsManager& Manager, FName AttributeName, float Value, bool bAutoInitializeCurrentFromMax = true);
	static bool TryGetStatValueForInitialization(const UStatsManager& Manager, const UBaseStatsData* InStatsData, const TMap<FName, float>& StatsMap, FName StatName, float& OutValue);
	static bool ApplyStatIfPresent(const UStatsManager& Manager, const UBaseStatsData* InStatsData, const TMap<FName, float>& StatsMap, FName StatName, bool bAutoInitializeCurrentFromMax = true);
	static bool ApplyCurrentVitalWithClamp(const UStatsManager& Manager, const UBaseStatsData* InStatsData, const TMap<FName, float>& StatsMap, FName CurrentStatName, FName MaxStatName, FName StarterPropertyName) ;
};

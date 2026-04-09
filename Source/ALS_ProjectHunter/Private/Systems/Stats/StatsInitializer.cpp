#include "Systems/Stats/StatsInitializer.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Data/BaseStatsData.h"
#include "GameplayEffect.h"
#include "Systems/Stats/Components/StatsManager.h"
#include "Systems/Stats/StatsAttributeResolver.h"
#include "UObject/UnrealType.h"

namespace StatsInitializerPrivate
{
	bool TryGetFloatPropertyValue(const UObject* Object, FName PropertyName, float& OutValue)
	{
		if (!Object)
		{
			return false;
		}

		const FFloatProperty* FloatProperty = FindFProperty<FFloatProperty>(Object->GetClass(), PropertyName);
		if (!FloatProperty)
		{
			return false;
		}

		OutValue = FloatProperty->GetPropertyValue_InContainer(Object);
		return true;
	}

	bool IsHandledByOrderedInitialization(FName StatName)
	{
		return StatName == TEXT("MaxHealth") ||
			StatName == TEXT("MaxMana") ||
			StatName == TEXT("MaxStamina") ||
			StatName == TEXT("MaxArcaneShield") ||
			StatName == TEXT("HealthRegenRate") ||
			StatName == TEXT("HealthRegenAmount") ||
			StatName == TEXT("ManaRegenRate") ||
			StatName == TEXT("ManaRegenAmount") ||
			StatName == TEXT("StaminaRegenRate") ||
			StatName == TEXT("StaminaRegenAmount") ||
			StatName == TEXT("Health") ||
			StatName == TEXT("Mana") ||
			StatName == TEXT("Stamina") ||
			StatName == TEXT("ArcaneShield");
	}
}

bool FStatsInitializer::TryInitializeConfiguredStats(UStatsManager& Manager, const TCHAR* Context)
{
	if (!Manager.StatsData)
	{
		Manager.LogWarningOnce(
			FString::Printf(TEXT("MissingStatsData:%s"), Context),
			FString::Printf(TEXT("StatsManager[%s]: No StatsData configured on actor=%s"), Context, *GetNameSafe(Manager.GetOwner())));
		return false;
	}

	if (Manager.bHasInitializedConfiguredStats)
	{
		return true;
	}

	if (!Manager.GetOwner() || !Manager.GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!FStatsAttributeResolver::GetAbilitySystemComponent(Manager))
	{
		Manager.LogWarningOnce(
			FString::Printf(TEXT("DeferredInitMissingASC:%s"), Context),
			FString::Printf(TEXT("StatsManager[%s]: Deferring stat initialization because ASC is missing on actor=%s"), Context, *GetNameSafe(Manager.GetOwner())));
		return false;
	}

	if (!FStatsAttributeResolver::HasExpectedLiveAttributeSet(Manager, true))
	{
		Manager.LogWarningOnce(
			FString::Printf(TEXT("DeferredInitMissingLiveSet:%s"), Context),
			FString::Printf(
				TEXT("StatsManager[%s]: Deferring stat initialization because SourceAttributeSetClass=%s is not live on ASC=%s for actor=%s"),
				Context,
				*GetNameSafe(FStatsAttributeResolver::GetSourceAttributeSetClass(Manager)),
				*GetNameSafe(FStatsAttributeResolver::GetAbilitySystemComponent(Manager)),
				*GetNameSafe(Manager.GetOwner())));
		return false;
	}

	InitializeFromDataAsset(Manager, Manager.StatsData);
	return Manager.bHasInitializedConfiguredStats;
}

void FStatsInitializer::InitializeFromDataAsset(UStatsManager& Manager, UBaseStatsData* InStatsData)
{
	if (!InStatsData)
	{
		PH_LOG_WARNING(LogStatsManager, "InitializeFromDataAsset failed: StatsData was null.");
		return;
	}

	AActor* Owner = Manager.GetOwner();
	if (!Owner)
	{
		PH_LOG_ERROR(LogStatsManager, "InitializeFromDataAsset failed: Owner was null.");
		return;
	}

	UE_LOG(LogStatsManager, Log, TEXT("InitializeFromDataAsset: Begin asset=%s owner=%s"),
		*GetNameSafe(InStatsData), *GetNameSafe(Owner));

	Manager.StatsData = InStatsData;
	FStatsAttributeResolver::RefreshCachedAbilitySystemState(Manager, TEXT("InitializeFromDataAsset"));

	UAbilitySystemComponent* ASC = FStatsAttributeResolver::GetAbilitySystemComponent(Manager);
	if (!ASC)
	{
		PH_LOG_ERROR(LogStatsManager, "InitializeFromDataAsset failed: AbilitySystemComponent was unavailable.");
		return;
	}

	if (!Owner->HasAuthority())
	{
		PH_LOG_WARNING(LogStatsManager, "InitializeFromDataAsset failed: Must be called on the server.");
		return;
	}

	const TSubclassOf<UAttributeSet> SourceClass = FStatsAttributeResolver::GetSourceAttributeSetClass(Manager);
	if (!SourceClass)
	{
		PH_LOG_ERROR(LogStatsManager, "InitializeFromDataAsset failed: StatsData=%s did not provide a valid SourceAttributeSetClass.", *GetNameSafe(InStatsData));
		return;
	}

	UHunterAttributeSet* AttributeSet = FStatsAttributeResolver::GetAttributeSet(Manager);
	if (!AttributeSet)
	{
		PH_LOG_ERROR(LogStatsManager, "InitializeFromDataAsset failed: ASC=%s had no live UHunterAttributeSet for Owner=%s SourceClass=%s.", *GetNameSafe(ASC), *GetNameSafe(Owner), *GetNameSafe(SourceClass));
		return;
	}

	if (!FStatsAttributeResolver::HasExpectedLiveAttributeSet(Manager, true))
	{
		PH_LOG_ERROR(LogStatsManager, "InitializeFromDataAsset failed: SourceAttributeSetClass=%s was not registered live on ASC=%s for Actor=%s.", *GetNameSafe(SourceClass), *GetNameSafe(ASC), *GetNameSafe(Owner));
		return;
	}

	const TMap<FName, float> StatsMap = InStatsData->GetAllStatsAsMap();

	TArray<FStatInitializationEntry> ReflectedDefinitions;
	UBaseStatsData::GatherStatDefinitionsFromAttributeSet(SourceClass, ReflectedDefinitions);

	AttributeSet->SetIsInitializingStats(true);

	int32 AppliedCount = 0;
	int32 SkippedCount = 0;

	auto ApplyIfPresent = [&Manager, InStatsData, &StatsMap, &AppliedCount, &SkippedCount](FName StatName, bool bAutoInitializeCurrentFromMax)
	{
		float Value = 0.0f;
		if (!TryGetStatValueForInitialization(Manager, InStatsData, StatsMap, StatName, Value))
		{
			return;
		}

		if (SetNumericAttributeByName(Manager, StatName, Value, bAutoInitializeCurrentFromMax))
		{
			++AppliedCount;
		}
		else
		{
			++SkippedCount;
		}
	};

	auto ApplyCurrent = [&Manager, InStatsData, &StatsMap, &AppliedCount, &SkippedCount](FName CurrentName, FName MaxName)
	{
		if (ApplyCurrentVitalWithClamp(Manager, InStatsData, StatsMap, CurrentName, MaxName, NAME_None))
		{
			++AppliedCount;
		}
		else
		{
			++SkippedCount;
		}
	};

	ApplyIfPresent(TEXT("MaxHealth"), false);
	ApplyIfPresent(TEXT("MaxMana"), false);
	ApplyIfPresent(TEXT("MaxStamina"), false);
	ApplyIfPresent(TEXT("MaxArcaneShield"), false);

	ApplyIfPresent(TEXT("HealthRegenRate"), false);
	ApplyIfPresent(TEXT("HealthRegenAmount"), false);
	ApplyIfPresent(TEXT("ManaRegenRate"), false);
	ApplyIfPresent(TEXT("ManaRegenAmount"), false);
	ApplyIfPresent(TEXT("StaminaRegenRate"), false);
	ApplyIfPresent(TEXT("StaminaRegenAmount"), false);

	for (const TPair<FName, float>& Pair : StatsMap)
	{
		if (StatsInitializerPrivate::IsHandledByOrderedInitialization(Pair.Key))
		{
			continue;
		}

		if (SetNumericAttributeByName(Manager, Pair.Key, Pair.Value, false))
		{
			++AppliedCount;
		}
		else
		{
			++SkippedCount;
		}
	}

	ApplyCurrent(TEXT("Health"), TEXT("MaxHealth"));
	ApplyCurrent(TEXT("Mana"), TEXT("MaxMana"));
	ApplyCurrent(TEXT("Stamina"), TEXT("MaxStamina"));
	ApplyCurrent(TEXT("ArcaneShield"), TEXT("MaxArcaneShield"));

	AttributeSet->SetIsInitializingStats(false);
	AttributeSet->RecalculateAllDerivedVitals();

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(Owner);

	for (TSubclassOf<UGameplayEffect> EffectClass : InStatsData->InitializationEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
		if (SpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	AttributeSet->RecalculateAllDerivedVitals();

	UE_LOG(LogStatsManager, Log,
		TEXT("Stats initialized from %s using AttributeSet %s. Reflected=%d Authored=%d Applied=%d Skipped=%d"),
		*InStatsData->GetName(),
		*GetNameSafe(SourceClass),
		ReflectedDefinitions.Num(),
		StatsMap.Num(),
		AppliedCount,
		SkippedCount);

	Manager.bHasInitializedConfiguredStats = true;
}

void FStatsInitializer::InitializeFromMap(const UStatsManager& Manager, const TMap<FName, float>& StatsMap)
{
	if (!Manager.GetOwner() || !Manager.GetOwner()->HasAuthority())
	{
		PH_LOG_WARNING(LogStatsManager, "InitializeFromMap failed: Must be called on the server.");
		return;
	}

	int32 AppliedCount = 0;
	int32 SkippedCount = 0;

	for (const TPair<FName, float>& Pair : StatsMap)
	{
		if (SetNumericAttributeByName(Manager, Pair.Key, Pair.Value, true))
		{
			++AppliedCount;
		}
		else
		{
			++SkippedCount;
		}
	}

	UE_LOG(LogStatsManager, Log, TEXT("Stats initialized from map. Applied=%d Skipped=%d"), AppliedCount, SkippedCount);
}

void FStatsInitializer::SetStatValue(const UStatsManager& Manager, FName AttributeName, float Value)
{
	SetNumericAttributeByName(Manager, AttributeName, Value, true);
}

bool FStatsInitializer::SetNumericAttributeByName(const UStatsManager& Manager, FName AttributeName, float Value, bool bAutoInitializeCurrentFromMax)
{
	UAbilitySystemComponent* ASC = FStatsAttributeResolver::GetAbilitySystemComponent(Manager);
	if (!ASC)
	{
		PH_LOG_ERROR(LogStatsManager, "SetNumericAttributeByName failed: AbilitySystemComponent was unavailable.");
		return false;
	}

	if (!Manager.GetOwner() || !Manager.GetOwner()->HasAuthority())
	{
		return false;
	}

	FStatInitializationEntry Definition;
	FGameplayAttribute Attribute;
	if (!FStatsAttributeResolver::ResolveAttributeByName(Manager, AttributeName, Attribute, &Definition))
	{
		const FString AssetName = Manager.StatsData ? Manager.StatsData->GetName() : TEXT("None");
		const FString AttributeSetClassName = GetNameSafe(FStatsAttributeResolver::GetSourceAttributeSetClass(Manager));
		PH_LOG_WARNING(LogStatsManager, "SetNumericAttributeByName failed: Could not resolve Stat=%s from StatsData=%s against AttributeSet=%s.", *AttributeName.ToString(), *AssetName, *AttributeSetClassName);
		return false;
	}

	if (!Attribute.IsValid())
	{
		PH_LOG_WARNING(LogStatsManager, "SetNumericAttributeByName failed: Stat=%s resolved to an invalid attribute.", *AttributeName.ToString());
		return false;
	}

	if (!FStatsAttributeResolver::HasLiveAttribute(Manager, Attribute))
	{
		PH_LOG_WARNING(LogStatsManager, "SetNumericAttributeByName failed: Attribute=%s resolved against SourceAttributeSetClass=%s but had no live backing set on ASC=%s for Actor=%s.", *AttributeName.ToString(), *GetNameSafe(FStatsAttributeResolver::GetSourceAttributeSetClass(Manager)), *GetNameSafe(ASC), *GetNameSafe(Manager.GetOwner()));
		return false;
	}

	ASC->SetNumericAttributeBase(Attribute, Value);

	if (!bAutoInitializeCurrentFromMax)
	{
		return true;
	}

	auto InitializePairedCurrentAttribute = [&Manager, ASC, Value](FName CurrentStatName)
	{
		FGameplayAttribute CurrentAttribute;
		if (FStatsAttributeResolver::ResolveAttributeByName(Manager, CurrentStatName, CurrentAttribute) && CurrentAttribute.IsValid())
		{
			ASC->SetNumericAttributeBase(CurrentAttribute, Value);
		}
	};

	if (AttributeName == TEXT("MaxHealth"))
	{
		InitializePairedCurrentAttribute(TEXT("Health"));
	}
	else if (AttributeName == TEXT("MaxMana"))
	{
		InitializePairedCurrentAttribute(TEXT("Mana"));
	}
	else if (AttributeName == TEXT("MaxStamina"))
	{
		InitializePairedCurrentAttribute(TEXT("Stamina"));
	}
	else if (AttributeName == TEXT("MaxArcaneShield"))
	{
		InitializePairedCurrentAttribute(TEXT("ArcaneShield"));
	}

	return true;
}

bool FStatsInitializer::TryGetStatValueForInitialization(const UStatsManager& Manager, const UBaseStatsData* InStatsData, const TMap<FName, float>& StatsMap, FName StatName, float& OutValue)
{
	if (const float* MapValue = StatsMap.Find(StatName))
	{
		OutValue = *MapValue;
		return true;
	}

	if (InStatsData && InStatsData->GetStatValue(StatName, OutValue))
	{
		return true;
	}

	return StatsInitializerPrivate::TryGetFloatPropertyValue(InStatsData, StatName, OutValue);
}

bool FStatsInitializer::ApplyStatIfPresent(const UStatsManager& Manager, const UBaseStatsData* InStatsData, const TMap<FName, float>& StatsMap, FName StatName, bool bAutoInitializeCurrentFromMax)
{
	float Value = 0.0f;
	if (!TryGetStatValueForInitialization(Manager, InStatsData, StatsMap, StatName, Value))
	{
		return false;
	}

	return SetNumericAttributeByName(Manager, StatName, Value, bAutoInitializeCurrentFromMax);
}

bool FStatsInitializer::ApplyCurrentVitalWithClamp(const UStatsManager& Manager, const UBaseStatsData* InStatsData, const TMap<FName, float>& StatsMap, FName CurrentStatName, FName MaxStatName, FName StarterPropertyName)
{
	float CurrentValue = 0.0f;
	bool bHasCurrentValue = TryGetStatValueForInitialization(Manager, InStatsData, StatsMap, CurrentStatName, CurrentValue);

	if (!bHasCurrentValue && StarterPropertyName != NAME_None)
	{
		bHasCurrentValue = StatsInitializerPrivate::TryGetFloatPropertyValue(InStatsData, StarterPropertyName, CurrentValue);
	}

	float MaxValue = 0.0f;
	bool bHasMaxValue = false;

	FGameplayAttribute MaxAttribute;
	if (FStatsAttributeResolver::ResolveAttributeByName(Manager, MaxStatName, MaxAttribute) && MaxAttribute.IsValid())
	{
		MaxValue = FStatsAttributeResolver::GetAttributeValue(Manager, MaxAttribute);
		bHasMaxValue = true;
	}
	else
	{
		bHasMaxValue = TryGetStatValueForInitialization(Manager, InStatsData, StatsMap, MaxStatName, MaxValue);
	}

	if (!bHasCurrentValue)
	{
		if (!bHasMaxValue)
		{
			return false;
		}

		CurrentValue = MaxValue;
	}

	CurrentValue = bHasMaxValue
		? FMath::Clamp(CurrentValue, 0.0f, MaxValue)
		: FMath::Max(CurrentValue, 0.0f);

	return SetNumericAttributeByName(Manager, CurrentStatName, CurrentValue, false);
}

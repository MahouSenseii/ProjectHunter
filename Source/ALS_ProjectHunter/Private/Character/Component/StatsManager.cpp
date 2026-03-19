// Character/Component/StatsManager.cpp

#include "Character/Component/StatsManager.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Character/Component/EquipmentManager.h"
#include "Data/BaseStatsData.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Item/ItemInstance.h"
#include "Item/Library/ItemStructs.h"
#include "Item/Library/AffixEnums.h"
#include "UObject/UnrealType.h"

namespace StatsManagerPrivate
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

DEFINE_LOG_CATEGORY(LogStatsManager);
UStatsManager::UStatsManager()
{
#if UE_BUILD_SHIPPING
	PrimaryComponentTick.bCanEverTick = false;
#else
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
#endif
	// Stats are mutated server-side only; attribute replication is handled by the
	// AttributeSet/ASC, not by this manager component itself.
	SetIsReplicatedByDefault(false);
}

void UStatsManager::BeginPlay()
{
	Super::BeginPlay();

	RefreshCachedAbilitySystemState(TEXT("BeginPlay"));
	TryInitializeConfiguredStats(TEXT("BeginPlay"));
	SetStatsDebugEnabled(DebugManager.bEnableDebug);
}

void UStatsManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if UE_BUILD_SHIPPING
	return;
#endif

	if (!DebugManager.bEnableDebug)
	{
		return;
	}

	DebugManager.DrawDebug(this, this);
}

void UStatsManager::NotifyAbilitySystemReady()
{
	RefreshCachedAbilitySystemState(TEXT("NotifyAbilitySystemReady"));
	TryInitializeConfiguredStats(TEXT("NotifyAbilitySystemReady"));
	SetStatsDebugEnabled(DebugManager.bEnableDebug);
}

void UStatsManager::SetStatsDebugEnabled(bool bEnable)
{
#if UE_BUILD_SHIPPING
	DebugManager.bEnableDebug = false;
	SetComponentTickEnabled(false);
#else
	const bool bWasEnabled = DebugManager.bEnableDebug;
	DebugManager.bEnableDebug = bEnable;

	if (bWasEnabled && !DebugManager.bEnableDebug)
	{
		DebugManager.DrawDebug(this, this);
	}

	SetComponentTickEnabled(DebugManager.bEnableDebug);
#endif
}

void UStatsManager::LogWarningOnce(const FString& Key, const FString& Message) const
{
	if (EmittedWarningKeys.Contains(Key))
	{
		return;
	}

	EmittedWarningKeys.Add(Key);
	UE_LOG(LogStatsManager, Warning, TEXT("%s"), *Message);
}

void UStatsManager::LogAbilitySystemState(const TCHAR* Context, UAbilitySystemComponent* ASC, const UAttributeSet* LiveAttributeSet) const
{
	UE_LOG(
		LogStatsManager,
		Log,
		TEXT("StatsManager[%s]: Owner=%s ASC=%s SourceAttributeSetClass=%s LiveAttributeSet=%s LiveAttributeSetClass=%s"),
		Context,
		*GetNameSafe(GetOwner()),
		*GetNameSafe(ASC),
		*GetNameSafe(GetSourceAttributeSetClass()),
		*GetNameSafe(LiveAttributeSet),
		LiveAttributeSet ? *GetNameSafe(LiveAttributeSet->GetClass()) : TEXT("None"));
}

const UAttributeSet* UStatsManager::GetLiveSourceAttributeSet(UAbilitySystemComponent* ASC, const UClass* DesiredClass, bool bLogIfMissing, FName AttributeName) const
{
	const UClass* DesiredClassPtr = DesiredClass ? DesiredClass : UHunterAttributeSet::StaticClass();
	if (!ASC)
	{
		if (bLogIfMissing)
		{
			LogWarningOnce(
				FString::Printf(TEXT("MissingASC:%s"), *AttributeName.ToString()),
				FString::Printf(
					TEXT("StatsManager: Missing ASC while resolving live AttributeSet on actor=%s SourceAttributeSetClass=%s Attribute=%s"),
					*GetNameSafe(GetOwner()),
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
		LogWarningOnce(
			FString::Printf(TEXT("MissingLiveAttributeSet:%s:%s"), *GetNameSafe(DesiredClassPtr), *AttributeName.ToString()),
			FString::Printf(
				TEXT("StatsManager: No live AttributeSet instance is registered on ASC. Actor=%s ASC=%s SourceAttributeSetClass=%s Attribute=%s SpawnedAttributeCount=%d"),
				*GetNameSafe(GetOwner()),
				*GetNameSafe(ASC),
				*GetNameSafe(DesiredClassPtr),
				AttributeName.IsNone() ? TEXT("None") : *AttributeName.ToString(),
				ASC->GetSpawnedAttributes().Num()));
	}

	return LiveAttributeSet;
}

bool UStatsManager::HasExpectedLiveAttributeSet(bool bLogIfMissing, FName AttributeName) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return GetLiveSourceAttributeSet(ASC, GetSourceAttributeSetClass().Get(), bLogIfMissing, AttributeName) != nullptr;
}

void UStatsManager::RefreshCachedAbilitySystemState(const TCHAR* Context) const
{
	UAbilitySystemComponent* ASC = nullptr;
	if (AActor* Owner = GetOwner())
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
		{
			ASC = ASI->GetAbilitySystemComponent();
		}

		if (!ASC)
		{
			ASC = Owner->FindComponentByClass<UAbilitySystemComponent>();
		}
	}

	const UAttributeSet* LiveAttributeSet = GetLiveSourceAttributeSet(ASC, GetSourceAttributeSetClass().Get(), false);

	CachedASC = ASC;
	CachedAttributeSet = Cast<UHunterAttributeSet>(const_cast<UAttributeSet*>(LiveAttributeSet));

	LogAbilitySystemState(Context, ASC, LiveAttributeSet);

	if (!ASC)
	{
		LogWarningOnce(
			FString::Printf(TEXT("MissingASCState:%s"), Context),
			FString::Printf(
				TEXT("StatsManager[%s]: No ASC found on actor=%s"),
				Context,
				*GetNameSafe(GetOwner())));
	}
	else if (!LiveAttributeSet)
	{
		LogWarningOnce(
			FString::Printf(TEXT("MissingLiveSetState:%s"), Context),
			FString::Printf(
				TEXT("StatsManager[%s]: ASC=%s exists, but SourceAttributeSetClass=%s has no live registered AttributeSet on actor=%s"),
				Context,
				*GetNameSafe(ASC),
				*GetNameSafe(GetSourceAttributeSetClass()),
				*GetNameSafe(GetOwner())));
	}
}

bool UStatsManager::TryInitializeConfiguredStats(const TCHAR* Context)
{
	if (!StatsData)
	{
		LogWarningOnce(
			FString::Printf(TEXT("MissingStatsData:%s"), Context),
			FString::Printf(TEXT("StatsManager[%s]: No StatsData configured on actor=%s"), Context, *GetNameSafe(GetOwner())));
		return false;
	}

	if (bHasInitializedConfiguredStats)
	{
		return true;
	}

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!GetAbilitySystemComponent())
	{
		LogWarningOnce(
			FString::Printf(TEXT("DeferredInitMissingASC:%s"), Context),
			FString::Printf(TEXT("StatsManager[%s]: Deferring stat initialization because ASC is missing on actor=%s"), Context, *GetNameSafe(GetOwner())));
		return false;
	}

	if (!HasExpectedLiveAttributeSet(true))
	{
		LogWarningOnce(
			FString::Printf(TEXT("DeferredInitMissingLiveSet:%s"), Context),
			FString::Printf(
				TEXT("StatsManager[%s]: Deferring stat initialization because SourceAttributeSetClass=%s is not live on ASC=%s for actor=%s"),
				Context,
				*GetNameSafe(GetSourceAttributeSetClass()),
				*GetNameSafe(GetAbilitySystemComponent()),
				*GetNameSafe(GetOwner())));
		return false;
	}

	InitializeFromDataAsset(StatsData);
	return bHasInitializedConfiguredStats;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* EQUIPMENT INTEGRATION                                                   */
/* ═══════════════════════════════════════════════════════════════════════ */

void UStatsManager::ApplyEquipmentStats(UItemInstance* Item)
{
	if (!Item)
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyEquipmentStats: Invalid item"));
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogStatsManager, Error, TEXT("StatsManager::ApplyEquipmentStats: No AbilitySystemComponent"));
		return;
	}

	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyEquipmentStats: Must be called on server"));
		return;
	}

	// Check if already applied
	if (ActiveEquipmentEffects.Contains(Item->UniqueID))
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager: Equipment stats already applied for %s"), *Item->GetName());
		return;
	}

	// Get all stats from item (Prefixes + Suffixes + Implicits + Crafted)
	TArray<FPHAttributeData> AllStats = Item->Stats.GetAllStats();
	
	if (AllStats.Num() == 0)
	{
		UE_LOG(LogStatsManager, Verbose, TEXT("StatsManager: Item %s has no stats to apply"), *Item->GetName());
		return;
	}

	// Create gameplay effect for this item
	FGameplayEffectSpecHandle EffectSpec = CreateEquipmentEffect(Item, AllStats);
	if (!EffectSpec.IsValid())
	{
		UE_LOG(LogStatsManager, Error, TEXT("StatsManager: Failed to create equipment effect for %s"), *Item->GetName());
		return;
	}

	// Apply effect and store handle
	FActiveGameplayEffectHandle EffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());

	if (EffectHandle.IsValid())
	{
		ActiveEquipmentEffects.Add(Item->UniqueID, EffectHandle);
		ActiveEquipmentItems.Add(Item->UniqueID, Item);

		UE_LOG(LogStatsManager, Log, TEXT("StatsManager: Applied %d stats from %s (GUID: %s)"),
			AllStats.Num(), *Item->GetName(), *Item->UniqueID.ToString());
	}
	else
	{
		UE_LOG(LogStatsManager, Error, TEXT("StatsManager: Failed to apply equipment effect for %s"), *Item->GetName());
	}
}

void UStatsManager::RemoveEquipmentStats(UItemInstance* Item)
{
	if (!Item)
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::RemoveEquipmentStats: Invalid item"));
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogStatsManager, Error, TEXT("StatsManager::RemoveEquipmentStats: No AbilitySystemComponent"));
		return;
	}

	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::RemoveEquipmentStats: Must be called on server"));
		return;
	}

	// Find effect handle
	FActiveGameplayEffectHandle* EffectHandle = ActiveEquipmentEffects.Find(Item->UniqueID);
	if (!EffectHandle || !EffectHandle->IsValid())
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager: No active equipment effect found for %s"), *Item->GetName());
		return;
	}

	// Remove effect
	ASC->RemoveActiveGameplayEffect(*EffectHandle);
	ActiveEquipmentEffects.Remove(Item->UniqueID);
	ActiveEquipmentItems.Remove(Item->UniqueID);

	UE_LOG(LogStatsManager, Log, TEXT("StatsManager: Removed equipment stats for %s (GUID: %s)"),
		*Item->GetName(), *Item->UniqueID.ToString());
}

void UStatsManager::RefreshEquipmentStats()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !GetOwner()->HasAuthority())
	{
		return;
	}

	TArray<TObjectPtr<UItemInstance>> ItemsToReapply;
	if (const UEquipmentManager* EquipmentManager = GetOwner()->FindComponentByClass<UEquipmentManager>())
	{
		for (UItemInstance* Item : EquipmentManager->GetAllEquippedItems())
		{
			if (IsValid(Item))
			{
				ItemsToReapply.Add(Item);
			}
		}
	}
	else
	{
		// Fall back to the local cache when an equipment component is not present.
		ActiveEquipmentItems.GenerateValueArray(ItemsToReapply);
	}

	const int32 NumEffects = ActiveEquipmentEffects.Num();

	for (const TPair<FGuid, FActiveGameplayEffectHandle>& Pair : ActiveEquipmentEffects)
	{
		if (Pair.Value.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Pair.Value);
		}
	}

	ActiveEquipmentEffects.Empty();
	ActiveEquipmentItems.Empty();

	int32 Reapplied = 0;
	for (UItemInstance* Item : ItemsToReapply)
	{
		if (IsValid(Item))
		{
			ApplyEquipmentStats(Item);
			++Reapplied;
		}
	}

	UE_LOG(LogStatsManager, Log, TEXT("StatsManager: Refreshed equipment stats (removed %d, reapplied %d)"), NumEffects, Reapplied);
}

bool UStatsManager::HasEquipmentStatsApplied(UItemInstance* Item) const
{
	if (!Item)
	{
		return false;
	}

	return ActiveEquipmentEffects.Contains(Item->UniqueID);
}

bool UStatsManager::ApplyGameplayEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass, float Level)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogStatsManager, Error, TEXT("StatsManager::ApplyGameplayEffectToSelf: No AbilitySystemComponent"));
		return false;
	}

	if (!EffectClass)
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyGameplayEffectToSelf: Invalid effect class"));
		return false;
	}

	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyGameplayEffectToSelf: Must be called on server"));
		return false;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, Level, EffectContext);
	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogStatsManager, Error, TEXT("StatsManager::ApplyGameplayEffectToSelf: Failed to create spec"));
		return false;
	}

	const FActiveGameplayEffectHandle EffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	return EffectHandle.IsValid();
}

bool UStatsManager::ApplyGameplayEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> EffectClass, float Level)
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponent();
	if (!SourceASC)
	{
		UE_LOG(LogStatsManager, Error, TEXT("StatsManager::ApplyGameplayEffectToTarget: No source AbilitySystemComponent"));
		return false;
	}

	if (!TargetActor)
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyGameplayEffectToTarget: Invalid target actor"));
		return false;
	}

	if (!EffectClass)
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyGameplayEffectToTarget: Invalid effect class"));
		return false;
	}

	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyGameplayEffectToTarget: Must be called on server"));
		return false;
	}

	IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(TargetActor);
	if (!TargetASI)
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyGameplayEffectToTarget: Target %s does not implement AbilitySystemInterface"),
			*TargetActor->GetName());
		return false;
	}

	UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
	if (!TargetASC)
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyGameplayEffectToTarget: Target %s has no AbilitySystemComponent"),
			*TargetActor->GetName());
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, Level, EffectContext);
	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogStatsManager, Error, TEXT("StatsManager::ApplyGameplayEffectToTarget: Failed to create spec"));
		return false;
	}

	const FActiveGameplayEffectHandle EffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	return EffectHandle.IsValid();
}

bool UStatsManager::ResolveAttributeByName(FName AttributeName, FGameplayAttribute& OutAttribute) const
{
	return ResolveAttributeByName(AttributeName, OutAttribute, nullptr);
}

bool UStatsManager::ResolveAttributeByName(FName AttributeName, FGameplayAttribute& OutAttribute, FStatInitializationEntry* OutDefinition) const
{
	OutAttribute = FGameplayAttribute();
	if (OutDefinition)
	{
		*OutDefinition = FStatInitializationEntry();
	}

	const TSubclassOf<UAttributeSet> AttributeSetClass = GetSourceAttributeSetClass();
	if (!AttributeSetClass)
	{
		LogWarningOnce(
			FString::Printf(TEXT("ResolveMissingSourceClass:%s"), *AttributeName.ToString()),
			FString::Printf(
				TEXT("ResolveAttributeByName: Missing SourceAttributeSetClass for actor=%s while resolving attribute=%s"),
				*GetNameSafe(GetOwner()),
				*AttributeName.ToString()));
		return false;
	}

	// UHunterAttributeSet is the only AttributeSet in this project that currently exposes
	// a name-based resolver. Additional sets should add their own resolver explicitly.
	if (AttributeSetClass->IsChildOf(UHunterAttributeSet::StaticClass()))
	{
		OutAttribute = UHunterAttributeSet::FindAttributeByName(AttributeName);
	}
	else
	{
		LogWarningOnce(
			FString::Printf(TEXT("ResolveUnsupportedSourceClass:%s:%s"), *GetNameSafe(AttributeSetClass), *AttributeName.ToString()),
			FString::Printf(
				TEXT("ResolveAttributeByName: SourceAttributeSetClass=%s is not supported by UHunterAttributeSet::FindAttributeByName for actor=%s attribute=%s"),
				*GetNameSafe(AttributeSetClass),
				*GetNameSafe(GetOwner()),
				*AttributeName.ToString()));
	}

	if (!OutAttribute.IsValid())
	{
		LogWarningOnce(
			FString::Printf(TEXT("ResolveFailed:%s:%s"), *GetNameSafe(AttributeSetClass), *AttributeName.ToString()),
			FString::Printf(
				TEXT("ResolveAttributeByName: Failed to resolve attribute=%s for actor=%s SourceAttributeSetClass=%s"),
				*AttributeName.ToString(),
				*GetNameSafe(GetOwner()),
				*GetNameSafe(AttributeSetClass)));
		return false;
	}

	if (OutDefinition && StatsData)
	{
		const TArray<FStatInitializationEntry>& Definitions = StatsData->GetBaseAttributes();

		if (const FStatInitializationEntry* Found =
			Definitions.FindByPredicate([&](const FStatInitializationEntry& Entry)
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

		if (const FStatInitializationEntry* Found =
			ReflectedDefinitions.FindByPredicate([&AttributeName](const FStatInitializationEntry& Entry)
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
		*GetNameSafe(GetOwner()),
		*GetNameSafe(AttributeSetClass));

	return true;
}

FGameplayEffectSpecHandle UStatsManager::CreateEquipmentEffect(UItemInstance* Item, const TArray<FPHAttributeData>& Stats)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !Item)
	{
		return FGameplayEffectSpecHandle();
	}

	// FIX: Use unique names based on item GUID to prevent naming collisions
	// Old code used same name for all items, causing issues when multiple items equipped
	FName EffectName = FName(*FString::Printf(TEXT("EquipEffect_%s"), *Item->UniqueID.ToString()));
	
	// Create the effect as a subobject of the owner (not transient package)
	// This ensures proper garbage collection and avoids naming conflicts
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetOwner(), EffectName);
	Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	

	// Add modifiers for each stat on the item (BEFORE creating spec)
	int32 ModifiersAdded = 0;
	for (const FPHAttributeData& Stat : Stats)
	{
		// Skip unidentified affixes
		if (!Stat.bIsIdentified)
		{
			continue;
		}

		// Get attribute (prefer ModifiedAttribute, fall back to AttributeName)
		FGameplayAttribute Attribute = Stat.ModifiedAttribute;
		if (!Attribute.IsValid() && Stat.AttributeName != NAME_None)
		{
			ResolveAttributeByName(Stat.AttributeName, Attribute);
		}

		if (!Attribute.IsValid())
		{
			UE_LOG(LogStatsManager, Warning, TEXT("StatsManager: Invalid attribute for stat '%s'"), 
				*Stat.AttributeName.ToString());
			continue;
		}

		if (ApplyStatModifier(Effect, Stat, Attribute))
		{
			ModifiersAdded++;
		}
	}

	if (ModifiersAdded == 0)
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager: No valid modifiers for item %s"), *Item->GetName());
		return FGameplayEffectSpecHandle();
	}

	// Create effect context
	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(Item);

	// Create spec
	FGameplayEffectSpecHandle SpecHandle;
	SpecHandle.Data = MakeShared<FGameplayEffectSpec>(Effect, EffectContext, 1.0f);
	
	UE_LOG(LogStatsManager, Verbose, TEXT("StatsManager: Created effect '%s' with %d modifiers"), 
		*EffectName.ToString(), ModifiersAdded);

	return SpecHandle;
}

bool UStatsManager::ApplyStatModifier(UGameplayEffect* Effect, const FPHAttributeData& Stat, const FGameplayAttribute& Attribute)
{
	if (!Effect || !Attribute.IsValid())
	{
		return false;
	}

	// Determine modifier operation based on ModifyType
	EGameplayModOp::Type ModOp;
	float FinalValue = Stat.RolledStatValue;

	switch (Stat.ModifyType)
	{
	case EModifyType::MT_Add:
		ModOp = EGameplayModOp::Additive;
		break;

	case EModifyType::MT_Multiply:
		// "Increase" style percentage that folds into the shared multiplicative bucket.
		ModOp = EGameplayModOp::Multiplicitive;
		FinalValue = 1.0f + (FinalValue / 100.0f);
		break;

	case EModifyType::MT_More:
		// "More" is authored as a percent too; normalize it so 15 becomes 1.15x,
		// not 15x. Any higher-level distinction from MT_Multiply is handled upstream.
		ModOp = EGameplayModOp::Multiplicitive;
		FinalValue = 1.0f + (FinalValue / 100.0f);
		break;

	case EModifyType::MT_Override:
		ModOp = EGameplayModOp::Override;
		break;

	default:
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager: Unsupported ModifyType %d for attribute %s"), 
			static_cast<int32>(Stat.ModifyType), *Attribute.GetName());
		return false;
	}

	// Create modifier
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = ModOp;
	Modifier.ModifierMagnitude = FScalableFloat(FinalValue);

	// Add to effect
	Effect->Modifiers.Add(Modifier);

	UE_LOG(LogStatsManager, VeryVerbose, TEXT("StatsManager: Added modifier: %s (%s) = %.2f [Op: %d]"), 
		*Attribute.GetName(), *Stat.AttributeName.ToString(), FinalValue, static_cast<int32>(ModOp));

	return true;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* INTERNAL HELPERS                                                        */
/* ═══════════════════════════════════════════════════════════════════════ */

UHunterAttributeSet* UStatsManager::GetAttributeSet() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return nullptr;
	}

	const UAttributeSet* LiveAttributeSet = GetLiveSourceAttributeSet(ASC, GetSourceAttributeSetClass().Get(), true);
	UHunterAttributeSet* HunterAttributeSet = Cast<UHunterAttributeSet>(const_cast<UAttributeSet*>(LiveAttributeSet));
	if (!HunterAttributeSet && LiveAttributeSet)
	{
		LogWarningOnce(
			FString::Printf(TEXT("LiveSetWrongType:%s"), *GetNameSafe(LiveAttributeSet->GetClass())),
			FString::Printf(
				TEXT("StatsManager: Live AttributeSet is not a UHunterAttributeSet. Actor=%s ASC=%s SourceAttributeSetClass=%s LiveAttributeSetClass=%s"),
				*GetNameSafe(GetOwner()),
				*GetNameSafe(ASC),
				*GetNameSafe(GetSourceAttributeSetClass()),
				*GetNameSafe(LiveAttributeSet->GetClass())));
	}

	CachedAttributeSet = HunterAttributeSet;
	return HunterAttributeSet;
}

UAbilitySystemComponent* UStatsManager::GetAbilitySystemComponent() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	UAbilitySystemComponent* ResolvedASC = nullptr;

	// Try to get ASC from owner (should implement IAbilitySystemInterface)
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
	{
		ResolvedASC = ASI->GetAbilitySystemComponent();
	}

	if (!ResolvedASC)
	{
		ResolvedASC = Owner->FindComponentByClass<UAbilitySystemComponent>();
	}

	CachedASC = ResolvedASC;
	return ResolvedASC;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* PRIMARY ATTRIBUTES (7)                                                  */
/* ═══════════════════════════════════════════════════════════════════════ */

bool UStatsManager::SetNumericAttributeByName(FName AttributeName, float Value, bool bAutoInitializeCurrentFromMax) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogStatsManager, Error, TEXT("SetNumericAttributeByName: No AbilitySystemComponent found"));
		return false;
	}

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	FStatInitializationEntry Definition;
	FGameplayAttribute Attribute;
	if (!ResolveAttributeByName(AttributeName, Attribute, &Definition))
	{
		const FString AssetName = StatsData ? StatsData->GetName() : TEXT("None");
		const FString AttributeSetClassName = GetNameSafe(GetSourceAttributeSetClass());
		UE_LOG(
			LogStatsManager,
			Warning,
			TEXT("SetNumericAttributeByName: Failed to resolve stat '%s' from data asset '%s' against AttributeSet '%s'"),
			*AttributeName.ToString(),
			*AssetName,
			*AttributeSetClassName);
		return false;
	}

	if (!Attribute.IsValid())
	{
		UE_LOG(LogStatsManager, Warning, TEXT("SetNumericAttributeByName: Invalid resolved attribute '%s'"), *AttributeName.ToString());
		return false;
	}

	if (!HasLiveAttribute(Attribute))
	{
		UE_LOG(
			LogStatsManager,
			Warning,
			TEXT("SetNumericAttributeByName: Attribute '%s' resolved against SourceAttributeSetClass=%s but has no live backing AttributeSet on ASC=%s for actor=%s"),
			*AttributeName.ToString(),
			*GetNameSafe(GetSourceAttributeSetClass()),
			*GetNameSafe(ASC),
			*GetNameSafe(GetOwner()));
		return false;
	}

	ASC->SetNumericAttributeBase(Attribute, Value);

	if (!bAutoInitializeCurrentFromMax)
	{
		return true;
	}

	auto InitializePairedCurrentAttribute = [this, ASC, Value](FName CurrentStatName)
	{
		FGameplayAttribute CurrentAttribute;
		if (ResolveAttributeByName(CurrentStatName, CurrentAttribute) && CurrentAttribute.IsValid())
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

TSubclassOf<UAttributeSet> UStatsManager::GetSourceAttributeSetClass() const
{
	if (StatsData)
	{
		return UBaseStatsData::ResolveSourceAttributeSetClass(StatsData);
	}

	if (CachedAttributeSet)
	{
		return CachedAttributeSet->GetClass();
	}

	return UHunterAttributeSet::StaticClass();
}

void UStatsManager::GatherStatDefinitions(TArray<FStatInitializationEntry>& OutDefinitions) const
{
	OutDefinitions.Reset();

	if (StatsData && StatsData->GetBaseAttributes().Num() > 0)
	{
		OutDefinitions = StatsData->GetBaseAttributes();
		return;
	}

	UBaseStatsData::GatherStatDefinitionsFromAttributeSet(GetSourceAttributeSetClass(), OutDefinitions);
}

bool UStatsManager::TryGetStatValueForInitialization(const UBaseStatsData* InStatsData, const TMap<FName, float>& StatsMap, FName StatName, float& OutValue) const
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

	return StatsManagerPrivate::TryGetFloatPropertyValue(InStatsData, StatName, OutValue);
}

bool UStatsManager::ApplyStatIfPresent(const UBaseStatsData* InStatsData, const TMap<FName, float>& StatsMap, FName StatName, bool bAutoInitializeCurrentFromMax) const
{
	float Value = 0.0f;
	if (!TryGetStatValueForInitialization(InStatsData, StatsMap, StatName, Value))
	{
		return false;
	}

	return SetNumericAttributeByName(StatName, Value, bAutoInitializeCurrentFromMax);
}

bool UStatsManager::ApplyCurrentVitalWithClamp(const UBaseStatsData* InStatsData, const TMap<FName, float>& StatsMap, FName CurrentStatName, FName MaxStatName, FName StarterPropertyName) const
{
	float CurrentValue = 0.0f;
	bool bHasCurrentValue = TryGetStatValueForInitialization(InStatsData, StatsMap, CurrentStatName, CurrentValue);

	if (!bHasCurrentValue && StarterPropertyName != NAME_None)
	{
		bHasCurrentValue = StatsManagerPrivate::TryGetFloatPropertyValue(InStatsData, StarterPropertyName, CurrentValue);
	}

	float MaxValue = 0.0f;
	bool bHasMaxValue = false;

	FGameplayAttribute MaxAttribute;
	if (ResolveAttributeByName(MaxStatName, MaxAttribute) && MaxAttribute.IsValid())
	{
		MaxValue = GetAttributeValue(MaxAttribute);
		bHasMaxValue = true;
	}
	else
	{
		bHasMaxValue = TryGetStatValueForInitialization(InStatsData, StatsMap, MaxStatName, MaxValue);
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

	return SetNumericAttributeByName(CurrentStatName, CurrentValue, false);
}

float UStatsManager::GetStrength() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	return AttrSet ? AttrSet->GetStrength() : 0.0f;
}

float UStatsManager::GetIntelligence() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	return AttrSet ? AttrSet->GetIntelligence() : 0.0f;
}

float UStatsManager::GetDexterity() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	return AttrSet ? AttrSet->GetDexterity() : 0.0f;
}

float UStatsManager::GetEndurance() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	return AttrSet ? AttrSet->GetEndurance() : 0.0f;
}

float UStatsManager::GetAffliction() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	return AttrSet ? AttrSet->GetAffliction() : 0.0f;
}

float UStatsManager::GetLuck() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	return AttrSet ? AttrSet->GetLuck() : 0.0f;
}

float UStatsManager::GetCovenant() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	return AttrSet ? AttrSet->GetCovenant() : 0.0f;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* SECONDARY/DERIVED ATTRIBUTES                                            */
/* ═══════════════════════════════════════════════════════════════════════ */

float UStatsManager::GetMagicFind() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	if (!AttrSet)
	{
		return 0.0f;
	}

	// FIX: Added GetMagicFind() for loot system integration
	// If MagicFind attribute exists in your AttributeSet, use it directly:
	// return AttrSet->GetMagicFind();
	
	// Otherwise, derive from Luck (common ARPG formula)
	// MagicFind = Luck * 0.5 (each point of luck gives 0.5% magic find)
	// You may want to add a dedicated MagicFind attribute to HunterAttributeSet
	return GetLuck() * 0.5f;
}

float UStatsManager::GetItemFind() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	if (!AttrSet)
	{
		return 0.0f;
	}

	// Similar to MagicFind - derive from Luck or use dedicated attribute
	return GetLuck() * 0.25f;
}

float UStatsManager::GetGoldFind() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	if (!AttrSet)
	{
		return 0.0f;
	}

	// Derive from Luck
	return GetLuck() * 0.75f;
}

float UStatsManager::GetExperienceBonus() const
{
	// Could be derived from Intelligence or a dedicated attribute
	return 0.0f;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* C ATTRIBUTES                                                        */
/* ═══════════════════════════════════════════════════════════════════════ */

float UStatsManager::GetHealth() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	return AttrSet ? AttrSet->GetHealth() : 0.0f;
}

float UStatsManager::GetMaxHealth() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	return AttrSet ? AttrSet->GetMaxHealth() : 0.0f;
}

float UStatsManager::GetHealthPercent() const
{
	const UHunterAttributeSet* AttrSet = GetAttributeSet();
	const float Max = AttrSet ? AttrSet->GetMaxEffectiveHealth() : 0.0f;
	return Max > 0.0f ? GetHealth() / Max : 0.0f;
}

float UStatsManager::GetMana() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	return AttrSet ? AttrSet->GetMana() : 0.0f;
}

float UStatsManager::GetMaxMana() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	return AttrSet ? AttrSet->GetMaxMana() : 0.0f;
}

float UStatsManager::GetManaPercent() const
{
	const UHunterAttributeSet* AttrSet = GetAttributeSet();
	const float Max = AttrSet ? AttrSet->GetMaxEffectiveMana() : 0.0f;
	return Max > 0.0f ? GetMana() / Max : 0.0f;
}

float UStatsManager::GetStamina() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	return AttrSet ? AttrSet->GetStamina() : 0.0f;
}

float UStatsManager::GetMaxStamina() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	return AttrSet ? AttrSet->GetMaxStamina() : 0.0f;
}

float UStatsManager::GetStaminaPercent() const
{
	const UHunterAttributeSet* AttrSet = GetAttributeSet();
	const float Max = AttrSet ? AttrSet->GetMaxEffectiveStamina() : 0.0f;
	return Max > 0.0f ? GetStamina() / Max : 0.0f;
}


/* ═══════════════════════════════════════════════════════════════════════ */
/* GENERIC ATTRIBUTE ACCESS                                                */
/* ═══════════════════════════════════════════════════════════════════════ */

float UStatsManager::GetAttributeByName(FName AttributeName) const
{
	FGameplayAttribute Attribute;
	if (!ResolveAttributeByName(AttributeName, Attribute) || !Attribute.IsValid())
	{
		return 0.0f;
	}

	return GetAttributeValue(Attribute);
}

bool UStatsManager::MeetsStatRequirements(const TMap<FName, float>& Requirements) const
{
	for (const TPair<FName, float>& Requirement : Requirements)
	{
		float CurrentValue = GetAttributeByName(Requirement.Key);
		
		if (CurrentValue < Requirement.Value)
		{
			return false;
		}
	}

	return true;
}

float UStatsManager::GetAttributeValue(const FGameplayAttribute& Attribute) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !Attribute.IsValid())
	{
		return 0.0f;
	}

	if (!HasLiveAttribute(Attribute))
	{
		return 0.0f;
	}

	return ASC->GetNumericAttribute(Attribute);
}

bool UStatsManager::HasLiveAttribute(const FGameplayAttribute& Attribute) const
{
	const FString AttributeName = Attribute.IsValid() ? Attribute.GetName() : TEXT("Invalid");
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		LogWarningOnce(
			FString::Printf(TEXT("HasLiveAttributeMissingASC:%s"), *AttributeName),
			FString::Printf(
				TEXT("HasLiveAttribute: No ASC for actor=%s while checking attribute=%s SourceAttributeSetClass=%s"),
				*GetNameSafe(GetOwner()),
				*AttributeName,
				*GetNameSafe(GetSourceAttributeSetClass())));
		return false;
	}

	if (!Attribute.IsValid())
	{
		LogWarningOnce(
			TEXT("HasLiveAttributeInvalidAttribute"),
			FString::Printf(TEXT("HasLiveAttribute: Invalid gameplay attribute supplied for actor=%s"), *GetNameSafe(GetOwner())));
		return false;
	}

	const UClass* AttributeSetClass = Attribute.GetAttributeSetClass();
	const UAttributeSet* LiveAttributeSet = GetLiveSourceAttributeSet(ASC, AttributeSetClass ? AttributeSetClass : GetSourceAttributeSetClass().Get(), true, FName(*AttributeName));
	const bool bHasLiveAttribute = LiveAttributeSet && ASC->HasAttributeSetForAttribute(Attribute);
	if (!bHasLiveAttribute)
	{
		LogWarningOnce(
			FString::Printf(TEXT("HasLiveAttributeMissingLiveSet:%s"), *AttributeName),
			FString::Printf(
				TEXT("HasLiveAttribute: Attribute=%s has no live backing set. Actor=%s ASC=%s SourceAttributeSetClass=%s AttributeSetClass=%s LiveAttributeSet=%s"),
				*AttributeName,
				*GetNameSafe(GetOwner()),
				*GetNameSafe(ASC),
				*GetNameSafe(GetSourceAttributeSetClass()),
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
			*GetNameSafe(GetOwner()),
			*GetNameSafe(ASC),
			*GetNameSafe(LiveAttributeSet));
	}

	return bHasLiveAttribute;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* POWER CALCULATIONS                                                      */
/* ═══════════════════════════════════════════════════════════════════════ */

float UStatsManager::GetPowerLevel() const
{
	// Simple power calculation based on primary stats
	float TotalPrimary = GetStrength() + GetIntelligence() + GetDexterity() + 
	                     GetEndurance() + GetAffliction() + GetLuck() + GetCovenant();
	
	float VitalBonus = GetMaxHealth() * 0.01f + GetMaxMana() * 0.01f;
	
	return TotalPrimary + VitalBonus;
}

float UStatsManager::GetPowerRatioAgainst(AActor* OtherActor) const
{
	if (!OtherActor)
	{
		return 1.0f;
	}

	UStatsManager* OtherStats = OtherActor->FindComponentByClass<UStatsManager>();
	if (!OtherStats)
	{
		return 1.0f;
	}

	float MyPower = GetPowerLevel();
	float TheirPower = OtherStats->GetPowerLevel();

	if (TheirPower <= 0.0f)
	{
		return 1.0f;
	}

	return MyPower / TheirPower;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* STAT INITIALIZATION                                                     */
/* ═══════════════════════════════════════════════════════════════════════ */

void UStatsManager::InitializeFromDataAsset(UBaseStatsData* InStatsData)
{
    if (!InStatsData)
    {
        UE_LOG(LogStatsManager, Warning, TEXT("InitializeFromDataAsset: StatsData is null"));
        return;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        UE_LOG(LogStatsManager, Error, TEXT("InitializeFromDataAsset: Owner is null"));
        return;
    }

    UE_LOG(LogStatsManager, Log, TEXT("InitializeFromDataAsset: Begin asset=%s owner=%s"),
        *GetNameSafe(InStatsData), *GetNameSafe(Owner));

    StatsData = InStatsData;
    RefreshCachedAbilitySystemState(TEXT("InitializeFromDataAsset"));

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        UE_LOG(LogStatsManager, Error, TEXT("InitializeFromDataAsset: No AbilitySystemComponent found"));
        return;
    }

    if (!Owner->HasAuthority())
    {
        UE_LOG(LogStatsManager, Warning, TEXT("InitializeFromDataAsset: Must be called on server"));
        return;
    }

    const TSubclassOf<UAttributeSet> SourceClass = GetSourceAttributeSetClass();
    if (!SourceClass)
    {
        UE_LOG(LogStatsManager, Error, TEXT("InitializeFromDataAsset: No valid SourceAttributeSetClass for asset=%s"),
            *GetNameSafe(InStatsData));
        return;
    }

	UHunterAttributeSet* AttributeSet =
	 const_cast<UHunterAttributeSet*>(ASC->GetSet<UHunterAttributeSet>());
    if (!AttributeSet)
    {
        UE_LOG(LogStatsManager, Error,
            TEXT("InitializeFromDataAsset: ASC=%s has no live UHunterAttributeSet for owner=%s sourceClass=%s"),
            *GetNameSafe(ASC), *GetNameSafe(Owner), *GetNameSafe(SourceClass));
        return;
    }

    CachedASC = ASC;
    CachedAttributeSet = AttributeSet;

    if (!HasExpectedLiveAttributeSet(true))
    {
        UE_LOG(LogStatsManager, Error,
            TEXT("InitializeFromDataAsset: SourceAttributeSetClass=%s is not registered live on ASC=%s for actor=%s"),
            *GetNameSafe(SourceClass), *GetNameSafe(ASC), *GetNameSafe(Owner));
        return;
    }

    const TMap<FName, float> StatsMap = InStatsData->GetAllStatsAsMap();

    TArray<FStatInitializationEntry> ReflectedDefinitions;
    UBaseStatsData::GatherStatDefinitionsFromAttributeSet(SourceClass, ReflectedDefinitions);

    AttributeSet->SetIsInitializingStats(true);

    int32 AppliedCount = 0;
    int32 SkippedCount = 0;

    auto ApplyIfPresent = [this, InStatsData, &StatsMap, &AppliedCount, &SkippedCount](FName StatName, bool bAutoInitializeCurrentFromMax)
    {
        float Value = 0.0f;
        if (!TryGetStatValueForInitialization(InStatsData, StatsMap, StatName, Value))
        {
            return;
        }

        if (SetNumericAttributeByName(StatName, Value, bAutoInitializeCurrentFromMax))
        {
            ++AppliedCount;
        }
        else
        {
            ++SkippedCount;
        }
    };

    auto ApplyCurrent = [this, InStatsData, &StatsMap, &AppliedCount, &SkippedCount](FName CurrentName, FName MaxName)
    {
        if (ApplyCurrentVitalWithClamp(InStatsData, StatsMap, CurrentName, MaxName, NAME_None))
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
        if (StatsManagerPrivate::IsHandledByOrderedInitialization(Pair.Key))
        {
            continue;
        }

        if (SetNumericAttributeByName(Pair.Key, Pair.Value, false))
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

    bHasInitializedConfiguredStats = true;
}
void UStatsManager::InitializeFromMap(const TMap<FName, float>& StatsMap) const
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogStatsManager, Warning, TEXT("InitializeFromMap: Must be called on server"));
		return;
	}

	int32 AppliedCount = 0;
	int32 SkippedCount = 0;

	for (const TPair<FName, float>& Pair : StatsMap)
	{
		if (SetNumericAttributeByName(Pair.Key, Pair.Value, true))
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

void UStatsManager::SetStatValue(FName AttributeName, float Value) const
{
	SetNumericAttributeByName(AttributeName, Value, true);
}

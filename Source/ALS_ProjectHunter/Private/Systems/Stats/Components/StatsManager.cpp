// Character/Component/StatsManager.cpp

#include "Systems/Stats/Components/StatsManager.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Item/ItemInstance.h"
#include "Systems/Stats/EquipmentStatsApplier.h"
#include "Systems/Stats/StatsAttributeResolver.h"
#include "Systems/Stats/StatsInitializer.h"

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

void UStatsManager::ResetStatsInitialization()
{
	// Clear the one-time guard so the next NotifyAbilitySystemReady() call
	// fully re-runs InitializeFromDataAsset.  Required for pool-recycled mobs
	// whose first life already set bHasInitializedConfiguredStats = true.
	bHasInitializedConfiguredStats = false;
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
	PH_LOG_WARNING(LogStatsManager, "%s", *Message);
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
	return FStatsAttributeResolver::GetLiveSourceAttributeSet(*this, ASC, DesiredClass, bLogIfMissing, AttributeName);
}

bool UStatsManager::HasExpectedLiveAttributeSet(bool bLogIfMissing, FName AttributeName) const
{
	return FStatsAttributeResolver::HasExpectedLiveAttributeSet(*this, bLogIfMissing, AttributeName);
}

void UStatsManager::RefreshCachedAbilitySystemState(const TCHAR* Context) const
{
	FStatsAttributeResolver::RefreshCachedAbilitySystemState(*this, Context);
}

bool UStatsManager::TryInitializeConfiguredStats(const TCHAR* Context)
{
	return FStatsInitializer::TryInitializeConfiguredStats(*this, Context);
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* EQUIPMENT INTEGRATION                                                   */
/* ═══════════════════════════════════════════════════════════════════════ */

void UStatsManager::ApplyEquipmentStats(UItemInstance* Item)
{
	FEquipmentStatsApplier::ApplyEquipmentStats(*this, Item);
}

void UStatsManager::RemoveEquipmentStats(UItemInstance* Item)
{
	FEquipmentStatsApplier::RemoveEquipmentStats(*this, Item);
}

void UStatsManager::RefreshEquipmentStats()
{
	FEquipmentStatsApplier::RefreshEquipmentStats(*this);
}

void UStatsManager::HandleEquipmentChanged(EEquipmentSlot Slot, UItemInstance* NewItem, UItemInstance* OldItem)
{
	(void)Slot;
	FEquipmentStatsApplier::HandleEquipmentChanged(*this, NewItem, OldItem);
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
		PH_LOG_ERROR(LogStatsManager, "ApplyGameplayEffectToSelf failed: AbilitySystemComponent was unavailable.");
		return false;
	}

	if (!EffectClass)
	{
		PH_LOG_WARNING(LogStatsManager, "ApplyGameplayEffectToSelf failed: EffectClass was invalid.");
		return false;
	}

	if (!GetOwner()->HasAuthority())
	{
		PH_LOG_WARNING(LogStatsManager, "ApplyGameplayEffectToSelf failed: Must be called on the server.");
		return false;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, Level, EffectContext);
	if (!SpecHandle.IsValid())
	{
		PH_LOG_ERROR(LogStatsManager, "ApplyGameplayEffectToSelf failed: Could not create an outgoing spec.");
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
		PH_LOG_ERROR(LogStatsManager, "ApplyGameplayEffectToTarget failed: Source AbilitySystemComponent was unavailable.");
		return false;
	}

	if (!TargetActor)
	{
		PH_LOG_WARNING(LogStatsManager, "ApplyGameplayEffectToTarget failed: TargetActor was invalid.");
		return false;
	}

	if (!EffectClass)
	{
		PH_LOG_WARNING(LogStatsManager, "ApplyGameplayEffectToTarget failed: EffectClass was invalid.");
		return false;
	}

	if (!GetOwner()->HasAuthority())
	{
		PH_LOG_WARNING(LogStatsManager, "ApplyGameplayEffectToTarget failed: Must be called on the server.");
		return false;
	}

	IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(TargetActor);
	if (!TargetASI)
	{
		PH_LOG_WARNING(LogStatsManager, "ApplyGameplayEffectToTarget failed: Target=%s does not implement AbilitySystemInterface.", *TargetActor->GetName());
		return false;
	}

	UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
	if (!TargetASC)
	{
		PH_LOG_WARNING(LogStatsManager, "ApplyGameplayEffectToTarget failed: Target=%s has no AbilitySystemComponent.", *TargetActor->GetName());
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, Level, EffectContext);
	if (!SpecHandle.IsValid())
	{
		PH_LOG_ERROR(LogStatsManager, "ApplyGameplayEffectToTarget failed: Could not create an outgoing spec.");
		return false;
	}

	const FActiveGameplayEffectHandle EffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	return EffectHandle.IsValid();
}

bool UStatsManager::ResolveAttributeByName(FName AttributeName, FGameplayAttribute& OutAttribute) const
{
	return FStatsAttributeResolver::ResolveAttributeByName(*this, AttributeName, OutAttribute, nullptr);
}

bool UStatsManager::ResolveAttributeByName(FName AttributeName, FGameplayAttribute& OutAttribute, FStatInitializationEntry* OutDefinition) const
{
	return FStatsAttributeResolver::ResolveAttributeByName(*this, AttributeName, OutAttribute, OutDefinition);
}

FGameplayEffectSpecHandle UStatsManager::CreateEquipmentEffect(UItemInstance* Item, const TArray<FPHAttributeData>& Stats)
{
	return FEquipmentStatsApplier::CreateEquipmentEffect(*this, Item, Stats);
}

bool UStatsManager::ApplyStatModifier(UGameplayEffect* Effect, const FPHAttributeData& Stat, const FGameplayAttribute& Attribute)
{
	return FEquipmentStatsApplier::ApplyStatModifier(Effect, Stat, Attribute);
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* INTERNAL HELPERS                                                        */
/* ═══════════════════════════════════════════════════════════════════════ */

UHunterAttributeSet* UStatsManager::GetAttributeSet() const
{
	return FStatsAttributeResolver::GetAttributeSet(*this);
}

UAbilitySystemComponent* UStatsManager::GetAbilitySystemComponent() const
{
	return FStatsAttributeResolver::GetAbilitySystemComponent(*this);
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* PRIMARY ATTRIBUTES (7)                                                  */
/* ═══════════════════════════════════════════════════════════════════════ */

bool UStatsManager::SetNumericAttributeByName(FName AttributeName, float Value, bool bAutoInitializeCurrentFromMax) const
{
	return FStatsInitializer::SetNumericAttributeByName(*this, AttributeName, Value, bAutoInitializeCurrentFromMax);
}

TSubclassOf<UAttributeSet> UStatsManager::GetSourceAttributeSetClass() const
{
	return FStatsAttributeResolver::GetSourceAttributeSetClass(*this);
}

void UStatsManager::GatherStatDefinitions(TArray<FStatInitializationEntry>& OutDefinitions) const
{
	FStatsAttributeResolver::GatherStatDefinitions(*this, OutDefinitions);
}

bool UStatsManager::TryGetStatValueForInitialization(const UBaseStatsData* InStatsData, const TMap<FName, float>& StatsMap, FName StatName, float& OutValue) const
{
	return FStatsInitializer::TryGetStatValueForInitialization(*this, InStatsData, StatsMap, StatName, OutValue);
}

bool UStatsManager::ApplyStatIfPresent(const UBaseStatsData* InStatsData, const TMap<FName, float>& StatsMap, FName StatName, bool bAutoInitializeCurrentFromMax) const
{
	return FStatsInitializer::ApplyStatIfPresent(*this, InStatsData, StatsMap, StatName, bAutoInitializeCurrentFromMax);
}

bool UStatsManager::ApplyCurrentVitalWithClamp(const UBaseStatsData* InStatsData, const TMap<FName, float>& StatsMap, FName CurrentStatName, FName MaxStatName, FName StarterPropertyName) const
{
	return FStatsInitializer::ApplyCurrentVitalWithClamp(*this, InStatsData, StatsMap, CurrentStatName, MaxStatName, StarterPropertyName);
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
	return FStatsAttributeResolver::GetAttributeValue(*this, Attribute);
}

bool UStatsManager::HasLiveAttribute(const FGameplayAttribute& Attribute) const
{
	return FStatsAttributeResolver::HasLiveAttribute(*this, Attribute);
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
	FStatsInitializer::InitializeFromDataAsset(*this, InStatsData);
}
void UStatsManager::InitializeFromMap(const TMap<FName, float>& StatsMap) const
{
	FStatsInitializer::InitializeFromMap(*this, StatsMap);
}

void UStatsManager::SetStatValue(FName AttributeName, float Value) const
{
	FStatsInitializer::SetStatValue(*this, AttributeName, Value);
}

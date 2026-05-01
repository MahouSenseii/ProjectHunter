// Character/Component/StatsManager.cpp

#include "Stats/Components/StatsManager.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Item/ItemInstance.h"
#include "Stats/EquipmentStatsApplier.h"
#include "Stats/StatsAttributeResolver.h"
#include "Stats/StatsInitializer.h"

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
	const UHunterAttributeSet* AttrSet = GetAttributeSet();
	if (!AttrSet)
	{
		return 0.0f;
	}

	const float IncreasedMultiplier = 1.0f + (AttrSet->GetGlobalXPGain() + AttrSet->GetLocalXPGain()) / 100.0f;
	const float MoreMultiplier = FMath::Max(AttrSet->GetXPGainMultiplier(), 1.0f);
	const float PenaltyMultiplier = FMath::Max(AttrSet->GetXPPenalty(), 0.0f);
	return (IncreasedMultiplier * MoreMultiplier * PenaltyMultiplier - 1.0f) * 100.0f;
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

float UStatsManager::GetAttributeByType(EHunterAttribute AttributeType) const
{
	// Direct accessor path — no string lookup, no FGameplayAttribute resolution.
	// Calls the typed getter on UHunterAttributeSet directly for maximum speed.
	const UHunterAttributeSet* Attrs = GetAttributeSet();
	if (!Attrs)
	{
		UE_LOG(LogStatsManager, Warning,
			TEXT("GetAttributeByType: HunterAttributeSet not available on %s."),
			*GetNameSafe(GetOwner()));
		return 0.f;
	}

	switch (AttributeType)
	{
	// ── Primary ───────────────────────────────────────────────────────────────
	case EHunterAttribute::Strength:             return Attrs->GetStrength();
	case EHunterAttribute::Intelligence:         return Attrs->GetIntelligence();
	case EHunterAttribute::Dexterity:            return Attrs->GetDexterity();
	case EHunterAttribute::Endurance:            return Attrs->GetEndurance();
	case EHunterAttribute::Affliction:           return Attrs->GetAffliction();
	case EHunterAttribute::Luck:                 return Attrs->GetLuck();
	case EHunterAttribute::Covenant:             return Attrs->GetCovenant();

	// ── Progression ───────────────────────────────────────────────────────────
	case EHunterAttribute::PlayerLevel:          return Attrs->GetPlayerLevel();

	// ── Experience ────────────────────────────────────────────────────────────
	case EHunterAttribute::GlobalXPGain:         return Attrs->GetGlobalXPGain();
	case EHunterAttribute::LocalXPGain:          return Attrs->GetLocalXPGain();
	case EHunterAttribute::XPGainMultiplier:     return Attrs->GetXPGainMultiplier();
	case EHunterAttribute::XPPenalty:            return Attrs->GetXPPenalty();

	// ── Health ────────────────────────────────────────────────────────────────
	case EHunterAttribute::Health:                      return Attrs->GetHealth();
	case EHunterAttribute::MaxHealth:                   return Attrs->GetMaxHealth();
	case EHunterAttribute::MaxEffectiveHealth:          return Attrs->GetMaxEffectiveHealth();
	case EHunterAttribute::HealthRegenRate:             return Attrs->GetHealthRegenRate();
	case EHunterAttribute::MaxHealthRegenRate:          return Attrs->GetMaxHealthRegenRate();
	case EHunterAttribute::HealthRegenAmount:           return Attrs->GetHealthRegenAmount();
	case EHunterAttribute::MaxHealthRegenAmount:        return Attrs->GetMaxHealthRegenAmount();
	case EHunterAttribute::ReservedHealth:              return Attrs->GetReservedHealth();
	case EHunterAttribute::MaxReservedHealth:           return Attrs->GetMaxReservedHealth();
	case EHunterAttribute::FlatReservedHealth:          return Attrs->GetFlatReservedHealth();
	case EHunterAttribute::PercentageReservedHealth:    return Attrs->GetPercentageReservedHealth();

	// ── Stamina ───────────────────────────────────────────────────────────────
	case EHunterAttribute::Stamina:                     return Attrs->GetStamina();
	case EHunterAttribute::MaxStamina:                  return Attrs->GetMaxStamina();
	case EHunterAttribute::MaxEffectiveStamina:         return Attrs->GetMaxEffectiveStamina();
	case EHunterAttribute::StaminaRegenRate:            return Attrs->GetStaminaRegenRate();
	case EHunterAttribute::StaminaDegenRate:            return Attrs->GetStaminaDegenRate();
	case EHunterAttribute::MaxStaminaRegenRate:         return Attrs->GetMaxStaminaRegenRate();
	case EHunterAttribute::StaminaRegenAmount:          return Attrs->GetStaminaRegenAmount();
	case EHunterAttribute::StaminaDegenAmount:          return Attrs->GetStaminaDegenAmount();
	case EHunterAttribute::MaxStaminaRegenAmount:       return Attrs->GetMaxStaminaRegenAmount();
	case EHunterAttribute::ReservedStamina:             return Attrs->GetReservedStamina();
	case EHunterAttribute::MaxReservedStamina:          return Attrs->GetMaxReservedStamina();
	case EHunterAttribute::FlatReservedStamina:         return Attrs->GetFlatReservedStamina();
	case EHunterAttribute::PercentageReservedStamina:   return Attrs->GetPercentageReservedStamina();

	// ── Mana ──────────────────────────────────────────────────────────────────
	case EHunterAttribute::Mana:                        return Attrs->GetMana();
	case EHunterAttribute::MaxMana:                     return Attrs->GetMaxMana();
	case EHunterAttribute::MaxEffectiveMana:            return Attrs->GetMaxEffectiveMana();
	case EHunterAttribute::ManaRegenRate:               return Attrs->GetManaRegenRate();
	case EHunterAttribute::MaxManaRegenRate:            return Attrs->GetMaxManaRegenRate();
	case EHunterAttribute::ManaRegenAmount:             return Attrs->GetManaRegenAmount();
	case EHunterAttribute::MaxManaRegenAmount:          return Attrs->GetMaxManaRegenAmount();
	case EHunterAttribute::ReservedMana:                return Attrs->GetReservedMana();
	case EHunterAttribute::MaxReservedMana:             return Attrs->GetMaxReservedMana();
	case EHunterAttribute::FlatReservedMana:            return Attrs->GetFlatReservedMana();
	case EHunterAttribute::PercentageReservedMana:      return Attrs->GetPercentageReservedMana();

	// ── Arcane Shield ─────────────────────────────────────────────────────────
	case EHunterAttribute::ArcaneShield:                    return Attrs->GetArcaneShield();
	case EHunterAttribute::MaxArcaneShield:                 return Attrs->GetMaxArcaneShield();
	case EHunterAttribute::MaxEffectiveArcaneShield:        return Attrs->GetMaxEffectiveArcaneShield();
	case EHunterAttribute::ArcaneShieldRegenRate:           return Attrs->GetArcaneShieldRegenRate();
	case EHunterAttribute::MaxArcaneShieldRegenRate:        return Attrs->GetMaxArcaneShieldRegenRate();
	case EHunterAttribute::ArcaneShieldRegenAmount:         return Attrs->GetArcaneShieldRegenAmount();
	case EHunterAttribute::MaxArcaneShieldRegenAmount:      return Attrs->GetMaxArcaneShieldRegenAmount();
	case EHunterAttribute::ReservedArcaneShield:            return Attrs->GetReservedArcaneShield();
	case EHunterAttribute::MaxReservedArcaneShield:         return Attrs->GetMaxReservedArcaneShield();
	case EHunterAttribute::FlatReservedArcaneShield:        return Attrs->GetFlatReservedArcaneShield();
	case EHunterAttribute::PercentageReservedArcaneShield:  return Attrs->GetPercentageReservedArcaneShield();

	// ── Damage — Global ───────────────────────────────────────────────────────
	case EHunterAttribute::GlobalDamages:           return Attrs->GetGlobalDamages();
	case EHunterAttribute::GlobalMoreDamage:        return Attrs->GetGlobalMoreDamage();
	case EHunterAttribute::DamageBonusWhileAtFullHP: return Attrs->GetDamageBonusWhileAtFullHP();
	case EHunterAttribute::DamageBonusWhileAtLowHP:  return Attrs->GetDamageBonusWhileAtLowHP();

	// ── Damage — Physical ─────────────────────────────────────────────────────
	case EHunterAttribute::MinPhysicalDamage:       return Attrs->GetMinPhysicalDamage();
	case EHunterAttribute::MaxPhysicalDamage:       return Attrs->GetMaxPhysicalDamage();
	case EHunterAttribute::PhysicalFlatDamage:      return Attrs->GetPhysicalFlatDamage();
	case EHunterAttribute::PhysicalPercentDamage:   return Attrs->GetPhysicalPercentDamage();
	case EHunterAttribute::PhysicalMoreDamage:      return Attrs->GetPhysicalMoreDamage();

	// ── Damage — Fire ─────────────────────────────────────────────────────────
	case EHunterAttribute::MinFireDamage:           return Attrs->GetMinFireDamage();
	case EHunterAttribute::MaxFireDamage:           return Attrs->GetMaxFireDamage();
	case EHunterAttribute::FireFlatDamage:          return Attrs->GetFireFlatDamage();
	case EHunterAttribute::FirePercentDamage:       return Attrs->GetFirePercentDamage();
	case EHunterAttribute::FireMoreDamage:          return Attrs->GetFireMoreDamage();

	// ── Damage — Ice ──────────────────────────────────────────────────────────
	case EHunterAttribute::MinIceDamage:            return Attrs->GetMinIceDamage();
	case EHunterAttribute::MaxIceDamage:            return Attrs->GetMaxIceDamage();
	case EHunterAttribute::IceFlatDamage:           return Attrs->GetIceFlatDamage();
	case EHunterAttribute::IcePercentDamage:        return Attrs->GetIcePercentDamage();
	case EHunterAttribute::IceMoreDamage:           return Attrs->GetIceMoreDamage();

	// ── Damage — Lightning ────────────────────────────────────────────────────
	case EHunterAttribute::MinLightningDamage:      return Attrs->GetMinLightningDamage();
	case EHunterAttribute::MaxLightningDamage:      return Attrs->GetMaxLightningDamage();
	case EHunterAttribute::LightningFlatDamage:     return Attrs->GetLightningFlatDamage();
	case EHunterAttribute::LightningPercentDamage:  return Attrs->GetLightningPercentDamage();
	case EHunterAttribute::LightningMoreDamage:     return Attrs->GetLightningMoreDamage();

	// ── Damage — Light ────────────────────────────────────────────────────────
	case EHunterAttribute::MinLightDamage:          return Attrs->GetMinLightDamage();
	case EHunterAttribute::MaxLightDamage:          return Attrs->GetMaxLightDamage();
	case EHunterAttribute::LightFlatDamage:         return Attrs->GetLightFlatDamage();
	case EHunterAttribute::LightPercentDamage:      return Attrs->GetLightPercentDamage();
	case EHunterAttribute::LightMoreDamage:         return Attrs->GetLightMoreDamage();

	// ── Damage — Corruption ───────────────────────────────────────────────────
	case EHunterAttribute::MinCorruptionDamage:     return Attrs->GetMinCorruptionDamage();
	case EHunterAttribute::MaxCorruptionDamage:     return Attrs->GetMaxCorruptionDamage();
	case EHunterAttribute::CorruptionFlatDamage:    return Attrs->GetCorruptionFlatDamage();
	case EHunterAttribute::CorruptionPercentDamage: return Attrs->GetCorruptionPercentDamage();
	case EHunterAttribute::CorruptionMoreDamage:    return Attrs->GetCorruptionMoreDamage();

	// ── Damage — Elemental ────────────────────────────────────────────────────
	case EHunterAttribute::ElementalMoreDamage:     return Attrs->GetElementalMoreDamage();
	case EHunterAttribute::ElementalDamage:         return Attrs->GetElementalDamage();

	// ── Offensive Stats ───────────────────────────────────────────────────────
	case EHunterAttribute::AreaDamage:              return Attrs->GetAreaDamage();
	case EHunterAttribute::AreaOfEffect:            return Attrs->GetAreaOfEffect();
	case EHunterAttribute::AttackRange:             return Attrs->GetAttackRange();
	case EHunterAttribute::AttackSpeed:             return Attrs->GetAttackSpeed();
	case EHunterAttribute::CastSpeed:               return Attrs->GetCastSpeed();
	case EHunterAttribute::CritChance:              return Attrs->GetCritChance();
	case EHunterAttribute::CritMultiplier:          return Attrs->GetCritMultiplier();
	case EHunterAttribute::DamageOverTime:          return Attrs->GetDamageOverTime();
	case EHunterAttribute::MeleeDamage:             return Attrs->GetMeleeDamage();
	case EHunterAttribute::SpellDamage:             return Attrs->GetSpellDamage();
	case EHunterAttribute::ProjectileCount:         return Attrs->GetProjectileCount();
	case EHunterAttribute::ProjectileSpeed:         return Attrs->GetProjectileSpeed();
	case EHunterAttribute::RangedDamage:            return Attrs->GetRangedDamage();
	case EHunterAttribute::SpellsCritChance:        return Attrs->GetSpellsCritChance();
	case EHunterAttribute::SpellsCritMultiplier:    return Attrs->GetSpellsCritMultiplier();
	case EHunterAttribute::ChainCount:              return Attrs->GetChainCount();
	case EHunterAttribute::ForkCount:               return Attrs->GetForkCount();
	case EHunterAttribute::ChainDamage:             return Attrs->GetChainDamage();

	// ── Damage Conversion ─────────────────────────────────────────────────────
	case EHunterAttribute::PhysicalToFire:          return Attrs->GetPhysicalToFire();
	case EHunterAttribute::PhysicalToIce:           return Attrs->GetPhysicalToIce();
	case EHunterAttribute::PhysicalToLightning:     return Attrs->GetPhysicalToLightning();
	case EHunterAttribute::PhysicalToLight:         return Attrs->GetPhysicalToLight();
	case EHunterAttribute::PhysicalToCorruption:    return Attrs->GetPhysicalToCorruption();
	case EHunterAttribute::FireToPhysical:          return Attrs->GetFireToPhysical();
	case EHunterAttribute::FireToIce:               return Attrs->GetFireToIce();
	case EHunterAttribute::FireToLightning:         return Attrs->GetFireToLightning();
	case EHunterAttribute::FireToLight:             return Attrs->GetFireToLight();
	case EHunterAttribute::FireToCorruption:        return Attrs->GetFireToCorruption();
	case EHunterAttribute::IceToPhysical:           return Attrs->GetIceToPhysical();
	case EHunterAttribute::IceToFire:               return Attrs->GetIceToFire();
	case EHunterAttribute::IceToLightning:          return Attrs->GetIceToLightning();
	case EHunterAttribute::IceToLight:              return Attrs->GetIceToLight();
	case EHunterAttribute::IceToCorruption:         return Attrs->GetIceToCorruption();
	case EHunterAttribute::LightningToPhysical:     return Attrs->GetLightningToPhysical();
	case EHunterAttribute::LightningToFire:         return Attrs->GetLightningToFire();
	case EHunterAttribute::LightningToIce:          return Attrs->GetLightningToIce();
	case EHunterAttribute::LightningToLight:        return Attrs->GetLightningToLight();
	case EHunterAttribute::LightningToCorruption:   return Attrs->GetLightningToCorruption();
	case EHunterAttribute::LightToPhysical:         return Attrs->GetLightToPhysical();
	case EHunterAttribute::LightToFire:             return Attrs->GetLightToFire();
	case EHunterAttribute::LightToIce:              return Attrs->GetLightToIce();
	case EHunterAttribute::LightToLightning:        return Attrs->GetLightToLightning();
	case EHunterAttribute::LightToCorruption:       return Attrs->GetLightToCorruption();
	case EHunterAttribute::CorruptionToPhysical:    return Attrs->GetCorruptionToPhysical();
	case EHunterAttribute::CorruptionToFire:        return Attrs->GetCorruptionToFire();
	case EHunterAttribute::CorruptionToIce:         return Attrs->GetCorruptionToIce();
	case EHunterAttribute::CorruptionToLightning:   return Attrs->GetCorruptionToLightning();
	case EHunterAttribute::CorruptionToLight:       return Attrs->GetCorruptionToLight();

	// ── Ailment Chances ───────────────────────────────────────────────────────
	case EHunterAttribute::ChanceToBleed:           return Attrs->GetChanceToBleed();
	case EHunterAttribute::ChanceToCorrupt:         return Attrs->GetChanceToCorrupt();
	case EHunterAttribute::ChanceToFreeze:          return Attrs->GetChanceToFreeze();
	case EHunterAttribute::ChanceToPurify:          return Attrs->GetChanceToPurify();
	case EHunterAttribute::ChanceToIgnite:          return Attrs->GetChanceToIgnite();
	case EHunterAttribute::ChanceToKnockBack:       return Attrs->GetChanceToKnockBack();
	case EHunterAttribute::ChanceToPetrify:         return Attrs->GetChanceToPetrify();
	case EHunterAttribute::ChanceToShock:           return Attrs->GetChanceToShock();
	case EHunterAttribute::ChanceToStun:            return Attrs->GetChanceToStun();

	// ── DoT Durations ─────────────────────────────────────────────────────────
	case EHunterAttribute::BurnDuration:            return Attrs->GetBurnDuration();
	case EHunterAttribute::BleedDuration:           return Attrs->GetBleedDuration();
	case EHunterAttribute::FreezeDuration:          return Attrs->GetFreezeDuration();
	case EHunterAttribute::CorruptionDuration:      return Attrs->GetCorruptionDuration();
	case EHunterAttribute::ShockDuration:           return Attrs->GetShockDuration();
	case EHunterAttribute::PetrifyBuildUpDuration:  return Attrs->GetPetrifyBuildUpDuration();
	case EHunterAttribute::PurifyDuration:          return Attrs->GetPurifyDuration();

	// ── Defense ───────────────────────────────────────────────────────────────
	case EHunterAttribute::GlobalDefenses:              return Attrs->GetGlobalDefenses();
	case EHunterAttribute::Armour:                      return Attrs->GetArmour();
	case EHunterAttribute::ArmourFlatBonus:             return Attrs->GetArmourFlatBonus();
	case EHunterAttribute::ArmourPercentBonus:          return Attrs->GetArmourPercentBonus();
	case EHunterAttribute::BlockStrength:               return Attrs->GetBlockStrength();
	case EHunterAttribute::FlatBlockAmount:             return Attrs->GetFlatBlockAmount();
	case EHunterAttribute::ChipDamageWhileBlocking:     return Attrs->GetChipDamageWhileBlocking();
	case EHunterAttribute::BlockStaminaCostMultiplier:  return Attrs->GetBlockStaminaCostMultiplier();
	case EHunterAttribute::GuardBreakThreshold:         return Attrs->GetGuardBreakThreshold();
	case EHunterAttribute::BlockAngle:                  return Attrs->GetBlockAngle();
	case EHunterAttribute::BlockPhysicalMultiplier:     return Attrs->GetBlockPhysicalMultiplier();
	case EHunterAttribute::BlockElementalMultiplier:    return Attrs->GetBlockElementalMultiplier();
	case EHunterAttribute::BlockCorruptionMultiplier:   return Attrs->GetBlockCorruptionMultiplier();

	// ── Resistances ───────────────────────────────────────────────────────────
	case EHunterAttribute::FireResistanceFlatBonus:         return Attrs->GetFireResistanceFlatBonus();
	case EHunterAttribute::FireResistancePercentBonus:      return Attrs->GetFireResistancePercentBonus();
	case EHunterAttribute::MaxFireResistance:               return Attrs->GetMaxFireResistance();
	case EHunterAttribute::IceResistanceFlatBonus:          return Attrs->GetIceResistanceFlatBonus();
	case EHunterAttribute::IceResistancePercentBonus:       return Attrs->GetIceResistancePercentBonus();
	case EHunterAttribute::MaxIceResistance:                return Attrs->GetMaxIceResistance();
	case EHunterAttribute::LightningResistanceFlatBonus:    return Attrs->GetLightningResistanceFlatBonus();
	case EHunterAttribute::LightningResistancePercentBonus: return Attrs->GetLightningResistancePercentBonus();
	case EHunterAttribute::MaxLightningResistance:          return Attrs->GetMaxLightningResistance();
	case EHunterAttribute::LightResistanceFlatBonus:        return Attrs->GetLightResistanceFlatBonus();
	case EHunterAttribute::LightResistancePercentBonus:     return Attrs->GetLightResistancePercentBonus();
	case EHunterAttribute::MaxLightResistance:              return Attrs->GetMaxLightResistance();
	case EHunterAttribute::CorruptionResistanceFlatBonus:   return Attrs->GetCorruptionResistanceFlatBonus();
	case EHunterAttribute::CorruptionResistancePercentBonus: return Attrs->GetCorruptionResistancePercentBonus();
	case EHunterAttribute::MaxCorruptionResistance:         return Attrs->GetMaxCorruptionResistance();

	// ── Damage Taken Multipliers ──────────────────────────────────────────────
	case EHunterAttribute::GlobalDamageTakenMultiplier:     return Attrs->GetGlobalDamageTakenMultiplier();
	case EHunterAttribute::PhysicalDamageTakenMultiplier:   return Attrs->GetPhysicalDamageTakenMultiplier();
	case EHunterAttribute::ElementalDamageTakenMultiplier:  return Attrs->GetElementalDamageTakenMultiplier();
	case EHunterAttribute::FireDamageTakenMultiplier:       return Attrs->GetFireDamageTakenMultiplier();
	case EHunterAttribute::IceDamageTakenMultiplier:        return Attrs->GetIceDamageTakenMultiplier();
	case EHunterAttribute::LightningDamageTakenMultiplier:  return Attrs->GetLightningDamageTakenMultiplier();
	case EHunterAttribute::LightDamageTakenMultiplier:      return Attrs->GetLightDamageTakenMultiplier();
	case EHunterAttribute::CorruptionDamageTakenMultiplier: return Attrs->GetCorruptionDamageTakenMultiplier();

	// ── Reflect ───────────────────────────────────────────────────────────────
	case EHunterAttribute::ReflectPhysical:         return Attrs->GetReflectPhysical();
	case EHunterAttribute::ReflectElemental:        return Attrs->GetReflectElemental();
	case EHunterAttribute::ReflectChancePhysical:   return Attrs->GetReflectChancePhysical();
	case EHunterAttribute::ReflectChanceElemental:  return Attrs->GetReflectChanceElemental();

	// ── Piercing ──────────────────────────────────────────────────────────────
	case EHunterAttribute::ArmourPiercing:          return Attrs->GetArmourPiercing();
	case EHunterAttribute::FirePiercing:            return Attrs->GetFirePiercing();
	case EHunterAttribute::IcePiercing:             return Attrs->GetIcePiercing();
	case EHunterAttribute::LightningPiercing:       return Attrs->GetLightningPiercing();
	case EHunterAttribute::LightPiercing:           return Attrs->GetLightPiercing();
	case EHunterAttribute::CorruptionPiercing:      return Attrs->GetCorruptionPiercing();

	// ── Resource & Utility ────────────────────────────────────────────────────
	case EHunterAttribute::ComboCounter:            return Attrs->GetComboCounter();
	case EHunterAttribute::Cooldown:                return Attrs->GetCooldown();
	case EHunterAttribute::Gems:                    return Attrs->GetGems();
	case EHunterAttribute::LifeLeech:               return Attrs->GetLifeLeech();
	case EHunterAttribute::ManaLeech:               return Attrs->GetManaLeech();
	case EHunterAttribute::StaminaLeechPercent:     return Attrs->GetStaminaLeechPercent();
	case EHunterAttribute::MovementSpeed:           return Attrs->GetMovementSpeed();
	case EHunterAttribute::Poise:                   return Attrs->GetPoise();
	case EHunterAttribute::Weight:                  return Attrs->GetWeight();
	case EHunterAttribute::PoiseResistance:         return Attrs->GetPoiseResistance();
	case EHunterAttribute::StunRecovery:            return Attrs->GetStunRecovery();
	case EHunterAttribute::ManaCostChanges:         return Attrs->GetManaCostChanges();
	case EHunterAttribute::HealthCostChanges:       return Attrs->GetHealthCostChanges();
	case EHunterAttribute::StaminaCostChanges:      return Attrs->GetStaminaCostChanges();
	case EHunterAttribute::LifeOnHit:               return Attrs->GetLifeOnHit();
	case EHunterAttribute::ManaOnHit:               return Attrs->GetManaOnHit();
	case EHunterAttribute::StaminaOnHit:            return Attrs->GetStaminaOnHit();
	case EHunterAttribute::AuraEffect:              return Attrs->GetAuraEffect();
	case EHunterAttribute::AuraRadius:              return Attrs->GetAuraRadius();

	default:
		UE_LOG(LogStatsManager, Warning,
			TEXT("GetAttributeByType: Unhandled EHunterAttribute value (%d) on %s."),
			static_cast<int32>(AttributeType), *GetNameSafe(GetOwner()));
		return 0.f;
	}
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

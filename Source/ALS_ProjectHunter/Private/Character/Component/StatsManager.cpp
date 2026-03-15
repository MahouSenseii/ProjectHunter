// Character/Component/StatsManager.cpp

#include "Character/Component/StatsManager.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Data/BaseStatsData.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Item/ItemInstance.h"
#include "Item/Library/ItemStructs.h"
#include "Item/Library/AffixEnums.h"
DEFINE_LOG_CATEGORY(LogStatsManager);
UStatsManager::UStatsManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(false); // Stats are replicated through AttributeSet
}

void UStatsManager::BeginPlay()
{
	Super::BeginPlay();

	// Cache references on begin play
	CachedASC = GetAbilitySystemComponent();
	CachedAttributeSet = GetAttributeSet();
	
	if (!CachedASC)
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager: No AbilitySystemComponent found on %s"), *GetOwner()->GetName());
	}
	
	if (!CachedAttributeSet)
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager: No HunterAttributeSet found on %s"), *GetOwner()->GetName());
	}

	if (StatsData)
	{
		InitializeFromDataAsset(StatsData);
	}
	else
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager: No StatsData found on %s"), *GetOwner()->GetName());
	}
}

void UStatsManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	DebugManager.DrawDebug(this, this);
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

	int32 NumEffects = ActiveEquipmentEffects.Num();
	
	// Remove all equipment effects
	for (const TPair<FGuid, FActiveGameplayEffectHandle>& Pair : ActiveEquipmentEffects)
	{
		if (Pair.Value.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Pair.Value);
		}
	}
	
	ActiveEquipmentEffects.Empty();

	UE_LOG(LogStatsManager, Log, TEXT("StatsManager: Refreshed equipment stats (removed %d effects)"), NumEffects);
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
			Attribute = UHunterAttributeSet::FindAttributeByName(Stat.AttributeName);
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
	FGameplayEffectSpec* Spec = new FGameplayEffectSpec(Effect, EffectContext, 1.0f);
	
	UE_LOG(LogStatsManager, Verbose, TEXT("StatsManager: Created effect '%s' with %d modifiers"), 
		*EffectName.ToString(), ModifiersAdded);

	return FGameplayEffectSpecHandle(Spec);
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
		// Multiplicative: multiply base by (1 + value)
		ModOp = EGameplayModOp::Multiplicitive;
		FinalValue = 1.0f + (FinalValue / 100.0f);
		break;

	case EModifyType::MT_More:
		// "More" in PoE terms = separate multiplier
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
	if (CachedAttributeSet)
	{
		return CachedAttributeSet;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		return const_cast<UHunterAttributeSet*>(ASC->GetSet<UHunterAttributeSet>());
	}

	return nullptr;
}

UAbilitySystemComponent* UStatsManager::GetAbilitySystemComponent() const
{
	if (CachedASC)
	{
		return CachedASC;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// Try to get ASC from owner (should implement IAbilitySystemInterface)
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
	{
		return ASI->GetAbilitySystemComponent();
	}

	return nullptr;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* PRIMARY ATTRIBUTES (7)                                                  */
/* ═══════════════════════════════════════════════════════════════════════ */

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
	float Max = GetMaxHealth();
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
	float Max = GetMaxMana();
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
	float Max = GetMaxStamina();
	return Max > 0.0f ? GetStamina() / Max : 0.0f;
}


/* ═══════════════════════════════════════════════════════════════════════ */
/* GENERIC ATTRIBUTE ACCESS                                                */
/* ═══════════════════════════════════════════════════════════════════════ */

float UStatsManager::GetAttributeByName(FName AttributeName) const
{
	FGameplayAttribute Attribute = UHunterAttributeSet::FindAttributeByName(AttributeName);
	if (!Attribute.IsValid())
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

	return ASC->GetNumericAttribute(Attribute);
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

	StatsData = InStatsData;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogStatsManager, Error, TEXT("InitializeFromDataAsset: No AbilitySystemComponent found"));
		return;
	}

	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogStatsManager, Warning, TEXT("InitializeFromDataAsset: Must be called on server"));
		return;
	}

	const TMap<FName, float> StatsMap = StatsData->GetAllStatsAsMap();

	for (const TPair<FName, float>& Pair : StatsMap)
	{
		SetStatValue(Pair.Key, Pair.Value);
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(GetOwner());

	for (TSubclassOf<UGameplayEffect> EffectClass : StatsData->InitializationEffects)
	{
		if (EffectClass)
		{
			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
			if (SpecHandle.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}

	UE_LOG(LogStatsManager, Log, TEXT("Stats initialized from %s"), *StatsData->StatSetName.ToString());
}

void UStatsManager::InitializeFromMap(const TMap<FName, float>& StatsMap) const
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogStatsManager, Warning, TEXT("InitializeFromMap: Must be called on server"));
		return;
	}

	for (const TPair<FName, float>& Pair : StatsMap)
	{
		SetStatValue(Pair.Key, Pair.Value);
	}

	UE_LOG(LogStatsManager, Log, TEXT("Stats initialized from map (%d attributes)"), StatsMap.Num());
}

void UStatsManager::SetStatValue(FName AttributeName, float Value) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	UHunterAttributeSet* AttrSet = GetAttributeSet();

	if (!ASC || !AttrSet)
	{
		UE_LOG(LogStatsManager, Error, TEXT("SetStatValue: ASC or AttributeSet is null"));
		return;
	}

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	FGameplayAttribute Attribute = UHunterAttributeSet::FindAttributeByName(AttributeName);
	
	if (!Attribute.IsValid())
	{
		UE_LOG(LogStatsManager, Warning, TEXT("SetStatValue: Unknown attribute '%s'"), *AttributeName.ToString());
		return;
	}

	ASC->SetNumericAttributeBase(Attribute, Value);

	// Auto-fill current values for vitals
	if (AttributeName == "MaxHealth")
	{
		FGameplayAttribute HealthAttr = UHunterAttributeSet::GetHealthAttribute();
		ASC->SetNumericAttributeBase(HealthAttr, Value);
	}
	else if (AttributeName == "MaxMana")
	{
		FGameplayAttribute ManaAttr = UHunterAttributeSet::GetManaAttribute();
		ASC->SetNumericAttributeBase(ManaAttr, Value);
	}
	else if (AttributeName == "MaxStamina")
	{
		FGameplayAttribute StaminaAttr = UHunterAttributeSet::GetStaminaAttribute();
		ASC->SetNumericAttributeBase(StaminaAttr, Value);
	}
	else if (AttributeName == "MaxArcaneShield")
	{
		FGameplayAttribute ShieldAttr = UHunterAttributeSet::GetArcaneShieldAttribute();
		ASC->SetNumericAttributeBase(ShieldAttr, Value);
	}
}

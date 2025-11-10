#include "Components/StatsManager.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "PHGameplayTags.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "AbilitySystem/Data/AttributeConfigDataAsset.h"
#include "Character/PHBaseCharacter.h"

DEFINE_LOG_CATEGORY(LogStatsManager);

/* ============================= */
/* ===   Lifecycle           === */
/* ============================= */

UStatsManager::UStatsManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStatsManager::BeginPlay()
{

	Super::BeginPlay();
	// Cache gameplay tags
	Tag_Sprinting = FGameplayTag::RequestGameplayTag(FName("Condition.State.Sprinting"));
	Tag_CannotRegenHP = FGameplayTag::RequestGameplayTag(FName("Condition.Self.CannotRegenHP"));
	Tag_CannotRegenMana = FGameplayTag::RequestGameplayTag(FName("Condition.Self.CannotRegenMana"));
	Tag_CannotRegenStamina = FGameplayTag::RequestGameplayTag(FName("Condition.Self.CannotRegenStamina"));
}

/* ============================= */
/* ===   Initialization      === */
/* ============================= */

void UStatsManager::Initialize()
{

	if (!Owner)
	{
		UE_LOG(LogStatsManager, Error, TEXT("StatsManager has no Owner set!"));
		return;
	}

	ASC = Owner->GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogStatsManager, Error, TEXT("Owner has no AbilitySystemComponent!"));
		return;
	}

	InitializeAttributeToEffectClassMap();
	InitializeAttributesFromConfig(AttributeConfig);
	InitializeCurrentVitalsToMax();
	InitializeRegenAndDegenEffects();
	UE_LOG(LogStatsManager, Log, TEXT("StatsManager initialized successfully"));
}

void UStatsManager::InitializeAttributesFromConfig(UAttributeConfigDataAsset* Config)
{
	if (!IsInitialized() || !Config)
	{
		UE_LOG(LogStatsManager, Error, TEXT("Cannot initialize - invalid ASC or Config"));
		return;
	}

	UE_LOG(LogStatsManager, Log, TEXT("=== Initializing Attributes ==="));

	const UPHAttributeSet* AttributeSet = Cast<UPHAttributeSet>(
		ASC->GetAttributeSet(UPHAttributeSet::StaticClass())
	);

	if (!AttributeSet)
	{
		UE_LOG(LogStatsManager, Error, TEXT("No AttributeSet found!"));
		return;
	}

	UE_LOG(LogStatsManager, Warning, TEXT("Config has %d attributes"), Config->Attributes.Num());

	int32 ProcessedCount = 0;
	int32 SkippedCount = 0;
	int32 SuccessCount = 0;
	
	for (const FAttributeInitConfig& Attr : Config->Attributes)
	{
		ProcessedCount++;
		
		if (!Attr.bEnabled)
		{
			UE_LOG(LogStatsManager, Warning, TEXT("Skipping disabled attribute: %s"), 
				*Attr.AttributeTag.ToString());
			SkippedCount++;
			continue;
		}
		
		if (!Attr.AttributeTag.IsValid())
		{
			UE_LOG(LogStatsManager, Warning, TEXT("Skipping attribute with invalid tag"));
			SkippedCount++;
			continue;
		}

		FGameplayAttribute GameplayAttr = FindAttributeByTag(Attr.AttributeTag);
		
		if (GameplayAttr.IsValid())
		{
			ASC->SetNumericAttributeBase(GameplayAttr, Attr.DefaultValue);
			UE_LOG(LogStatsManager, Log, TEXT("✓ Set %s = %.2f"), 
				*Attr.AttributeTag.ToString(), Attr.DefaultValue);
			SuccessCount++;
		}
		else
		{
			UE_LOG(LogStatsManager, Warning, TEXT("No attribute found for tag: %s"), 
				*Attr.AttributeTag.ToString());
		}
	}

	UE_LOG(LogStatsManager, Log, TEXT("Processed: %d, Skipped: %d, Success: %d"), 
		ProcessedCount, SkippedCount, SuccessCount);

	UPHAttributeSet* MutableAttributeSet = const_cast<UPHAttributeSet*>(AttributeSet);
	if (MutableAttributeSet)
	{
		// Trigger reserve recalculations for all vitals
		const float MaxHealth = AttributeSet->GetMaxHealth();
		const float MaxMana = AttributeSet->GetMaxMana();
		const float MaxStamina = AttributeSet->GetMaxStamina();
		const float MaxShield = AttributeSet->GetMaxArcaneShield();
		
		// Set effective maxes (no reserves by default)
		ASC->SetNumericAttributeBase(UPHAttributeSet::GetMaxEffectiveHealthAttribute(), MaxHealth);
		ASC->SetNumericAttributeBase(UPHAttributeSet::GetMaxEffectiveManaAttribute(), MaxMana);
		ASC->SetNumericAttributeBase(UPHAttributeSet::GetMaxEffectiveStaminaAttribute(), MaxStamina);
		ASC->SetNumericAttributeBase(UPHAttributeSet::GetMaxEffectiveArcaneShieldAttribute(), MaxShield);
		
		UE_LOG(LogStatsManager, Log, TEXT("✓ Set effective maxes: Health=%d, Mana=%d, Stamina=%d, Shield=%d"),
			(int32)MaxHealth, (int32)MaxMana, (int32)MaxStamina, (int32)MaxShield);
	}

	UE_LOG(LogStatsManager, Log, TEXT("=== Initialization Complete ==="));
}

void UStatsManager::InitializeCurrentVitalsToMax()
{
	if (!IsInitialized())
	{
		UE_LOG(LogStatsManager, Error, TEXT("Cannot initialize vitals - ASC not set"));
		return;
	}

	const UPHAttributeSet* AttributeSet = Cast<UPHAttributeSet>(
		ASC->GetAttributeSet(UPHAttributeSet::StaticClass())
	);

	if (!AttributeSet)
	{
		UE_LOG(LogStatsManager, Error, TEXT("No AttributeSet found"));
		return;
	}

	// Set current vitals to their max values
	ASC->SetNumericAttributeBase(UPHAttributeSet::GetHealthAttribute(), 
		AttributeSet->GetMaxHealth());
	ASC->SetNumericAttributeBase(UPHAttributeSet::GetManaAttribute(), 
		AttributeSet->GetMaxMana());
	ASC->SetNumericAttributeBase(UPHAttributeSet::GetStaminaAttribute(), 
		AttributeSet->GetMaxStamina());
	ASC->SetNumericAttributeBase(UPHAttributeSet::GetArcaneShieldAttribute(), 
		AttributeSet->GetMaxArcaneShield());

	UE_LOG(LogStatsManager, Log, TEXT("✓ Current vitals set to max values"));
}

void UStatsManager::InitializeAttributeToEffectClassMap()
{
	if (!IsInitialized())
	{
		UE_LOG(LogStatsManager, Warning, TEXT("Cannot initialize attribute map - ASC not set"));
		return;
	}

	AttributeToEffectClassMap.Empty();

	// Map damage attributes to equipment damage effect
	if (EquipmentDamageEffectClass)
	{
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetMinPhysicalDamageAttribute(), EquipmentDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetMaxPhysicalDamageAttribute(), EquipmentDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetMinFireDamageAttribute(), EquipmentDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetMaxFireDamageAttribute(), EquipmentDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetMinIceDamageAttribute(), EquipmentDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetMaxIceDamageAttribute(), EquipmentDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetMinLightningDamageAttribute(), EquipmentDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetMaxLightningDamageAttribute(), EquipmentDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetMinLightDamageAttribute(), EquipmentDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetMaxLightDamageAttribute(), EquipmentDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetMinCorruptionDamageAttribute(), EquipmentDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetMaxCorruptionDamageAttribute(), EquipmentDamageEffectClass);
	}

	if (EquipmentFlatResistances)
	{
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetArmourFlatBonusAttribute(), EquipmentFlatResistances);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetFireResistanceFlatBonusAttribute(), EquipmentFlatResistances);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetIceResistanceFlatBonusAttribute(), EquipmentFlatResistances);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetLightningResistanceFlatBonusAttribute(), EquipmentFlatResistances);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetLightResistanceFlatBonusAttribute(), EquipmentFlatResistances);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetCorruptionResistanceFlatBonusAttribute(), EquipmentFlatResistances);
	}

	if (EquipmentFlatBonusDamageEffectClass)
	{
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetPhysicalFlatBonusAttribute(), EquipmentFlatBonusDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetFireFlatBonusAttribute(), EquipmentFlatBonusDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetIceFlatBonusAttribute(), EquipmentFlatBonusDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetLightningFlatBonusAttribute(), EquipmentFlatBonusDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetLightFlatBonusAttribute(), EquipmentFlatBonusDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetCorruptionFlatBonusAttribute(), EquipmentFlatBonusDamageEffectClass);
	}

	if (EquipmentPercentBonusDamageEffectClass)
	{
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetPhysicalPercentBonusAttribute(), EquipmentPercentBonusDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetFirePercentBonusAttribute(), EquipmentPercentBonusDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetIcePercentBonusAttribute(), EquipmentPercentBonusDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetLightningPercentBonusAttribute(), EquipmentPercentBonusDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetLightPercentBonusAttribute(), EquipmentPercentBonusDamageEffectClass);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetCorruptionPercentBonusAttribute(), EquipmentPercentBonusDamageEffectClass);
	}

	if (EquipmentPiercingDamage)
	{
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetArmourPiercingAttribute(), EquipmentPiercingDamage);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetFirePiercingAttribute(), EquipmentPiercingDamage);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetIcePiercingAttribute(), EquipmentPiercingDamage);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetLightningPiercingAttribute(), EquipmentPiercingDamage);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetLightPiercingAttribute(), EquipmentPiercingDamage);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetCorruptionPiercingAttribute(), EquipmentPiercingDamage);
	}
	
	// Map resistance attributes
	if (EquipmentPercentResistances)
	{
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetArmourPercentBonusAttribute(), EquipmentPercentResistances);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetFireResistancePercentBonusAttribute(), EquipmentPercentResistances);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetIceResistancePercentBonusAttribute(), EquipmentPercentResistances);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetLightningResistancePercentBonusAttribute(), EquipmentPercentResistances);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetLightResistancePercentBonusAttribute(), EquipmentPercentResistances);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetCorruptionResistancePercentBonusAttribute(), EquipmentPercentResistances);
	}

	if (EquipmentAttributes)
	{
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetStrengthAttribute(), EquipmentAttributes);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetIntelligenceAttribute(), EquipmentAttributes);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetDexterityAttribute(), EquipmentAttributes);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetEnduranceAttribute(), EquipmentAttributes);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetAfflictionAttribute(), EquipmentAttributes);
		AttributeToEffectClassMap.Add(UPHAttributeSet::GetLuckAttribute(), EquipmentAttributes);
	}

	UE_LOG(LogStatsManager, Log, TEXT("✓ Attribute to Effect Class map initialized with %d mappings"), 
		AttributeToEffectClassMap.Num());
}

/* ============================= */
/* ===   Regen / Degen       === */
/* ============================= */

void UStatsManager::InitializeRegenAndDegenEffects()
{
	if (!IsInitialized())
	{
		UE_LOG(LogStatsManager, Warning, TEXT("Cannot initialize regen - not initialized"));
		return;
	}

	const UPHAttributeSet* AttrSet = Cast<UPHAttributeSet>(
		ASC->GetAttributeSet(UPHAttributeSet::StaticClass())
	);
	
	if (!AttrSet)
	{
		UE_LOG(LogStatsManager, Error, TEXT("No attribute set for regen"));
		return;
	}

	StopAllRegenEffects();

	// Health Regen - only if not blocked
	if (!ASC->HasMatchingGameplayTag(Tag_CannotRegenHP))
	{
		float HealthRegenAmount = AttrSet->GetHealthRegenAmount();
		float HealthRegenRate = AttrSet->GetHealthRegenRate();
		
		if (GE_HealthRegen && HealthRegenAmount > 0.0f && HealthRegenRate > 0.0f)
		{
			HealthRegenHandle = ApplyRegenEffect(GE_HealthRegen);
			UE_LOG(LogStatsManager, Log, TEXT("✓ Health Regen Active: %f per %f seconds"), 
				HealthRegenAmount, HealthRegenRate);
		}
	}
	else
	{
		UE_LOG(LogStatsManager, Log, TEXT("✗ Health Regen blocked by CannotRegenHP tag"));
	}

	// Mana Regen - only if not blocked
	if (!ASC->HasMatchingGameplayTag(Tag_CannotRegenMana))
	{
		float ManaRegenAmount = AttrSet->GetManaRegenAmount();
		float ManaRegenRate = AttrSet->GetManaRegenRate();
		
		if (GE_ManaRegen && ManaRegenAmount > 0.0f && ManaRegenRate > 0.0f)
		{
			ManaRegenHandle = ApplyRegenEffect(GE_ManaRegen);
			UE_LOG(LogStatsManager, Log, TEXT("✓ Mana Regen Active: %f per %f seconds"), 
				ManaRegenAmount, ManaRegenRate);
		}
	}
	else
	{
		UE_LOG(LogStatsManager, Log, TEXT("✗ Mana Regen blocked by CannotRegenMana tag"));
	}

	// Stamina Regen - only if not blocked
	if (!ASC->HasMatchingGameplayTag(Tag_CannotRegenStamina))
	{
		float StaminaRegenAmount = AttrSet->GetStaminaRegenAmount();
		float StaminaRegenRate = AttrSet->GetStaminaRegenRate();
		
		if (GE_StaminaRegen && StaminaRegenAmount > 0.0f && StaminaRegenRate > 0.0f)
		{
			StaminaRegenHandle = ApplyRegenEffect(GE_StaminaRegen);
			UE_LOG(LogStatsManager, Log, TEXT("✓ Stamina Regen Active: %f per %f seconds"), 
				StaminaRegenAmount, StaminaRegenRate);
		}
	}
	else
	{
		UE_LOG(LogStatsManager, Log, TEXT("✗ Stamina Regen blocked by CannotRegenStamina tag"));
	}

	// Register listener for sprinting tag
	if (Tag_Sprinting.IsValid())
	{
		SprintingTagDelegateHandle = ASC->RegisterGameplayTagEvent(
			Tag_Sprinting, 
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &UStatsManager::OnSprintingTagChanged);
		
		UE_LOG(LogStatsManager, Log, TEXT("✓ Registered sprinting listener"));
	}
}

void UStatsManager::StopAllRegenEffects()
{
	if (!IsInitialized())
	{
		return;
	}

	// Unregister sprinting listener
	if (SprintingTagDelegateHandle.IsValid())
	{
		ASC->UnregisterGameplayTagEvent(SprintingTagDelegateHandle, Tag_Sprinting, EGameplayTagEventType::NewOrRemoved);
		SprintingTagDelegateHandle.Reset();
	}

	if (HealthRegenHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(HealthRegenHandle);
		HealthRegenHandle.Invalidate();
		UE_LOG(LogStatsManager, Log, TEXT("✗ Stopped Health Regen"));
	}

	if (ManaRegenHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(ManaRegenHandle);
		ManaRegenHandle.Invalidate();
		UE_LOG(LogStatsManager, Log, TEXT("✗ Stopped Mana Regen"));
	}

	if (StaminaRegenHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(StaminaRegenHandle);
		StaminaRegenHandle.Invalidate();
		UE_LOG(LogStatsManager, Log, TEXT("✗ Stopped Stamina Regen"));
	}

	if (StaminaDegenHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(StaminaDegenHandle);
		StaminaDegenHandle.Invalidate();
		UE_LOG(LogStatsManager, Log, TEXT("✗ Stopped Stamina Degen"));
	}
}

/* ============================= */
/* ===   Attribute Access    === */
/* ============================= */

float UStatsManager::GetAttributeBase(const FGameplayAttribute& Attribute) const
{
	if (!IsInitialized() || !HasAttribute(Attribute))
	{
		return 0.0f;
	}

	return ASC->GetNumericAttributeBase(Attribute);
}

float UStatsManager::GetAttributeCurrent(const FGameplayAttribute& Attribute) const
{
	if (!IsInitialized() || !HasAttribute(Attribute))
	{
		return 0.0f;
	}

	return ASC->GetNumericAttribute(Attribute);
}

bool UStatsManager::HasAttribute(const FGameplayAttribute& Attribute) const
{
	if (!IsInitialized())
	{
		return false;
	}

	return ASC->HasAttributeSetForAttribute(Attribute);
}

FGameplayAttribute UStatsManager::FindAttributeByTag(const FGameplayTag& Tag) const
{
	if (!IsInitialized())
	{
		return FGameplayAttribute();
	}
	
	// Use the attribute set's tag-to-attribute mapping
	return FPHGameplayTags::GetAttributeFromTag(Tag);
}

TSubclassOf<UGameplayEffect> UStatsManager::GetEffectClassForAttribute(const FGameplayAttribute& Attribute) const
{
	if (const TSubclassOf<UGameplayEffect>* FoundClass = AttributeToEffectClassMap.Find(Attribute))
	{
		return *FoundClass;
	}

	return nullptr;
}

/* ============================= */
/* ===   Direct Modification === */
/* ============================= */

void UStatsManager::SetAttributeBase(const FGameplayAttribute& Attribute, float NewValue)
{
	if (!IsInitialized() || !HasAttribute(Attribute))
	{
		UE_LOG(LogStatsManager, Warning, TEXT("Cannot set attribute base - invalid state"));
		return;
	}

	ASC->SetNumericAttributeBase(Attribute, NewValue);
	UE_LOG(LogStatsManager, Verbose, TEXT("Set %s base to %.2f"), 
		*Attribute.GetName(), NewValue);
}

/* ============================= */
/* ===   Modifier Application === */
/* ============================= */

FActiveGameplayEffectHandle UStatsManager::ApplyFlatModifier(
	const FGameplayAttribute& Attribute,
	float Value,
	bool bIsTemporary)
{
	if (!IsInitialized() || !HasAttribute(Attribute))
	{
		UE_LOG(LogStatsManager, Warning, TEXT("Cannot apply flat modifier - invalid state"));
		return FActiveGameplayEffectHandle();
	}

	TSubclassOf<UGameplayEffect> EffectClass = GetEffectClassForAttribute(Attribute);
	if (!EffectClass)
	{
		UE_LOG(LogStatsManager, Warning, TEXT("No effect class mapped for attribute: %s"), 
			*Attribute.GetName());
		return FActiveGameplayEffectHandle();
	}

	// Create effect context
	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(Owner);

	// Create spec
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogStatsManager, Error, TEXT("Failed to create spec for flat modifier"));
		return FActiveGameplayEffectHandle();
	}

	const FString AttributeName = Attribute.GetName();
	
	UE_LOG(LogStatsManager, Log, TEXT("Attribute name: %s"), *AttributeName);

	// Build the tag string
	const FString TagString = FString::Printf(
		TEXT("Attributes.Secondary.Damage.%s"),
		*AttributeName
	);

	UE_LOG(LogStatsManager, Log, TEXT("Full damage tag: %s"), *TagString);
	FGameplayTag DataTag = FPHGameplayTags::AttributeToTagMap.FindRef(Attribute);

	if (!DataTag.IsValid())
	{
		UE_LOG(LogStatsManager, Error, TEXT("No tag mapping found for attribute"));
		return FActiveGameplayEffectHandle();
	}
	
	SpecHandle.Data->SetSetByCallerMagnitude(DataTag, Value);

	// Apply effect
	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	if (bIsTemporary && Handle.IsValid())
	{
		TemporaryModifiers.Add(Handle);
	}

	return Handle;
}

FActiveGameplayEffectHandle UStatsManager::ApplyFlatModifierFinal(
	const FGameplayAttribute& Attribute,
	float Value,
	bool bIsTemporary)
{
	// Same as ApplyFlatModifier but uses a different GE with Additive operation
	// This is applied AFTER multiplicative modifiers
	return ApplyFlatModifier(Attribute, Value, bIsTemporary);
}

FActiveGameplayEffectHandle UStatsManager::ApplyPercentageModifier(
	const FGameplayAttribute& Attribute,
	float Percentage,
	bool bIsTemporary)
{
	if (!IsInitialized() || !HasAttribute(Attribute))
	{
		UE_LOG(LogStatsManager, Warning, TEXT("Cannot apply percentage modifier - invalid state"));
		return FActiveGameplayEffectHandle();
	}

	TSubclassOf<UGameplayEffect> EffectClass = GetEffectClassForAttribute(Attribute);
	if (!EffectClass)
	{
		UE_LOG(LogStatsManager, Warning, TEXT("No effect class mapped for attribute: %s"), 
			*Attribute.GetName());
		return FActiveGameplayEffectHandle();
	}

	// Convert percentage to multiplier (e.g., 25% = 0.25)
	float Multiplier = Percentage / 100.0f;

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(Owner);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayTag DataTag = FGameplayTag::RequestGameplayTag(
		FName(*FString::Printf(TEXT("Data.Modifier.%s"), *Attribute.GetName()))
	);
	SpecHandle.Data->SetSetByCallerMagnitude(DataTag, Multiplier);

	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	if (bIsTemporary && Handle.IsValid())
	{
		TemporaryModifiers.Add(Handle);
	}

	return Handle;
}

FActiveGameplayEffectHandle UStatsManager::ApplyMultiplierModifier(
	const FGameplayAttribute& Attribute,
	float Multiplier,
	bool bIsTemporary)
{
	// Similar to percentage but expects direct multiplier value
	return ApplyPercentageModifier(Attribute, Multiplier * 100.0f, bIsTemporary);
}

FActiveGameplayEffectHandle UStatsManager::ApplyOverrideModifier(
	const FGameplayAttribute& Attribute,
	float Value)
{
	if (!IsInitialized() || !HasAttribute(Attribute))
	{
		UE_LOG(LogStatsManager, Warning, TEXT("Cannot apply override modifier - invalid state"));
		return FActiveGameplayEffectHandle();
	}

	TSubclassOf<UGameplayEffect> EffectClass = GetEffectClassForAttribute(Attribute);
	if (!EffectClass)
	{
		UE_LOG(LogStatsManager, Warning, TEXT("No effect class mapped for attribute: %s"), 
			*Attribute.GetName());
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(Owner);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayTag DataTag = FGameplayTag::RequestGameplayTag(
		FName(*FString::Printf(TEXT("Data.Modifier.%s"), *Attribute.GetName()))
	);
	SpecHandle.Data->SetSetByCallerMagnitude(DataTag, Value);

	return ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

/* ============================= */
/* ===   Modifier Removal    === */
/* ============================= */

bool UStatsManager::RemoveModifier(FActiveGameplayEffectHandle Handle)
{
	if (!IsInitialized() || !Handle.IsValid())
	{
		return false;
	}

	bool bRemoved = ASC->RemoveActiveGameplayEffect(Handle);
	
	if (bRemoved)
	{
		TemporaryModifiers.Remove(Handle);
		UE_LOG(LogStatsManager, Verbose, TEXT("Removed modifier"));
	}

	return bRemoved;
}

int32 UStatsManager::RemoveAllModifiersFromAttribute(const FGameplayAttribute& Attribute)
{
	if (!IsInitialized() || !HasAttribute(Attribute))
	{
		return 0;
	}

	int32 RemovedCount = 0;
    
	// Get all active effects that modify this attribute
	FGameplayEffectQuery Query;
	Query.OwningTagQuery = FGameplayTagQuery();
	Query.EffectTagQuery = FGameplayTagQuery();
	Query.ModifyingAttribute = Attribute;
    
	// Get handles of effects that match the query
	TArray<FActiveGameplayEffectHandle> EffectsToRemove = ASC->GetActiveEffects(Query);

	// Remove each effect
	for (const FActiveGameplayEffectHandle& Handle : EffectsToRemove)
	{
		if (Handle.IsValid() && ASC->RemoveActiveGameplayEffect(Handle))
		{
			RemovedCount++;
			TemporaryModifiers.Remove(Handle);
		}
	}

	UE_LOG(LogStatsManager, Log, TEXT("Removed %d modifiers from %s"), 
		RemovedCount, *Attribute.GetName());

	return RemovedCount;
}

int32 UStatsManager::RemoveAllTemporaryModifiers()
{
	if (!IsInitialized())
	{
		return 0;
	}

	int32 RemovedCount = 0;

	for (const FActiveGameplayEffectHandle& Handle : TemporaryModifiers)
	{
		if (Handle.IsValid() && ASC->RemoveActiveGameplayEffect(Handle))
		{
			RemovedCount++;
		}
	}

	TemporaryModifiers.Empty();

	UE_LOG(LogStatsManager, Log, TEXT("Removed %d temporary modifiers"), RemovedCount);

	return RemovedCount;
}

/* ============================= */
/* ===   Effect Application  === */
/* ============================= */

FActiveGameplayEffectHandle UStatsManager::ApplyGameplayEffectToSelf(
	TSubclassOf<UGameplayEffect> EffectClass,
	float Level)
{
	if (!IsInitialized() || !EffectClass)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(Owner);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, Level, EffectContext);
	
	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogStatsManager, Error, TEXT("Failed to create spec for effect"));
		return FActiveGameplayEffectHandle();
	}

	return ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

FActiveGameplayEffectHandle UStatsManager::ApplyGameplayEffectSpecToSelf(const FGameplayEffectSpec& Spec)
{
	if (!IsInitialized())
	{
		return FActiveGameplayEffectHandle();
	}

	return ASC->ApplyGameplayEffectSpecToSelf(Spec);
}

/* ============================= */
/* ===   Protected Functions === */
/* ============================= */

bool UStatsManager::ApplyInitializationEffect(
	const TSubclassOf<UGameplayEffect>& EffectClass,
	const TMap<FGameplayTag, float>& AttributeValues)
{
	if (!IsInitialized() || !EffectClass)
	{
		UE_LOG(LogStatsManager, Error, TEXT("Cannot apply init effect - invalid state"));
		return false;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(Owner);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
	
	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogStatsManager, Error, TEXT("Failed to create initialization effect spec"));
		return false;
	}

	// Set all attribute values using SetByCaller
	for (const TPair<FGameplayTag, float>& Pair : AttributeValues)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(Pair.Key, Pair.Value);
		UE_LOG(LogStatsManager, Verbose, TEXT("Set %s = %.2f"), 
			*Pair.Key.ToString(), Pair.Value);
	}

	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	
	if (!Handle.IsValid())
	{
		UE_LOG(LogStatsManager, Error, TEXT("Failed to apply initialization effect"));
		return false;
	}

	return true;
}

bool UStatsManager::ApplySimpleInitEffect(const TSubclassOf<UGameplayEffect>& EffectClass) const
{
	if (!IsInitialized() || !EffectClass)
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(Owner);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
	
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	return Handle.IsValid();
}

FActiveGameplayEffectHandle UStatsManager::ApplyRegenEffect(
	TSubclassOf<UGameplayEffect> EffectClass) const
{
	if (!EffectClass)
	{
		UE_LOG(LogStatsManager, Warning, TEXT("Invalid effect class for regen"));
		return FActiveGameplayEffectHandle();
	}

	if (!IsInitialized())
	{
		UE_LOG(LogStatsManager, Warning, TEXT("ASC not initialized"));
		return FActiveGameplayEffectHandle();
	}

	// Create effect context
	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(GetOwner());

	// Create the spec from the configured effect class
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
		EffectClass, 
		1.0f, 
		EffectContext);
    
	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogStatsManager, Error, TEXT("Failed to create spec for regen effect"));
		return FActiveGameplayEffectHandle();
	}

	// Apply the effect - the SetByCaller values are handled by your MMCs
	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	if (Handle.IsValid())
	{
		UE_LOG(LogStatsManager, Log, TEXT("Applied regen effect: %s"), 
			*EffectClass->GetName());
	}
	else
	{
		UE_LOG(LogStatsManager, Error, TEXT("Failed to apply regen effect - invalid handle"));
	}

	return Handle;
}

void UStatsManager::OnSprintingTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		// Sprinting started
		UE_LOG(LogStatsManager, Log, TEXT("→ Sprint Started"));
		
		if (!IsInitialized() || !GE_StaminaDegen)
		{
			return;
		}

		// Stop stamina regen
		if (StaminaRegenHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(StaminaRegenHandle);
			StaminaRegenHandle.Invalidate();
			UE_LOG(LogStatsManager, Log, TEXT("  ✗ Stamina Regen paused"));
		}

		// Start stamina degen if not blocked
		if (ASC->HasMatchingGameplayTag(Tag_CannotRegenStamina))
		{
			UE_LOG(LogStatsManager, Log, TEXT("  ✗ Stamina Degen blocked by CannotRegenStamina"));
			return;
		}

		if (StaminaDegenHandle.IsValid())
		{
			return; // Already active
		}

		const UPHAttributeSet* AttrSet = Cast<UPHAttributeSet>(
			ASC->GetAttributeSet(UPHAttributeSet::StaticClass())
		);
		
		if (!AttrSet)
		{
			return;
		}

		float StaminaDegenAmount = AttrSet->GetStaminaDegenAmount();
		float StaminaDegenRate = AttrSet->GetStaminaDegenRate();

		if (StaminaDegenAmount > 0.0f && StaminaDegenRate > 0.0f)
		{
			StaminaDegenHandle = ApplyRegenEffect(GE_StaminaDegen);

			UE_LOG(LogStatsManager, Log, TEXT("  ✓ Stamina Degen Active: -%f per %f seconds"), 
				StaminaDegenAmount, StaminaDegenRate);
		}
	}
	else
	{
		// Sprinting stopped
		UE_LOG(LogStatsManager, Log, TEXT("→ Sprint Stopped"));
		
		if (!IsInitialized())
		{
			return;
		}

		// Stop degen
		if (StaminaDegenHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(StaminaDegenHandle);
			StaminaDegenHandle.Invalidate();
			UE_LOG(LogStatsManager, Log, TEXT("  ✗ Stamina Degen stopped"));
		}

		// Restart stamina regen if not blocked
		if (ASC->HasMatchingGameplayTag(Tag_CannotRegenStamina))
		{
			UE_LOG(LogStatsManager, Log, TEXT("  ✗ Stamina Regen blocked by CannotRegenStamina"));
			return;
		}

		if (GE_StaminaRegen && !StaminaRegenHandle.IsValid())
		{
			const UPHAttributeSet* AttrSet = Cast<UPHAttributeSet>(
				ASC->GetAttributeSet(UPHAttributeSet::StaticClass())
			);
			
			if (AttrSet)
			{
				float StaminaRegenAmount = AttrSet->GetStaminaRegenAmount();
				float StaminaRegenRate = AttrSet->GetStaminaRegenRate();
				
				if (StaminaRegenAmount > 0.0f && StaminaRegenRate > 0.0f)
				{
					StaminaRegenHandle = ApplyRegenEffect(GE_StaminaRegen);
					UE_LOG(LogStatsManager, Log, TEXT("  ✓ Stamina Regen resumed"));
				}
			}
		}
	}
}
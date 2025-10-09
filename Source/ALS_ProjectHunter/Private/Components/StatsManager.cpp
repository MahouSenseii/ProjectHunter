#include "Components/StatsManager.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "PHGameplayTags.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "AbilitySystem/Data/AttributeConfigDataAsset.h"
#include "Character/PHBaseCharacter.h"

DEFINE_LOG_CATEGORY(LogStatsManager);

UStatsManager::UStatsManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	ASC = nullptr;
}

void UStatsManager::BeginPlay()
{
	Super::BeginPlay();
	
	// Try to auto-initialize if owner has an ASC
	if (Owner)
	{
		if (Owner->FindComponentByClass<UAbilitySystemComponent>())
		{
			Initialize();
			//InitializeAttributes(); 
			InitializeAttributesFromConfig(AttributeConfig);
		}
	}
}

void UStatsManager::Initialize()
{
	if (ASC) //ALREADY initialized
	{
		UE_LOG(LogStatsManager, Warning, TEXT("StatsManager already initialized"));
		return;
	}
	
	// Get ASC from owner
	if (Owner)
	{
		ASC = Owner->FindComponentByClass<UAbilitySystemComponent>();
		if (!ASC)
		{
			UE_LOG(LogStatsManager, Error, TEXT("Owner has no ASC!"));
			return;
		}
	}
	else
	{
		UE_LOG(LogStatsManager, Error, TEXT("No owner set!"));
		return;
	}
	
	UE_LOG(LogStatsManager, Log, TEXT("StatsManager initialized for %s"), *GetOwner()->GetName());
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
    	
        
        UE_LOG(LogStatsManager, Log, TEXT("✓ Set effective maxes: Health=%d, Mana=%d, Stamina=%d, Shield=%d, HeathRegen=%d per%d"),
            (int32)MaxHealth, (int32)MaxMana, (int32)MaxStamina, (int32)MaxShield,
            (int32)AttributeSet->GetHealthRegenAmount(), (int32)AttributeSet->GetHealthRegenRate());
    }

    UE_LOG(LogStatsManager, Log, TEXT("=== Initialization Complete ==="));
}



FGameplayAttribute UStatsManager::FindAttributeByTag(const FGameplayTag& Tag) const
{
	// Use the pre-built map from PHGameplayTags
	const FGameplayAttribute* Found = FPHGameplayTags::AllAttributesMap.Find(Tag.ToString());
    
	if (!Found)
	{
		UE_LOG(LogStatsManager, Verbose, TEXT("No attribute found for tag: %s"), *Tag.ToString());
		return FGameplayAttribute();
	}
    
	return *Found;
}


// New helper function to set current vitals to max
void UStatsManager::InitializeCurrentVitalsToMax()
{
    if (!IsInitialized() || !Owner)
    {
        return;
    }

    const UPHAttributeSet* AttributeSet = Cast<UPHAttributeSet>(
	    ASC->GetAttributeSet(UPHAttributeSet::StaticClass())
    );

    if (!AttributeSet)
    {
        UE_LOG(LogStatsManager, Error, TEXT("Could not get AttributeSet to initialize vitals"));
        return;
    }

    // Set current vitals to their max values
    ASC->SetNumericAttributeBase(UPHAttributeSet::GetHealthAttribute(), 
        AttributeSet->GetMaxHealth());
    
    ASC->SetNumericAttributeBase(UPHAttributeSet::GetManaAttribute(), 
        AttributeSet->GetMaxMana());
    
    ASC->SetNumericAttributeBase(UPHAttributeSet::GetStaminaAttribute(), 
        AttributeSet->GetMaxStamina());

    UE_LOG(LogStatsManager, Log, TEXT("✓ Current vitals set to max values"));
}
/* ============================= */
/* ===   Attribute Access    === */
/* ============================= */

float UStatsManager::GetAttributeBase(const FGameplayAttribute& Attribute) const
{
	if (!IsInitialized() || !Attribute.IsValid())
	{
		return 0.0f;
	}

	return ASC->GetNumericAttributeBase(Attribute);
}

float UStatsManager::GetAttributeCurrent(const FGameplayAttribute& Attribute) const
{
	if (!IsInitialized() || !Attribute.IsValid())
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

	bool bAttributeFound = false;
	ASC->GetGameplayAttributeValue(Attribute, bAttributeFound);
	return bAttributeFound;
}

void UStatsManager::SetAttributeBase(const FGameplayAttribute& Attribute, float NewValue)
{
	if (!IsInitialized() || !Attribute.IsValid())
	{
		UE_LOG(LogStatsManager, Warning, TEXT("Cannot set attribute - invalid ASC or attribute"));
		return;
	}

	ASC->SetNumericAttributeBase(Attribute, NewValue);
}

/* ============================= */
/* ===   Modifier Application === */
/* ============================= */

FActiveGameplayEffectHandle UStatsManager::ApplyFlatModifier(
	const FGameplayAttribute& Attribute, 
	float Value,
	bool bIsTemporary)
{
	if (!IsInitialized() || !Attribute.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	UGameplayEffect* Effect = CreateModifierEffect(Attribute, EGameplayModOp::AddBase, Value);
	if (!Effect)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
		Effect->GetClass(), 
		1.0f, 
		Context);

	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);

	if (bIsTemporary && ActiveHandle.IsValid())
	{
		TemporaryModifiers.Add(ActiveHandle);
	}

	UE_LOG(LogStatsManager, Log, TEXT("Applied flat modifier (Base): %s %+.2f"), 
		*Attribute.GetName(), Value);

	return ActiveHandle;
}

FActiveGameplayEffectHandle UStatsManager::ApplyFlatModifierFinal(
	const FGameplayAttribute& Attribute, 
	float Value,
	bool bIsTemporary)
{
	if (!IsInitialized() || !Attribute.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	UGameplayEffect* Effect = CreateModifierEffect(Attribute, EGameplayModOp::AddFinal, Value);
	if (!Effect)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
		Effect->GetClass(), 
		1.0f, 
		Context);

	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);

	if (bIsTemporary && ActiveHandle.IsValid())
	{
		TemporaryModifiers.Add(ActiveHandle);
	}

	UE_LOG(LogStatsManager, Log, TEXT("Applied flat modifier (Final): %s %+.2f"), 
		*Attribute.GetName(), Value);

	return ActiveHandle;
}

FActiveGameplayEffectHandle UStatsManager::ApplyPercentageModifier(
	const FGameplayAttribute& Attribute, 
	float Percentage,
	bool bIsTemporary)
{
	if (!IsInitialized() || !Attribute.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	// Convert percentage to decimal (25% -> 0.25)
	// MultiplyAdditive uses format: 1.0 + bonus, so 25% = 1.25
	float Multiplier = 1.0f + (Percentage / 100.0f);

	UGameplayEffect* Effect = CreateModifierEffect(
		Attribute, 
		EGameplayModOp::MultiplyAdditive, 
		Multiplier);

	if (!Effect)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
		Effect->GetClass(), 
		1.0f, 
		Context);

	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);

	if (bIsTemporary && ActiveHandle.IsValid())
	{
		TemporaryModifiers.Add(ActiveHandle);
	}

	UE_LOG(LogStatsManager, Log, TEXT("Applied percentage modifier (Additive): %s %+.1f%%"), 
		*Attribute.GetName(), Percentage);

	return ActiveHandle;
}

FActiveGameplayEffectHandle UStatsManager::ApplyMultiplierModifier(
	const FGameplayAttribute& Attribute, 
	float Multiplier,
	bool bIsTemporary)
{
	if (!IsInitialized() || !Attribute.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	UGameplayEffect* Effect = CreateModifierEffect(
		Attribute, 
		EGameplayModOp::MultiplyCompound, 
		Multiplier);

	if (!Effect)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
		Effect->GetClass(), 
		1.0f, 
		Context);

	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);

	if (bIsTemporary && ActiveHandle.IsValid())
	{
		TemporaryModifiers.Add(ActiveHandle);
	}

	UE_LOG(LogStatsManager, Log, TEXT("Applied multiplier (Compound): %s x%.2f"), 
		*Attribute.GetName(), Multiplier);

	return ActiveHandle;
}

FActiveGameplayEffectHandle UStatsManager::ApplyOverrideModifier(
	const FGameplayAttribute& Attribute, 
	float Value)
{
	if (!IsInitialized() || !Attribute.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	UGameplayEffect* Effect = CreateModifierEffect(
		Attribute, 
		EGameplayModOp::Override, 
		Value);

	if (!Effect)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
		Effect->GetClass(), 
		1.0f, 
		Context);

	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);

	UE_LOG(LogStatsManager, Log, TEXT("Applied override: %s = %.2f"), 
		*Attribute.GetName(), Value);

	return ActiveHandle;
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
	if (!IsInitialized() || !Attribute.IsValid())
	{
		return 0;
	}

	int32 RemovedCount = 0;
	
	// GetActiveEffects returns the array directly, not as an out parameter
	FGameplayEffectQuery Query;
	Query.EffectSource = ASC;
	
	TArray<FActiveGameplayEffectHandle> EffectsToRemove = ASC->GetActiveEffects(Query);

	for (const FActiveGameplayEffectHandle& Handle : EffectsToRemove)
	{
		const UGameplayEffect* Effect = ASC->GetGameplayEffectDefForHandle(Handle);
		if (!Effect)
		{
			continue;
		}

		// Check if this effect modifies our target attribute
		for (const FGameplayModifierInfo& Modifier : Effect->Modifiers)
		{
			if (Modifier.Attribute == Attribute)
			{
				if (ASC->RemoveActiveGameplayEffect(Handle))
				{
					RemovedCount++;
					TemporaryModifiers.Remove(Handle);
				}
				break;
			}
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

	for (int32 i = TemporaryModifiers.Num() - 1; i >= 0; --i)
	{
		if (TemporaryModifiers[i].IsValid())
		{
			if (ASC->RemoveActiveGameplayEffect(TemporaryModifiers[i]))
			{
				RemovedCount++;
			}
		}
		TemporaryModifiers.RemoveAt(i);
	}

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

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, Level, Context);
	
	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	return ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

FActiveGameplayEffectHandle UStatsManager::ApplyGameplayEffectSpecToSelf(
	const FGameplayEffectSpec& Spec)
{
	if (!IsInitialized())
	{
		return FActiveGameplayEffectHandle();
	}

	return ASC->ApplyGameplayEffectSpecToSelf(Spec);
}

void UStatsManager::InitRegen()
{
	// Apply regen effects
	if (ASC)
	{
		// Apply health regen
		if (HealthRegenEffectClass)
		{
			ApplyGameplayEffectToSelf(HealthRegenEffectClass, 1.0f);
		}
        
		// Apply mana regen
		if (ManaRegenEffectClass)
		{
			ApplyGameplayEffectToSelf(ManaRegenEffectClass, 1.0f);
		}
        
		// Apply stamina regen
		if (StaminaRegenEffectClass)
		{
			ApplyGameplayEffectToSelf(StaminaRegenEffectClass, 1.0f);
		}
	}
}

bool UStatsManager::ApplyInitializationEffect(
    const TSubclassOf<UGameplayEffect>& EffectClass,
    const TMap<FGameplayTag, float>& AttributeValues)
{
    if (!IsInitialized() || !EffectClass)
    {
        UE_LOG(LogStatsManager, Error, TEXT("Cannot apply init effect - ASC: %s, EffectClass: %s"),
            ASC ? TEXT("Valid") : TEXT("NULL"),
            EffectClass ? TEXT("Valid") : TEXT("NULL"));
        return false;
    }

    FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
    Context.AddSourceObject(GetOwner());

    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, Context);
    
    if (!SpecHandle.IsValid())
    {
        UE_LOG(LogStatsManager, Error, TEXT("Failed to create spec for %s"), 
            *EffectClass->GetName());
        return false;
    }

    // Validate SpecHandle.Data before using it
    if (!SpecHandle.Data.IsValid())
    {
        UE_LOG(LogStatsManager, Error, TEXT("SpecHandle.Data is invalid for %s"), 
            *EffectClass->GetName());
        return false;
    }

    // Set all the SetByCaller magnitudes
    for (const auto& Pair : AttributeValues)
    {
        SpecHandle.Data->SetSetByCallerMagnitude(Pair.Key, Pair.Value);
        UE_LOG(LogStatsManager, Verbose, TEXT("  Set %s = %.2f"), 
            *Pair.Key.ToString(), Pair.Value);
    }

    // Final validation before applying
    UE_LOG(LogStatsManager, Warning, TEXT("About to apply spec - ASC Valid: %s, SpecHandle Valid: %s"),
        ASC ? TEXT("YES") : TEXT("NO"),
        SpecHandle.IsValid() ? TEXT("YES") : TEXT("NO"));

    // Store spec pointer to help compiler
    FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
    if (!Spec)
    {
        UE_LOG(LogStatsManager, Error, TEXT("Spec pointer is null after Get()"));
        return false;
    }

    // Apply the effect
    FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec);
    
    if (!Handle.IsValid())
    {
        UE_LOG(LogStatsManager, Error, TEXT("Failed to apply effect %s"), 
            *EffectClass->GetName());
        return false;
    }

    UE_LOG(LogStatsManager, Log, TEXT("Successfully applied effect %s"), 
        *EffectClass->GetName());

    return true;
}

bool UStatsManager::ApplySimpleInitEffect(const TSubclassOf<UGameplayEffect>& EffectClass) const
{
	if (!IsInitialized() || !EffectClass)
	{
		return false;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, Context);
	
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
	return Handle.IsValid();
}

/* ============================= */
/* ===   Internal Helpers    === */
/* ============================= */

UGameplayEffect* UStatsManager::CreateModifierEffect(
	const FGameplayAttribute& Attribute,
	EGameplayModOp::Type ModifierOp,
	float Magnitude)
{
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), 
		FName(TEXT("DynamicModifier")));

	Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = ModifierOp;
	Modifier.ModifierMagnitude = FScalableFloat(Magnitude);

	Effect->Modifiers.Add(Modifier);

	return Effect;
}
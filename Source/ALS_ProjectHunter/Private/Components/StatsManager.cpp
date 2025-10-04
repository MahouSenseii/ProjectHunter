#include "Components/StatsManager.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
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

void UStatsManager::InitializeAttributes()
{
	if (!IsInitialized())
	{
		UE_LOG(LogStatsManager, Error, TEXT("Cannot initialize attributes - ASC not initialized"));
		return;
	}

	UE_LOG(LogStatsManager, Log, TEXT("=== Initializing Attributes ==="));

	// Apply in specific order: Primary -> Secondary Max -> Secondary Current -> Vitals
	// This ensures dependencies are met (e.g., MaxHealth calculated before setting Health)

	if (DefaultPrimaryAttributes)
	{
		if (ApplySimpleInitEffect(DefaultPrimaryAttributes))
		{
			UE_LOG(LogStatsManager, Log, TEXT("✓ Primary attributes initialized"));
		}
		else
		{
			UE_LOG(LogStatsManager, Error, TEXT("✗ Failed to initialize primary attributes"));
		}
	}

	if (DefaultSecondaryMaxAttributes)
	{
		if (ApplySimpleInitEffect(DefaultSecondaryMaxAttributes))
		{
			UE_LOG(LogStatsManager, Log, TEXT("✓ Secondary max attributes initialized"));
		}
		else
		{
			UE_LOG(LogStatsManager, Error, TEXT("✗ Failed to initialize secondary max attributes"));
		}
	}

	if (DefaultSecondaryCurrentAttributes)
	{
		if (ApplySimpleInitEffect(DefaultSecondaryCurrentAttributes))
		{
			UE_LOG(LogStatsManager, Log, TEXT("✓ Secondary current attributes initialized"));
		}
		else
		{
			UE_LOG(LogStatsManager, Error, TEXT("✗ Failed to initialize secondary current attributes"));
		}
	}

	if (DefaultVitalAttributes)
	{
		if (ApplySimpleInitEffect(DefaultVitalAttributes))
		{
			UE_LOG(LogStatsManager, Log, TEXT("✓ Vital attributes initialized"));
		}
		else
		{
			UE_LOG(LogStatsManager, Error, TEXT("✗ Failed to initialize vital attributes"));
		}
	}

	UE_LOG(LogStatsManager, Log, TEXT("=== Attribute Initialization Complete ==="));
}


void UStatsManager::InitializeAttributesFromConfig(UAttributeConfigDataAsset* Config)
{
    if (!IsInitialized() || !Config)
    {
        UE_LOG(LogStatsManager, Error, TEXT("Cannot initialize - invalid ASC or Config"));
        return;
    }

    UE_LOG(LogStatsManager, Log, TEXT("=== Initializing Attributes from Config ==="));

    // Build value maps from config
    TMap<FGameplayTag, float> PrimaryValues;
    TMap<FGameplayTag, float> SecondaryMaxValues;
    TMap<FGameplayTag, float> SecondaryCurrentValues;
    TMap<FGameplayTag, float> VitalValues;

    // Primary
    for (const FAttributeInitConfig& Attr : Config->PrimaryAttributes)
    {
        if (Attr.bEnabled && Attr.AttributeTag.IsValid())
        {
            PrimaryValues.Add(Attr.AttributeTag, Attr.DefaultValue);
        }
    }

    // Secondary (split into Max vs Current if you have them separated in data;
    // if your Config only has one Secondary array, you can branch by tag prefix or an enum in FAttributeInitConfig)
    for (const FAttributeInitConfig& Attr : Config->SecondaryAttributes)
    {
        if (Attr.bEnabled && Attr.AttributeTag.IsValid())
        {
            // Example split by naming convention; adjust to your schema:
            const FString TagStr = Attr.AttributeTag.ToString();
            if (TagStr.Contains(TEXT(".Max")) || TagStr.EndsWith(TEXT("Max")))
            {
                SecondaryMaxValues.Add(Attr.AttributeTag, Attr.DefaultValue);
            }
            else
            {
                SecondaryCurrentValues.Add(Attr.AttributeTag, Attr.DefaultValue);
            }
        }
    }

    // Vital (usually Max values live here)
    for (const FAttributeInitConfig& Attr : Config->VitalAttributes)
    {
        if (Attr.bEnabled && Attr.AttributeTag.IsValid())
        {
            VitalValues.Add(Attr.AttributeTag, Attr.DefaultValue);
        }
    }

    // --- Apply, using the correct path for each GE ---

    // Primary (expects SetByCaller)
    if (DefaultPrimaryAttributes && PrimaryValues.Num() > 0)
    {
        if (!ApplyInitializationEffect(DefaultPrimaryAttributes, PrimaryValues))
        {
            UE_LOG(LogStatsManager, Error, TEXT("Failed to apply effect %s"), *DefaultPrimaryAttributes->GetName());
        }
    }

    // Secondary Max (DO NOT use ApplySimpleInitEffect if this GE uses SetByCaller)
    if (DefaultSecondaryMaxAttributes)
    {
        if (SecondaryMaxValues.Num() > 0)
        {
            if (!ApplyInitializationEffect(DefaultSecondaryMaxAttributes, SecondaryMaxValues))
            {
                UE_LOG(LogStatsManager, Error, TEXT("Failed to apply effect %s"), *DefaultSecondaryMaxAttributes->GetName());
            }
        }
        else
        {
            // Only safe to do this if the GE uses constants/curves (no SetByCaller)
            if (!ApplySimpleInitEffect(DefaultSecondaryMaxAttributes))
            {
                UE_LOG(LogStatsManager, Error, TEXT("Failed to apply effect %s"), *DefaultSecondaryMaxAttributes->GetName());
            }
        }
    }

    // Vital Max (expects SetByCaller)
    if (DefaultVitalAttributes && VitalValues.Num() > 0)
    {
        if (!ApplyInitializationEffect(DefaultVitalAttributes, VitalValues))
        {
            UE_LOG(LogStatsManager, Error, TEXT("Failed to apply effect %s"), *DefaultVitalAttributes->GetName());
        }

        // (Optional) Snap current to max once on init — either via direct set or a tiny GE.
        // If you prefer GE-based, build a map of Current tags = same values and apply another SetByCaller GE.
        // Otherwise, do a one-time direct write in your AttributeSet/manager.
    }

    // Secondary Current (expects SetByCaller)
    if (DefaultSecondaryCurrentAttributes && SecondaryCurrentValues.Num() > 0)
    {
        if (!ApplyInitializationEffect(DefaultSecondaryCurrentAttributes, SecondaryCurrentValues))
        {
            UE_LOG(LogStatsManager, Error, TEXT("Failed to apply effect %s"), *DefaultSecondaryCurrentAttributes->GetName());
        }
    }

    UE_LOG(LogStatsManager, Log, TEXT("=== Initialization Complete ==="));
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

bool UStatsManager::ApplyInitializationEffect(const TSubclassOf<UGameplayEffect>& EffectClass,
	const TMap<FGameplayTag, float>& AttributeValues) const
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
		UE_LOG(LogStatsManager, Error, TEXT("Failed to create spec for %s"), 
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

	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
	
	if (!Handle.IsValid())
	{
		UE_LOG(LogStatsManager, Error, TEXT("Failed to apply effect %s"), 
			*EffectClass->GetName());
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
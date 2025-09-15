// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/StatsManager.h"
#include "AbilitySystem/Data/AttributeConfigDataAsset.h"
#include "Library/AttributeStructsLibrary.h"
#include "Library/PHCharacterStructLibrary.h"
#include "AbilitySystem/PHAbilitySystemComponent.h"
#include "Character/PHBaseCharacter.h"

namespace
{
	constexpr float MinAllowedEffectPeriod = 0.1f;
}

DEFINE_LOG_CATEGORY(LogStatsManager);

#if WITH_EDITOR
void UStatsManager::DebugPrintStartupEffects() const
{
	UE_LOG(LogStatsManager, Display, TEXT("=== Startup Effects Configuration ==="));
	for (int32 i = 0; i < StartupEffects.Num(); i++)
	{
		const FInitialGameplayEffectInfo& Info = StartupEffects[i];
		UE_LOG(LogStatsManager, Display, TEXT("[%d] %s:"), i, *Info.DisplayName);
		UE_LOG(LogStatsManager, Display, TEXT("  - Class: %s"), 
			Info.EffectClass ? *Info.EffectClass->GetName() : TEXT("NULL"));
		UE_LOG(LogStatsManager, Display, TEXT("  - SetByCallerTag: %s"), 
			Info.SetByCallerTag.IsValid() ? *Info.SetByCallerTag.ToString() : TEXT("None"));
		UE_LOG(LogStatsManager, Display, TEXT("  - TriggerTag: %s"), 
			Info.TriggerTag.IsValid() ? *Info.TriggerTag.ToString() : TEXT("None"));
		UE_LOG(LogStatsManager, Display, TEXT("  - Rate: %.2f (Attr: %s)"), 
			Info.DefaultRate, 
			Info.RateAttribute.IsValid() ? *Info.RateAttribute.GetName() : TEXT("None"));
		UE_LOG(LogStatsManager, Display, TEXT("  - Amount: %.2f (Attr: %s)"), 
			Info.DefaultAmount, 
			Info.AmountAttribute.IsValid() ? *Info.AmountAttribute.GetName() : TEXT("None"));
		UE_LOG(LogStatsManager, Display, TEXT("  - Level: %.1f"), Info.Level);
	}
}
#endif

// Sets default values for this component's properties
UStatsManager::UStatsManager()
{
	// We don't need a tick for this component
	PrimaryComponentTick.bCanEverTick = false;
	
	// Don't initialize Owner here - it will be null in the constructor
	Owner = nullptr;
	ASC = nullptr;
	bIsInitialized = false;
	bIsInitializingAttributes = false;
}

void UStatsManager::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize Owner here when the component is properly attached
	Owner = GetOwner<APHBaseCharacter>();
	
	// Auto-initialize if the Owner has ASC ready
	if (Owner && Owner->GetAbilitySystemComponent())
	{
		InitStatsManager(Cast<UPHAbilitySystemComponent>(Owner->GetAbilitySystemComponent()));
	}
}

void UStatsManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	ClearAllPeriodicEffects();
	
	// Clear delegates to prevent dangling references
	if (ASC)
	{
		for (const FInitialGameplayEffectInfo& EffectInfo : StartupEffects)
		{
			if (EffectInfo.RateAttribute.IsValid())
			{
				ASC->GetGameplayAttributeValueChangeDelegate(EffectInfo.RateAttribute)
					.RemoveAll(this);
			}

			if (EffectInfo.AmountAttribute.IsValid())
			{
				ASC->GetGameplayAttributeValueChangeDelegate(EffectInfo.AmountAttribute)
					.RemoveAll(this);
			}

			if (EffectInfo.TriggerTag.IsValid())
			{
				ASC->RegisterGameplayTagEvent(EffectInfo.TriggerTag, EGameplayTagEventType::NewOrRemoved)
					.RemoveAll(this);
			}
		}
	}
}

float UStatsManager::GetStatBase(const FGameplayAttribute& Attr) const
{
	if (!Attr.IsValid() || !ASC) return 0.0f;

	return ASC->GetNumericAttribute(Attr);
}

void UStatsManager::ApplyFlatStatModifier(const FGameplayAttribute& Attr, const float InValue) const
{
	if (!Attr.IsValid() || !ASC) return;

	const float CurrentValue = ASC->GetNumericAttribute(Attr);
	const float NewValue = CurrentValue + InValue;

	ASC->SetNumericAttributeBase(Attr, NewValue);
}

void UStatsManager::RemoveFlatStatModifier(const FGameplayAttribute& Attribute, float Delta)
{
	if (!Attribute.IsValid() || !ASC) return;

	const float CurrentValue = ASC->GetNumericAttribute(Attribute);
	const float NewValue = CurrentValue - Delta;

	ASC->SetNumericAttributeBase(Attribute, NewValue);
}

void UStatsManager::InitStatsManager(UPHAbilitySystemComponent* InASC)
{
    if (bIsInitialized)
    {
        UE_LOG(LogStatsManager, Warning, TEXT("StatsManager already initialized!"));
        return;
    }

    // FIRST: Validate and assign ASC
    if (!InASC)
    {
        UE_LOG(LogStatsManager, Error, TEXT("InitStatsManager called with null ASC!"));
        return;
    }
    
    APHBaseCharacter* SafeOwner = GetOwnerSafe();
    if (!IsValid(SafeOwner))
    {
        UE_LOG(LogStatsManager, Error, TEXT("StatsManager has invalid owner!"));
        return;
    }
    
    // Assign ASC BEFORE using it
    ASC = InASC;

    // SECOND: Validate default effect classes
    if (!DefaultPrimaryAttributes || !DefaultSecondaryCurrentAttributes || 
        !DefaultSecondaryMaxAttributes || !DefaultVitalAttributes)
    {
        UE_LOG(LogStatsManager, Error, TEXT("StatsManager missing required default effect classes!"));
        return;
    }

    // THIRD: Validate StartupEffects
	for (const FInitialGameplayEffectInfo& EffectInfo : StartupEffects)
	{
		if (!EffectInfo.EffectClass)
		{
			UE_LOG(LogStatsManager, Error, TEXT("StartupEffect '%s' missing EffectClass!"), 
				*EffectInfo.DisplayName);
			continue;
		}
        
		if (!EffectInfo.SetByCallerTag.IsValid())
		{
			UE_LOG(LogStatsManager, Warning, TEXT("StartupEffect '%s' missing SetByCallerTag!"), 
				*EffectInfo.DisplayName);
		}
        
		if (EffectInfo.RateAttribute.IsValid())
		{
			bool bFound = false;
			ASC->GetGameplayAttributeValue(EffectInfo.RateAttribute, bFound);
			if (!bFound)
			{
				UE_LOG(LogStatsManager, Warning, TEXT("StartupEffect '%s' references invalid RateAttribute"), 
					*EffectInfo.DisplayName);
			}
		}
        
		if (EffectInfo.AmountAttribute.IsValid())
		{
			bool bFound = false;
			ASC->GetGameplayAttributeValue(EffectInfo.AmountAttribute, bFound);
			if (!bFound)
			{
				UE_LOG(LogStatsManager, Warning, TEXT("StartupEffect '%s' references invalid AmountAttribute"), 
					*EffectInfo.DisplayName);
			}
		}
	}

    // Temporarily disable callbacks during initialization to prevent recursion
    bIsInitializingAttributes = true;

    // Bind delegates for startup effects
    for (const FInitialGameplayEffectInfo& EffectInfo : StartupEffects)
    {
        // Bind rate/amount change listeners
        if (EffectInfo.RateAttribute.IsValid())
        {
            ASC->GetGameplayAttributeValueChangeDelegate(EffectInfo.RateAttribute)
                .AddUObject(this, &UStatsManager::OnAnyVitalPeriodicStatChanged);
        }

        if (EffectInfo.AmountAttribute.IsValid())
        {
            ASC->GetGameplayAttributeValueChangeDelegate(EffectInfo.AmountAttribute)
                .AddUObject(this, &UStatsManager::OnAnyVitalPeriodicStatChanged);
        }

        // Bind trigger tag change listener
        if (EffectInfo.TriggerTag.IsValid())
        {
            ASC->RegisterGameplayTagEvent(EffectInfo.TriggerTag, EGameplayTagEventType::NewOrRemoved)
                .AddUObject(this, &UStatsManager::OnRegenTagChanged);
        }
    }

    // Initialize attributes FIRST (they need to exist before effects can use them)
    InitializeDefaultAttributes();

    // Apply periodic effects AFTER attributes are initialized
    for (const FInitialGameplayEffectInfo& EffectInfo : StartupEffects)
    {
        ApplyPeriodicEffectToSelf(EffectInfo);
    }

#if WITH_EDITOR
	DebugPrintStartupEffects();
#endif

    bIsInitializingAttributes = false;
    bIsInitialized = true;
    
    UE_LOG(LogStatsManager, Log, TEXT("StatsManager initialized successfully for %s"), 
        *SafeOwner->GetName());
}

void UStatsManager::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float InLevel) const
{
	APHBaseCharacter* SafeOwner = GetOwnerSafe();
	if (!IsValid(ASC) || !IsValid(SafeOwner) || !GameplayEffectClass)
	{
		UE_LOG(LogStatsManager, Warning, TEXT("Cannot apply effect - invalid ASC, Owner, or Effect class"));
		return;
	}
	
	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(SafeOwner);
	
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GameplayEffectClass, InLevel, ContextHandle);
	if (SpecHandle.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), ASC);
	}
}

FActiveGameplayEffectHandle UStatsManager::ApplyEffectToSelfWithReturn(TSubclassOf<UGameplayEffect> InEffect, float InLevel)
{
	if (!InEffect || !ASC) return FActiveGameplayEffectHandle();

	const FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(InEffect, InLevel, Context);

	if (SpecHandle.IsValid())
	{
		return ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

	return FActiveGameplayEffectHandle();
}

void UStatsManager::ApplyPeriodicEffectToSelf(const FInitialGameplayEffectInfo& EffectInfo)
{
	if (!IsValid(ASC) || !EffectInfo.EffectClass)
	{
		return;
	}

	// Remove the existing effect if it exists
	if (EffectInfo.SetByCallerTag.IsValid())
	{
		RemovePeriodicEffectByTag(EffectInfo.SetByCallerTag);
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectInfo.EffectClass, EffectInfo.Level, Context);

	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogStatsManager, Error, TEXT("Failed to create spec for effect '%s'"), 
			*EffectInfo.DisplayName);
		return;
	}

	const float RawRate = EffectInfo.RateAttribute.IsValid()
		? ASC->GetNumericAttribute(EffectInfo.RateAttribute)
		: EffectInfo.DefaultRate;

	const float ClampedRate = FMath::Max(RawRate, MinAllowedEffectPeriod);

	const float RawAmount = EffectInfo.AmountAttribute.IsValid()
		? ASC->GetNumericAttribute(EffectInfo.AmountAttribute)
		: EffectInfo.DefaultAmount;

	if (EffectInfo.SetByCallerTag.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(EffectInfo.SetByCallerTag, RawAmount);
	}
	else
	{
		UE_LOG(LogStatsManager, Warning, TEXT("[StatsManager] Missing SetByCallerTag for %s! Default magnitude not set."),
			*EffectInfo.EffectClass->GetName());
	}
	
	SpecHandle.Data->Period = ClampedRate;
	
	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), ASC);

	if (Handle.IsValid())
    {
        if (EffectInfo.SetByCallerTag.IsValid())
        {
            ActivePeriodicEffects.Add(EffectInfo.SetByCallerTag, Handle);
        }
        
        UE_LOG(LogStatsManager, Verbose, TEXT("Applied '%s': Amount=%.2f, Period=%.2f"), 
            *EffectInfo.DisplayName, RawAmount, ClampedRate);
    }
    else
    {
        UE_LOG(LogStatsManager, Error, TEXT("Failed to apply periodic effect '%s'"), 
            *EffectInfo.DisplayName);
    }

#if WITH_EDITOR
	UE_LOG(LogStatsManager, Log, TEXT("[GAS] Applied %s with Amount %.2f, Period %.2f"),
		*EffectInfo.EffectClass->GetName(), RawAmount, ClampedRate);
#endif
}

void UStatsManager::ClearAllPeriodicEffects()
{
	if (!ASC) return;
		
	TArray<FActiveGameplayEffectHandle> Handles;
	ActivePeriodicEffects.GenerateValueArray(Handles);

	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	ActivePeriodicEffects.Empty();
}

bool UStatsManager::HasValidPeriodicEffect(FGameplayTag EffectTag) const
{
	if (!ASC) return false;

	TArray<FActiveGameplayEffectHandle> Handles;
	ActivePeriodicEffects.MultiFind(EffectTag, Handles);

	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		if (Handle.IsValid() && ASC->GetActiveGameplayEffect(Handle))
		{
			return true;
		}
	}

	return false;
}

void UStatsManager::PurgeInvalidPeriodicHandles()
{
	TArray<FGameplayTag> PHTags;
	ActivePeriodicEffects.GetKeys(PHTags);

	for (const FGameplayTag& Tag : PHTags)
	{
		TArray<FActiveGameplayEffectHandle> Handles;
		ActivePeriodicEffects.MultiFind(Tag, Handles);

		for (int32 i = Handles.Num() - 1; i >= 0; --i)
		{
			if (!Handles[i].IsValid())
			{
				ActivePeriodicEffects.RemoveSingle(Tag, Handles[i]);
			}
		}
	}
}

void UStatsManager::RemovePeriodicEffectByTag(FGameplayTag EffectTag)
{
	if (!ASC || !EffectTag.IsValid()) return;

	TArray<FActiveGameplayEffectHandle> Handles;
	ActivePeriodicEffects.MultiFind(EffectTag, Handles);

	int32 RemovedCount = 0;
	
	// Remove valid handles from ASC and clean up invalid ones
	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		if (Handle.IsValid() && ASC->GetActiveGameplayEffect(Handle))
		{
			ASC->RemoveActiveGameplayEffect(Handle);
			RemovedCount++;
		}
	}

	// Remove all entries for this tag (both valid and invalid)
	if (ActivePeriodicEffects.Contains(EffectTag))
	{
		ActivePeriodicEffects.Remove(EffectTag);
	}
    
#if WITH_EDITOR
	if (RemovedCount > 0)
	{
		UE_LOG(LogStatsManager, Verbose, TEXT("Removed %d periodic effects for tag %s"), 
			RemovedCount, *EffectTag.ToString());
	}
#endif
}

void UStatsManager::OnRegenTagChanged(FGameplayTag ChangedTag, int32 NewCount)
{
	if (bIsInitializingAttributes) return;

	for (const FInitialGameplayEffectInfo& EffectInfo : StartupEffects)
	{
		if (EffectInfo.TriggerTag == ChangedTag)
		{
			if (NewCount > 0)
			{
				ApplyPeriodicEffectToSelf(EffectInfo);
				UE_LOG(LogStatsManager, Verbose, TEXT("Tag %s activated - applying '%s'"), 
					*ChangedTag.ToString(), *EffectInfo.DisplayName);
			}
			else
			{
				RemovePeriodicEffectByTag(EffectInfo.SetByCallerTag);
				UE_LOG(LogStatsManager, Verbose, TEXT("Tag %s deactivated - removing '%s'"), 
					*ChangedTag.ToString(), *EffectInfo.DisplayName);
			}
		}
	}
}

void UStatsManager::OnAnyVitalPeriodicStatChanged(const FOnAttributeChangeData& Data)
{
	// Prevent recursive calls during initialization
	if (bIsInitializingAttributes) return;

	for (const FInitialGameplayEffectInfo& EffectInfo : StartupEffects)
	{
		if (EffectInfo.RateAttribute == Data.Attribute || EffectInfo.AmountAttribute == Data.Attribute)
		{
			ApplyPeriodicEffectToSelf(EffectInfo);
		}
	}
}

APHBaseCharacter* UStatsManager::GetOwnerSafe() const
{
	if (GetOwner())
	{
		return Cast<APHBaseCharacter>(GetOwner());
	}
	return nullptr; 
}

float UStatsManager::ValidateInitValue(float Value, const FString& AttributeName)
{
	if (FMath::IsNaN(Value) || !FMath::IsFinite(Value))
	{
		UE_LOG(LogStatsManager, Error, TEXT("Invalid init value for %s: %f"), *AttributeName, Value);
		return 0.0f;
	}
	return FMath::Clamp(Value, -9999.0f, 9999.0f);
}

void UStatsManager::InitializeAttributesFromConfig(const TSubclassOf<UGameplayEffect>& EffectClass, 
                                                   const TArray<FAttributeInitConfig>& AttributeConfigs,
                                                   const FString& CategoryName) const
{
	if (!EffectClass || !ASC || AttributeConfigs.IsEmpty()) 
	{
		UE_LOG(LogStatsManager, Warning, TEXT("Cannot initialize %s attributes - invalid parameters"), *CategoryName);
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(Owner);
	
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, Context);
	if (!SpecHandle.IsValid()) 
	{
		UE_LOG(LogStatsManager, Error, TEXT("Failed to create effect spec for %s attributes"), *CategoryName);
		return;
	}

	int32 SetCount = 0;
	int32 SkippedCount = 0;
	
	for (const FAttributeInitConfig& Config : AttributeConfigs)
	{
		// Skip disabled attributes
		if (!Config.bEnabled)
		{
			SkippedCount++;
			continue;
		}
		
		if (!Config.AttributeTag.IsValid())
		{
			UE_LOG(LogStatsManager, Warning, TEXT("Invalid tag in %s config: %s"), 
				   *CategoryName, *Config.DisplayName);
			SkippedCount++;
			continue;
		}

		// Validate and clamp the value
		float ClampedValue = FMath::Clamp(Config.DefaultValue, Config.MinValue, Config.MaxValue);
		ClampedValue = ValidateInitValue(ClampedValue, Config.DisplayName);
		
		// Set the magnitude
		SpecHandle.Data->SetSetByCallerMagnitude(Config.AttributeTag, ClampedValue);
		SetCount++;
		
		UE_LOG(LogStatsManager, Verbose, TEXT("Set %s.%s = %.2f (range: %.1f-%.1f)"), 
			   *CategoryName, *Config.DisplayName, ClampedValue, Config.MinValue, Config.MaxValue);
	}

	// Apply the effect if we set any values
	if (SetCount > 0)
	{
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
		UE_LOG(LogStatsManager, Log, TEXT("Applied %s attributes: %d set, %d skipped"), 
			   *CategoryName, SetCount, SkippedCount);
	}
	else
	{
		UE_LOG(LogStatsManager, Warning, TEXT("No valid %s attributes to apply"), *CategoryName);
	}
}

void UStatsManager::ApplyLevelScalingToConfig() const
{
	if (!AttributeConfig || MobLevel <= 1) return;

	float HealthScaling = 1.0f + (0.15f * (MobLevel - 1)); // 15% HP per level
	float DamageScaling = 1.0f + (0.10f * (MobLevel - 1)); // 10% damage per level
	
	// Apply scaling to specific attributes
	for (FAttributeInitConfig& Config : AttributeConfig->VitalAttributes)
	{
		if (Config.AttributeTag.MatchesTag(FGameplayTag::RequestGameplayTag("Attributes.Vital.Health")))
		{
			Config.DefaultValue *= HealthScaling;
		}
	}

	for (FAttributeInitConfig& Config : AttributeConfig->PrimaryAttributes) 
	{
		if (Config.AttributeTag.MatchesTag(FGameplayTag::RequestGameplayTag("Attributes.Primary.Strength")))
		{
			Config.DefaultValue *= DamageScaling;
		}
	}
}

void UStatsManager::InitializeDefaultAttributes()
{
	check(Owner);
	if (!IsValid(ASC)) return;
	
	bIsInitializingAttributes = true;

	if (AttributeConfig)
	{
		// Apply level scaling to mob configs if needed
		if (MobLevel > 1)
		{
			ApplyLevelScalingToConfig();
		}

		// Use the data asset configuration
		InitializeAttributesFromConfig(DefaultPrimaryAttributes, AttributeConfig->PrimaryAttributes, TEXT("Primary"));
		
		// Apply max attributes first (they set the caps)
		ApplyEffectToSelf(DefaultSecondaryMaxAttributes, 1.0f);
		
		// Then apply current secondary attributes
		InitializeAttributesFromConfig(DefaultSecondaryCurrentAttributes, AttributeConfig->SecondaryAttributes, TEXT("Secondary"));
		
		// Finally, apply vital attributes
		InitializeAttributesFromConfig(DefaultVitalAttributes, AttributeConfig->VitalAttributes, TEXT("Vital"));
	}
	else
	{
		UE_LOG(LogStatsManager, Warning, TEXT("No AttributeConfig asset assigned for %s!"), *Owner->GetName());
	}

	bIsInitializingAttributes = false;
	UE_LOG(LogStatsManager, Log, TEXT("Attributes initialized for %s (Level: %d)"), *Owner->GetName(), MobLevel);
}

void UStatsManager::InitializeMobAttributesAtLevel(int32 Level)
{
	if (!Owner || !ASC) return;

	// Calculate scaling factors
	float LevelMultiplier = 1.0f + (0.15f * (Level - 1)); // 15% per level
	float EliteMultiplier = bIsElite ? 1.5f : 1.0f; // 50% more of elite
	float FinalMultiplier = LevelMultiplier * DifficultyMultiplier * EliteMultiplier;

	if (AttributeConfig)
	{
		// Copy config arrays so we can modify them
		TArray<FAttributeInitConfig> ModifiedPrimary = AttributeConfig->PrimaryAttributes;
		TArray<FAttributeInitConfig> ModifiedSecondary = AttributeConfig->SecondaryAttributes;
		TArray<FAttributeInitConfig> ModifiedVital = AttributeConfig->VitalAttributes;

		// Apply scaling to health and damage
		for (FAttributeInitConfig& Config : ModifiedVital)
		{
			if (Config.AttributeTag.MatchesTagExact(FGameplayTag::RequestGameplayTag("Attributes.Vital.Health")))
			{
				Config.DefaultValue = FMath::RoundToInt(Config.DefaultValue * FinalMultiplier);
			}
		}

		for (FAttributeInitConfig& Config : ModifiedPrimary)
		{
			if (Config.AttributeTag.MatchesTagExact(FGameplayTag::RequestGameplayTag("Attributes.Primary.Strength")))
			{
				Config.DefaultValue = FMath::RoundToInt(Config.DefaultValue * (1.0f + (0.10f * (Level - 1))));
			}
		}

		// Apply the scaled attributes
		bIsInitializingAttributes = true;
		InitializeAttributesFromConfig(DefaultPrimaryAttributes, ModifiedPrimary, TEXT("Primary"));
		ApplyEffectToSelf(DefaultSecondaryMaxAttributes, 1.0f);
		InitializeAttributesFromConfig(DefaultSecondaryCurrentAttributes, ModifiedSecondary, TEXT("Secondary"));
		InitializeAttributesFromConfig(DefaultVitalAttributes, ModifiedVital, TEXT("Vital"));
		bIsInitializingAttributes = false;

		UE_LOG(LogStatsManager, Log, TEXT("Mob %s: Level=%d, Elite=%s, Multiplier=%.2fx"), 
			   *Owner->GetName(), Level, bIsElite ? TEXT("Yes") : TEXT("No"), FinalMultiplier);
	}
	else
	{
		UE_LOG(LogStatsManager, Warning, TEXT("No AttributeConfig for mob level initialization!"));
		InitializeDefaultAttributes();
	}
}

void UStatsManager::ReapplyAllStartupRegenEffects()
{
	for (const FInitialGameplayEffectInfo& EffectInfo : StartupEffects)
	{
		if (EffectInfo.SetByCallerTag.MatchesTag(FGameplayTag::RequestGameplayTag("Attribute.Secondary.Vital.ManaRegenAmount")) ||
			EffectInfo.SetByCallerTag.MatchesTag(FGameplayTag::RequestGameplayTag("Attribute.Secondary.Vital.StaminaRegenAmount")) ||
			EffectInfo.SetByCallerTag.MatchesTag(FGameplayTag::RequestGameplayTag("Attribute.Secondary.Vital.HealthRegenAmount")))
		{
			ApplyPeriodicEffectToSelf(EffectInfo);
		}
	}
}
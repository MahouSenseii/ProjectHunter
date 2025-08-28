// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/StatsManager.h"

#include "PHGameplayTags.h"
#include "AbilitySystem/PHAbilitySystemComponent.h"
#include "Character/PHBaseCharacter.h"
#include "Library/PHCharacterStructLibrary.h"

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
	// We don't need tick for this component
	PrimaryComponentTick.bCanEverTick = false;
	
	// Don't initialize Owner here - it will be null in constructor
	Owner = nullptr;
	ASC = nullptr;
	bIsInitialized = false; // Add this flag to header file
}

void UStatsManager::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize Owner here when component is properly attached
	Owner = GetOwner<APHBaseCharacter>();
	
	// Auto-initialize if Owner has ASC ready
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

	// May add later if needed if going - causes issues or to clamp max  
	// const float ClampedValue = FMath::Clamp(NewValue, 0.f, MaxAllowed);

	ASC->SetNumericAttributeBase(Attr, NewValue);
}

void UStatsManager::RemoveFlatStatModifier(const FGameplayAttribute& Attribute, float Delta)
{
	if (!Attribute.IsValid() || !ASC) return;

	const float CurrentValue = ASC->GetNumericAttribute(Attribute);
	const float NewValue = CurrentValue - Delta;

	// Optional clamp to prevent negatives if needed
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

    // THIRD: NOW we can validate StartupEffects (after ASC is assigned)
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
	DebugPrintStartupEffects();
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

	// Remove existing effect if it exists
	if (EffectInfo.SetByCallerTag.IsValid())
	{
		RemovePeriodicEffectByTag(EffectInfo.SetByCallerTag); // Use dedicated removal method
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

	if (EffectInfo.SetByCallerTag.IsValid() && Handle.IsValid())
	{
		ActivePeriodicEffects.Add(EffectInfo.SetByCallerTag, Handle);
	}

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

void UStatsManager::InitializeDefaultAttributes()
{
    check(Owner);
    if (!IsValid(ASC)) return;

    auto ValidateInitValue = [](float Value, const FString& Name) -> float
    {
        if (FMath::IsNaN(Value) || !FMath::IsFinite(Value))
        {
            UE_LOG(LogStatsManager, Error, TEXT("Invalid init value for %s: %f"), *Name, Value);
            return 0.0f;
        }
        return FMath::Clamp(Value, -9999.0f, 9999.0f);
    };
    
    FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
    Context.AddSourceObject(Owner);

    const auto& PHTags = FPHGameplayTags::Get();

    // === PRIMARY ATTRIBUTES ===
    FGameplayEffectSpecHandle PrimarySpec = ASC->MakeOutgoingSpec(DefaultPrimaryAttributes, 1.0f, Context);
    if (PrimarySpec.IsValid())
    {
        PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Strength, 
            ValidateInitValue(PrimaryInitAttributes.Strength, TEXT("Strength")));
        PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Intelligence, 
            ValidateInitValue(PrimaryInitAttributes.Intelligence, TEXT("Intelligence")));
        PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Dexterity, 
            ValidateInitValue(PrimaryInitAttributes.Dexterity, TEXT("Dexterity")));
        PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Endurance, 
            ValidateInitValue(PrimaryInitAttributes.Endurance, TEXT("Endurance")));
        PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Affliction, 
            ValidateInitValue(PrimaryInitAttributes.Affliction, TEXT("Affliction")));
        PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Luck, 
            ValidateInitValue(PrimaryInitAttributes.Luck, TEXT("Luck")));
        PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Covenant, 
            ValidateInitValue(PrimaryInitAttributes.Covenant, TEXT("Covenant")));

        ASC->ApplyGameplayEffectSpecToSelf(*PrimarySpec.Data);
    }

    // === SECONDARY MAX ATTRIBUTES (Caps / Limits) ===
    FGameplayEffectSpecHandle SecondaryMaxSpec = ASC->MakeOutgoingSpec(DefaultSecondaryMaxAttributes, 1.0f, Context);
    if (SecondaryMaxSpec.IsValid())
    {
        ASC->ApplyGameplayEffectSpecToSelf(*SecondaryMaxSpec.Data);
    }

    // === SECONDARY ATTRIBUTES (Rates, Amounts, Resistances, Bonuses) ===
    FGameplayEffectSpecHandle SecondarySpec = ASC->MakeOutgoingSpec(DefaultSecondaryCurrentAttributes, 1.0f, Context);
    if (SecondarySpec.IsValid())
    {
        // Vital Regen Rates
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_HealthRegenRate, 
            ValidateInitValue(SecondaryInitAttributes.HealthRegenRate, TEXT("HealthRegenRate")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_ManaRegenRate, 
            ValidateInitValue(SecondaryInitAttributes.ManaRegenRate, TEXT("ManaRegenRate")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_StaminaRegenRate, 
            ValidateInitValue(SecondaryInitAttributes.StaminaRegenRate, TEXT("StaminaRegenRate")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_ArcaneShieldRegenRate, 
            ValidateInitValue(SecondaryInitAttributes.ArcaneShieldRegenRate, TEXT("ArcaneShieldRegenRate")));

        // Vital Regen Amounts
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_HealthRegenAmount, 
            ValidateInitValue(SecondaryInitAttributes.HealthRegenAmount, TEXT("HealthRegenAmount")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_ManaRegenAmount, 
            ValidateInitValue(SecondaryInitAttributes.ManaRegenAmount, TEXT("ManaRegenAmount")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_StaminaRegenAmount, 
            ValidateInitValue(SecondaryInitAttributes.StaminaRegenAmount, TEXT("StaminaRegenAmount")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_ArcaneShieldRegenAmount, 
            ValidateInitValue(SecondaryInitAttributes.ArcaneShieldRegenAmount, TEXT("ArcaneShieldRegenAmount")));

        // Flat Reserved Amounts
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_HealthFlatReservedAmount, 
            ValidateInitValue(SecondaryInitAttributes.FlatReservedHealth, TEXT("FlatReservedHealth")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_ManaFlatReservedAmount, 
            ValidateInitValue(SecondaryInitAttributes.FlatReservedMana, TEXT("FlatReservedMana")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_StaminaFlatReservedAmount, 
            ValidateInitValue(SecondaryInitAttributes.FlatReservedStamina, TEXT("FlatReservedStamina")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_ArcaneShieldFlatReservedAmount, 
            ValidateInitValue(SecondaryInitAttributes.FlatReservedArcaneShield, TEXT("FlatReservedArcaneShield")));

        // Percentage Reserved
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_HealthPercentageReserved, 
            ValidateInitValue(SecondaryInitAttributes.PercentageReservedHealth, TEXT("PercentageReservedHealth")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_ManaPercentageReserved, 
            ValidateInitValue(SecondaryInitAttributes.PercentageReservedMana, TEXT("PercentageReservedMana")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_StaminaPercentageReserved, 
            ValidateInitValue(SecondaryInitAttributes.PercentageReservedStamina, TEXT("PercentageReservedStamina")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_ArcaneShieldPercentageReserved, 
            ValidateInitValue(SecondaryInitAttributes.PercentageReservedArcaneShield, TEXT("PercentageReservedArcaneShield")));

        // Flat Resistances
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_FireResistanceFlat, 
            ValidateInitValue(SecondaryInitAttributes.FireResistanceFlat, TEXT("FireResistanceFlat")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_IceResistanceFlat, 
            ValidateInitValue(SecondaryInitAttributes.IceResistanceFlat, TEXT("IceResistanceFlat")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_LightningResistanceFlat, 
            ValidateInitValue(SecondaryInitAttributes.LightningResistanceFlat, TEXT("LightningResistanceFlat")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_LightResistanceFlat, 
            ValidateInitValue(SecondaryInitAttributes.LightResistanceFlat, TEXT("LightResistanceFlat")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_CorruptionResistanceFlat, 
            ValidateInitValue(SecondaryInitAttributes.CorruptionResistanceFlat, TEXT("CorruptionResistanceFlat")));

        // Percentage Resistances
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_FireResistancePercentage, 
            ValidateInitValue(SecondaryInitAttributes.FireResistancePercent, TEXT("FireResistancePercent")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_IceResistancePercentage, 
            ValidateInitValue(SecondaryInitAttributes.IceResistancePercent, TEXT("IceResistancePercent")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_LightningResistancePercentage, 
            ValidateInitValue(SecondaryInitAttributes.LightningResistancePercent, TEXT("LightningResistancePercent")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_LightResistancePercentage, 
            ValidateInitValue(SecondaryInitAttributes.LightResistancePercent, TEXT("LightResistancePercent")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_CorruptionResistancePercentage, 
            ValidateInitValue(SecondaryInitAttributes.CorruptionResistancePercent, TEXT("CorruptionResistancePercent")));

        // Misc Stats
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Money_Gems, 
            ValidateInitValue(SecondaryInitAttributes.Gems, TEXT("Gems")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Misc_LifeLeech, 
            ValidateInitValue(SecondaryInitAttributes.LifeLeech, TEXT("LifeLeech")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Misc_ManaLeech, 
            ValidateInitValue(SecondaryInitAttributes.ManaLeech, TEXT("ManaLeech")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Misc_MovementSpeed, 
            ValidateInitValue(SecondaryInitAttributes.MovementSpeed, TEXT("MovementSpeed")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Misc_CritChance, 
            ValidateInitValue(SecondaryInitAttributes.CritChance, TEXT("CritChance")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Misc_CritMultiplier, 
            ValidateInitValue(SecondaryInitAttributes.CritMultiplier, TEXT("CritMultiplier")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Misc_Poise, 
            ValidateInitValue(SecondaryInitAttributes.Poise, TEXT("Poise")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Misc_StunRecovery, 
            ValidateInitValue(SecondaryInitAttributes.StunRecovery, TEXT("StunRecovery")));

        // Max Vital Regen Rates
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_MaxHealthRegenRate, 
            ValidateInitValue(SecondaryInitAttributes.HealthRegenRate, TEXT("MaxHealthRegenRate")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_MaxManaRegenRate, 
            ValidateInitValue(SecondaryInitAttributes.ManaRegenRate, TEXT("MaxManaRegenRate")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_MaxStaminaRegenRate, 
            ValidateInitValue(SecondaryInitAttributes.StaminaRegenRate, TEXT("MaxStaminaRegenRate")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_MaxArcaneShieldRegenRate, 
            ValidateInitValue(SecondaryInitAttributes.ArcaneShieldRegenRate, TEXT("MaxArcaneShieldRegenRate")));

        // Max Vital Regen Amounts
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_MaxHealthRegenAmount, 
            ValidateInitValue(SecondaryInitAttributes.HealthRegenAmount, TEXT("MaxHealthRegenAmount")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_MaxManaRegenAmount, 
            ValidateInitValue(SecondaryInitAttributes.ManaRegenAmount, TEXT("MaxManaRegenAmount")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_MaxStaminaRegenAmount, 
            ValidateInitValue(SecondaryInitAttributes.StaminaRegenAmount, TEXT("MaxStaminaRegenAmount")));
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_MaxArcaneShieldRegenAmount, 
            ValidateInitValue(SecondaryInitAttributes.ArcaneShieldRegenAmount, TEXT("MaxArcaneShieldRegenAmount")));

        ASC->ApplyGameplayEffectSpecToSelf(*SecondarySpec.Data);
    }

    // === STARTING CURRENT HEALTH / MANA / STAMINA ===
    FGameplayEffectSpecHandle VitalSpec = ASC->MakeOutgoingSpec(DefaultVitalAttributes, 1.0f, Context);
    if (VitalSpec.IsValid())
    {
        VitalSpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Vital_Health, 
            ValidateInitValue(VitalInitAttributes.Health, TEXT("Health")));
        VitalSpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Vital_Mana, 
            ValidateInitValue(VitalInitAttributes.Mana, TEXT("Mana")));
        VitalSpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Vital_Stamina, 
            ValidateInitValue(VitalInitAttributes.Stamina, TEXT("Stamina")));

        ASC->ApplyGameplayEffectSpecToSelf(*VitalSpec.Data);
    }

    UE_LOG(LogStatsManager, Log, TEXT("Attributes initialized successfully for %s"), 
        *Owner->GetName());
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


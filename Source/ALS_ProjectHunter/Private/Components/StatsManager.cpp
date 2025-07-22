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


// Sets default values for this component's properties
UStatsManager::UStatsManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	Owner = GetOwner<APHBaseCharacter>();
	// ...
}

void UStatsManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	ClearAllPeriodicEffects();
}

float UStatsManager::GetStatBase(const FGameplayAttribute& Attr) const
{
	if (!Attr.IsValid()) return 0.0f;

	if (ASC)
	{
		return ASC->GetNumericAttribute(Attr);
	}

	return 0.0f;
}


void UStatsManager::ApplyFlatStatModifier( const FGameplayAttribute& Attr, const float InValue) const
{
	if (!Attr.IsValid()) return;

	if (ASC)
	{
		const float CurrentValue = ASC->GetNumericAttribute(Attr);
		const float NewValue = CurrentValue + InValue;

		
		// May add later if needed if going - causes issues or to clamp max  
		// const float ClampedValue = FMath::Clamp(NewValue, 0.f, MaxAllowed);

		ASC->SetNumericAttributeBase(Attr, NewValue);
	}
}

void UStatsManager::RemoveFlatStatModifier(const FGameplayAttribute& Attribute, float Delta)
{
	if (!Attribute.IsValid()) return;

	if (ASC)
	{
		const float CurrentValue = ASC->GetNumericAttribute(Attribute);
		const float NewValue = CurrentValue - Delta;

		// Optional clamp to prevent negatives if needed
		ASC->SetNumericAttributeBase(Attribute, NewValue);
	}
}

// Called when the game starts
void UStatsManager::BeginPlay()
{
	Super::BeginPlay();

}

void UStatsManager::InitStatsManager(UPHAbilitySystemComponent* InASC)
{
	check(IsValid(Owner));
	ASC = InASC;
	if (!ASC) return;

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

		// Apply effect initially
		ApplyPeriodicEffectToSelf(EffectInfo);
	}

	InitializeDefaultAttributes();
}


void UStatsManager::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float InLevel) const
{
	check(IsValid( ASC));
	check(IsValid( Owner));
	check(GameplayEffectClass);
	
	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(Owner);
	
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GameplayEffectClass, InLevel, ContextHandle);
	 ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(),  ASC);
}

FActiveGameplayEffectHandle UStatsManager::ApplyEffectToSelfWithReturn(TSubclassOf<UGameplayEffect> InEffect,
	float InLevel)
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

	
	if (EffectInfo.SetByCallerTag.IsValid())
	{
		if (const FActiveGameplayEffectHandle* FoundHandle = ActivePeriodicEffects.Find(EffectInfo.SetByCallerTag))
		{
			if (FoundHandle->IsValid())
			{
				ASC->RemoveActiveGameplayEffect(*FoundHandle);
			}
		}
	}

	
	FGameplayEffectContextHandle Context =  ASC->MakeEffectContext();


	FGameplayEffectSpecHandle SpecHandle =  ASC->MakeOutgoingSpec(
		EffectInfo.EffectClass, EffectInfo.Level, Context);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	
	const float RawRate = EffectInfo.RateAttribute.IsValid()
		?  ASC->GetNumericAttribute(EffectInfo.RateAttribute)
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
		UE_LOG(LogTemp, Warning, TEXT("[StatsManager] Missing SetByCallerTag for %s! Default magnitude not set."),
			*EffectInfo.EffectClass->GetName());
	}
	
	SpecHandle.Data->Period = ClampedRate;
	
	
	FActiveGameplayEffectHandle Handle =  ASC->ApplyGameplayEffectSpecToTarget(
		*SpecHandle.Data.Get(), ASC);

	if (EffectInfo.SetByCallerTag.IsValid() && Handle.IsValid())
	{
		ActivePeriodicEffects.Add(EffectInfo.SetByCallerTag, Handle);
	}

#if WITH_EDITOR
	UE_LOG(LogTemp, Log, TEXT("[GAS] Applied %s with Amount %.2f, Period %.2f"),
		*EffectInfo.EffectClass->GetName(), RawAmount, ClampedRate);
#endif
}

void UStatsManager::ClearAllPeriodicEffects()
{
		
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
	ActivePeriodicEffects.GetKeys( PHTags);

	for (const FGameplayTag& Tag :  PHTags)
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

void UStatsManager::RemoveAllPeriodicEffectsByTag(FGameplayTag EffectTag)
{
	TArray<FActiveGameplayEffectHandle> Handles;
	ActivePeriodicEffects.MultiFind(EffectTag, Handles);

	// Filter and remove invalid handles first
	for (int32 i = Handles.Num() - 1; i >= 0; --i)
	{
		if (!Handles[i].IsValid())
		{
			Handles.RemoveAt(i);
		}
	}

	// Remove valid handles from ASC
	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	// Remove entries from the TMultiMap
	ActivePeriodicEffects.Remove(EffectTag);
}

void UStatsManager::OnRegenTagChanged(FGameplayTag ChangedTag, int32 NewCount)
{
	for (const FInitialGameplayEffectInfo& EffectInfo : StartupEffects)
	{
		if (EffectInfo.TriggerTag == ChangedTag)
		{
			ApplyPeriodicEffectToSelf(EffectInfo);
		}
	}
}

void UStatsManager::OnAnyVitalPeriodicStatChanged(const FOnAttributeChangeData& Data)
{
	for (const FInitialGameplayEffectInfo& EffectInfo : StartupEffects)
	{
		if (EffectInfo.RateAttribute == Data.Attribute || EffectInfo.AmountAttribute == Data.Attribute)
		{
			ApplyPeriodicEffectToSelf(EffectInfo);
		}
	}
}


void UStatsManager::InitializeDefaultAttributes()
{
    check(Owner);
    if (!IsValid(ASC)) return;

    FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
    Context.AddSourceObject(Owner);

    const auto& PHTags = FPHGameplayTags::Get();

    // === PRIMARY ATTRIBUTES ===
    FGameplayEffectSpecHandle PrimarySpec = ASC->MakeOutgoingSpec(DefaultPrimaryAttributes, 1.0f, Context);
    if (PrimarySpec.IsValid())
    {
        PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Strength, PrimaryInitAttributes.Strength);
        PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Intelligence, PrimaryInitAttributes.Intelligence);
        PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Dexterity, PrimaryInitAttributes.Dexterity);
        PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Endurance, PrimaryInitAttributes.Endurance);
        PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Affliction, PrimaryInitAttributes.Affliction);
        PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Luck, PrimaryInitAttributes.Luck);
        PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Covenant, PrimaryInitAttributes.Covenant);

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
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_HealthRegenRate, SecondaryInitAttributes.HealthRegenRate);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_ManaRegenRate, SecondaryInitAttributes.ManaRegenRate);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_StaminaRegenRate, SecondaryInitAttributes.StaminaRegenRate);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_ArcaneShieldRegenRate, SecondaryInitAttributes.ArcaneShieldRegenRate);

        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_HealthRegenAmount, SecondaryInitAttributes.HealthRegenAmount);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_ManaRegenAmount, SecondaryInitAttributes.ManaRegenAmount);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_StaminaRegenAmount, SecondaryInitAttributes.StaminaRegenAmount);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_ArcaneShieldRegenAmount, SecondaryInitAttributes.ArcaneShieldRegenAmount);

        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_HealthFlatReservedAmount, SecondaryInitAttributes.FlatReservedHealth);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_ManaFlatReservedAmount, SecondaryInitAttributes.FlatReservedMana);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_StaminaFlatReservedAmount, SecondaryInitAttributes.FlatReservedStamina);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_ArcaneShieldFlatReservedAmount, SecondaryInitAttributes.FlatReservedArcaneShield);

        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_HealthPercentageReserved, SecondaryInitAttributes.PercentageReservedHealth);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_ManaPercentageReserved, SecondaryInitAttributes.PercentageReservedMana);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_StaminaPercentageReserved, SecondaryInitAttributes.PercentageReservedStamina);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_ArcaneShieldPercentageReserved, SecondaryInitAttributes.PercentageReservedArcaneShield);

        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_FireResistanceFlat, SecondaryInitAttributes.FireResistanceFlat);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_IceResistanceFlat, SecondaryInitAttributes.IceResistanceFlat);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_LightningResistanceFlat, SecondaryInitAttributes.LightningResistanceFlat);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_LightResistanceFlat, SecondaryInitAttributes.LightResistanceFlat);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_CorruptionResistanceFlat, SecondaryInitAttributes.CorruptionResistanceFlat);

        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_FireResistancePercentage, SecondaryInitAttributes.FireResistancePercent);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_IceResistancePercentage, SecondaryInitAttributes.IceResistancePercent);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_LightningResistancePercentage, SecondaryInitAttributes.LightningResistancePercent);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_LightResistancePercentage, SecondaryInitAttributes.LightResistancePercent);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Resistances_CorruptionResistancePercentage, SecondaryInitAttributes.CorruptionResistancePercent);

        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Money_Gems, SecondaryInitAttributes.Gems);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Misc_LifeLeech, SecondaryInitAttributes.LifeLeech);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Misc_ManaLeech, SecondaryInitAttributes.ManaLeech);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Misc_MovementSpeed, SecondaryInitAttributes.MovementSpeed);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Misc_CritChance, SecondaryInitAttributes.CritChance);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Misc_CritMultiplier, SecondaryInitAttributes.CritMultiplier);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Misc_Poise, SecondaryInitAttributes.Poise);
        SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Misc_StunRecovery, SecondaryInitAttributes.StunRecovery);

    	SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_MaxHealthRegenRate, SecondaryInitAttributes.HealthRegenRate);
    	SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_MaxManaRegenRate, SecondaryInitAttributes.ManaRegenRate);
    	SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_MaxStaminaRegenRate, SecondaryInitAttributes.StaminaRegenRate);
    	SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_MaxArcaneShieldRegenRate, SecondaryInitAttributes.ArcaneShieldRegenRate);
		
    	SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_MaxHealthRegenAmount, SecondaryInitAttributes.HealthRegenAmount);
    	SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_MaxManaRegenAmount, SecondaryInitAttributes.ManaRegenAmount);
    	SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_MaxStaminaRegenAmount, SecondaryInitAttributes.StaminaRegenAmount);
    	SecondarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Secondary_Vital_MaxArcaneShieldRegenAmount, SecondaryInitAttributes.ArcaneShieldRegenAmount);


        ASC->ApplyGameplayEffectSpecToSelf(*SecondarySpec.Data);
    }

	// === STARTING CURRENT HEALTH / MANA / STAMINA ===
	FGameplayEffectSpecHandle VitalSpec = ASC->MakeOutgoingSpec(DefaultVitalAttributes, 1.0f, Context);
	if (VitalSpec.IsValid())
	{
		VitalSpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Vital_Health, VitalInitAttributes.Health);
		VitalSpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Vital_Mana, VitalInitAttributes.Mana);
		VitalSpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Vital_Stamina, VitalInitAttributes.Stamina);

		ASC->ApplyGameplayEffectSpecToSelf(*VitalSpec.Data);
	}


    for (const FInitialGameplayEffectInfo& EffectInfo : StartupEffects)
    {
        ApplyPeriodicEffectToSelf(EffectInfo);
    }

    ReapplyAllStartupRegenEffects();
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


// Called every frame
void UStatsManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


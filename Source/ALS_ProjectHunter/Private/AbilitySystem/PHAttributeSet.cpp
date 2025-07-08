// Copyright@2024 Quentin Davis 


#include "AbilitySystem/PHAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Library/PHCharacterEnumLibrary.h"
#include "Library/PHTagUtilityLibrary.h"
#include "Net/UnrealNetwork.h"

UPHAttributeSet::UPHAttributeSet()
{
	
}


void UPHAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Indicators
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CombatAlignment, COND_None, REPNOTIFY_Always);

	// Primary Attribute
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Strength, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Intelligence, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Dexterity, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Endurance, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Affliction, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Luck, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Covenant, COND_OwnerOnly, REPNOTIFY_Always);

	//Secondary  Max Attribute

	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxEffectiveHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxEffectiveStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxEffectiveMana, COND_None, REPNOTIFY_Always);

	// Regeneration 
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, HealthRegenRate, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, HealthRegenAmount, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ReservedHealth, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxReservedHealth, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FlatReservedHealth, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PercentageReservedHealth, COND_OwnerOnly, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaRegenRate, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaRegenAmount, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ReservedMana, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxReservedMana, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FlatReservedMana, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PercentageReservedMana, COND_OwnerOnly, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaRegenRate, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaRegenAmount, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaDegenRate, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaDegenAmount, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ReservedStamina, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxReservedStamina, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FlatReservedStamina, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PercentageReservedStamina, COND_OwnerOnly, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ArcaneShieldRegenRate, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet,ArcaneShieldRegenAmount, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ReservedArcaneShield, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxReservedArcaneShield, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FlatReservedArcaneShield, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PercentageReservedArcaneShield, COND_OwnerOnly, REPNOTIFY_Always);

	//Damages
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, GlobalDamages, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinPhysicalDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinFireDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinLightDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinLightningDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinCorruptionDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinIceDamage, COND_OwnerOnly, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxPhysicalDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxFireDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxLightDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxLightningDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxCorruptionDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxIceDamage, COND_OwnerOnly, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PhysicalFlatBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FireFlatBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightFlatBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningFlatBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionFlatBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, IceFlatBonus, COND_OwnerOnly, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PhysicalPercentBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FirePercentBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightPercentBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningPercentBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionPercentBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, IcePercentBonus, COND_OwnerOnly, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, DamageBonusWhileAtFullHP, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, DamageBonusWhileAtLowHP, COND_OwnerOnly, REPNOTIFY_Always);

	// Other Offensive Stats
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, AreaDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, AreaOfEffect, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, AttackRange, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, AttackSpeed, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CastSpeed, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CritChance, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CritMultiplier, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, DamageOverTime, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ElementalDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, SpellsCritChance, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, SpellsCritMultiplier, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MeleeDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ProjectileCount, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ProjectileSpeed, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet,  RangedDamage, COND_OwnerOnly, REPNOTIFY_Always);


	//Duration
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, BurnDuration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, BleedDuration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FreezeDuration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionDuration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ShockDuration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet,PetrifyBuildUpDuration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet,  PurifyDuration, COND_None, REPNOTIFY_Always);

	

	//Resistances
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, GlobalDefenses, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Armour, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ArmourFlatBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ArmourPercentBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FireResistanceFlatBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightResistanceFlatBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningResistanceFlatBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionResistanceFlatBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, IceResistanceFlatBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet,  BlockStrength, COND_None, REPNOTIFY_Always);

	
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxFireResistance, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxLightResistance, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxLightningResistance, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxCorruptionResistance, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet,  MaxIceResistance, COND_OwnerOnly, REPNOTIFY_Always);



	//Piercing
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ArmourPiercing, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FirePiercing, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningPiercing, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionPiercing, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet,  IcePiercing, COND_OwnerOnly, REPNOTIFY_Always);

	//Chance to apply but will still have a build up meeter 
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToBleed, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToCorrupt, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToFreeze, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToIgnite, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToPetrify, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToPurify, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToShock, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToStun, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToKnockBack, COND_OwnerOnly, REPNOTIFY_Always);

	//Misc
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ComboCounter, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CoolDown, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LifeLeech, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaLeech, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MovementSpeed, COND_OwnerOnly, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Poise, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StunRecovery, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaCostChanges, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LifeOnHit, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaOnHit, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaOnHit, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaCostChanges, COND_OwnerOnly, REPNOTIFY_Always);

	//Secondary Current Attribute
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Mana, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Stamina, COND_OwnerOnly, REPNOTIFY_Always);


	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Gems, COND_OwnerOnly, REPNOTIFY_Always);
}

float UPHAttributeSet::GetAttributeValue(const FGameplayAttribute& Attribute) const 
{
	if (const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
	{
		return ASC->GetNumericAttribute(Attribute);
	}
	return 0.0f;
}


// for use in BP will show enum not float values 
ECombatAlignment UPHAttributeSet::GetCombatAlignmentBP() const
{
	// Convert the float value stored in CombatAlignment to the ECombatAlignment enum
	return static_cast<ECombatAlignment>(CombatAlignment.GetCurrentValue());
}

// for use in BP will show enum not float values 
void UPHAttributeSet::SetCombatAlignmentBP(ECombatAlignment NewAlignment)
{
	// Set the CombatAlignment value using the float representation of the enum
	SetCombatAlignment(static_cast<float>(NewAlignment));

	// Broadcast the change
	OnCombatAlignmentChange.Broadcast(static_cast<float>(NewAlignment));
}

void UPHAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Handle health attribute changes.
	if(Attribute == GetHealthAttribute())
	{
		{
			NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxEffectiveHealth());
		}
	}
	else if(Attribute == GetManaAttribute())
	{
		
		NewValue = FMath::Clamp(NewValue, 0.0f,GetMaxMana());
	}
	else if(Attribute == GetStaminaAttribute())
	{
		
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
}

void UPHAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	FEffectProperties Props;
	SetEffectProperties(Data, Props);

	const FGameplayAttribute Attribute = Data.EvaluatedData.Attribute;

	// Optional: check for source/target tags if needed
	const FGameplayTagContainer* SourceTags = Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Data.EffectSpec.CapturedTargetTags.GetAggregatedTags();

	// Reusable clamp logic
	auto ClampResource = [](TFunction<float()> Getter, TFunction<float()> MaxGetter, TFunction<void(float)> Setter)
	{
		Setter(FMath::Clamp(Getter(), 0.0f, FMath::Max(1.0f, MaxGetter())));
	};

	// Clamp and handle each relevant resource
	if (Attribute == GetHealthAttribute())
	{
		ClampResource(
			[this]() { return GetHealth(); },
			[this]() { return GetMaxEffectiveHealth(); },
			[this](float V) { SetHealth(V); }
		);

		// Handle death
		if (GetHealth() <= 0.0f)
		{
			// Trigger death logic here, e.g.:
			// OnCharacterDeath.Broadcast();
		}
	}
	else if (Attribute == GetManaAttribute())
	{
		ClampResource(
			[this]() { return GetMana(); },
			[this]() { return GetMaxMana(); },
			[this](float V) { SetMana(V); }
		);
	}
	else if (Attribute == GetStaminaAttribute())
	{
		ClampResource(
			[this]() { return GetStamina(); },
			[this]() { return GetMaxStamina(); },
			[this](float V) { SetStamina(V); }
		);
	}
	else if (Attribute == GetArcaneShieldAttribute())
	{
		ClampResource(
			[this]() { return GetArcaneShield(); },
			[this]() { return GetMaxArcaneShield(); },
			[this](float V) { SetArcaneShield(V); }
		);
	}

	// Update threshold tags after any valid change
	if (Props.TargetASC && ShouldUpdateThresholdTags(Attribute))
	{
		UPHTagUtilityLibrary::UpdateAttributeThresholdTags(Props.TargetASC, this);
	}
}



bool UPHAttributeSet::ShouldUpdateThresholdTags(const FGameplayAttribute& Attribute) const
{
	static const TSet<FGameplayAttribute> TrackedThresholdAttributes = {
		GetHealthAttribute(),
		GetManaAttribute(),
		GetStaminaAttribute(),
		GetArcaneShieldAttribute()
	};

	return TrackedThresholdAttributes.Contains(Attribute);
}

void UPHAttributeSet::SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props)
{
	Props.EffectContextHandle = Data.EffectSpec.GetContext();
	Props.SourceAsc = Props.EffectContextHandle.GetOriginalInstigatorAbilitySystemComponent();

	if(IsValid(Props.SourceAsc) && Props.SourceAsc->AbilityActorInfo.IsValid() && Props.SourceAsc->AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.SourceAvatarActor = Props.SourceAsc->AbilityActorInfo->AvatarActor.Get();
		Props.SourceController = Props.SourceAsc->AbilityActorInfo->PlayerController.Get();
		if(Props.SourceController == nullptr && Props.SourceAvatarActor == nullptr)
		{
			if(const APawn* Pawn = Cast<APawn>(Props.SourceAvatarActor))
			{
				Props.SourceAvatarActor = Pawn->GetController();
			}
		}
		if(Props.SourceController)
		{
			Props.SourceCharacter = Cast<ACharacter>(Props.SourceController->GetPawn());
		}
	}

	if(Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.TargetAvatarActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		Props.TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		Props.TargetCharacter = Cast<ACharacter>(Props.TargetAvatarActor);
		Props.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Props.TargetAvatarActor);
	}
}

//
void UPHAttributeSet::OnRep_CombatAlignment(const FGameplayAttributeData& OldCombatAlignment) const
{
	// Notify listeners of the attribute change
	OnCombatAlignmentChange.Broadcast(GetCombatAlignment());
	
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet, CombatAlignment, OldCombatAlignment);
}

//Health
void UPHAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Health, OldHealth)
}

void UPHAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxHealth, OldMaxHealth)
}

void UPHAttributeSet::OnRep_MaxEffectiveHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxEffectiveHealth, OldAmount);
}

void UPHAttributeSet::OnRep_HealthRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,HealthRegenRate, OldAmount);
}

void UPHAttributeSet::OnRep_MaxHealthRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxHealthRegenRate, OldAmount);
}

void UPHAttributeSet::OnRep_HealthRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,HealthRegenAmount, OldAmount);
}


void UPHAttributeSet::OnRep_MaxHealthRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxHealthRegenAmount, OldAmount);
}

void UPHAttributeSet::OnRep_ReservedHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ReservedHealth, OldAmount);
}

void UPHAttributeSet::OnRep_MaxReservedHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxReservedHealth, OldAmount);
}

void UPHAttributeSet::OnRep_FlatReservedHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , FlatReservedHealth, OldAmount);
}

void UPHAttributeSet::OnRep_PercentageReservedHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , PercentageReservedHealth, OldAmount);
}

//Health End

//Stamina 
void UPHAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , Stamina,OldStamina)
}

void UPHAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxStamina, OldMaxStamina)
}

void UPHAttributeSet::OnRep_MaxEffectiveStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxEffectiveStamina, OldAmount)

}

void UPHAttributeSet::OnRep_StaminaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaRegenRate, OldAmount)
}

void UPHAttributeSet::OnRep_StaminaDegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaDegenRate, OldAmount)
}

void UPHAttributeSet::OnRep_MaxStaminaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxStaminaRegenRate, OldAmount)
}

void UPHAttributeSet::OnRep_StaminaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaRegenAmount, OldAmount)
}

void UPHAttributeSet::OnRep_StaminaDegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaDegenAmount, OldAmount)
}

void UPHAttributeSet::OnRep_MaxStaminaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxStaminaRegenAmount,OldAmount)
}

void UPHAttributeSet::OnRep_ReservedStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ReservedStamina, OldAmount)
}

void UPHAttributeSet::OnRep_MaxReservedStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxReservedStamina, OldAmount)
}


void UPHAttributeSet::OnRep_FlatReservedStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FlatReservedStamina, OldAmount)
}

void UPHAttributeSet::OnRep_PercentageReservedStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PercentageReservedStamina, OldAmount)
}

//Stamina End

//Mana
void UPHAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Mana, OldMana)
}

void UPHAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxMana, OldMaxMana)

}

void UPHAttributeSet::OnRep_MaxEffectiveMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxEffectiveMana, OldAmount)
}

void UPHAttributeSet::OnRep_ManaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ManaRegenRate,OldAmount)
}

void UPHAttributeSet::OnRep_MaxManaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxManaRegenRate,OldAmount)
}

void UPHAttributeSet::OnRep_ManaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ManaRegenAmount,OldAmount)
}

void UPHAttributeSet::OnRep_MaxManaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxManaRegenAmount,OldAmount)
}

void UPHAttributeSet::OnRep_ReservedMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ReservedMana,OldAmount)
}

void UPHAttributeSet::OnRep_MaxReservedMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxReservedMana,OldAmount)
}

void UPHAttributeSet::OnRep_FlatReservedMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , FlatReservedMana,OldAmount)
}

void UPHAttributeSet::OnRep_PercentageReservedMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , PercentageReservedMana,OldAmount)
}

void UPHAttributeSet::OnRep_ArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , ArcaneShield ,OldAmount)
}

void UPHAttributeSet::OnRep_MaxArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , MaxArcaneShield ,OldAmount)
}

void UPHAttributeSet::OnRep_MaxEffectiveArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , MaxEffectiveArcaneShield ,OldAmount)
}

void UPHAttributeSet::OnRep_ArcaneShieldRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , ArcaneShieldRegenRate ,OldAmount)
}

void UPHAttributeSet::OnRep_MaxArcaneShieldRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , MaxArcaneShieldRegenRate ,OldAmount)
}

void UPHAttributeSet::OnRep_ArcaneShieldRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , ArcaneShieldRegenAmount ,OldAmount)
}

void UPHAttributeSet::OnRep_MaxArcaneShieldRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , MaxArcaneShieldRegenAmount ,OldAmount)
}

void UPHAttributeSet::OnRep_ReservedArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , ReservedArcaneShield ,OldAmount)
}

void UPHAttributeSet::OnRep_MaxReservedArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , MaxReservedArcaneShield ,OldAmount)
}

void UPHAttributeSet::OnRep_FlatReservedArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , FlatReservedArcaneShield ,OldAmount)
}

//Mana End

void UPHAttributeSet::OnRep_PercentageReservedArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , PercentageReservedArcaneShield ,OldAmount)
}

//Gems
void UPHAttributeSet::OnRep_Gems(const FGameplayAttributeData& OldGems) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Gems, OldGems)
}

void UPHAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Strength, OldAmount)
}

void UPHAttributeSet::OnRep_Intelligence(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Intelligence, OldAmount)
}

void UPHAttributeSet::OnRep_Dexterity(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Dexterity, OldAmount)
}

void UPHAttributeSet::OnRep_Endurance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Endurance, OldAmount)
}

void UPHAttributeSet::OnRep_Affliction(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Affliction, OldAmount)
}

void UPHAttributeSet::OnRep_Luck(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Luck, OldAmount)
}

void UPHAttributeSet::OnRep_Covenant(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Covenant, OldAmount)
}

void UPHAttributeSet::OnRep_GlobalDamages(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,GlobalDamages, OldAmount)
}

void UPHAttributeSet::OnRep_MinCorruptionDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinCorruptionDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxCorruptionDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxCorruptionDamage, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionPercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionPercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_MinFireDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinFireDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxFireDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxFireDamage, OldAmount)
}

void UPHAttributeSet::OnRep_FireFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FireFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_FirePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FirePercentBonus, OldAmount)
}


void UPHAttributeSet::OnRep_MinIceDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinIceDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxIceDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxIceDamage, OldAmount)
}

void UPHAttributeSet::OnRep_IceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IceFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_IcePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IcePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_MinLightDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinLightDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxLightDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxLightDamage, OldAmount)
}

void UPHAttributeSet::OnRep_LightFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_LightPercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightPercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_MinLightningDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinLightningDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxLightningDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxLightningDamage, OldAmount)
}

void UPHAttributeSet::OnRep_LightningFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_LightningPercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningPercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_MinPhysicalDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinPhysicalDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxPhysicalDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxPhysicalDamage, OldAmount)
}

void UPHAttributeSet::OnRep_PhysicalFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PhysicalFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_PhysicalPercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PhysicalPercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_AreaDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,AreaDamage, OldAmount)
}

void UPHAttributeSet::OnRep_AreaOfEffect(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,AreaOfEffect, OldAmount)
}

void UPHAttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,AttackRange, OldAmount)
}

void UPHAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,AttackSpeed, OldAmount)
}

void UPHAttributeSet::OnRep_CastSpeed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CastSpeed, OldAmount)
}

void UPHAttributeSet::OnRep_CritChance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CritChance, OldAmount)
}

void UPHAttributeSet::OnRep_CritMultiplier(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CritMultiplier, OldAmount)
}

void UPHAttributeSet::OnRep_DamageOverTime(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,DamageOverTime, OldAmount)
}

void UPHAttributeSet::OnRep_ElementalDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ElementalDamage, OldAmount)
}

void UPHAttributeSet::OnRep_SpellsCritChance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,SpellsCritChance, OldAmount)
}

void UPHAttributeSet::OnRep_SpellsCritMultiplier(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,SpellsCritMultiplier, OldAmount)
}

void UPHAttributeSet::OnRep_DamageBonusWhileAtFullHP(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,DamageBonusWhileAtFullHP, OldAmount)
}

void UPHAttributeSet::OnRep_MaxCorruptionResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxCorruptionResistance, OldAmount)
}

void UPHAttributeSet::OnRep_ArmourPiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ArmourPiercing, OldAmount)
}

void UPHAttributeSet::OnRep_FirePiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , FirePiercing, OldAmount)
}

void UPHAttributeSet::OnRep_LightPiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightPiercing, OldAmount)
}

void UPHAttributeSet::OnRep_LightningPiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningPiercing, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionPiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionPiercing, OldAmount)
}

void UPHAttributeSet::OnRep_IcePiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IcePiercing, OldAmount)
}

void UPHAttributeSet::OnRep_MaxFireResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxFireResistance, OldAmount)
}

void UPHAttributeSet::OnRep_MaxIceResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxIceResistance, OldAmount)
}

void UPHAttributeSet::OnRep_MaxLightResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxLightResistance, OldAmount)
}

void UPHAttributeSet::OnRep_MaxLightningResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxLightningResistance, OldAmount)
}

void UPHAttributeSet::OnRep_DamageBonusWhileAtLowHP(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,DamageBonusWhileAtLowHP, OldAmount)
}

void UPHAttributeSet::OnRep_MeleeDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MeleeDamage, OldAmount)
}

void UPHAttributeSet::OnRep_SpellDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , SpellDamage, OldAmount);
}

void UPHAttributeSet::OnRep_ProjectileCount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ProjectileCount, OldAmount)
}

void UPHAttributeSet::OnRep_ProjectileSpeed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ProjectileSpeed, OldAmount)
}

void UPHAttributeSet::OnRep_RangedDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,RangedDamage, OldAmount)
}

void UPHAttributeSet::OnRep_GlobalDefenses(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,GlobalDefenses, OldAmount)
}

void UPHAttributeSet::OnRep_Armour(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Armour, OldAmount)
}

void UPHAttributeSet::OnRep_ArmourFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ArmourFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_ArmourPercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ArmourPercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionResistanceFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_FireResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FireResistanceFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_IceResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IceResistanceFlatBonus, OldAmount)
}


void UPHAttributeSet::OnRep_LightResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightResistanceFlatBonus, OldAmount)
}



void UPHAttributeSet::OnRep_LightningResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningResistanceFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionResistancePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_FireResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FireResistancePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_IceResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IceResistancePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_LightResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightResistancePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_LightningResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningResistancePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_BlockStrength(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,BlockStrength, OldAmount)
}

void UPHAttributeSet::OnRep_ComboCounter(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ComboCounter, OldAmount)
}

void UPHAttributeSet::OnRep_CoolDown(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CoolDown, OldAmount)
}

void UPHAttributeSet::OnRep_LifeLeech(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LifeLeech, OldAmount)
}

void UPHAttributeSet::OnRep_ManaLeech(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ManaLeech, OldAmount)
}

void UPHAttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MovementSpeed, OldAmount)
}

void UPHAttributeSet::OnRep_Poise(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Poise, OldAmount)
}

void UPHAttributeSet::OnRep_PoiseResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PoiseResistance, OldAmount)
}

void UPHAttributeSet::OnRep_StunRecovery(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StunRecovery, OldAmount)
}

void UPHAttributeSet::OnRep_ManaCostChanges(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ManaCostChanges, OldAmount)
}

void UPHAttributeSet::OnRep_LifeOnHit(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LifeOnHit, OldAmount)
}

void UPHAttributeSet::OnRep_ManaOnHit(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ManaOnHit, OldAmount)
}

void UPHAttributeSet::OnRep_StaminaOnHit(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaOnHit, OldAmount)
}

void UPHAttributeSet::OnRep_StaminaCostChanges(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaCostChanges, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToBleed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToBleed, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToCorrupt(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToCorrupt, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToFreeze(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToFreeze, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToIgnite(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToIgnite, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToKnockBack(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToKnockBack, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToPetrify(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToPetrify, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToShock(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToShock, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToStun(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToStun, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToPurify(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToPurify, OldAmount)
}

void UPHAttributeSet::OnRep_BurnDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,BurnDuration, OldAmount)
}

void UPHAttributeSet::OnRep_BleedDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,BleedDuration, OldAmount)
}

void UPHAttributeSet::OnRep_FreezeDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FreezeDuration, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionDuration, OldAmount)
}

void UPHAttributeSet::OnRep_ShockDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ShockDuration, OldAmount)
}

void UPHAttributeSet::OnRep_PetrifyBuildUpDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PetrifyBuildUpDuration, OldAmount)
}

void UPHAttributeSet::OnRep_PurifyDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PurifyDuration, OldAmount)
}


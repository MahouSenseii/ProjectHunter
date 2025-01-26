// Copyright@2024 Quentin Davis 


#include "AbilitySystem/PHAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Library/PHCharacterEnumLibrary.h"
#include "PHGameplayTags.h"
#include "Net/UnrealNetwork.h"

UPHAttributeSet::UPHAttributeSet()
{
	const FPHGameplayTags& GameplayTags = FPHGameplayTags::Get();
	
	//Primary 
	TagsToAttributes.Add(GameplayTags.Attributes_Primary_Strength, GetStrengthAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Primary_Intelligence, GetIntelligenceAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Primary_Endurance, GetEnduranceAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Primary_Affliction, GetAfflictionAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Primary_Dexterity, GetDexterityAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Primary_Luck, GetLuckAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Primary_Covenant, GetCovenantAttribute);

	
	//Secondary
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_HealthRegenRate, GetHealthRegenRateAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_HealthRegenAmount, GetHealthRegenAmountAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxHealthRegenRate, GetMaxHealthRegenRateAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxHealthRegenAmount, GetMaxHealthRegenAmountAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_HealthReservedAmount, GetReservedHealthAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxHealthReservedAmount, GetMaxReservedHealthAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_HealthFlatReservedAmount, GetFlatReservedHealthAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_HealthPercentageReserved, GetPercentageReservedHealthAttribute);
	
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_ManaRegenRate, GetManaRegenRateAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_ManaRegenAmount, GetManaRegenAmountAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxManaRegenRate, GetMaxManaRegenRateAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxManaRegenAmount, GetMaxManaRegenAmountAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_ManaReservedAmount, GetReservedManaAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxManaReservedAmount, GetMaxReservedManaAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_ManaFlatReservedAmount, GetFlatReservedManaAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_ManaPercentageReserved, GetPercentageReservedManaAttribute);
	
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_StaminaRegenRate, GetStaminaRegenRateAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_StaminaRegenAmount, GetStaminaRegenAmountAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxStaminaRegenRate, GetMaxStaminaRegenRateAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxStaminaRegenAmount, GetMaxStaminaRegenAmountAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_StaminaReservedAmount, GetReservedStaminaAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxStaminaReservedAmount, GetMaxReservedStaminaAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_StaminaFlatReservedAmount, GetFlatReservedStaminaAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_StaminaPercentageReserved, GetPercentageReservedStaminaAttribute);

	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxHealth,  GetMaxHealthAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxEffectiveHealth,  GetMaxEffectiveHealthAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxStamina, GetMaxStaminaAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxEffectiveStamina,  GetMaxEffectiveStaminaAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxMana, GetMaxManaAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxEffectiveMana,  GetMaxEffectiveManaAttribute);

	TagsToAttributes.Add(GameplayTags.Attributes_Vital_Health,  GetHealthAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Vital_Stamina, GetStaminaAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Vital_Mana, GetManaAttribute);



	// Adding primary attribute tags
	TagsMinMax.Add(GameplayTags.Attributes_Primary_Strength, GameplayTags.Attributes_Primary_Strength);
	TagsMinMax.Add(GameplayTags.Attributes_Primary_Intelligence, GameplayTags.Attributes_Primary_Intelligence);
	TagsMinMax.Add(GameplayTags.Attributes_Primary_Endurance, GameplayTags.Attributes_Primary_Endurance);
	TagsMinMax.Add(GameplayTags.Attributes_Primary_Affliction, GameplayTags.Attributes_Primary_Affliction);
	TagsMinMax.Add(GameplayTags.Attributes_Primary_Dexterity, GameplayTags.Attributes_Primary_Dexterity);
	TagsMinMax.Add(GameplayTags.Attributes_Primary_Luck, GameplayTags.Attributes_Primary_Luck);
	TagsMinMax.Add(GameplayTags.Attributes_Primary_Covenant, GameplayTags.Attributes_Primary_Covenant);

	// Adding secondary attribute tags
	TagsMinMax.Add(GameplayTags.Attributes_Secondary_HealthReservedAmount, GameplayTags.Attributes_Secondary_MaxHealthReservedAmount);
	TagsMinMax.Add(GameplayTags.Attributes_Secondary_HealthRegenRate, GameplayTags.Attributes_Secondary_MaxHealthRegenRate);
	TagsMinMax.Add(GameplayTags.Attributes_Secondary_HealthRegenAmount, GameplayTags.Attributes_Secondary_MaxHealthRegenAmount);
	TagsMinMax.Add(GameplayTags.Attributes_Secondary_ManaRegenRate, GameplayTags.Attributes_Secondary_MaxManaRegenRate);
	TagsMinMax.Add(GameplayTags.Attributes_Secondary_ManaRegenAmount, GameplayTags.Attributes_Secondary_MaxManaRegenAmount);
	TagsMinMax.Add(GameplayTags.Attributes_Secondary_ManaReservedAmount, GameplayTags.Attributes_Secondary_MaxManaReservedAmount);
	TagsMinMax.Add(GameplayTags.Attributes_Secondary_StaminaRegenRate, GameplayTags.Attributes_Secondary_MaxStaminaRegenRate);
	TagsMinMax.Add(GameplayTags.Attributes_Secondary_StaminaRegenAmount, GameplayTags.Attributes_Secondary_MaxStaminaRegenAmount);
	TagsMinMax.Add(GameplayTags.Attributes_Secondary_StaminaReservedAmount, GameplayTags.Attributes_Secondary_MaxStaminaReservedAmount);

	
	
	
}



void UPHAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Indicators
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CombatAlignment, COND_None, REPNOTIFY_Always);

	// Primary Attribute
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Intelligence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Dexterity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Endurance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Affliction, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Luck, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Covenant, COND_None, REPNOTIFY_Always);

	//Secondary  Max Attribute

	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxEffectiveHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxEffectiveStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxEffectiveMana, COND_None, REPNOTIFY_Always);

	// Regeneration 
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, HealthRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, HealthRegenAmount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, HealthRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ReservedHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxReservedHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FlatReservedHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PercentageReservedHealth, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaRegenAmount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ReservedMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxReservedMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FlatReservedMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PercentageReservedMana, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaRegenAmount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ReservedStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxReservedStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FlatReservedStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PercentageReservedStamina, COND_None, REPNOTIFY_Always);

	//Utilities
	/*DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, AttackRange, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaximumLife, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaximumMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, GlobalDefences, COND_None, REPNOTIFY_Always);*/
	
	


	//Secondary Current Attribute
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Stamina, COND_None, REPNOTIFY_Always);


	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Gems, COND_None, REPNOTIFY_Always);
	
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
	
	if(Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxEffectiveHealth()));
		SetMaxEffectiveHealth(GetMaxEffectiveHealth());
		SetMaxHealth(GetMaxHealth());
		SetReservedHealth(GetReservedHealth());
	}
	if(Attribute == GetManaAttribute())
	{
		SetMana(  FMath::Clamp(GetMana(), 0.0f, GetMaxMana()));
		SetMaxEffectiveMana(GetMaxEffectiveMana());
		SetMaxMana(GetMaxMana());
		SetReservedMana(GetReservedMana());
	}
	if(Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
		SetMaxEffectiveStamina(GetMaxEffectiveStamina());
		SetMaxStamina(GetMaxStamina());
		SetReservedStamina(GetReservedStamina());
	}

}




void UPHAttributeSet::SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props)
{
	Props.EffectContextHandle = Data.EffectSpec.GetContext();
	Props.SourceASC = Props.EffectContextHandle.GetOriginalInstigatorAbilitySystemComponent();

	if(IsValid(Props.SourceASC) && Props.SourceASC->AbilityActorInfo.IsValid() && Props.SourceASC->AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.SourceAvatarActor = Props.SourceASC->AbilityActorInfo->AvatarActor.Get();
		Props.SourceController = Props.SourceASC->AbilityActorInfo->PlayerController.Get();
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

void UPHAttributeSet::OnRep_MaxStaminaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxStaminaRegenRate, OldAmount)
}

void UPHAttributeSet::OnRep_StaminaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaRegenAmount, OldAmount)
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

//Mana End

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


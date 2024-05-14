// Copyright@2024 Quentin Davis 


#include "Character/PHBaseCharacter.h"

#include "AbilitySystem/PHAttributeSet.h"
#include "Character/Player/State/PHPlayerState.h"

class APHPlayerState;

APHBaseCharacter::APHBaseCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

}

UAbilitySystemComponent* APHBaseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APHBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if(AbilitySystemComponent)
	{
		if (const UPHAttributeSet* AttributeSetPtr = AbilitySystemComponent->GetSet<UPHAttributeSet>())
		{
			// Use the new helper function to set up the timers.
			SetRegenerationTimer(HealthRegenTimer, &APHBaseCharacter::HealthRegeneration, AttributeSetPtr->GetHealthRegenRate());
			SetRegenerationTimer(ManaRegenTimer, &APHBaseCharacter::ManaRegeneration, AttributeSetPtr->GetManaRegenRate());
			SetRegenerationTimer(StaminaRegenTimer, &APHBaseCharacter::StaminaRegeneration, AttributeSetPtr->GetStaminaRegenRate());
		}

	}
}

bool APHBaseCharacter::CanSprint() const
{
	const bool bReturnValue =  Super::CanSprint();

		
	if(bIsInRecovery)
	{
		return false;
	}
	return bReturnValue;
}


void APHBaseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	StaminaDegen(DeltaSeconds);
}


void APHBaseCharacter::SetRegenerationTimer(FTimerHandle& TimerHandle, void(APHBaseCharacter::* RegenFunction)() const, float RegenRate)
{
	auto& TimerManager = GetWorldTimerManager();

	// Check and clear the timer if it's already set.
	if (TimerManager.IsTimerActive(TimerHandle))
	{
		TimerManager.ClearTimer(TimerHandle);
	}

	// Set the new timer with the provided regeneration rate and function.
	TimerManager.SetTimer(TimerHandle, this, RegenFunction, RegenRate, true);
}

void APHBaseCharacter::HealthRegenRateChange()
{
	const UPHAttributeSet* AttributeSetPtr = AbilitySystemComponent->GetSet<UPHAttributeSet>();
	if (AttributeSetPtr)
	{
		SetRegenerationTimer(HealthRegenTimer, &APHBaseCharacter::HealthRegeneration, AttributeSetPtr->GetHealthRegenRate());
	}
}

void APHBaseCharacter::ManaRegenRateChange()
{
	const UPHAttributeSet* AttributeSetPtr = AbilitySystemComponent->GetSet<UPHAttributeSet>();
	if (AttributeSetPtr)
	{
		SetRegenerationTimer(ManaRegenTimer, &APHBaseCharacter::ManaRegeneration, AttributeSetPtr->GetManaRegenRate());
	}
}

void APHBaseCharacter::StaminaRegenRateChange()
{
	const UPHAttributeSet* AttributeSetPtr = AbilitySystemComponent->GetSet<UPHAttributeSet>();
	if (AttributeSetPtr)
	{
		SetRegenerationTimer(StaminaRegenTimer, &APHBaseCharacter::StaminaRegeneration, AttributeSetPtr->GetStaminaRegenRate());
	}
}

void APHBaseCharacter::HealthRegeneration() const
{
	check(HealthRegenEffect)
	ApplyEffectToSelf(HealthRegenEffect,1);
}

void APHBaseCharacter::ManaRegeneration() const
{
	check(ManaRegenEffect)
	ApplyEffectToSelf(ManaRegenEffect,1);
}

void APHBaseCharacter::StaminaRegeneration() const
{
	check(StaminaRegenEffect)
	ApplyEffectToSelf(StaminaRegenEffect,1);
	
}

void APHBaseCharacter::StaminaDegen(const float DeltaTime)
{
	check(StaminaDegenEffect);
	if(const UPHAttributeSet* PHAttributeSet = Cast<UPHAttributeSet>(AttributeSet); !bIsInRecovery && PHAttributeSet->GetStamina() <= 0)
	{
		bIsInRecovery = true;
	}

	
	if(GetGait() == EALSGait::Sprinting)
	{
		ApplyEffectToSelf(StaminaDegenEffect, 1);
	}

	
	if(bIsInRecovery)
	{
		TimeSinceLastRecovery += DeltaTime;
		if(TimeSinceLastRecovery >= 3.0f)
		{
			bIsInRecovery = false;
			TimeSinceLastRecovery = 0.0f;
		}
	}
	

}

void APHBaseCharacter::InitAbilityActorInfo()
{

}

void APHBaseCharacter::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level) const
{
	check(IsValid(GetAbilitySystemComponent()));
	check(GameplayEffectClass);
	FGameplayEffectContextHandle ContextHandle =  GetAbilitySystemComponent()->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	const FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(GameplayEffectClass, Level, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), GetAbilitySystemComponent());
}

void APHBaseCharacter::InitializeDefaultAttributes() const
{
	ApplyEffectToSelf(DefaultPrimaryAttributes, 1.f);
	ApplyEffectToSelf(DefaultSecondaryMaxAttributes,1.f);
	ApplyEffectToSelf(DefaultSecondaryCurrentAttributes,1.f);
	ApplyEffectToSelf(DefaultVitalAttributes, 1.f);

}

// Copyright@2024 Quentin Davis 


#include "Character/PHBaseCharacter.h"

#include "AbilitySystem/PHAbilitySystemComponent.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Character/ALSPlayerController.h"
#include "Character/Player/State/PHPlayerState.h"

#include "Components/EquipmentManager.h"
#include "Components/InventoryManager.h"
#include "UI/HUD/PHHUD.h"

class APHPlayerState;

APHBaseCharacter::APHBaseCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	EquipmentManager = CreateDefaultSubobject<UEquipmentManager>(TEXT("EquipmentManager"));
	InventoryManager = CreateDefaultSubobject<UInventoryManager>(TEXT("InventoryManager"));
	if(!bIsPlayer)
	{
		AbilitySystemComponent = CreateDefaultSubobject<UPHAbilitySystemComponent>("Ability System Component");
		AbilitySystemComponent->SetIsReplicated(true);
		AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
		AttributeSet = CreateDefaultSubobject<UPHAttributeSet>("Attribute Set");	
	}
}

auto APHBaseCharacter::PossessedBy(AController* NewController) -> void
{
	Super::PossessedBy(NewController);
	InitAbilityActorInfo();
}

UAbilitySystemComponent* APHBaseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

int32 APHBaseCharacter::GetPlayerLevel()
{
	if(bIsPlayer)
	{
		const APHPlayerState* LocalPlayerState = GetPlayerState<APHPlayerState>();
		check(LocalPlayerState)
		return  LocalPlayerState->GetPlayerLevel();
	}

	return Level;
	
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
	if (!bIsPlayer)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		Cast<UPHAbilitySystemComponent>(AbilitySystemComponent)->AbilityActorInfoSet();
	}
	else
	{
		
		APHPlayerState* LocalPlayerState = GetPlayerState<APHPlayerState>();
		check(LocalPlayerState);
		LocalPlayerState->GetAbilitySystemComponent()->InitAbilityActorInfo(LocalPlayerState, this);
		Cast<UPHAbilitySystemComponent>(LocalPlayerState->GetAbilitySystemComponent())->AbilityActorInfoSet();
		AbilitySystemComponent = LocalPlayerState->GetAbilitySystemComponent();
		InitializeDefaultAttributes();
		AttributeSet = LocalPlayerState->GetAttributeSet();

		if (AALSPlayerController* PHPlayerController = Cast<AALSPlayerController>(GetController()))
		{
			if (APHHUD* LocalPHHUD = Cast<APHHUD>(PHPlayerController->GetHUD()))
			{
				// Initialize HUD overlay with necessary components
				LocalPHHUD->InitOverlay(PHPlayerController, GetPlayerState<APHPlayerState>(), AbilitySystemComponent, AttributeSet);
			}
		}
	}

}

void APHBaseCharacter::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float InLevel) const
{
	check(IsValid(GetAbilitySystemComponent()));
	check(GameplayEffectClass);
	FGameplayEffectContextHandle ContextHandle =  GetAbilitySystemComponent()->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	const FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(GameplayEffectClass, InLevel, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), GetAbilitySystemComponent());
}

void APHBaseCharacter::InitializeDefaultAttributes() const
{
	ApplyEffectToSelf(DefaultPrimaryAttributes, 1.f);
	ApplyEffectToSelf(DefaultSecondaryMaxAttributes,1.f);
	ApplyEffectToSelf(DefaultSecondaryCurrentAttributes,1.f);
	ApplyEffectToSelf(DefaultVitalAttributes, 1.f);

}

// Copyright@2024 Quentin Davis 


#include "Character/AI/PHAICharacter.h"

#include "AbilitySystem/PHAbilitySystemComponent.h"
#include "AbilitySystem/PHAttributeSet.h"

APHAICharacter::APHAICharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	AbilitySystemComponent = CreateDefaultSubobject<UPHAbilitySystemComponent>("Ability System Component");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	AttributeSet = CreateDefaultSubobject<UPHAttributeSet>("Attribute Set");	
}

int32 APHAICharacter::GetPlayerLevel()
{
	return Level;
}

void APHAICharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void APHAICharacter::InitAbilityActorInfo()
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	Cast<UPHAbilitySystemComponent>(AbilitySystemComponent)->AbilityActorInfoSet();
}

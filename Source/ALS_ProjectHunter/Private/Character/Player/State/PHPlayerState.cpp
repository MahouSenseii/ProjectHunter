// Copyright@2024 Quentin Davis 


#include "Character/Player/State/PHPlayerState.h"

#include "AbilitySystem/PHAbilitySystemComponent.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Net/UnrealNetwork.h"



APHPlayerState::APHPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UPHAbilitySystemComponent>("Ability System Component");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	AttributeSet = CreateDefaultSubobject<UPHAttributeSet>("Attribute Set");
	NetUpdateFrequency = 100.0f;
}

void APHPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APHPlayerState, Level);
	DOREPLIFETIME(APHPlayerState, XP);
	DOREPLIFETIME(APHPlayerState, AttributePoints);
	DOREPLIFETIME(APHPlayerState, MaxAttributePoints);
	
}

UAbilitySystemComponent* APHPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APHPlayerState::AddToXP(int32 InXp)
{
	XP += InXp;
	OnXPChangedDelegate.Broadcast(XP);
}

void APHPlayerState::SetXP(int32 InXP)
{
	XP = InXP;
	OnXPChangedDelegate.Broadcast(XP);
}

void APHPlayerState::AddToLevel()
{
	Level+= 1;
}

void APHPlayerState::SetLevel(int32 InLevel)
{
	Level = InLevel;
	OnLevelChangedDelegate.Broadcast(Level, false);
}

void APHPlayerState::SetAttributePoints(int32 InPoints)
{
	AttributePoints = InPoints;
	OnAttributePointsChangedDelegate.Broadcast(AttributePoints);
}

void APHPlayerState::SetMaxAttributePoints(int32 InPoints)
{
	AttributePoints = InPoints;
	OnMaxAttributePointsChangedDelegate.Broadcast(MaxAttributePoints);
}

void APHPlayerState::OnRep_Level(int32 OldLevel)
{
	OnLevelChangedDelegate.Broadcast(Level, true);
}

void APHPlayerState::OnRep_XP(int32 OldXP)
{
	OnXPChangedDelegate.Broadcast(XP);
}

void APHPlayerState::OnRep_AttributePoints(int32 InPoints)
{
	OnAttributePointsChangedDelegate.Broadcast(AttributePoints);
}

void APHPlayerState::OnRep_MaxAttributePoints(int32 InPoints)
{
	AttributePoints += InPoints;
	OnAttributePointsChangedDelegate.Broadcast(AttributePoints);
}

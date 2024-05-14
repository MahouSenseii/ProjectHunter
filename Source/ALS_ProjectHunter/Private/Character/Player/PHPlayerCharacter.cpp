// Copyright@2024 Quentin Davis 

#include "Character/Player/PHPlayerCharacter.h"

#include "AbilitySystem/PHAbilitySystemComponent.h"
#include "Character/ALSPlayerController.h"
#include "Character/Player/State/PHPlayerState.h"
#include "Kismet/KismetInputLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/HUD/PHHUD.h"

APHPlayerCharacter::APHPlayerCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	
}

void APHPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitAbilityActorInfo();
}

void APHPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitAbilityActorInfo();
}

int32 APHPlayerCharacter::GetPlayerLevel()
{
	const APHPlayerState* LocalPlayerState = GetPlayerState<APHPlayerState>();
	check(LocalPlayerState)
	return  LocalPlayerState->GetPlayerLevel();
}

void APHPlayerCharacter::InitAbilityActorInfo()
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


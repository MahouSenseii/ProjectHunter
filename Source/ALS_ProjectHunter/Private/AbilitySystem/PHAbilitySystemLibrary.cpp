// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/PHAbilitySystemLibrary.h"

#include "Character/Player/State/PHPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "UI/WidgetController/PHWidgetController.h"
#include "UI/HUD/PHHUD.h"

UPHOverlayWidgetController* UPHAbilitySystemLibrary::GetOverlayWidgetController(const UObject* WorldContextObject)
{
	if(APlayerController* PC =  UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if(APHHUD* HUD = Cast<APHHUD>(PC->GetHUD()))
		{
			APHPlayerState* PS = PC->GetPlayerState<APHPlayerState>();
			UAbilitySystemComponent* ASC =  PS->GetAbilitySystemComponent();
			UAttributeSet* AS = PS->GetAttributeSet();
			const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
			return HUD->GetOverlayWidgetController(WidgetControllerParams);
		}
	}
	return nullptr;
}

UAttributeMenuWidgetController* UPHAbilitySystemLibrary::GetAttibuteMenuWidgetController(const UObject* WorldContextObject)
{
	if(APlayerController* PC =  UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if(APHHUD* HUD = Cast<APHHUD>(PC->GetHUD()))
		{
			APHPlayerState* PS = PC->GetPlayerState<APHPlayerState>();
			UAbilitySystemComponent* ASC =  PS->GetAbilitySystemComponent();
			UAttributeSet* AS = PS->GetAttributeSet();
			const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
			return HUD->GetAttributeWidgetController(WidgetControllerParams);
		}
	}
	return nullptr;
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/PHOverlayWidgetController.h"

#include "AbilitySystem/PHAbilitySystemComponent.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "AbilitySystem/Data/LevelUpInfo.h"
#include "Character/Player/State/PHPlayerState.h"

void UPHOverlayWidgetController::BroadcastInitialValues()
{
	Super::BroadcastInitialValues();

	const UPHAttributeSet* PHAttributeSet = CastChecked<UPHAttributeSet>(AttributeSet);


	OnHealthChange.Broadcast(PHAttributeSet->GetHealth());
	OnMaxHealthChange.Broadcast(PHAttributeSet->GetMaxHealth());
	OnHealthReservedChange.Broadcast(PHAttributeSet->GetReservedHealth());
	OnMaxEffectiveHealthChange.Broadcast(PHAttributeSet->GetMaxEffectiveHealth());
	
	
	OnManaChange.Broadcast(PHAttributeSet->GetMana());
	OnMaxManaChange.Broadcast(PHAttributeSet->GetMaxMana());
	OnManaReservedChange.Broadcast(PHAttributeSet->GetReservedMana());
	
	OnStaminaChange.Broadcast(PHAttributeSet->GetStamina());
	OnMaxStaminaChange.Broadcast(PHAttributeSet->GetMaxStamina());
	OnStaminaReservedChange.Broadcast(PHAttributeSet->GetReservedStamina());

	
	

	
	
}

void UPHOverlayWidgetController::BindCallbacksToDependencies()
{
	Super::BindCallbacksToDependencies();
	APHPlayerState* PHPlayerState = CastChecked<APHPlayerState>(PlayerState);
	PHPlayerState->OnXPChangedDelegate.AddUObject(this, &UPHOverlayWidgetController::OnXPChange);
	
	
	const UPHAttributeSet* PHAttributeSet = CastChecked<UPHAttributeSet>(AttributeSet);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnHealthChange.Broadcast(Data.NewValue);
			}
		);
		
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetMaxHealthAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnMaxHealthChange.Broadcast(Data.NewValue);
		}
	);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetReservedHealthAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnHealthReservedChange.Broadcast(Data.NewValue);
		}
	);
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetStaminaAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnStaminaChange.Broadcast(Data.NewValue);
		}
	);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetMaxStaminaAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnMaxStaminaChange.Broadcast(Data.NewValue);
		}
	);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetReservedStaminaAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnStaminaReservedChange.Broadcast(Data.NewValue);
		}
	);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetManaAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnManaChange.Broadcast(Data.NewValue);
		}
	);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetMaxManaAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnMaxManaChange.Broadcast(Data.NewValue);
		}
	);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetReservedManaAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnManaReservedChange.Broadcast(Data.NewValue);
			}
		);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetMaxEffectiveHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnMaxEffectiveHealthChange.Broadcast(Data.NewValue);
			}
		);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetMaxEffectiveManaAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnMaxEffectiveManaChange.Broadcast(Data.NewValue);
			}
		);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetMaxEffectiveStaminaAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnMaxEffectiveStaminaChange.Broadcast(Data.NewValue);
			}
		);
	Cast<UPHAbilitySystemComponent>(AbilitySystemComponent)->EffectAssetTags.AddLambda(
		[](const FGameplayTagContainer& AssetTags)
		{
			for (const FGameplayTag Tag : AssetTags)
			{
				//TODO: Broadcast the tag to the Widget Controller.
			}
		}
	);
}


void UPHOverlayWidgetController::OnXPChange(int32 NewXP) const
{
	APHPlayerState* PHPlayerState = CastChecked<APHPlayerState>(PlayerState);

	//Get Level up info 
	ULevelUpInfo* LevelUpInfo = PHPlayerState->LevelUpInfo;
	checkf(LevelUpInfo, TEXT("Unable to find LevelUpInfo. Please fill out in PlayerState Blueprint."))

	const int32 CurrentLevel = PHPlayerState->GetPlayerLevel();

	if(const int32 MaxLevel =  LevelUpInfo->LevelUpInformation.Num(); CurrentLevel <= MaxLevel && CurrentLevel > 0 )
	{
		const int32 LevelUpRequirement = LevelUpInfo->LevelUpInformation[CurrentLevel].LevelUpRequirement;

		int32 XPNeededToLevel  = LevelUpInfo->GetXpNeededForLevelUp(CurrentLevel);
		int32 FinalXP  = NewXP; 
		if( FinalXP <= XPNeededToLevel )
		{
			FinalXP =  LevelUpInfo->LevelUp(NewXP, CurrentLevel);
			PHPlayerState->AddToLevel();
			XPNeededToLevel  =  LevelUpInfo->GetXpNeededForLevelUp(CurrentLevel);
		}
		const float XPBarPercent =  static_cast<float>(FinalXP)  / static_cast<float>(XPNeededToLevel);
		OnXPPercentChangeDelegate.Broadcast(XPBarPercent);
	}
	
}

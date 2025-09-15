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
    
    // Validate required components first
    if (!IsValid(PlayerState) || !IsValid(AbilitySystemComponent) || !IsValid(AttributeSet))
    {
        UE_LOG(LogTemp, Error, TEXT("UPHOverlayWidgetController::BindCallbacksToDependencies - Missing required components"));
        return;
    }

    // Safe cast to player state
    APHPlayerState* PHPlayerState = Cast<APHPlayerState>(PlayerState);
    if (!IsValid(PHPlayerState))
    {
        UE_LOG(LogTemp, Error, TEXT("UPHOverlayWidgetController::BindCallbacksToDependencies - Failed to cast to APHPlayerState"));
        return;
    }

    // Bind XP change delegate
    PHPlayerState->OnXPChangedDelegate.AddDynamic(this, &UPHOverlayWidgetController::OnXPChange);

    // Safe cast to attribute set
    const UPHAttributeSet* PHAttributeSet = Cast<UPHAttributeSet>(AttributeSet);
    if (!IsValid(PHAttributeSet))
    {
        UE_LOG(LogTemp, Error, TEXT("UPHOverlayWidgetController::BindCallbacksToDependencies - Failed to cast to UPHAttributeSet"));
        return;
    }

    // Bind attribute change delegates with validation
    if (PHAttributeSet->GetHealthAttribute().IsValid())
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetHealthAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnHealthChange.Broadcast(Data.NewValue);
            }
        );
    }

    if (PHAttributeSet->GetMaxHealthAttribute().IsValid())
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetMaxHealthAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnMaxHealthChange.Broadcast(Data.NewValue);
            }
        );
    }

    if (PHAttributeSet->GetReservedHealthAttribute().IsValid())
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetReservedHealthAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnHealthReservedChange.Broadcast(Data.NewValue);
            }
        );
    }

    if (PHAttributeSet->GetStaminaAttribute().IsValid())
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetStaminaAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnStaminaChange.Broadcast(Data.NewValue);
            }
        );
    }

    if (PHAttributeSet->GetMaxStaminaAttribute().IsValid())
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetMaxStaminaAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnMaxStaminaChange.Broadcast(Data.NewValue);
            }
        );
    }

    if (PHAttributeSet->GetReservedStaminaAttribute().IsValid())
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetReservedStaminaAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnStaminaReservedChange.Broadcast(Data.NewValue);
            }
        );
    }

    if (PHAttributeSet->GetManaAttribute().IsValid())
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetManaAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnManaChange.Broadcast(Data.NewValue);
            }
        );
    }

    if (PHAttributeSet->GetMaxManaAttribute().IsValid())
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetMaxManaAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnMaxManaChange.Broadcast(Data.NewValue);
            }
        );
    }

    if (PHAttributeSet->GetReservedManaAttribute().IsValid())
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetReservedManaAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnManaReservedChange.Broadcast(Data.NewValue);
            }
        );
    }

    if (PHAttributeSet->GetMaxEffectiveHealthAttribute().IsValid())
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetMaxEffectiveHealthAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnMaxEffectiveHealthChange.Broadcast(Data.NewValue);
            }
        );
    }

    if (PHAttributeSet->GetMaxEffectiveManaAttribute().IsValid())
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetMaxEffectiveManaAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnMaxEffectiveManaChange.Broadcast(Data.NewValue);
            }
        );
    }

    if (PHAttributeSet->GetMaxEffectiveStaminaAttribute().IsValid())
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PHAttributeSet->GetMaxEffectiveStaminaAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnMaxEffectiveStaminaChange.Broadcast(Data.NewValue);
            }
        );
    }

    // SAFE cast to UPHAbilitySystemComponent for EffectAssetTags
    UPHAbilitySystemComponent* PHAbilitySystemComponent = Cast<UPHAbilitySystemComponent>(AbilitySystemComponent);
    if (IsValid(PHAbilitySystemComponent))
    {
        PHAbilitySystemComponent->EffectAssetTags.AddLambda(
            [this](const FGameplayTagContainer& AssetTags)
            {
                for (const FGameplayTag& Tag : AssetTags)
                {
                    // TODO: Broadcast the tag to the Widget Controller.
                    UE_LOG(LogTemp, Log, TEXT("Effect Asset Tag: %s"), *Tag.ToString());
                }
            }
        );
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UPHOverlayWidgetController::BindCallbacksToDependencies - AbilitySystemComponent is not a UPHAbilitySystemComponent"));
    }
}


void UPHOverlayWidgetController::OnXPChange(int32 NewXP)
{
	const APHPlayerState* PHPlayerState = CastChecked<APHPlayerState>(PlayerState);
	const ULevelUpInfo* LevelUpInfo = PHPlayerState->LevelUpInfo;

	checkf(LevelUpInfo, TEXT("LevelUpInfo missing on PlayerState!"))

	const int32 CurrentLevel = PHPlayerState->GetPlayerLevel();
	const int32 XPNeededToLevel = LevelUpInfo->GetXpNeededForLevelUp(CurrentLevel);
	const int32 ClampedXP = FMath::Clamp(NewXP, 0, XPNeededToLevel);

	const float XPBarPercent = static_cast<float>(ClampedXP) / static_cast<float>(XPNeededToLevel);
	OnXPPercentChangeDelegate.Broadcast(XPBarPercent);
}


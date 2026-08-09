#include "Framework/System/Cheats/HunterCheatComponent.h"

#include "AbilitySystem/HunterAbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogHunterCheats, Log, All);

namespace HunterCheatComponentPrivate
{
	void PrintCheatMessage(const FString& Message, const FColor& Color = FColor::Green, const float Duration = 5.0f)
	{
		UE_LOG(LogHunterCheats, Warning, TEXT("%s"), *Message);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, Duration, Color, Message);
		}
	}

	bool ValidateASC(const UHunterAbilitySystemComponent* ASC, const TCHAR* CheatName)
	{
		if (ASC)
		{
			return true;
		}

		PrintCheatMessage(FString::Printf(TEXT("%s failed: No Hunter ASC found."), CheatName), FColor::Red);
		return false;
	}

	UHunterAbilitySystemComponent* GetHunterASCFromActor(AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}

		if (const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Actor))
		{
			if (UAbilitySystemComponent* ASC = AbilityInterface->GetAbilitySystemComponent())
			{
				return Cast<UHunterAbilitySystemComponent>(ASC);
			}
		}

		return Actor->FindComponentByClass<UHunterAbilitySystemComponent>();
	}

	UHunterAbilitySystemComponent* GetHunterASCFromPawn(APawn* Pawn)
	{
		if (UHunterAbilitySystemComponent* ASC = GetHunterASCFromActor(Pawn))
		{
			return ASC;
		}

		APlayerState* PlayerState = Pawn ? Pawn->GetPlayerState() : nullptr;
		return GetHunterASCFromActor(PlayerState);
	}
}

UHunterCheatComponent::UHunterCheatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHunterCheatComponent::DisableStaminaDrain()
{
#if UE_BUILD_SHIPPING
	return;
#else
	UHunterAbilitySystemComponent* ASC = GetTargetASC();
	if (!HunterCheatComponentPrivate::ValidateASC(ASC, TEXT("DisableStaminaDrain")))
	{
		return;
	}

	ASC->Debug_DisableStaminaDrain();
	HunterCheatComponentPrivate::PrintCheatMessage(TEXT("Cheat: stamina drain disabled."));
#endif
}

void UHunterCheatComponent::ReactivateStaminaDrain()
{
#if UE_BUILD_SHIPPING
	return;
#else
	UHunterAbilitySystemComponent* ASC = GetTargetASC();
	if (!HunterCheatComponentPrivate::ValidateASC(ASC, TEXT("ReactivateStaminaDrain")))
	{
		return;
	}

	ASC->Debug_ReactivateStaminaDrain();
	HunterCheatComponentPrivate::PrintCheatMessage(TEXT("Cheat: stamina drain reactivated."));
#endif
}

void UHunterCheatComponent::RefillHealth()
{
#if UE_BUILD_SHIPPING
	return;
#else
	UHunterAbilitySystemComponent* ASC = GetTargetASC();
	if (!HunterCheatComponentPrivate::ValidateASC(ASC, TEXT("RefillHealth")))
	{
		return;
	}

	ASC->Debug_RefillHealth();
	HunterCheatComponentPrivate::PrintCheatMessage(TEXT("Cheat: health refilled."));
#endif
}

void UHunterCheatComponent::RefillStamina()
{
#if UE_BUILD_SHIPPING
	return;
#else
	UHunterAbilitySystemComponent* ASC = GetTargetASC();
	if (!HunterCheatComponentPrivate::ValidateASC(ASC, TEXT("RefillStamina")))
	{
		return;
	}

	ASC->Debug_RefillStamina();
	HunterCheatComponentPrivate::PrintCheatMessage(TEXT("Cheat: stamina refilled."));
#endif
}

void UHunterCheatComponent::ReserveHealth(const float NewValue)
{
#if UE_BUILD_SHIPPING
	return;
#else
	UHunterAbilitySystemComponent* ASC = GetTargetASC();
	if (!HunterCheatComponentPrivate::ValidateASC(ASC, TEXT("ReserveHealth")))
	{
		return;
	}

	ASC->Debug_ReserveHealth(NewValue);
	HunterCheatComponentPrivate::PrintCheatMessage(FString::Printf(TEXT("Cheat: reserved health set to %.2f."), NewValue));
#endif
}

void UHunterCheatComponent::ShowCheatList()
{
#if UE_BUILD_SHIPPING
	return;
#else
	const FString HelpText =
		TEXT("\nProject Hunter Cheat Commands\n")
		TEXT("ShowCheatList\n")
		TEXT("DisableStaminaDrain\n")
		TEXT("ReactivateStaminaDrain\n")
		TEXT("RefillHealth\n")
		TEXT("RefillStamina\n")
		TEXT("ReserveHealth (Amount)\n");

	HunterCheatComponentPrivate::PrintCheatMessage(HelpText, FColor::Green, 12.0f);
#endif
}

UHunterAbilitySystemComponent* UHunterCheatComponent::GetTargetASC() const
{
	AActor* Owner = GetOwner();
	if (UHunterAbilitySystemComponent* OwnerASC = HunterCheatComponentPrivate::GetHunterASCFromActor(Owner))
	{
		return OwnerASC;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Owner))
	{
		return HunterCheatComponentPrivate::GetHunterASCFromPawn(PlayerController->GetPawn());
	}

	return HunterCheatComponentPrivate::GetHunterASCFromPawn(Cast<APawn>(Owner));
}

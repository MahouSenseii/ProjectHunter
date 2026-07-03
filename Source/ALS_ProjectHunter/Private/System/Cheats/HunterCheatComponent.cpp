#include "System/Cheats/HunterCheatComponent.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystem/HunterAbilitySystemComponent.h"

#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

namespace
{
	void PrintCheatMessage(const FString& Message, const FColor& Color = FColor::Green, float Duration = 5.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				Duration,
				Color,
				Message
			);
		}
	}

	bool ValidateASC(UHunterAbilitySystemComponent* ASC, const FString& CheatName)
	{
		if (!ASC)
		{
			PrintCheatMessage(
				FString::Printf(TEXT("%s failed: No Hunter ASC found."), *CheatName),
				FColor::Red
			);

			return false;
		}

		return true;
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
#endif

	UHunterAbilitySystemComponent* ASC = GetTargetASC();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("DisableStaminaDrain failed: No Hunter ASC found."));
		return;
	}

	ASC->Debug_DisableStaminaDrain();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.0f,
			FColor::Green,
			TEXT("Cheat: Stamina drain disabled.")
		);
	}
}

void UHunterCheatComponent::ReactivateStaminaDrain()
{
#if UE_BUILD_SHIPPING
	return;
#endif

	UHunterAbilitySystemComponent* ASC = GetTargetASC();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReactivateStaminaDrain failed: No Hunter ASC found."));
		return;
	}

	ASC->Debug_ReactivateStaminaDrain();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.0f,
			FColor::Green,
			TEXT("Cheat: Stamina drain reactivated.")
		);
	}
}

void UHunterCheatComponent::RefillHealth()
{
#if UE_BUILD_SHIPPING
	return;
#endif

	UHunterAbilitySystemComponent* ASC = GetTargetASC();
	if (!ValidateASC(ASC, TEXT("RefillHealth")))
	{
		return;
	}

	ASC->Debug_RefillHealth();

	PrintCheatMessage(TEXT("Cheat: health refilled."));
}

void UHunterCheatComponent::RefillStamina()
{
#if UE_BUILD_SHIPPING
	return;
#endif

	UHunterAbilitySystemComponent* ASC = GetTargetASC();
	if (!ValidateASC(ASC, TEXT("RefillStamina")))
	{
		return;
	}

	ASC->Debug_RefillStamina();

	PrintCheatMessage(TEXT("Cheat: stamina refilled."));
}

void UHunterCheatComponent::ReserveHealth(float NewValue)
{
#if UE_BUILD_SHIPPING
	return;
#endif

	UHunterAbilitySystemComponent* ASC = GetTargetASC();
	if (!ValidateASC(ASC, TEXT("ReserveHealth")))
	{
		return;
	}

	ASC->Debug_ReserveHealth(NewValue);

	PrintCheatMessage(
		FString::Printf(TEXT("Cheat: reserved health set to %.2f."), NewValue)
	);
}

void UHunterCheatComponent::ShowCheatList()
{
#if UE_BUILD_SHIPPING
	return;
#endif

	const FString HelpText =
		TEXT("\n==== Project Hunter Cheat Commands ====\n")
		TEXT("ShowCheatList\n")
		TEXT("DisableStaminaDrain\n")
		TEXT("ReactivateStaminaDrain\n")
		TEXT("RefillHealth\n")
		TEXT("RefillStamina\n")
		TEXT("ReserveHealth (Amount)\n")
		TEXT("=======================================\n");

	UE_LOG(LogTemp, Warning, TEXT("%s"), *HelpText);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			12.0f,
			FColor::Green,
			HelpText
		);
	}
}

void UHunterCheatComponent::BeginPlay()
{
	Super::BeginPlay();
}

UHunterAbilitySystemComponent* UHunterCheatComponent::GetTargetASC() const
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return nullptr;
	}

	APawn* Pawn = PC->GetPawn();
	if (!Pawn)
	{
		return nullptr;
	}

	if (const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Pawn))
	{
		return Cast<UHunterAbilitySystemComponent>(
			AbilityInterface->GetAbilitySystemComponent()
		);
	}

	if (APlayerState* PlayerState = Pawn->GetPlayerState())
	{
		if (const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(PlayerState))
		{
			return Cast<UHunterAbilitySystemComponent>(
				AbilityInterface->GetAbilitySystemComponent()
			);
		}
	}

	return nullptr;
}

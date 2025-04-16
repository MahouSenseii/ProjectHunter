// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/PHTagUtilityLibrary.h"

#include "PHGameplayTags.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Character/ALSCharacter.h"
#include "Character/PHBaseCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

class UPHAttributeSet;
struct FGameplayTagContainer;

void UPHTagUtilityLibrary::UpdateAttributeThresholdTags(UAbilitySystemComponent* ASC, const UAttributeSet* AttributeSet)
{
	if (!ASC || !AttributeSet) return;

	const UPHAttributeSet* PHAttributes = Cast<UPHAttributeSet>(AttributeSet);
	if (!PHAttributes) return;


	constexpr float LowThreshold = 0.3f;
	constexpr float FullThreshold = 0.99f;

	const float Health = PHAttributes->GetHealth();
	const float MaxHealth = PHAttributes->GetMaxHealth();
	const float Mana = PHAttributes->GetMana();
	const float MaxMana = PHAttributes->GetMaxMana();
	const float Stamina = PHAttributes->GetStamina();
	const float MaxStamina = PHAttributes->GetMaxStamina();
	const float ArcaneShield = PHAttributes->GetArcaneShield();
	const float MaxArcaneShield = PHAttributes->GetMaxArcaneShield();

	auto UpdateTagPair = [ASC](const bool bLow, const bool bFull, const FGameplayTag& LowTag, const FGameplayTag& FullTag)
	{
		if (bLow)
		{
			ASC->AddLooseGameplayTag(LowTag);
			ASC->RemoveLooseGameplayTag(FullTag);
		}
		else if (bFull)
		{
			ASC->AddLooseGameplayTag(FullTag);
			ASC->RemoveLooseGameplayTag(LowTag);
		}
		else
		{
			ASC->RemoveLooseGameplayTag(LowTag);
			ASC->RemoveLooseGameplayTag(FullTag);
		}
	};

	UpdateTagPair(
		MaxHealth > 0.f && Health / MaxHealth <= LowThreshold,
		MaxHealth > 0.f && Health / MaxHealth >= FullThreshold,
		FPHGameplayTags::Get().Condition_OnLowHealth,
		FPHGameplayTags::Get().Condition_OnFullHealth
	);

	UpdateTagPair(
		MaxMana > 0.f && Mana / MaxMana <= LowThreshold,
		MaxMana > 0.f && Mana / MaxMana >= FullThreshold,
		FPHGameplayTags::Get().Condition_OnLowMana,
		FPHGameplayTags::Get().Condition_OnFullMana
	);

	UpdateTagPair(
		MaxStamina > 0.f && Stamina / MaxStamina <= LowThreshold,
		MaxStamina > 0.f && Stamina / MaxStamina >= FullThreshold,
		FPHGameplayTags::Get().Condition_OnLowStamina,
		FPHGameplayTags::Get().Condition_OnFullStamina
	);

	UpdateTagPair(
		MaxArcaneShield > 0.f && ArcaneShield / MaxArcaneShield <= LowThreshold,
		MaxArcaneShield > 0.f && ArcaneShield / MaxArcaneShield >= FullThreshold,
		FPHGameplayTags::Get().Condition_OnLowArcaneShield,
		FPHGameplayTags::Get().Condition_OnFullArcaneShield
	);
}

void UPHTagUtilityLibrary::UpdateMovementTags(UAbilitySystemComponent* ASC, APHBaseCharacter* Character)
{
	{
		if (!ASC || !Character) return;

		const FVector Velocity = Character->GetVelocity();
		const float SpeedSquared = Velocity.SizeSquared();

		const EALSGait Gait = Character->GetGait(); // ALS native
		const bool bIsMoving = SpeedSquared > KINDA_SMALL_NUMBER;
		const bool bIsIdle = !bIsMoving || Gait == EALSGait::Walking && SpeedSquared < 5.0f;

		// === WhileMoving / Stationary ===
		const bool bHasMovingTag = ASC->HasMatchingGameplayTag(FPHGameplayTags::Get().Condition_WhileMoving);
		const bool bHasStationaryTag = ASC->HasMatchingGameplayTag(FPHGameplayTags::Get().Condition_WhileStationary);

		if (bIsMoving && !bHasMovingTag)
			ASC->AddLooseGameplayTag(FPHGameplayTags::Get().Condition_WhileMoving);
		else if (!bIsMoving && bHasMovingTag)
			ASC->RemoveLooseGameplayTag(FPHGameplayTags::Get().Condition_WhileMoving);

		if (bIsIdle && !bHasStationaryTag)
			ASC->AddLooseGameplayTag(FPHGameplayTags::Get().Condition_WhileStationary);
		else if (!bIsIdle && bHasStationaryTag)
			ASC->RemoveLooseGameplayTag(FPHGameplayTags::Get().Condition_WhileStationary);

		// === Sprinting ===
		const bool bIsSprinting = Gait == EALSGait::Sprinting;
		const bool bHasSprintTag = ASC->HasMatchingGameplayTag(FPHGameplayTags::Get().Condition_Sprinting);

		if (bIsSprinting && !bHasSprintTag)
			ASC->AddLooseGameplayTag(FPHGameplayTags::Get().Condition_Sprinting);
		else if (!bIsSprinting && bHasSprintTag)
			ASC->RemoveLooseGameplayTag(FPHGameplayTags::Get().Condition_Sprinting);
	}
}

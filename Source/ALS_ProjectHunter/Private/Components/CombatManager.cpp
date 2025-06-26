// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/CombatManager.h"
#include "EngineUtils.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Character/PHBaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Item/WeaponItem.h"
#include "Library/PHCombatStructLibrary.h"

/* =========================== */
/* === Constructor & Setup === */
/* =========================== */

// Sets default values for this component's properties
UCombatManager::UCombatManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void UCombatManager::BeginPlay()
{
	Super::BeginPlay();
}

/* ==================================== */
/* === Damage Calculation Functions === */
/* ==================================== */


bool UCombatManager::RollForStatusEffect(const APHBaseCharacter* Attacker, const EDamageTypes DamageType)
{
    const UPHAttributeSet* Attributes = Cast<UPHAttributeSet>(Attacker->GetAttributeSet());

    float StatusChance = 0.f;

    switch (DamageType)
    {
    case EDamageTypes::DT_Fire:
        StatusChance = Attributes->GetChanceToIgnite();
        break;
    case EDamageTypes::DT_Ice:
        StatusChance = Attributes->GetChanceToFreeze();
        break;
        // Add more here
    default:
        break;
    }

    return FMath::FRand() <= StatusChance;
}

/* ============================= */
/* ========== Defenses ========= */
/* ============================= */

/* ============================= */
/* === Poise & Stagger System === */
/* ============================= */

void UCombatManager::ApplyPoiseDamage(APHBaseCharacter* Defender, float DamageTaken)
{
    if (!Defender || !Defender->HasAuthority()) return;

    const UPHAttributeSet* Attributes = Cast<UPHAttributeSet>(Defender->GetAttributeSet());
    if (!Attributes) return;

    const float PoiseThreshold = Attributes->GetPoise();
    const float PoiseResistance = Attributes->GetPoiseResistance();

    // Calculate Poise Damage
    const float PoiseDamage = DamageTaken * (1.0f - FMath::Clamp(PoiseResistance / 100.0f, 0.0f, 1.0f));

    Defender->ApplyPoiseDamage(PoiseDamage);

    // Check for Poise Break
    if (Defender->GetPoisePercentage() <= 0.0f)
    {
        StaggerCharacter(Defender);
        Defender->ApplyPoiseDamage(-Defender->GetCurrentPoiseDamage()); // Reset poise
    }

    Defender->ForceNetUpdate();
}

void UCombatManager::StaggerCharacter(APHBaseCharacter* Defender)
{
    if (!Defender || !Defender->HasAuthority()) return;

    if (UAnimMontage* StaggerMontage = Defender->GetStaggerAnimation())
    {
        Defender->PlayAnimMontage(StaggerMontage);
    }

    if (UCharacterMovementComponent* Movement = Defender->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
        Movement->DisableMovement();

        FTimerHandle StaggerTimerHandle;
        Defender->GetWorldTimerManager().SetTimer(
            StaggerTimerHandle,
            FTimerDelegate::CreateWeakLambda(Defender, [Movement]() {
                if (Movement) Movement->SetMovementMode(MOVE_Walking);
            }),
            1.0f, 
            false
        );
    }

    UE_LOG(LogTemp, Warning, TEXT("%s is staggered!"), *Defender->GetName());
}

/* ============================= */
/* === Utility & Other Logic === */
/* ============================= */

void UCombatManager::RegeneratePoise(float DeltaTime) const
{
    for (TActorIterator<APHBaseCharacter> It(GetWorld()); It; ++It)
    {
        APHBaseCharacter* Character = *It;
        if (!Character || !Character->HasAuthority()) continue;

        const float TimeSinceLastHit = Character->GetTimeSinceLastPoiseHit();
        const float CurrentPoiseDamage = Character->GetCurrentPoiseDamage();

        if (CurrentPoiseDamage <= 0.0f) continue;

        if (TimeSinceLastHit >= 2.0f)
        {
            const float RegenRate = 10.0f * DeltaTime;
            Character->ApplyPoiseDamage(-RegenRate);
        }

        Character->SetTimeSinceLastPoiseHit(TimeSinceLastHit + DeltaTime);
    }
}




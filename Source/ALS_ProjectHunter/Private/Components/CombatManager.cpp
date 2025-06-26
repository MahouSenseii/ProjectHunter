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


/* ============================= */
/* === Utility & Other Logic === */
/* ============================= */




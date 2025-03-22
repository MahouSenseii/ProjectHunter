// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/PHBaseCharacter.h"
#include "Components/ActorComponent.h"
#include "Item/WeaponItem.h"
#include "Library/PHCombatStructLibrary.h"
#include "CombatManager.generated.h"




UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALS_PROJECTHUNTER_API UCombatManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCombatManager();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION()
	static FDamageByType GetAllDamageByType(const APHBaseCharacter* Attacker, const UEquippableItem* Weapon);

	UFUNCTION()
	static float CalculateBaseDamage(const APHBaseCharacter* Attacker, const EAttackType AttackType);

	UFUNCTION()
	static float CalculateDamageAfterDefenses(float RawDamage, const APHBaseCharacter* Defender, EDamageTypes DamageType);

	UFUNCTION()
	static bool RollForCriticalHit(const APHBaseCharacter* Attacker, const EAttackType AttackType);

	UFUNCTION()
	static float GetCriticalMultiplier(const APHBaseCharacter* Attacker,  const EAttackType AttackType);


	UFUNCTION(BlueprintCallable, Category = "Combat")
	static FCombatHitResult CalculateFinalDamage(
		const APHBaseCharacter* Attacker,
		const APHBaseCharacter* Defender,
		const UEquippableItem* Weapon,
		EAttackType AttackType,
		EDamageTypes PrimaryDamageType
	);


	static FDamageHitResultByType CalculateAllFinalDamageByType(
	const APHBaseCharacter* Attacker,
	const APHBaseCharacter* Defender,
	const FDamageByType& RawDamage,
	const EAttackType AttackType);


	UFUNCTION()
	static bool RollForStatusEffect(const APHBaseCharacter* Attacker, EDamageTypes DamageType);

	static FDamageByType GetAllDamageByType(const APHBaseCharacter* Attacker, const UEquippableItem* Weapon, bool bIsSpell);

	UFUNCTION()
	static FDefenseStats GetAllDefenses(const APHBaseCharacter* Defender);


	/* ============================= */
	/* === Poise & Stagger System === */
	/* ============================= */

	UFUNCTION(BlueprintCallable, Category = "Combat|Stagger")
	void ApplyPoiseDamage(APHBaseCharacter* Defender, float DamageTaken);
	void RegeneratePoise(float DeltaTime) const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Stagger")
	static void StaggerCharacter(APHBaseCharacter* Defender);

public:	
	
		
};

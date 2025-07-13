// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/PHBaseCharacter.h"
#include "Components/ActorComponent.h"
#include "Library/PHCombatStructLibrary.h"
#include "CombatManager.generated.h"




UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALS_PROJECTHUNTER_API UCombatManager : public UActorComponent
{
        GENERATED_BODY()

public:
        // Sets default values for this component's properties
        UCombatManager();

       /** Returns true if the owner is currently blocking */
       UFUNCTION(BlueprintCallable, Category = "Combat")
       bool IsBlocking() const { return bIsBlocking; }

       /** Set the blocking state */
       UFUNCTION(BlueprintCallable, Category = "Combat")
       void SetBlocking(bool bNewBlocking) { bIsBlocking = bNewBlocking; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;


       UFUNCTION()
       static bool RollForStatusEffect(const APHBaseCharacter* Attacker, EDamageTypes DamageType);

       /** Calculate the final damage values after applying attacker bonuses and defender resistances */
       UFUNCTION(BlueprintCallable, Category = "Combat")
       FDamageHitResultByType CalculateDamage(const APHBaseCharacter* Attacker,
                                              const APHBaseCharacter* Defender) const;

       /** Apply calculated damage and handle poise/stagger */
       UFUNCTION(BlueprintCallable, Category = "Combat")
       void ApplyDamage(const APHBaseCharacter* Attacker,
                        APHBaseCharacter* Defender,
                        float PoiseDamage = 0.f);

       /** Returns the damage type with the highest value in a hit result */
       UFUNCTION(BlueprintPure, Category = "Combat")
       static EDamageTypes GetHighestDamageType(const FDamageHitResultByType& HitResult);

public:

	
	/** Animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim", meta = (AllowPrivateAccess = "true"))
       UAnimMontage* StaggerMontage;

private:
       /** True when the character is actively blocking with a shield */
       UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat", meta = (AllowPrivateAccess = "true"))
       bool bIsBlocking = false;
		
};

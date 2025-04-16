// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "PHTagUtilityLibrary.generated.h"

class UAttributeSet;
/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHTagUtilityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	/** Handles health, mana, stamina, and arcane shield thresholds */
	UFUNCTION(BlueprintCallable, Category = "Tags|Threshold")
	static void UpdateAttributeThresholdTags(UAbilitySystemComponent* ASC, const UAttributeSet* AttributeSet);

	/** Handles tags related to motion: WhileMoving, WhileStationary, Sprinting */
	UFUNCTION(BlueprintCallable, Category = "Tags|Movement")
	static void UpdateMovementTags(UAbilitySystemComponent* ASC, APHBaseCharacter* Character);

	/** Combat tags like UsingMelee, TakingDamage */
	/*UFUNCTION(BlueprintCallable, Category = "Tags|Combat")
	static void UpdateCombatStateTags(UAbilitySystemComponent* ASC, ACharacter* Character);

	/** Zone or environment-based triggers 
	UFUNCTION(BlueprintCallable, Category = "Tags|Environment")
	static void UpdateEnvironmentTags(UAbilitySystemComponent* ASC, AActor* SourceActor);*/
};

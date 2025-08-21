// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PHCombatStructLibrary.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CombatFunctionLibrary.generated.h"

/**
 * Converts different types of combat damage results into human-readable text for UI or logging purposes.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UCombatFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Converts combat damage result into readable FText for UI/logging */
	UFUNCTION(BlueprintPure, Category = "Combat|Text")
	static FText FormatDamageHitResult(const FDamageHitResultByType& HitResult);
	

	UFUNCTION(BlueprintPure, Category = "Combat|Text")
	static FLinearColor GetDamageColor(EDamageTypes Type);


	UFUNCTION(BlueprintPure, Category = "Combat|Text")
	static FText FormatPopupDamageText(const FDamageHitResultByType& HitResult);

	UFUNCTION(BlueprintPure, Category = "Combat|Text")
	static FText GetStrongestDamageTypeName(const FDamageHitResultByType& Hit);

	UFUNCTION(BlueprintPure, Category = "Combat|Display")
	static FDamageTypeDisplayInfo GetStrongestDamageTypeInfo(const FDamageHitResultByType& HitResult);

	static FString DamageTypeToString(EDamageTypes Type);
	
};

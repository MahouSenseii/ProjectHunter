// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PHGameplayTagLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHGameplayTagLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Returns true if Actor has Tag (hierarchical by default, e.g. X.Y matches X)
	UFUNCTION(BlueprintPure, Category="GameplayTags", meta=(DefaultToSelf="Actor"))
	static bool CheckTag(const AActor* Actor, FGameplayTag Tag, bool bExactMatch = false);

	// Convenience: pass a tag name (safe; returns false if tag doesn’t exist)
	UFUNCTION(BlueprintPure, Category="GameplayTags", meta=(DefaultToSelf="Actor"))
	static bool CheckTagByName(const AActor* Actor, FName TagName, bool bExactMatch = false);

	// Containers (useful for gating abilities/effects)
	UFUNCTION(BlueprintPure, Category="GameplayTags", meta=(DefaultToSelf="Actor"))
	static bool CheckTagsAny(const AActor* Actor, const FGameplayTagContainer& Tags, bool bExactMatch = false);

	UFUNCTION(BlueprintPure, Category="GameplayTags", meta=(DefaultToSelf="Actor"))
	static bool CheckTagsAll(const AActor* Actor, const FGameplayTagContainer& Tags, bool bExactMatch = false);

	// (Optional) Actor’s native name tags (AActor::Tags), not GAS tags
	UFUNCTION(BlueprintPure, Category="GameplayTags", meta=(DefaultToSelf="Actor"))
	static bool CheckActorNameTag(const AActor* Actor, FName NameTag);
	
};

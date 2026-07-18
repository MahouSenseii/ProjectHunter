#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Library/Enums/PHAbilityEnums.h"
#include "GameplayEffectTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PHAbilitySystemFunctionLibrary.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS()
class ALS_PROJECTHUNTER_API UPHAbilitySystemFunctionLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Rules")
	static bool ShouldActivateOnInputPressed(EPHAbilityActivationPolicy ActivationPolicy);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Rules")
	static bool ShouldActivateWhileInputHeld(EPHAbilityActivationPolicy ActivationPolicy);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Rules")
	static bool ShouldActivateOnSpawn(EPHAbilityActivationPolicy ActivationPolicy);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Rules")
	static bool IsInputActivatedPolicy(EPHAbilityActivationPolicy ActivationPolicy);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Rules")
	static bool IsIndependentActivationGroup(EPHAbilityActivationGroup ActivationGroup);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Rules")
	static bool IsReplaceableActivationGroup(EPHAbilityActivationGroup ActivationGroup);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Rules")
	static bool IsBlockingActivationGroup(EPHAbilityActivationGroup ActivationGroup);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Rules")
	static bool IsExclusiveActivationGroup(EPHAbilityActivationGroup ActivationGroup);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Rules")
	static bool IsValidActivationGroup(EPHAbilityActivationGroup ActivationGroup);

	static bool IsGameplayEffectClassCompatible(
		TSubclassOf<UGameplayEffect> ConfiguredClass,
		TSubclassOf<UGameplayEffect> RequiredParentClass);

	static TSubclassOf<UGameplayEffect> ResolveGameplayEffectClass(
		TSubclassOf<UGameplayEffect> ConfiguredClass,
		TSubclassOf<UGameplayEffect> NativeClass);

	static FGameplayEffectSpecHandle MakeSelfEffectSpec(
		UAbilitySystemComponent* AbilitySystemComponent,
		TSubclassOf<UGameplayEffect> GameplayEffectClass,
		UObject* SourceObject,
		float Level = 1.0f);
};

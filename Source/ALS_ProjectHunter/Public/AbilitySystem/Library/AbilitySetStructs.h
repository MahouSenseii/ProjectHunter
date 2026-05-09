// AbilitySystem/Library/AbilitySetStructs.h
// Structs used by UPHAbilitySet to grant abilities, effects, and attribute sets.
//
// Dependency chain:
//   PHAbilityEnumLibrary.h  →  AbilitySetStructs.h  →  (system headers)
//
// Include this alone when you need to pass or receive ability-set grant data
// (e.g. equipment, skill-tree) without pulling in the full UPHAbilitySet header.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "AttributeSet.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "AbilitySetStructs.generated.h"

class UGameplayEffect;
class UHunterAbilitySystemComponent;
class UPHGameplayAbility;

// ─────────────────────────────────────────────────────────────────────────────
// Grant entries — authored inside UPHAbilitySet data assets
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FPHAbilitySet_GameplayAbility
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	TSubclassOf<UPHGameplayAbility> Ability;

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	int32 AbilityLevel = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

USTRUCT(BlueprintType)
struct FPHAbilitySet_GameplayEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> GameplayEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float EffectLevel = 1.0f;
};

USTRUCT(BlueprintType)
struct FPHAbilitySet_AttributeSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Attributes")
	TSubclassOf<UAttributeSet> AttributeSet;
};

// ─────────────────────────────────────────────────────────────────────────────
// FPHAbilitySet_GrantedHandles — tracks everything granted by one ability set
// so it can be revoked cleanly (e.g. on equipment unequip).
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FPHAbilitySet_GrantedHandles
{
	GENERATED_BODY()

	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);
	void AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle);
	void AddAttributeSet(UAttributeSet* Set);
	void TakeFromAbilitySystem(UHunterAbilitySystemComponent* HunterASC);
	bool IsEmpty() const;

private:
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> GameplayEffectHandles;

	UPROPERTY()
	TArray<TObjectPtr<UAttributeSet>> GrantedAttributeSets;
};

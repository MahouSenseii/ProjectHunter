#pragma once

#include "CoreMinimal.h"
#include "Combat/Library/Structs/CombatStructs.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "Item/Library/Enums/AffixEnums.h"

struct ALS_PROJECTHUNTER_API FResolvedStatModifier
{
	EGameplayModOp::Type ModOp = EGameplayModOp::Additive;
	float Magnitude = 0.0f;
	bool bCreatesGameplayModifier = false;
};

/** Runtime context used to gate conditional modifiers from any source type. */
struct ALS_PROJECTHUNTER_API FStatModifierEvaluationContext
{
	FGameplayTagContainer SourceTags;
	FGameplayTagContainer TargetTags;
	bool bIsSkillHit = false;
	bool bIsDualWielding = false;
	bool bIsUnarmed = false;
	bool bHasShield = false;
	bool bIsMoving = false;
	float SourceHealthPercent = 1.f;
};

/** Accumulated modifier operations for one contextual attribute. */
struct ALS_PROJECTHUNTER_API FContextualAttributeModifier
{
	float Additive = 0.f;
	float Product = 1.f;
	float Override = 0.f;
	bool bHasOverride = false;

	void Accumulate(EModifyType ModifyType, float Value);
	float Resolve(float BaseValue) const;
};

/** Immutable per-evaluation result consumed by stateless calculators. */
struct ALS_PROJECTHUNTER_API FContextualStatModifierSnapshot
{
	TMap<FGameplayAttribute, FContextualAttributeModifier> ByAttribute;
	TArray<FCombatDamageConversionRule> DamageConversionRules;

	void Accumulate(const FGameplayAttribute& Attribute, EModifyType ModifyType, float Value);
	float Resolve(const FGameplayAttribute& Attribute, float BaseValue) const;
	bool IsEmpty() const { return ByAttribute.IsEmpty() && DamageConversionRules.IsEmpty(); }
};

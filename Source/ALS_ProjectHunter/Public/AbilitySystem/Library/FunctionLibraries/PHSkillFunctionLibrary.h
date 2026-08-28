#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Library/Structs/PHSkillStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Stats/Library/Structs/ResolvedItemStats.h"
#include "PHSkillFunctionLibrary.generated.h"

class UHunterAttributeSet;

/** Stateless authored-data to runtime-snapshot conversion. */
class ALS_PROJECTHUNTER_API FPHSkillDataResolver
{
public:
	static FPHResolvedSkillData Resolve(
		const FPHSkillData& SkillData,
		const FGameplayTagContainer& SkillTags,
		const UHunterAttributeSet* AttributeSet,
		const FResolvedWeaponStats* WeaponStats = nullptr);

	/** Adds category metadata without overwriting explicitly authored hit flags. */
	static void MergeSkillTagsIntoDamageInfo(
		FAnimationDamageInfo& DamageInfo,
		const FGameplayTagContainer& SkillTags);
};

/** Blueprint access for non-ability consumers such as projectiles and aura actors. */
UCLASS()
class ALS_PROJECTHUNTER_API UPHSkillFunctionLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Skill")
	static FPHResolvedSkillData ResolveSkillData(
		const FPHSkillData& SkillData,
		const FGameplayTagContainer& SkillTags,
		const UHunterAttributeSet* AttributeSet);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Skill")
	static FPHResolvedSkillData ResolveSkillDataWithWeapon(
		const FPHSkillData& SkillData,
		const FGameplayTagContainer& SkillTags,
		const UHunterAttributeSet* AttributeSet,
		const FResolvedWeaponStats& WeaponStats);
};

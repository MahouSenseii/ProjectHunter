#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Library/Enums/PHAbilityEnums.h"
#include "AbilitySystem/Library/Structs/PHSkillStructs.h"
#include "Stats/Library/Structs/ResolvedItemStats.h"
#include "PHGameplayAbility.generated.h"

class APHBaseCharacter;
class UHunterAbilitySystemComponent;

/**
 * Project Hunter gameplay ability base.
 */
UCLASS(Abstract, Blueprintable, HideCategories = Input)
class ALS_PROJECTHUNTER_API UPHGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

	friend class UHunterAbilitySystemComponent;

public:
	UPHGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Project Hunter|Ability")
	UHunterAbilitySystemComponent* GetHunterAbilitySystemComponentFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "Project Hunter|Ability")
	APHBaseCharacter* GetPHCharacterFromActorInfo() const;

	EPHAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }
	EPHAbilityActivationGroup GetActivationGroup() const { return ActivationGroup; }

	void TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const;

	UFUNCTION(BlueprintCallable, Category = "Project Hunter|Ability", meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool CanChangeActivationGroup(EPHAbilityActivationGroup NewGroup) const;

	UFUNCTION(BlueprintCallable, Category = "Project Hunter|Ability", meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool ChangeActivationGroup(EPHAbilityActivationGroup NewGroup);
	
	UFUNCTION(BlueprintCallable, Category = "Project Hunter|Base")
	FName GetAbilityName() const;

	UFUNCTION(BlueprintPure, Category = "Project Hunter|Skill")
	FPHSkillData GetSkillData() const { return SkillData; }

	/** Standard GAS asset tags are the single source of truth for skill keywords. */
	UFUNCTION(BlueprintPure, Category = "Project Hunter|Skill")
	FGameplayTagContainer GetSkillTags() const { return GetAssetTags(); }

	/** Resolve authored data against the current actor's global attributes. */
	UFUNCTION(BlueprintPure, Category = "Project Hunter|Skill")
	FPHResolvedSkillData ResolveSkillData() const;

	/** Resolve authored data against global attributes and one item-local weapon snapshot. */
	UFUNCTION(BlueprintPure, Category = "Project Hunter|Skill")
	FPHResolvedSkillData ResolveSkillDataWithWeapon(const FResolvedWeaponStats& WeaponStats) const;

protected:
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void SetCanBeCanceled(bool bCanBeCanceled) override;
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	virtual void OnPawnAvatarSet();

	UFUNCTION(BlueprintImplementableEvent, Category = "Project Hunter|Ability", DisplayName = "On Ability Added")
	void K2_OnAbilityAdded();

	UFUNCTION(BlueprintImplementableEvent, Category = "Project Hunter|Ability", DisplayName = "On Ability Removed")
	void K2_OnAbilityRemoved();

	UFUNCTION(BlueprintImplementableEvent, Category = "Project Hunter|Ability", DisplayName = "On Pawn Avatar Set")
	void K2_OnPawnAvatarSet();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Project Hunter|Ability Activation")
	EPHAbilityActivationPolicy ActivationPolicy = EPHAbilityActivationPolicy::OnInputTriggered;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Project Hunter|Ability Activation")
	EPHAbilityActivationGroup ActivationGroup = EPHAbilityActivationGroup::Independent;

	/** Authored by the existing data-only BP_GameplayAbility base and its children. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Project Hunter|Skill")
	FPHSkillData SkillData;
	
	/** Legacy serialized identifier. SkillData.SkillId takes precedence when set. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Project Hunter|Base")
	FName AbilityName;
};

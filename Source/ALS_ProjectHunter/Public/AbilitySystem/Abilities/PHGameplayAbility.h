#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Library/PHAbilityEnumLibrary.h"
#include "PHGameplayAbility.generated.h"

class APHBaseCharacter;
class UHunterAbilitySystemComponent;

/**
 * Project Hunter gameplay ability base.
 *
 * This mirrors Lyra's useful activation metadata while staying PH-specific.
 * ARPG costs, stat reads, damage math, and equipment integration should remain
 * in PH systems and attribute helpers instead of being copied from Lyra.
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
};

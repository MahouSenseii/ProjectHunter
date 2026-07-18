#include "AbilitySystem/PHAbilitySet.h"

#include "AbilitySystem/Abilities/PHGameplayAbility.h"
#include "AbilitySystem/HunterAbilitySystemComponent.h"
#include "AbilitySystem/Library/FunctionLibraries/PHAbilitySystemFunctionLibrary.h"

void UPHAbilitySet::GiveToAbilitySystem(UHunterAbilitySystemComponent* HunterASC, FPHAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject) const
{
	if (!HunterASC || !HunterASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	for (int32 SetIndex = 0; SetIndex < GrantedAttributes.Num(); ++SetIndex)
	{
		const FPHAbilitySet_AttributeSet& SetToGrant = GrantedAttributes[SetIndex];
		if (!IsValid(SetToGrant.AttributeSet))
		{
			UE_LOG(LogHunterGAS, Error, TEXT("GrantedAttributes[%d] on ability set [%s] is not valid."), SetIndex, *GetNameSafe(this));
			continue;
		}

		UAttributeSet* NewSet = NewObject<UAttributeSet>(HunterASC->GetOwner(), SetToGrant.AttributeSet);
		HunterASC->AddAttributeSetSubobject(NewSet);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAttributeSet(NewSet);
		}
	}

	for (int32 AbilityIndex = 0; AbilityIndex < GrantedGameplayAbilities.Num(); ++AbilityIndex)
	{
		const FPHAbilitySet_GameplayAbility& AbilityToGrant = GrantedGameplayAbilities[AbilityIndex];
		if (!IsValid(AbilityToGrant.Ability))
		{
			UE_LOG(LogHunterGAS, Error, TEXT("GrantedGameplayAbilities[%d] on ability set [%s] is not valid."), AbilityIndex, *GetNameSafe(this));
			continue;
		}

		UPHGameplayAbility* AbilityCDO = AbilityToGrant.Ability->GetDefaultObject<UPHGameplayAbility>();
		FGameplayAbilitySpec AbilitySpec(AbilityCDO, AbilityToGrant.AbilityLevel);
		AbilitySpec.SourceObject = SourceObject;
		if (AbilityToGrant.InputTag.IsValid())
		{
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilityToGrant.InputTag);
		}

		const FGameplayAbilitySpecHandle AbilitySpecHandle = HunterASC->GiveAbility(AbilitySpec);
		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAbilitySpecHandle(AbilitySpecHandle);
		}
	}

	for (int32 EffectIndex = 0; EffectIndex < GrantedGameplayEffects.Num(); ++EffectIndex)
	{
		const FPHAbilitySet_GameplayEffect& EffectToGrant = GrantedGameplayEffects[EffectIndex];
		if (!IsValid(EffectToGrant.GameplayEffect))
		{
			UE_LOG(LogHunterGAS, Error, TEXT("GrantedGameplayEffects[%d] on ability set [%s] is not valid."), EffectIndex, *GetNameSafe(this));
			continue;
		}

		const FGameplayEffectSpecHandle GameplayEffectSpec =
			UPHAbilitySystemFunctionLibrary::MakeSelfEffectSpec(
				HunterASC,
				EffectToGrant.GameplayEffect,
				nullptr,
				EffectToGrant.EffectLevel);
		if (!GameplayEffectSpec.IsValid())
		{
			continue;
		}

		const FActiveGameplayEffectHandle GameplayEffectHandle =
			HunterASC->ApplyGameplayEffectSpecToSelf(*GameplayEffectSpec.Data.Get());

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddGameplayEffectHandle(GameplayEffectHandle);
		}
	}
}

#include "AbilitySystem/Library/FunctionLibraries/PHAbilitySystemFunctionLibrary.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

bool UPHAbilitySystemFunctionLibrary::ShouldActivateOnInputPressed(const EPHAbilityActivationPolicy ActivationPolicy)
{
	return ActivationPolicy == EPHAbilityActivationPolicy::OnInputTriggered;
}

bool UPHAbilitySystemFunctionLibrary::ShouldActivateWhileInputHeld(const EPHAbilityActivationPolicy ActivationPolicy)
{
	return ActivationPolicy == EPHAbilityActivationPolicy::WhileInputActive;
}

bool UPHAbilitySystemFunctionLibrary::ShouldActivateOnSpawn(const EPHAbilityActivationPolicy ActivationPolicy)
{
	return ActivationPolicy == EPHAbilityActivationPolicy::OnSpawn;
}

bool UPHAbilitySystemFunctionLibrary::IsInputActivatedPolicy(const EPHAbilityActivationPolicy ActivationPolicy)
{
	return ShouldActivateOnInputPressed(ActivationPolicy) || ShouldActivateWhileInputHeld(ActivationPolicy);
}

bool UPHAbilitySystemFunctionLibrary::IsIndependentActivationGroup(const EPHAbilityActivationGroup ActivationGroup)
{
	return ActivationGroup == EPHAbilityActivationGroup::Independent;
}

bool UPHAbilitySystemFunctionLibrary::IsReplaceableActivationGroup(const EPHAbilityActivationGroup ActivationGroup)
{
	return ActivationGroup == EPHAbilityActivationGroup::Exclusive_Replaceable;
}

bool UPHAbilitySystemFunctionLibrary::IsBlockingActivationGroup(const EPHAbilityActivationGroup ActivationGroup)
{
	return ActivationGroup == EPHAbilityActivationGroup::Exclusive_Blocking;
}

bool UPHAbilitySystemFunctionLibrary::IsExclusiveActivationGroup(const EPHAbilityActivationGroup ActivationGroup)
{
	return IsReplaceableActivationGroup(ActivationGroup) || IsBlockingActivationGroup(ActivationGroup);
}

bool UPHAbilitySystemFunctionLibrary::IsValidActivationGroup(const EPHAbilityActivationGroup ActivationGroup)
{
	return IsIndependentActivationGroup(ActivationGroup) || IsExclusiveActivationGroup(ActivationGroup);
}

bool UPHAbilitySystemFunctionLibrary::IsGameplayEffectClassCompatible(
	const TSubclassOf<UGameplayEffect> ConfiguredClass,
	const TSubclassOf<UGameplayEffect> RequiredParentClass)
{
	if (!ConfiguredClass || !RequiredParentClass)
	{
		return false;
	}

	return ConfiguredClass.Get()->IsChildOf(RequiredParentClass.Get());
}

TSubclassOf<UGameplayEffect> UPHAbilitySystemFunctionLibrary::ResolveGameplayEffectClass(
	const TSubclassOf<UGameplayEffect> ConfiguredClass,
	const TSubclassOf<UGameplayEffect> NativeClass)
{
	if (!NativeClass)
	{
		return ConfiguredClass;
	}

	return IsGameplayEffectClassCompatible(ConfiguredClass, NativeClass)
		? ConfiguredClass
		: NativeClass;
}

FGameplayEffectSpecHandle UPHAbilitySystemFunctionLibrary::MakeSelfEffectSpec(
	UAbilitySystemComponent* AbilitySystemComponent,
	const TSubclassOf<UGameplayEffect> GameplayEffectClass,
	UObject* SourceObject,
	const float Level)
{
	if (!AbilitySystemComponent || !GameplayEffectClass)
	{
		return FGameplayEffectSpecHandle();
	}

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	if (SourceObject)
	{
		Context.AddSourceObject(SourceObject);
	}

	return AbilitySystemComponent->MakeOutgoingSpec(GameplayEffectClass, Level, Context);
}

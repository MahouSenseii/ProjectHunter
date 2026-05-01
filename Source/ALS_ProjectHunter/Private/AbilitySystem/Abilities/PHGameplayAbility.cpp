#include "AbilitySystem/Abilities/PHGameplayAbility.h"

#include "AbilitySystem/HunterAbilitySystemComponent.h"
#include "Character/PHBaseCharacter.h"

#include "AbilitySystemLog.h"

#define ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(FunctionName, ReturnValue) \
{ \
	if (!ensure(IsInstantiated())) \
	{ \
		ABILITY_LOG(Error, TEXT("%s: " #FunctionName " cannot be called on a non-instanced ability. Check the instancing policy."), *GetPathName()); \
		return ReturnValue; \
	} \
}

UPHGameplayAbility::UPHGameplayAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	ActivationPolicy = EPHAbilityActivationPolicy::OnInputTriggered;
	ActivationGroup = EPHAbilityActivationGroup::Independent;
}

UHunterAbilitySystemComponent* UPHGameplayAbility::GetHunterAbilitySystemComponentFromActorInfo() const
{
	return CurrentActorInfo ? Cast<UHunterAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr;
}

APHBaseCharacter* UPHGameplayAbility::GetPHCharacterFromActorInfo() const
{
	return CurrentActorInfo ? Cast<APHBaseCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
}

bool UPHGameplayAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return false;
	}

	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UHunterAbilitySystemComponent* HunterASC = Cast<UHunterAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
	if (HunterASC && HunterASC->IsActivationGroupBlocked(ActivationGroup))
	{
		return false;
	}

	return true;
}

void UPHGameplayAbility::SetCanBeCanceled(bool bCanBeCanceled)
{
	if (!bCanBeCanceled && ActivationGroup == EPHAbilityActivationGroup::Exclusive_Replaceable)
	{
		UE_LOG(LogHunterGAS, Error, TEXT("SetCanBeCanceled: Ability [%s] cannot block canceling while replaceable."), *GetName());
		return;
	}

	Super::SetCanBeCanceled(bCanBeCanceled);
}

void UPHGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	K2_OnAbilityAdded();
	TryActivateAbilityOnSpawn(ActorInfo, Spec);
}

void UPHGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	K2_OnAbilityRemoved();

	Super::OnRemoveAbility(ActorInfo, Spec);
}

void UPHGameplayAbility::OnPawnAvatarSet()
{
	K2_OnPawnAvatarSet();
}

void UPHGameplayAbility::TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const
{
	if (!ActorInfo || Spec.IsActive() || ActivationPolicy != EPHAbilityActivationPolicy::OnSpawn)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	const AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!ASC || !AvatarActor || AvatarActor->GetTearOff() || AvatarActor->GetLifeSpan() > 0.0f)
	{
		return;
	}

	const bool bIsLocalExecution =
		NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalPredicted ||
		NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalOnly;
	const bool bIsServerExecution =
		NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly ||
		NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	const bool bClientShouldActivate = ActorInfo->IsLocallyControlled() && bIsLocalExecution;
	const bool bServerShouldActivate = ActorInfo->IsNetAuthority() && bIsServerExecution;
	if (bClientShouldActivate || bServerShouldActivate)
	{
		ASC->TryActivateAbility(Spec.Handle);
	}
}

bool UPHGameplayAbility::CanChangeActivationGroup(EPHAbilityActivationGroup NewGroup) const
{
	if (!IsInstantiated() || !IsActive())
	{
		return false;
	}

	if (ActivationGroup == NewGroup)
	{
		return true;
	}

	UHunterAbilitySystemComponent* HunterASC = GetHunterAbilitySystemComponentFromActorInfo();
	if (!HunterASC)
	{
		return false;
	}

	if (ActivationGroup != EPHAbilityActivationGroup::Exclusive_Blocking && HunterASC->IsActivationGroupBlocked(NewGroup))
	{
		return false;
	}

	if (NewGroup == EPHAbilityActivationGroup::Exclusive_Replaceable && !CanBeCanceled())
	{
		return false;
	}

	return true;
}

bool UPHGameplayAbility::ChangeActivationGroup(EPHAbilityActivationGroup NewGroup)
{
	ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(ChangeActivationGroup, false);

	if (!CanChangeActivationGroup(NewGroup))
	{
		return false;
	}

	if (ActivationGroup != NewGroup)
	{
		UHunterAbilitySystemComponent* HunterASC = GetHunterAbilitySystemComponentFromActorInfo();
		if (!HunterASC)
		{
			return false;
		}

		HunterASC->RemoveAbilityFromActivationGroup(ActivationGroup, this);
		HunterASC->AddAbilityToActivationGroup(NewGroup, this);
		ActivationGroup = NewGroup;
	}

	return true;
}

#undef ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN

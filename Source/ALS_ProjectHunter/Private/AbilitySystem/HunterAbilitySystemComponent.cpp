#include "AbilitySystem/HunterAbilitySystemComponent.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/Effects/HunterGE_HealthRegen.h"
#include "AbilitySystem/Effects/HunterGE_ManaRegen.h"
#include "AbilitySystem/Effects/HunterGE_StaminaDegen.h"
#include "AbilitySystem/Effects/HunterGE_StaminaRegen.h"
#include "AbilitySystem/Effects/HunterGE_ArcaneShieldRegen.h"
#include "Tags/Components/TagManager.h"
#include "Character/PHBaseCharacter.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PHGameplayTags.h"

namespace HunterAbilitySystemComponentPrivate
{
	UTagManager* ResolveTagManager(const UAbilitySystemComponent* ASC)
	{
		if (!ASC)
		{
			return nullptr;
		}

		if (const APHBaseCharacter* HunterCharacter = Cast<APHBaseCharacter>(ASC->GetAvatarActor()))
		{
			return HunterCharacter->GetTagManager();
		}

		if (const APHBaseCharacter* HunterCharacter = Cast<APHBaseCharacter>(ASC->GetOwner()))
		{
			return HunterCharacter->GetTagManager();
		}

		return nullptr;
	}

	void ForceStopSprinting(UHunterAbilitySystemComponent* ASC)
	{
		if (!ASC)
		{
			return;
		}

		const FPHGameplayTags& Tags = FPHGameplayTags::Get();
		if (APHBaseCharacter* HunterCharacter = Cast<APHBaseCharacter>(ASC->GetAvatarActor()))
		{
			HunterCharacter->SetDesiredGait(EALSGait::Running);
			HunterCharacter->bWallTraversalHeld = false;
			HunterCharacter->StopWallTraversal();
		}

		if (UTagManager* TagManager = ResolveTagManager(ASC))
		{
			TagManager->SetTagState(Tags.Condition_Sprinting, false);
			TagManager->RefreshBaseConditionTags();
		}
	}

	TSubclassOf<UGameplayEffect> ResolveNativeGameplayEffectClass(
		const TSubclassOf<UGameplayEffect>& ConfiguredClass,
		TSubclassOf<UGameplayEffect> NativeClass,
		const TCHAR* SlotName,
		bool& bWarned)
	{
		if (ConfiguredClass && ConfiguredClass.Get()->IsChildOf(NativeClass.Get()))
		{
			return ConfiguredClass;
		}

		if (ConfiguredClass && !bWarned)
		{
			bWarned = true;
			UE_LOG(
				LogHunterGAS,
				Warning,
				TEXT("%s=%s is not a child of %s. Using native class so stale SetByCaller GE assets cannot block runtime resource updates."),
				SlotName,
				*GetNameSafe(ConfiguredClass.Get()),
				*GetNameSafe(NativeClass.Get()));
		}

		return NativeClass;
	}
}

DEFINE_LOG_CATEGORY(LogHunterGAS);

#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<int32> CVarDebugEffects(
	TEXT("Hunter.Debug.Effects"),
	0,
	TEXT("Debug gameplay effect applications\n")
	TEXT("0: Disabled (default)\n")
	TEXT("1: Show on-screen messages\n")
	TEXT("2: Show on-screen + log to console"),
	ECVF_Cheat
);

static TAutoConsoleVariable<float> CVarDebugEffectsDuration(
	TEXT("Hunter.Debug.EffectsDuration"),
	3.0f,
	TEXT("Duration in seconds for effect debug messages (default: 3.0)"),
	ECVF_Cheat
);
#endif

UHunterAbilitySystemComponent::UHunterAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
	ReplicationMode = EGameplayEffectReplicationMode::Mixed;
	HealthRegenGE       = UHunterGE_HealthRegen::StaticClass();
	ManaRegenGE         = UHunterGE_ManaRegen::StaticClass();
	StaminaRegenGE      = UHunterGE_StaminaRegen::StaticClass();
	ArcaneShieldRegenGE = UHunterGE_ArcaneShieldRegen::StaticClass();
	SprintStaminaDrainGE = UHunterGE_StaminaDegen::StaticClass();

	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
	FMemory::Memzero(ActivationGroupCounts, sizeof(ActivationGroupCounts));
}

void UHunterAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	const AActor* PreviousAvatar = AbilityActorInfo.IsValid() ? AbilityActorInfo->AvatarActor.Get() : nullptr;

	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	const bool bHasNewAvatar = InAvatarActor && InAvatarActor != PreviousAvatar;
	if (bHasNewAvatar)
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
			ensureMsgf(
				!AbilitySpec.Ability || AbilitySpec.Ability->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced,
				TEXT("InitAbilityActorInfo: PH abilities should be instanced. NonInstanced abilities cannot receive avatar lifecycle events."));
PRAGMA_ENABLE_DEPRECATION_WARNINGS

			TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
			for (UGameplayAbility* AbilityInstance : Instances)
			{
				if (UPHGameplayAbility* PHAbilityInstance = Cast<UPHGameplayAbility>(AbilityInstance))
				{
					PHAbilityInstance->OnPawnAvatarSet();
				}
			}
		}

		TryActivateAbilitiesOnSpawn();
	}
}

void UHunterAbilitySystemComponent::TryActivateAbilitiesOnSpawn()
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (const UPHGameplayAbility* PHAbilityCDO = Cast<UPHGameplayAbility>(AbilitySpec.Ability))
		{
			PHAbilityCDO->TryActivateAbilityOnSpawn(AbilityActorInfo.Get(), AbilitySpec);
		}
	}
}

void UHunterAbilitySystemComponent::CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc, bool bReplicateCancelAbility)
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (!AbilitySpec.IsActive())
		{
			continue;
		}

		const UPHGameplayAbility* PHAbilityCDO = Cast<UPHGameplayAbility>(AbilitySpec.Ability);
		if (!PHAbilityCDO)
		{
			continue;
		}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
		ensureMsgf(
			AbilitySpec.Ability->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced,
			TEXT("CancelAbilitiesByFunc: PH abilities should be instanced."));
PRAGMA_ENABLE_DEPRECATION_WARNINGS

		TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
		for (UGameplayAbility* AbilityInstance : Instances)
		{
			UPHGameplayAbility* PHAbilityInstance = Cast<UPHGameplayAbility>(AbilityInstance);
			if (!PHAbilityInstance)
			{
				continue;
			}

			if (ShouldCancelFunc(PHAbilityInstance, AbilitySpec.Handle))
			{
				if (PHAbilityInstance->CanBeCanceled())
				{
					PHAbilityInstance->CancelAbility(
						AbilitySpec.Handle,
						AbilityActorInfo.Get(),
						PHAbilityInstance->GetCurrentActivationInfo(),
						bReplicateCancelAbility);
				}
				else
				{
					UE_LOG(LogHunterGAS, Error, TEXT("CancelAbilitiesByFunc: Cannot cancel ability [%s] because CanBeCanceled is false."), *PHAbilityInstance->GetName());
				}
			}
		}
	}
}

void UHunterAbilitySystemComponent::CancelInputActivatedAbilities(bool bReplicateCancelAbility)
{
	auto ShouldCancelFunc = [](const UPHGameplayAbility* PHAbility, FGameplayAbilitySpecHandle Handle)
	{
		(void)Handle;
		const EPHAbilityActivationPolicy ActivationPolicy = PHAbility->GetActivationPolicy();
		return ActivationPolicy == EPHAbilityActivationPolicy::OnInputTriggered ||
			ActivationPolicy == EPHAbilityActivationPolicy::WhileInputActive;
	};

	CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

void UHunterAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	if (Spec.IsActive())
	{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		const FPredictionKey OriginalPredictionKey = Instance
			? Instance->GetCurrentActivationInfo().GetActivationPredictionKey()
			: Spec.ActivationInfo.GetActivationPredictionKey();
PRAGMA_ENABLE_DEPRECATION_WARNINGS

		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, OriginalPredictionKey);
	}
}

void UHunterAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	if (Spec.IsActive())
	{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		const FPredictionKey OriginalPredictionKey = Instance
			? Instance->GetCurrentActivationInfo().GetActivationPredictionKey()
			: Spec.ActivationInfo.GetActivationPredictionKey();
PRAGMA_ENABLE_DEPRECATION_WARNINGS

		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, OriginalPredictionKey);
	}
}

void UHunterAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (AbilitySpec.Ability && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
			InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
		}
	}
}

void UHunterAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (AbilitySpec.Ability && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
			InputHeldSpecHandles.Remove(AbilitySpec.Handle);
		}
	}
}

void UHunterAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	(void)DeltaTime;
	(void)bGamePaused;

	const FGameplayTag InputBlockedTag = FGameplayTag::RequestGameplayTag(TEXT("Gameplay.AbilityInputBlocked"), false);
	if (InputBlockedTag.IsValid() && HasMatchingGameplayTag(InputBlockedTag))
	{
		ClearAbilityInput();
		return;
	}

	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability && !AbilitySpec->IsActive())
			{
				const UPHGameplayAbility* PHAbilityCDO = Cast<UPHGameplayAbility>(AbilitySpec->Ability);
				if (PHAbilityCDO && PHAbilityCDO->GetActivationPolicy() == EPHAbilityActivationPolicy::WhileInputActive)
				{
					AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
				}
			}
		}
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = true;

				if (AbilitySpec->IsActive())
				{
					AbilitySpecInputPressed(*AbilitySpec);
				}
				else
				{
					const UPHGameplayAbility* PHAbilityCDO = Cast<UPHGameplayAbility>(AbilitySpec->Ability);
					if (PHAbilityCDO && PHAbilityCDO->GetActivationPolicy() == EPHAbilityActivationPolicy::OnInputTriggered)
					{
						AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
					}
				}
			}
		}
	}

	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(AbilitySpecHandle);
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = false;

				if (AbilitySpec->IsActive())
				{
					AbilitySpecInputReleased(*AbilitySpec);
				}
			}
		}
	}

	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UHunterAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

void UHunterAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability)
{
	Super::NotifyAbilityActivated(Handle, Ability);

	if (UPHGameplayAbility* PHAbility = Cast<UPHGameplayAbility>(Ability))
	{
		AddAbilityToActivationGroup(PHAbility->GetActivationGroup(), PHAbility);
	}
}

void UHunterAbilitySystemComponent::NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled)
{
	Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);

	if (UPHGameplayAbility* PHAbility = Cast<UPHGameplayAbility>(Ability))
	{
		RemoveAbilityFromActivationGroup(PHAbility->GetActivationGroup(), PHAbility);
	}
}

bool UHunterAbilitySystemComponent::IsActivationGroupBlocked(EPHAbilityActivationGroup Group) const
{
	switch (Group)
	{
	case EPHAbilityActivationGroup::Independent:
		return false;

	case EPHAbilityActivationGroup::Exclusive_Replaceable:
	case EPHAbilityActivationGroup::Exclusive_Blocking:
		return ActivationGroupCounts[static_cast<uint8>(EPHAbilityActivationGroup::Exclusive_Blocking)] > 0;

	default:
		checkf(false, TEXT("IsActivationGroupBlocked: Invalid ActivationGroup [%d]."), static_cast<uint8>(Group));
		return false;
	}
}

void UHunterAbilitySystemComponent::AddAbilityToActivationGroup(EPHAbilityActivationGroup Group, UPHGameplayAbility* PHAbility)
{
	check(PHAbility);
	check(ActivationGroupCounts[static_cast<uint8>(Group)] < INT32_MAX);

	ActivationGroupCounts[static_cast<uint8>(Group)]++;

	const bool bReplicateCancelAbility = false;
	switch (Group)
	{
	case EPHAbilityActivationGroup::Independent:
		break;

	case EPHAbilityActivationGroup::Exclusive_Replaceable:
	case EPHAbilityActivationGroup::Exclusive_Blocking:
		CancelActivationGroupAbilities(EPHAbilityActivationGroup::Exclusive_Replaceable, PHAbility, bReplicateCancelAbility);
		break;

	default:
		checkf(false, TEXT("AddAbilityToActivationGroup: Invalid ActivationGroup [%d]."), static_cast<uint8>(Group));
		break;
	}

	const int32 ExclusiveCount =
		ActivationGroupCounts[static_cast<uint8>(EPHAbilityActivationGroup::Exclusive_Replaceable)] +
		ActivationGroupCounts[static_cast<uint8>(EPHAbilityActivationGroup::Exclusive_Blocking)];
	if (!ensure(ExclusiveCount <= 1))
	{
		UE_LOG(LogHunterGAS, Error, TEXT("AddAbilityToActivationGroup: Multiple exclusive PH abilities are running."));
	}
}

void UHunterAbilitySystemComponent::RemoveAbilityFromActivationGroup(EPHAbilityActivationGroup Group, UPHGameplayAbility* PHAbility)
{
	check(PHAbility);

	int32& GroupCount = ActivationGroupCounts[static_cast<uint8>(Group)];
	if (ensure(GroupCount > 0))
	{
		GroupCount--;
	}
}

void UHunterAbilitySystemComponent::CancelActivationGroupAbilities(EPHAbilityActivationGroup Group, UPHGameplayAbility* IgnorePHAbility, bool bReplicateCancelAbility)
{
	auto ShouldCancelFunc = [Group, IgnorePHAbility](const UPHGameplayAbility* PHAbility, FGameplayAbilitySpecHandle Handle)
	{
		(void)Handle;
		return PHAbility && PHAbility->GetActivationGroup() == Group && PHAbility != IgnorePHAbility;
	};

	CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

void UHunterAbilitySystemComponent::AbilityActorInfoSet()
{
	if (!bEffectAppliedDelegateBound)
	{
		OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UHunterAbilitySystemComponent::EffectApplied);
		bEffectAppliedDelegateBound = true;
	}

	if (!bSprintingTagDelegateBound)
	{
		const FGameplayTag SprintingTag = FPHGameplayTags::Get().Condition_Sprinting;
		RegisterGameplayTagEvent(SprintingTag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UHunterAbilitySystemComponent::HandleSprintingTagChanged);
		bSprintingTagDelegateBound = true;
		HandleSprintingTagChanged(SprintingTag, GetTagCount(SprintingTag));
	}

	if (!bStaminaExhaustedTagDelegateBound)
	{
		const FGameplayTag StaminaExhaustedTag = FPHGameplayTags::Get().Effect_Stamina_Exhausted;
		RegisterGameplayTagEvent(StaminaExhaustedTag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UHunterAbilitySystemComponent::HandleStaminaExhaustedTagChanged);
		bStaminaExhaustedTagDelegateBound = true;
		HandleStaminaExhaustedTagChanged(StaminaExhaustedTag, GetTagCount(StaminaExhaustedTag));
	}

	StartPassiveRegen();

	UE_LOG(
		LogHunterGAS,
		Verbose,
		TEXT("AbilityActorInfoSet: Initialized runtime delegates for ASC=%s Owner=%s Avatar=%s"),
		*GetName(),
		*GetNameSafe(GetOwner()),
		AbilityActorInfo.IsValid() ? *GetNameSafe(AbilityActorInfo->AvatarActor.Get()) : TEXT("None"));
}

void UHunterAbilitySystemComponent::EffectApplied(UAbilitySystemComponent* AbilitySystemComponent,
	const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);
	EffectAssetTags.Broadcast(TagContainer);

#if !UE_BUILD_SHIPPING
	if (CVarDebugEffects.GetValueOnGameThread() > 0)
	{
		ShowEffectDebug(EffectSpec, TagContainer);
	}
#endif
}

void UHunterAbilitySystemComponent::HandleSprintingTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	(void)CallbackTag;

	const AActor* AvatarActorInstance = GetAvatarActor();
	if (!AvatarActorInstance || !AvatarActorInstance->HasAuthority())
	{
		return;
	}

	if (NewCount > 0)
	{
		bSprintStaminaDegenRequested = true;
	}
	else
	{
		bSprintStaminaDegenRequested = false;
	}

	RefreshStaminaDegenEffect();
}

void UHunterAbilitySystemComponent::HandleStaminaExhaustedTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	(void)CallbackTag;

	if (!bShouldCheckExhaustion)
	{
		ClearStaminaExhaustionRecoveryTimer();
		RefreshStaminaDegenEffect();
		return;
	}

	if (NewCount > 0)
	{
		bSprintStaminaDegenRequested = false;
		bWallRunningStaminaDegenRequested = false;
		HunterAbilitySystemComponentPrivate::ForceStopSprinting(this);
		StopSprintStaminaDegen();
		RefreshStaminaExhaustionRecovery();
		return;
	}

	ClearStaminaExhaustionRecoveryTimer();
	ActiveStaminaExhaustionHandle.Invalidate();
	RefreshStaminaDegenEffect();
}

void UHunterAbilitySystemComponent::NotifyStaminaMovementInputChanged()
{
	RefreshStaminaExhaustionRecovery();
	RefreshStaminaDegenEffect();
}

void UHunterAbilitySystemComponent::SetWallRunningStaminaDegenActive(const bool bActive)
{
	bWallRunningStaminaDegenRequested = bActive;
	RefreshStaminaExhaustionRecovery();
	RefreshStaminaDegenEffect();
}

void UHunterAbilitySystemComponent::RefreshStaminaDegenEffect()
{
	const AActor* AvatarActorInstance = GetAvatarActor();
	if (!AvatarActorInstance || !AvatarActorInstance->HasAuthority())
	{
		return;
	}

	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	const bool bCanSprintDegen = bSprintStaminaDegenRequested && !IsAvatarAirborneForStamina();
	const bool bShouldDegen = (bCanSprintDegen || bWallRunningStaminaDegenRequested)
		&& (!bShouldCheckExhaustion || !HasMatchingGameplayTag(Tags.Effect_Stamina_Exhausted));

	if (bShouldDegen)
	{
		StartSprintStaminaDegen();
	}
	else
	{
		StopSprintStaminaDegen();
	}
}

void UHunterAbilitySystemComponent::StartSprintStaminaDegen()
{
	const AActor* AvatarActorInstance = GetAvatarActor();
	if (!AvatarActorInstance || !AvatarActorInstance->HasAuthority() || ActiveSprintStaminaDrainHandle.IsValid())
	{
		return;
	}

	const UHunterAttributeSet* AttributeSet = GetHunterAttributeSet();
	if (!AttributeSet)
	{
		return;
	}

	if (IsAvatarAirborneForStamina() && !bWallRunningStaminaDegenRequested)
	{
		return;
	}

	if (bShouldCheckExhaustion && AttributeSet->GetStamina() <= KINDA_SMALL_NUMBER)
	{
		HandleStaminaDepleted();
		return;
	}

	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	if (bShouldCheckExhaustion && HasMatchingGameplayTag(Tags.Effect_Stamina_Exhausted))
	{
		RefreshStaminaExhaustionRecovery();
		return;
	}

	TSubclassOf<UGameplayEffect> SprintDrainClass = UHunterGE_StaminaDegen::StaticClass();
	if (SprintStaminaDrainGE && SprintStaminaDrainGE.Get()->IsChildOf(UHunterGE_StaminaDegen::StaticClass()))
	{
		SprintDrainClass = SprintStaminaDrainGE;
	}
	else if (SprintStaminaDrainGE && !bWarnedNonNativeSprintDrainGE)
	{
		bWarnedNonNativeSprintDrainGE = true;
		UE_LOG(
			LogHunterGAS,
			Warning,
			TEXT("StartSprintStaminaDegen: SprintStaminaDrainGE=%s is not a child of UHunterGE_StaminaDegen. Using native drain GE so copied regen assets cannot block stamina drain."),
			*GetNameSafe(SprintStaminaDrainGE.Get()));
	}

	FGameplayEffectContextHandle Context = MakeEffectContext();
	Context.AddSourceObject(GetOwner());

	const FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(SprintDrainClass, 1.f, Context);
	if (Spec.IsValid())
	{
		ActiveSprintStaminaDrainHandle = ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		if (ActiveSprintStaminaDrainHandle.IsValid())
		{
			AddLooseGameplayTag(Tags.Effect_Stamina_DegenActive);
		}
	}
}

void UHunterAbilitySystemComponent::StopSprintStaminaDegen()
{
	if (ActiveSprintStaminaDrainHandle.IsValid())
	{
		RemoveActiveGameplayEffect(ActiveSprintStaminaDrainHandle);
		ActiveSprintStaminaDrainHandle.Invalidate();
	}

	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	if (GetTagCount(Tags.Effect_Stamina_DegenActive) > 0)
	{
		RemoveLooseGameplayTag(Tags.Effect_Stamina_DegenActive);
	}
}

bool UHunterAbilitySystemComponent::IsStaminaMovementInputHeldForRecovery() const
{
	if (const APHBaseCharacter* HunterCharacter = Cast<APHBaseCharacter>(GetAvatarActor()))
	{
		return HunterCharacter->IsStaminaMovementInputHeld();
	}

	if (const APHBaseCharacter* HunterCharacter = Cast<APHBaseCharacter>(GetOwner()))
	{
		return HunterCharacter->IsStaminaMovementInputHeld();
	}

	return bSprintStaminaDegenRequested || bWallRunningStaminaDegenRequested;
}

bool UHunterAbilitySystemComponent::IsAvatarAirborneForStamina() const
{
	const APHBaseCharacter* HunterCharacter = Cast<APHBaseCharacter>(GetAvatarActor());
	if (!HunterCharacter)
	{
		HunterCharacter = Cast<APHBaseCharacter>(GetOwner());
	}

	if (!HunterCharacter)
	{
		return false;
	}

	const UCharacterMovementComponent* Movement = HunterCharacter->GetCharacterMovement();
	if (Movement && Movement->IsFalling())
	{
		return true;
	}

	return HunterCharacter->GetMovementState() == EALSMovementState::InAir;
}

void UHunterAbilitySystemComponent::RefreshStaminaExhaustionRecovery()
{
	const AActor* AvatarActorInstance = GetAvatarActor();
	if (!AvatarActorInstance || !AvatarActorInstance->HasAuthority())
	{
		return;
	}

	if (!bShouldCheckExhaustion ||
		!HasMatchingGameplayTag(FPHGameplayTags::Get().Effect_Stamina_Exhausted) ||
		IsStaminaMovementInputHeldForRecovery() ||
		IsAvatarAirborneForStamina())
	{
		ClearStaminaExhaustionRecoveryTimer();
		return;
	}

	if (StaminaExhaustionRecoveryDelay <= 0.f)
	{
		CompleteStaminaExhaustionRecovery();
		return;
	}

	UWorld* World = GetWorld();
	if (!World || World->GetTimerManager().IsTimerActive(StaminaExhaustionRecoveryTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		StaminaExhaustionRecoveryTimerHandle,
		this,
		&UHunterAbilitySystemComponent::CompleteStaminaExhaustionRecovery,
		StaminaExhaustionRecoveryDelay,
		false);
}

void UHunterAbilitySystemComponent::CompleteStaminaExhaustionRecovery()
{
	ClearStaminaExhaustionRecoveryTimer();

	const AActor* AvatarActorInstance = GetAvatarActor();
	if (!AvatarActorInstance || !AvatarActorInstance->HasAuthority())
	{
		return;
	}

	if (!bShouldCheckExhaustion ||
		IsStaminaMovementInputHeldForRecovery() ||
		IsAvatarAirborneForStamina())
	{
		return;
	}

	RemoveStaminaExhaustionEffect();
	if (APHBaseCharacter* HunterCharacter = Cast<APHBaseCharacter>(GetAvatarActor()))
	{
		HunterCharacter->RefreshStaminaMovementInput();
	}
}

void UHunterAbilitySystemComponent::ClearStaminaExhaustionRecoveryTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StaminaExhaustionRecoveryTimerHandle);
	}
}

void UHunterAbilitySystemComponent::RemoveStaminaExhaustionEffect()
{
	const FGameplayTag StaminaExhaustedTag = FPHGameplayTags::Get().Effect_Stamina_Exhausted;
	if (ActiveStaminaExhaustionHandle.IsValid())
	{
		RemoveActiveGameplayEffect(ActiveStaminaExhaustionHandle);
		ActiveStaminaExhaustionHandle.Invalidate();
	}

	if (HasMatchingGameplayTag(StaminaExhaustedTag))
	{
		FGameplayTagContainer ExhaustedTags;
		ExhaustedTags.AddTag(StaminaExhaustedTag);
		RemoveActiveEffectsWithGrantedTags(ExhaustedTags);
	}

	SetLooseGameplayTagCount(StaminaExhaustedTag, 0);

	RefreshStaminaDegenEffect();
}

void UHunterAbilitySystemComponent::StartPassiveRegen()
{
	if (bPassiveRegenStarted)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	const FPHGameplayTags& PHT = FPHGameplayTags::Get();
	AddLooseGameplayTag(PHT.Effect_Health_RegenActive);
	AddLooseGameplayTag(PHT.Effect_Mana_RegenActive);
	AddLooseGameplayTag(PHT.Effect_Stamina_RegenActive);
	AddLooseGameplayTag(PHT.Effect_ArcaneShield_RegenActive);

	const TSubclassOf<UGameplayEffect> HealthRegenClass =
		HunterAbilitySystemComponentPrivate::ResolveNativeGameplayEffectClass(
			HealthRegenGE,
			UHunterGE_HealthRegen::StaticClass(),
			TEXT("HealthRegenGE"),
			bWarnedNonNativeHealthRegenGE);
	const TSubclassOf<UGameplayEffect> ManaRegenClass =
		HunterAbilitySystemComponentPrivate::ResolveNativeGameplayEffectClass(
			ManaRegenGE,
			UHunterGE_ManaRegen::StaticClass(),
			TEXT("ManaRegenGE"),
			bWarnedNonNativeManaRegenGE);
	const TSubclassOf<UGameplayEffect> StaminaRegenClass =
		HunterAbilitySystemComponentPrivate::ResolveNativeGameplayEffectClass(
			StaminaRegenGE,
			UHunterGE_StaminaRegen::StaticClass(),
			TEXT("StaminaRegenGE"),
			bWarnedNonNativeStaminaRegenGE);
	const TSubclassOf<UGameplayEffect> ArcaneShieldRegenClass =
		HunterAbilitySystemComponentPrivate::ResolveNativeGameplayEffectClass(
			ArcaneShieldRegenGE,
			UHunterGE_ArcaneShieldRegen::StaticClass(),
			TEXT("ArcaneShieldRegenGE"),
			bWarnedNonNativeArcaneShieldRegenGE);

	auto MakeSpec = [this](TSubclassOf<UGameplayEffect> GEClass) -> FGameplayEffectSpecHandle
	{
		if (!GEClass)
		{
			return FGameplayEffectSpecHandle();
		}
		FGameplayEffectContextHandle Context = MakeEffectContext();
		Context.AddSourceObject(GetOwner());
		FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(GEClass, 1.f, Context);
		return Spec;
	};

	CachedHealthRegenSpec       = MakeSpec(HealthRegenClass);
	CachedManaRegenSpec         = MakeSpec(ManaRegenClass);
	CachedStaminaRegenSpec      = MakeSpec(StaminaRegenClass);
	CachedArcaneShieldRegenSpec = MakeSpec(ArcaneShieldRegenClass);

	// Apply each resource GE once as an infinite, server-driven periodic effect.
	// The native MMCs read Rate and Amount live and return zero when their
	// resource's CannotRegen or Exhausted tags block recovery. Applying once
	// (and tracking the handles) prevents infinite-periodic GEs from stacking.
	// GEs are server-authoritative and replicate down (Mixed mode), so only apply
	// them on the server. The loose RegenActive tags above stay on both sides for
	// local HUD. The previous authority check lived inside the now-removed tick.
	ActivePassiveEffectHandles.Reset();
	const AActor* AvatarForApply = GetAvatarActor();
	if (AvatarForApply && AvatarForApply->HasAuthority())
	{
		auto ApplyOnce = [this](const FGameplayEffectSpecHandle& InSpec)
		{
			if (InSpec.IsValid())
			{
				ActivePassiveEffectHandles.Add(ApplyGameplayEffectSpecToSelf(*InSpec.Data.Get()));
			}
		};
		ApplyOnce(CachedHealthRegenSpec);
		ApplyOnce(CachedManaRegenSpec);
		ApplyOnce(CachedStaminaRegenSpec);
		ApplyOnce(CachedArcaneShieldRegenSpec);
	}

	bPassiveRegenStarted = true;
	UE_LOG(LogHunterGAS, Verbose, TEXT("StartPassiveRegen: ASC=%s — RegenActive tags granted"), *GetName());
}

void UHunterAbilitySystemComponent::StopPassiveRegen()
{
	for (const FActiveGameplayEffectHandle& Handle : ActivePassiveEffectHandles)
	{
		if (Handle.IsValid())
		{
			RemoveActiveGameplayEffect(Handle);
		}
	}
	ActivePassiveEffectHandles.Reset();
	StopSprintStaminaDegen();

	CachedHealthRegenSpec       = FGameplayEffectSpecHandle();
	CachedManaRegenSpec         = FGameplayEffectSpecHandle();
	CachedStaminaRegenSpec      = FGameplayEffectSpecHandle();
	CachedArcaneShieldRegenSpec = FGameplayEffectSpecHandle();

	const FPHGameplayTags& PHT = FPHGameplayTags::Get();
	RemoveLooseGameplayTag(PHT.Effect_Health_RegenActive);
	RemoveLooseGameplayTag(PHT.Effect_Mana_RegenActive);
	RemoveLooseGameplayTag(PHT.Effect_Stamina_RegenActive);
	RemoveLooseGameplayTag(PHT.Effect_ArcaneShield_RegenActive);

	bPassiveRegenStarted = false;
	UE_LOG(LogHunterGAS, Verbose, TEXT("StopPassiveRegen: ASC=%s — RegenActive tags removed"), *GetName());
}

void UHunterAbilitySystemComponent::HandleStaminaDepleted()
{
	const AActor* AvatarActorInstance = GetAvatarActor();
	if (!AvatarActorInstance || !AvatarActorInstance->HasAuthority())
	{
		return;
	}

	if (!bShouldCheckExhaustion)
	{
		return;
	}

	bSprintStaminaDegenRequested = false;
	bWallRunningStaminaDegenRequested = false;

	// Stop stamina-draining movement so the drain GE deactivates immediately.
	HunterAbilitySystemComponentPrivate::ForceStopSprinting(this);
	StopSprintStaminaDegen();

	// Apply the exhaustion GE once. It grants Effect.Stamina.Exhausted, which
	// inhibits stamina regen until sprint/wall-run input is released and the
	// ASC recovery timer removes it.
	// Skip if already exhausted so the window is not refreshed while pinned at 0.
	if (StaminaExhaustionGE &&
		!HasMatchingGameplayTag(FPHGameplayTags::Get().Effect_Stamina_Exhausted))
	{
		FGameplayEffectContextHandle Context = MakeEffectContext();
		Context.AddSourceObject(GetOwner());
		const FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(StaminaExhaustionGE, 1.f, Context);
		if (Spec.IsValid())
		{
			ActiveStaminaExhaustionHandle = ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			RefreshStaminaExhaustionRecovery();
		}
	}
}

void UHunterAbilitySystemComponent::HandleManaDepleted()
{
	const AActor* AvatarActorInstance = GetAvatarActor();
	if (!AvatarActorInstance || !AvatarActorInstance->HasAuthority())
	{
		return;
	}

	if (ManaExhaustionGE &&
		!HasMatchingGameplayTag(FPHGameplayTags::Get().Effect_Mana_Exhausted))
	{
		FGameplayEffectContextHandle Context = MakeEffectContext();
		Context.AddSourceObject(GetOwner());
		const FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(ManaExhaustionGE, 1.f, Context);
		if (Spec.IsValid())
		{
			ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

const UHunterAttributeSet* UHunterAbilitySystemComponent::GetHunterAttributeSet() const
{
	return GetSet<UHunterAttributeSet>();
}

#if !UE_BUILD_SHIPPING
void UHunterAbilitySystemComponent::ShowEffectDebug(const FGameplayEffectSpec& EffectSpec,
	const FGameplayTagContainer& TagContainer) const
{
	const int32 DebugLevel = CVarDebugEffects.GetValueOnGameThread();
	if (DebugLevel <= 0) return;

	const UGameplayEffect* EffectDef = EffectSpec.Def;
	if (!EffectDef) return;

	const FString EffectName = EffectDef->GetName();
	const AActor* HunterActor = GetOwner();
	const FString OwnerName = HunterActor ? HunterActor->GetName() : TEXT("Unknown");
    
	FString MagnitudeInfo;
	for (const FGameplayModifierInfo& Modifier : EffectDef->Modifiers)
	{
		float Magnitude = 0.0f;
		if (Modifier.ModifierMagnitude.AttemptCalculateMagnitude(EffectSpec, Magnitude))
		{
			const FString AttributeName = Modifier.Attribute.GetName();
			MagnitudeInfo += FString::Printf(TEXT("\n  - %s: %.2f"), *AttributeName, Magnitude);
		}
	}

	const FString DebugMessage = FString::Printf(
		TEXT("[EFFECT APPLIED] %s\nEffect: %s\nTags: %s%s"),
		*OwnerName,
		*EffectName,
		*TagContainer.ToStringSimple(),
		*MagnitudeInfo
	);

	if (GEngine)
	{
		const float Duration = CVarDebugEffectsDuration.GetValueOnGameThread();
		const FColor Color = FColor::Cyan;
        
		GEngine->AddOnScreenDebugMessage(
			INDEX_NONE,
			Duration,
			Color,
			DebugMessage
		);
	}

	if (DebugLevel >= 2)
	{
		UE_LOG(LogHunterGAS, Log, TEXT("%s"), *DebugMessage);
	}
}
#endif

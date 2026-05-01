// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#include "AbilitySystem/HunterAbilitySystemComponent.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/Effects/HunterGE_HealthRegen.h"
#include "AbilitySystem/Effects/HunterGE_ManaRegen.h"
#include "AbilitySystem/Effects/HunterGE_StaminaRegen.h"
#include "AbilitySystem/Effects/HunterGE_ArcaneShieldRegen.h"
#include "Tags/Components/TagManager.h"
#include "Character/PHBaseCharacter.h"
#include "Engine/Engine.h"
#include "PHGameplayTags.h"

namespace HunterAbilitySystemComponentPrivate
{
	constexpr float SprintStaminaDegenTickInterval = 0.1f;

	/** Base tick interval for passive regen accumulators (seconds). */
	constexpr float PassiveRegenBaseTickInterval = 0.1f;

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
		}

		if (UTagManager* TagManager = ResolveTagManager(ASC))
		{
			TagManager->SetTagState(Tags.Condition_Sprinting, false);
			TagManager->RefreshBaseConditionTags();
		}
	}
}

// Define log category
DEFINE_LOG_CATEGORY(LogHunterGAS);

#if !UE_BUILD_SHIPPING
// Console variable to toggle effect debugging
static TAutoConsoleVariable<int32> CVarDebugEffects(
	TEXT("Hunter.Debug.Effects"),
	0,
	TEXT("Debug gameplay effect applications\n")
	TEXT("0: Disabled (default)\n")
	TEXT("1: Show on-screen messages\n")
	TEXT("2: Show on-screen + log to console"),
	ECVF_Cheat
);

// Console variable for debug message duration
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
	// Show debug if enabled
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
		StartSprintStaminaDegen();
	}
	else
	{
		StopSprintStaminaDegen();
	}
}

void UHunterAbilitySystemComponent::StartSprintStaminaDegen()
{
	UWorld* World = GetWorld();
	const UHunterAttributeSet* AttributeSet = GetHunterAttributeSet();
	if (!World || !AttributeSet)
	{
		return;
	}

	// N-13 FIX: Cache these values so TickSprintStaminaDegen does not re-query every tick.
	CachedDegenRate   = FMath::Max(AttributeSet->GetStaminaDegenRate(), 0.f);
	CachedDegenAmount = FMath::Max(AttributeSet->GetStaminaDegenAmount(), 0.f);
	if (CachedDegenRate <= 0.f || CachedDegenAmount <= 0.f)
	{
		StopSprintStaminaDegen();
		return;
	}

	// OPT-SPRINT: Pre-build the GE spec once at sprint start so TickSprintStaminaDegen
	// only updates the SetByCaller magnitude instead of allocating context + spec 10x/sec.
	if (SprintStaminaDrainGE)
	{
		FGameplayEffectContextHandle Context = MakeEffectContext();
		CachedSprintDrainSpec = MakeOutgoingSpec(SprintStaminaDrainGE, 1.f, Context);
	}
	else
	{
		CachedSprintDrainSpec = FGameplayEffectSpecHandle(); // clear
	}

	if (!World->GetTimerManager().IsTimerActive(SprintStaminaDegenTimerHandle))
	{
		World->GetTimerManager().SetTimer(
			SprintStaminaDegenTimerHandle,
			this,
			&UHunterAbilitySystemComponent::TickSprintStaminaDegen,
			HunterAbilitySystemComponentPrivate::SprintStaminaDegenTickInterval,
			true);
	}

	if (!bSprintDegenEffectTagApplied)
	{
		if (UTagManager* TagManager = HunterAbilitySystemComponentPrivate::ResolveTagManager(this))
		{
			TagManager->SetTagState(FPHGameplayTags::Get().Effect_Stamina_DegenActive, true);
			bSprintDegenEffectTagApplied = true;
		}
	}
	UE_LOG(LogHunterGAS, Verbose, TEXT("StartSprintStaminaDegen: ASC=%s Rate=%.2f Amount=%.2f"), *GetName(), CachedDegenRate, CachedDegenAmount);
}

void UHunterAbilitySystemComponent::StopSprintStaminaDegen()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SprintStaminaDegenTimerHandle);
	}

	// OPT-SPRINT: Release cached spec — it holds refs to the context/effect CDO.
	CachedSprintDrainSpec = FGameplayEffectSpecHandle();

	if (bSprintDegenEffectTagApplied)
	{
		if (UTagManager* TagManager = HunterAbilitySystemComponentPrivate::ResolveTagManager(this))
		{
			TagManager->SetTagState(FPHGameplayTags::Get().Effect_Stamina_DegenActive, false);
		}
		bSprintDegenEffectTagApplied = false;
	}
	UE_LOG(LogHunterGAS, Verbose, TEXT("StopSprintStaminaDegen: ASC=%s"), *GetName());
}

void UHunterAbilitySystemComponent::TickSprintStaminaDegen()
{
	const AActor* AvatarActorInstance = GetAvatarActor();
	if (!AvatarActorInstance || !AvatarActorInstance->HasAuthority())
	{
		StopSprintStaminaDegen();
		return;
	}

	const FPHGameplayTags& GameplayTags = FPHGameplayTags::Get();
	if (!HasMatchingGameplayTag(GameplayTags.Condition_Sprinting))
	{
		StopSprintStaminaDegen();
		return;
	}
	if (CachedDegenRate <= 0.f || CachedDegenAmount <= 0.f)
	{
		StopSprintStaminaDegen();
		return;
	}

	const UHunterAttributeSet* AttributeSet = GetHunterAttributeSet();
	if (!AttributeSet)
	{
		StopSprintStaminaDegen();
		return;
	}

	const float CurrentStamina = FMath::Max(AttributeSet->GetStamina(), 0.f);
	const float DrainAmount = CachedDegenRate * CachedDegenAmount * HunterAbilitySystemComponentPrivate::SprintStaminaDegenTickInterval;
	if (DrainAmount <= 0.f)
	{
		return;
	}
	if (CurrentStamina <= KINDA_SMALL_NUMBER)
	{
		HunterAbilitySystemComponentPrivate::ForceStopSprinting(this);
		StopSprintStaminaDegen();
		return;
	}

	const float NewStamina = FMath::Max(0.f, CurrentStamina - DrainAmount);
	if (!FMath::IsNearlyEqual(CurrentStamina, NewStamina))
	{
		bool bAppliedViaGE = false;
		if (CachedSprintDrainSpec.IsValid())
		{
			CachedSprintDrainSpec.Data->SetSetByCallerMagnitude(
				FPHGameplayTags::Get().Data_Damage_Stamina, -DrainAmount);
			ApplyGameplayEffectSpecToSelf(*CachedSprintDrainSpec.Data.Get());
			bAppliedViaGE = true;
		}

		if (!bAppliedViaGE)
		{
			// Legacy fallback: direct base-value write.
			// Configure SprintStaminaDrainGE in Blueprint to remove this path.
			UE_LOG(LogHunterGAS, Warning,
				TEXT("TickSprintStaminaDegen: SprintStaminaDrainGE not set on %s — "
				     "falling back to SetNumericAttributeBase (bypasses GAS pipeline). "
				     "Configure SprintStaminaDrainGE in Blueprint defaults."),
				*GetName());
			SetNumericAttributeBase(UHunterAttributeSet::GetStaminaAttribute(), NewStamina);
		}

		if (UTagManager* TagManager = HunterAbilitySystemComponentPrivate::ResolveTagManager(this))
		{
			TagManager->RefreshBaseConditionTags();
		}

		if (NewStamina <= KINDA_SMALL_NUMBER)
		{
			HunterAbilitySystemComponentPrivate::ForceStopSprinting(this);
			StopSprintStaminaDegen();
		}
	}
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
	
	auto MakeSpec = [this](TSubclassOf<UGameplayEffect> GEClass) -> FGameplayEffectSpecHandle
	{
		if (!GEClass)
		{
			return FGameplayEffectSpecHandle();
		}
		FGameplayEffectContextHandle Context = MakeEffectContext();
		Context.AddSourceObject(GetOwner());
		return MakeOutgoingSpec(GEClass, 1.f, Context);
	};

	CachedHealthRegenSpec       = MakeSpec(HealthRegenGE);
	CachedManaRegenSpec         = MakeSpec(ManaRegenGE);
	CachedStaminaRegenSpec      = MakeSpec(StaminaRegenGE);
	CachedArcaneShieldRegenSpec = MakeSpec(ArcaneShieldRegenGE);

	// Reset accumulators so the first heal fires after exactly one Rate interval.
	HealthRegenAccumulator       = 0.f;
	ManaRegenAccumulator         = 0.f;
	StaminaRegenAccumulator      = 0.f;
	ArcaneShieldRegenAccumulator = 0.f;
	
	const FPHGameplayTags& PHT = FPHGameplayTags::Get();
	AddLooseGameplayTag(PHT.Effect_Health_RegenActive);
	AddLooseGameplayTag(PHT.Effect_Mana_RegenActive);
	AddLooseGameplayTag(PHT.Effect_Stamina_RegenActive);
	AddLooseGameplayTag(PHT.Effect_ArcaneShield_RegenActive);

	World->GetTimerManager().SetTimer(
		PassiveRegenTimerHandle,
		this,
		&UHunterAbilitySystemComponent::TickPassiveRegen,
		HunterAbilitySystemComponentPrivate::PassiveRegenBaseTickInterval,
		/*bLoop=*/true);

	bPassiveRegenStarted = true;
	UE_LOG(LogHunterGAS, Verbose, TEXT("StartPassiveRegen: ASC=%s — RegenActive tags granted"), *GetName());
}

void UHunterAbilitySystemComponent::StopPassiveRegen()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PassiveRegenTimerHandle);
	}

	CachedHealthRegenSpec       = FGameplayEffectSpecHandle();
	CachedManaRegenSpec         = FGameplayEffectSpecHandle();
	CachedStaminaRegenSpec      = FGameplayEffectSpecHandle();
	CachedArcaneShieldRegenSpec = FGameplayEffectSpecHandle();

	HealthRegenAccumulator       = 0.f;
	ManaRegenAccumulator         = 0.f;
	StaminaRegenAccumulator      = 0.f;
	ArcaneShieldRegenAccumulator = 0.f;

	// Remove the RegenActive loose tags that were added in StartPassiveRegen.
	const FPHGameplayTags& PHT = FPHGameplayTags::Get();
	RemoveLooseGameplayTag(PHT.Effect_Health_RegenActive);
	RemoveLooseGameplayTag(PHT.Effect_Mana_RegenActive);
	RemoveLooseGameplayTag(PHT.Effect_Stamina_RegenActive);
	RemoveLooseGameplayTag(PHT.Effect_ArcaneShield_RegenActive);

	bPassiveRegenStarted = false;
	UE_LOG(LogHunterGAS, Verbose, TEXT("StopPassiveRegen: ASC=%s — RegenActive tags removed"), *GetName());
}

void UHunterAbilitySystemComponent::TickPassiveRegen()
{
	const AActor* AvatarActorInstance = GetAvatarActor();
	if (!AvatarActorInstance || !AvatarActorInstance->HasAuthority())
	{
		return;
	}

	const UHunterAttributeSet* AS = GetHunterAttributeSet();
	if (!AS)
	{
		return;
	}

	const FPHGameplayTags& Tags  = FPHGameplayTags::Get();
	constexpr float        DeltaT = HunterAbilitySystemComponentPrivate::PassiveRegenBaseTickInterval;
	bool bAnyChanged = false;
	
	auto ApplyHeal = [this, &bAnyChanged](
		float                     Amount,
		float                     CurValue,
		float                     MaxValue,
		FGameplayEffectSpecHandle& Spec,
		FName                     RecoveryName,
		FGameplayAttribute         Attribute) -> void
	{
		if (Amount <= 0.f || CurValue >= MaxValue)
		{
			return;
		}

		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(RecoveryName, Amount);
			ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
		else
		{
			// Legacy fallback: bypasses PostGameplayEffectExecute but
			// PreAttributeChange / PostAttributeChange still fire.
			const float NewValue = FMath::Min(CurValue + Amount, MaxValue);
			SetNumericAttributeBase(Attribute, NewValue);
		}
		bAnyChanged = true;
	};

	// ── Health ────────────────────────────────────────────────────────────
	// Gated by Effect.Health.RegenActive. Remove the tag to pause regen
	// (e.g. a "CannotRegenHP" status GE can simply remove this loose tag).
	if (HasMatchingGameplayTag(Tags.Effect_Health_RegenActive))
	{
		const float Rate = AS->GetHealthRegenRate();
		if (Rate > 0.f)
		{
			HealthRegenAccumulator += DeltaT;
			if (HealthRegenAccumulator >= Rate)
			{
				HealthRegenAccumulator -= Rate; // carry overshoot for precision

				const float Amount    = FMath::Max(AS->GetHealthRegenAmount(), 0.f);
				const float CurHealth = FMath::Max(AS->GetHealth(), 0.f);
				const float MaxHealth = FMath::Max(AS->GetMaxEffectiveHealth(), 0.f);

				// Do not regen a dead character.
				if (CurHealth > 0.f)
				{
					ApplyHeal(Amount, CurHealth, MaxHealth,
						CachedHealthRegenSpec, FName("Data.Recovery.Health"),
						UHunterAttributeSet::GetHealthAttribute());
				}
			}
		}
		else
		{
			HealthRegenAccumulator = 0.f;
		}
	}
	else
	{
		HealthRegenAccumulator = 0.f;
	}

	// ── Mana ──────────────────────────────────────────────────────────────
	// Gated by Effect.Mana.RegenActive.
	if (HasMatchingGameplayTag(Tags.Effect_Mana_RegenActive))
	{
		const float Rate = AS->GetManaRegenRate();
		if (Rate > 0.f)
		{
			ManaRegenAccumulator += DeltaT;
			if (ManaRegenAccumulator >= Rate)
			{
				ManaRegenAccumulator -= Rate;

				const float Amount  = FMath::Max(AS->GetManaRegenAmount(), 0.f);
				const float CurMana = FMath::Max(AS->GetMana(), 0.f);
				const float MaxMana = FMath::Max(AS->GetMaxEffectiveMana(), 0.f);

				ApplyHeal(Amount, CurMana, MaxMana,
					CachedManaRegenSpec, FName("Data.Recovery.Mana"),
					UHunterAttributeSet::GetManaAttribute());
			}
		}
		else
		{
			ManaRegenAccumulator = 0.f;
		}
	}
	else
	{
		ManaRegenAccumulator = 0.f;
	}

	// ── Stamina ───────────────────────────────────────────────────────────
	// Gated by Effect.Stamina.RegenActive AND suppressed while sprinting
	// (the degen timer is running and regen would fight it).
	if (HasMatchingGameplayTag(Tags.Effect_Stamina_RegenActive) &&
	    !HasMatchingGameplayTag(Tags.Condition_Sprinting))
	{
		const float Rate = AS->GetStaminaRegenRate();
		if (Rate > 0.f)
		{
			StaminaRegenAccumulator += DeltaT;
			if (StaminaRegenAccumulator >= Rate)
			{
				StaminaRegenAccumulator -= Rate;

				const float Amount     = FMath::Max(AS->GetStaminaRegenAmount(), 0.f);
				const float CurStamina = FMath::Max(AS->GetStamina(), 0.f);
				const float MaxStamina = FMath::Max(AS->GetMaxEffectiveStamina(), 0.f);

				ApplyHeal(Amount, CurStamina, MaxStamina,
					CachedStaminaRegenSpec, FName("Data.Recovery.Stamina"),
					UHunterAttributeSet::GetStaminaAttribute());
			}
		}
		else
		{
			StaminaRegenAccumulator = 0.f;
		}
	}
	else
	{
		// Reset accumulator when suppressed so regen doesn't fire immediately
		// on the first tick after the block is lifted — player earns the full interval.
		StaminaRegenAccumulator = 0.f;
	}

	// ── ArcaneShield ──────────────────────────────────────────────────────
	// Gated by Effect.ArcaneShield.RegenActive.
	if (HasMatchingGameplayTag(Tags.Effect_ArcaneShield_RegenActive))
	{
		const float Rate = AS->GetArcaneShieldRegenRate();
		if (Rate > 0.f)
		{
			ArcaneShieldRegenAccumulator += DeltaT;
			if (ArcaneShieldRegenAccumulator >= Rate)
			{
				ArcaneShieldRegenAccumulator -= Rate;

				const float Amount        = FMath::Max(AS->GetArcaneShieldRegenAmount(), 0.f);
				const float CurAS         = FMath::Max(AS->GetArcaneShield(), 0.f);
				const float MaxAS         = FMath::Max(AS->GetMaxEffectiveArcaneShield(), 0.f);

				ApplyHeal(Amount, CurAS, MaxAS,
					CachedArcaneShieldRegenSpec, FName("Data.Recovery.ArcaneShield"),
					UHunterAttributeSet::GetArcaneShieldAttribute());
			}
		}
		else
		{
			ArcaneShieldRegenAccumulator = 0.f;
		}
	}
	else
	{
		ArcaneShieldRegenAccumulator = 0.f;
	}

	if (bAnyChanged)
	{
		if (UTagManager* TagManager = HunterAbilitySystemComponentPrivate::ResolveTagManager(this))
		{
			TagManager->RefreshBaseConditionTags();
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
    
	// Get magnitude info
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

	// Build debug message (using ASCII for compatibility)
	const FString DebugMessage = FString::Printf(
		TEXT("[EFFECT APPLIED] %s\nEffect: %s\nTags: %s%s"),
		*OwnerName,
		*EffectName,
		*TagContainer.ToStringSimple(),
		*MagnitudeInfo
	);

	// On-screen message
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

	// Console log if level 2+
	if (DebugLevel >= 2)
	{
		UE_LOG(LogHunterGAS, Log, TEXT("%s"), *DebugMessage);
	}
}
#endif
 

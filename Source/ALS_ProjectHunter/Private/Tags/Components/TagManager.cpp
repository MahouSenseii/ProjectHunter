#include "Tags/Components/TagManager.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Character/PHBaseCharacter.h"
#include "GameFramework/Character.h"
#include "Tags/PHGameplayTags.h"
#include "Tags/Debug/TagDebugManager.h"
#include "Tags/Helpers/TagConditionEvaluator.h"

DEFINE_LOG_CATEGORY(LogTagManager);

namespace TagManagerPrivate
{
	float GetEffectiveMaxValue(const float EffectiveMaxValue, const float RawMaxValue)
	{
		return EffectiveMaxValue > 0.0f ? EffectiveMaxValue : FMath::Max(RawMaxValue, 0.0f);
	}

	bool IsActivelySprinting(const APHBaseCharacter* HunterCharacter, const bool bMoving)
	{
		return HunterCharacter
			&& bMoving
			&& HunterCharacter->GetDesiredGait() == EALSGait::Sprinting;
	}

	bool CanProjectAuthoritativeConditions(const UTagManager& Manager)
	{
		const AActor* Owner = Manager.GetOwner();
		return !Owner || Owner->HasAuthority();
	}

	EGameplayTagReplicationState GetReplicationState(const UTagManager& Manager)
	{
		const AActor* Owner = Manager.GetOwner();
		return Owner && Owner->HasAuthority()
			? EGameplayTagReplicationState::TagOnly
			: EGameplayTagReplicationState::None;
	}

	bool HasInitializedSiblingTagManager(AActor* Owner, const UTagManager* CurrentManager)
	{
		if (!Owner)
		{
			return false;
		}

		TArray<UTagManager*> AllManagers;
		Owner->GetComponents<UTagManager>(AllManagers);

		for (UTagManager* OtherManager : AllManagers)
		{
			if (OtherManager != CurrentManager && OtherManager && OtherManager->IsInitialized())
			{
				return true;
			}
		}

		return false;
	}
}

UTagManager::UTagManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(false);
}

void UTagManager::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!ASC)
	{
		if (TagManagerPrivate::HasInitializedSiblingTagManager(Owner, this))
		{
			UE_LOG(
				LogTagManager,
				Warning,
				TEXT("TagManager::BeginPlay: '%s' has multiple TagManager components and this one (%s) is not initialized. Remove the extra TagManager from the Blueprint Components panel."),
				*GetNameSafe(Owner),
				*GetName());
			SetComponentTickEnabled(false);
			return;
		}

		if (IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Owner))
		{
			if (UAbilitySystemComponent* OwnerASC = AbilitySystemInterface->GetAbilitySystemComponent())
			{
				Initialize(OwnerASC);
			}
			else
			{
				UE_LOG(
					LogTagManager,
					Verbose,
					TEXT("TagManager::BeginPlay: '%s' ASC not available yet. Tags will initialize through the character ASC ready path."),
					*GetNameSafe(Owner));
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (DebugManager.bEnableDebug)
	{
		SetComponentTickEnabled(true);
	}
#endif
}

void UTagManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindAttributeChangeDelegates();
	ClearManagedTagStates(ASC);
	ASC = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UTagManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !UE_BUILD_SHIPPING
	if (DebugManager.bEnableDebug)
	{
		DebugManager.DrawDebug(this, this);
	}
#endif

	if (!ASC)
	{
		return;
	}

	if (bBaseConditionsDirty)
	{
		bBaseConditionsDirty = false;
		RefreshBaseConditionTags();
	}

	ConditionRefreshAccumulator += DeltaTime;
	if (ConditionRefreshAccumulator >= FMath::Max(ConditionThresholds.MovementRefreshInterval, 0.01f))
	{
		ConditionRefreshAccumulator = 0.0f;
		RefreshMovementConditionTags();
	}
}

void UTagManager::Initialize(UAbilitySystemComponent* InASC)
{
	if (!InASC)
	{
		return;
	}

	if (ASC && ASC != InASC)
	{
		UnbindAttributeChangeDelegates();
		ClearManagedTagStates(ASC);
	}
	else if (ASC)
	{
		UnbindAttributeChangeDelegates();
	}

	ASC = InASC;
	const bool bProjectsConditions = TagManagerPrivate::CanProjectAuthoritativeConditions(*this);

#if UE_BUILD_SHIPPING
	SetComponentTickEnabled(ASC != nullptr && bProjectsConditions);
#else
	SetComponentTickEnabled((ASC != nullptr && bProjectsConditions) || DebugManager.bEnableDebug);
#endif

	UE_LOG(LogTagManager, Verbose, TEXT("Initialized TagManager for owner %s with ASC %s."), *GetNameSafe(GetOwner()), *GetNameSafe(ASC));

	ApplyPendingStates();
	if (bProjectsConditions)
	{
		BindAttributeChangeDelegates();
		RefreshBaseConditionTags();
	}
}

void UTagManager::AddTag(const FGameplayTag& Tag)
{
	if (!Tag.IsValid())
	{
		return;
	}

	if (!ASC)
	{
		PendingTagStates.Add(Tag, true);
		return;
	}

	PendingTagStates.Remove(Tag);

	if (ManagedLooseTags.Contains(Tag))
	{
		return;
	}

	ManagedLooseTags.Add(Tag);
	ASC->AddLooseGameplayTag(Tag, 1, TagManagerPrivate::GetReplicationState(*this));
	UE_LOG(LogTagManager, VeryVerbose, TEXT("Added tag %s to owner %s."), *Tag.ToString(), *GetNameSafe(GetOwner()));
}

void UTagManager::RemoveTag(const FGameplayTag& Tag)
{
	if (!Tag.IsValid())
	{
		return;
	}

	if (!ASC)
	{
		PendingTagStates.Add(Tag, false);
		return;
	}

	PendingTagStates.Remove(Tag);

	if (!ManagedLooseTags.Remove(Tag))
	{
		return;
	}

	if (ASC->GetTagCount(Tag) > 0)
	{
		ASC->RemoveLooseGameplayTag(Tag, 1, TagManagerPrivate::GetReplicationState(*this));
	}
	UE_LOG(LogTagManager, VeryVerbose, TEXT("Removed managed tag %s from owner %s."), *Tag.ToString(), *GetNameSafe(GetOwner()));
}

void UTagManager::SetTagState(const FGameplayTag& Tag, const bool bEnabled)
{
	if (bEnabled)
	{
		AddTag(Tag);
	}
	else
	{
		RemoveTag(Tag);
	}
}

void UTagManager::SetDeadState(const bool bDead)
{
	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	SetTagState(Tags.Condition_Alive, !bDead);
	SetTagState(Tags.Condition_Dead, bDead);
	OnDeadStateChanged.Broadcast(bDead);
}

bool UTagManager::HasTag(const FGameplayTag& Tag) const
{
	if (!Tag.IsValid())
	{
		return false;
	}

	if (ASC)
	{
		return ASC->HasMatchingGameplayTag(Tag);
	}

	return HasPendingEnabledTag(Tag);
}

bool UTagManager::HasAnyTags(const FGameplayTagContainer& Tags) const
{
	if (ASC)
	{
		return ASC->HasAnyMatchingGameplayTags(Tags);
	}

	for (const FGameplayTag& Tag : Tags)
	{
		if (HasPendingEnabledTag(Tag))
		{
			return true;
		}
	}

	return false;
}

bool UTagManager::HasAllTags(const FGameplayTagContainer& Tags) const
{
	if (ASC)
	{
		return ASC->HasAllMatchingGameplayTags(Tags);
	}

	for (const FGameplayTag& Tag : Tags)
	{
		if (!HasPendingEnabledTag(Tag))
		{
			return false;
		}
	}

	return true;
}

void UTagManager::RefreshBaseConditionTags()
{
	if (!ASC || !TagManagerPrivate::CanProjectAuthoritativeConditions(*this))
	{
		return;
	}

	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	const UHunterAttributeSet* Attributes = GetHunterAttributeSet();
	const APHBaseCharacter* HunterCharacter = Cast<APHBaseCharacter>(GetOwner());
	const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());

	if (Attributes)
	{
		const float Health = FMath::Max(Attributes->GetHealth(), 0.0f);
		const float MaxHealth = TagManagerPrivate::GetEffectiveMaxValue(Attributes->GetMaxEffectiveHealth(), Attributes->GetMaxHealth());
		const float Mana = FMath::Max(Attributes->GetMana(), 0.0f);
		const float MaxMana = TagManagerPrivate::GetEffectiveMaxValue(Attributes->GetMaxEffectiveMana(), Attributes->GetMaxMana());
		const float Stamina = FMath::Max(Attributes->GetStamina(), 0.0f);
		const float MaxStamina = TagManagerPrivate::GetEffectiveMaxValue(Attributes->GetMaxEffectiveStamina(), Attributes->GetMaxStamina());
		const float ArcaneShield = FMath::Max(Attributes->GetArcaneShield(), 0.0f);
		const float MaxArcaneShield = TagManagerPrivate::GetEffectiveMaxValue(Attributes->GetMaxEffectiveArcaneShield(), Attributes->GetMaxArcaneShield());

		const bool bDead = HasTag(Tags.Condition_Dead);
		SetTagState(Tags.Condition_Alive, Health > 0.0f && !bDead);

		RefreshResourceConditionTags(Tags.Condition_OnLowHealth, Tags.Condition_OnFullHealth, Health, MaxHealth);
		RefreshResourceConditionTags(Tags.Condition_OnLowMana, Tags.Condition_OnFullMana, Mana, MaxMana);
		RefreshResourceConditionTags(Tags.Condition_OnLowStamina, Tags.Condition_OnFullStamina, Stamina, MaxStamina);
		RefreshResourceConditionTags(
			Tags.Condition_OnLowArcaneShield,
			Tags.Condition_OnFullArcaneShield,
			ArcaneShield,
			MaxArcaneShield);
	}

	const bool bMoving = ComputeMovementConditionState(CharacterOwner);
	SetTagState(Tags.Condition_WhileMoving, bMoving);
	SetTagState(Tags.Condition_WhileStationary, !bMoving);

	const bool bActivelySprinting = HunterCharacter
		? TagManagerPrivate::IsActivelySprinting(HunterCharacter, bMoving)
		: HasExternalTagSource(Tags.Condition_Sprinting);
	SetTagState(Tags.Condition_Sprinting, bActivelySprinting);

	const bool bInCombat = HasExternalTagSource(Tags.Condition_InCombat)
		|| HasTag(Tags.Condition_TakingDamage)
		|| HasTag(Tags.Condition_DealingDamage)
		|| HasTag(Tags.Condition_RecentlyHit)
		|| HasTag(Tags.Condition_RecentlyUsedSkill);
	SetTagState(Tags.Condition_InCombat, bInCombat);
	SetTagState(Tags.Condition_OutOfCombat, !bInCombat);
}

void UTagManager::PrintActiveTags() const
{
	if (!ASC)
	{
		UE_LOG(LogTagManager, Log, TEXT("PrintActiveTags: owner %s has no ASC. Pending tag count=%d"), *GetNameSafe(GetOwner()), PendingTagStates.Num());
		return;
	}

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	UE_LOG(LogTagManager, Log, TEXT("Active tags for %s: %s"), *GetNameSafe(GetOwner()), *OwnedTags.ToStringSimple());
}

void UTagManager::ApplyPendingStates()
{
	if (!ASC)
	{
		return;
	}

	TArray<TPair<FGameplayTag, bool>> PendingEntries;
	PendingEntries.Reserve(PendingTagStates.Num());
	for (const TPair<FGameplayTag, bool>& Pair : PendingTagStates)
	{
		PendingEntries.Add(Pair);
	}

	PendingTagStates.Reset();

	for (const TPair<FGameplayTag, bool>& Pair : PendingEntries)
	{
		SetTagState(Pair.Key, Pair.Value);
	}
}

void UTagManager::ClearManagedTagStates(UAbilitySystemComponent* TargetASC)
{
	if (!TargetASC)
	{
		ManagedLooseTags.Reset();
		return;
	}

	TArray<FGameplayTag> TagsToRemove = ManagedLooseTags.Array();
	ManagedLooseTags.Reset();

	for (const FGameplayTag& Tag : TagsToRemove)
	{
		if (Tag.IsValid() && TargetASC->GetTagCount(Tag) > 0)
		{
			TargetASC->RemoveLooseGameplayTag(Tag, 1, TagManagerPrivate::GetReplicationState(*this));
		}
	}
}

bool UTagManager::HasExternalTagSource(const FGameplayTag& Tag) const
{
	if (!ASC || !Tag.IsValid())
	{
		return false;
	}

	const int32 ManagedCount = ManagedLooseTags.Contains(Tag) ? 1 : 0;
	return ASC->GetTagCount(Tag) > ManagedCount;
}

bool UTagManager::HasPendingEnabledTag(const FGameplayTag& Tag) const
{
	if (const bool* bPendingEnabled = PendingTagStates.Find(Tag))
	{
		return *bPendingEnabled;
	}

	return false;
}

bool UTagManager::ComputeMovementConditionState(const ACharacter* CharacterOwner)
{
	if (!CharacterOwner)
	{
		bHasMovementConditionState = true;
		bLastMovementConditionMoving = false;
		return false;
	}

	const float SpeedSq = CharacterOwner->GetVelocity().SizeSquared2D();
	const bool bMoving = FTagConditionEvaluator::EvaluateMovement(
		SpeedSq,
		bHasMovementConditionState,
		bLastMovementConditionMoving,
		ConditionThresholds);

	bHasMovementConditionState = true;
	bLastMovementConditionMoving = bMoving;
	return bMoving;
}

void UTagManager::RefreshResourceConditionTags(
	const FGameplayTag& LowTag,
	const FGameplayTag& FullTag,
	const float CurrentValue,
	const float MaxValue)
{
	const FTagResourceConditionState State = FTagConditionEvaluator::EvaluateResource(
		CurrentValue,
		MaxValue,
		ManagedLooseTags.Contains(LowTag),
		ManagedLooseTags.Contains(FullTag),
		ConditionThresholds);

	SetTagState(LowTag, State.bLow);
	SetTagState(FullTag, State.bFull);
}

const UHunterAttributeSet* UTagManager::GetHunterAttributeSet() const
{
	return ASC ? ASC->GetSet<UHunterAttributeSet>() : nullptr;
}

void UTagManager::SetTagDebugEnabled(bool bEnable)
{
#if UE_BUILD_SHIPPING
	DebugManager.bEnableDebug = false;
	(void)bEnable;
#else
	const bool bWasEnabled = DebugManager.bEnableDebug;
	DebugManager.bEnableDebug = bEnable;

	SetComponentTickEnabled(bEnable || (ASC != nullptr && TagManagerPrivate::CanProjectAuthoritativeConditions(*this)));

	if (bWasEnabled && !DebugManager.bEnableDebug)
	{
		DebugManager.DrawDebug(this, this);
	}
#endif
}

bool UTagManager::GetOwnedTags(FGameplayTagContainer& OutTags) const
{
	if (!ASC)
	{
		return false;
	}

	ASC->GetOwnedGameplayTags(OutTags);
	return true;
}

void UTagManager::RefreshMovementConditionTags()
{
	if (!ASC || !TagManagerPrivate::CanProjectAuthoritativeConditions(*this))
	{
		return;
	}

	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	const APHBaseCharacter* HunterCharacter = Cast<APHBaseCharacter>(GetOwner());
	const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());

	const bool bMoving = ComputeMovementConditionState(CharacterOwner);
	SetTagState(Tags.Condition_WhileMoving, bMoving);
	SetTagState(Tags.Condition_WhileStationary, !bMoving);

	const bool bActivelySprinting = HunterCharacter
		? TagManagerPrivate::IsActivelySprinting(HunterCharacter, bMoving)
		: HasExternalTagSource(Tags.Condition_Sprinting);
	SetTagState(Tags.Condition_Sprinting, bActivelySprinting);

	const bool bInCombat = HasExternalTagSource(Tags.Condition_InCombat)
		|| HasTag(Tags.Condition_TakingDamage)
		|| HasTag(Tags.Condition_DealingDamage)
		|| HasTag(Tags.Condition_RecentlyHit)
		|| HasTag(Tags.Condition_RecentlyUsedSkill);
	SetTagState(Tags.Condition_InCombat, bInCombat);
	SetTagState(Tags.Condition_OutOfCombat, !bInCombat);
}

void UTagManager::BindAttributeChangeDelegates()
{
	if (!ASC)
	{
		return;
	}

	UnbindAttributeChangeDelegates();

	auto OnResourceChanged = [this](const FOnAttributeChangeData& Data)
	{
		(void)Data;
		bBaseConditionsDirty = true;
	};

	if (!GetHunterAttributeSet())
	{
		return;
	}

	auto BindAttributeChange = [this, &OnResourceChanged](const FGameplayAttribute& Attribute)
	{
		FDelegateHandle Handle = ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddLambda(OnResourceChanged);
		AttributeDelegateBindings.Add({ Attribute, Handle });
	};

	BindAttributeChange(UHunterAttributeSet::GetHealthAttribute());
	BindAttributeChange(UHunterAttributeSet::GetMaxHealthAttribute());
	BindAttributeChange(UHunterAttributeSet::GetMaxEffectiveHealthAttribute());
	BindAttributeChange(UHunterAttributeSet::GetManaAttribute());
	BindAttributeChange(UHunterAttributeSet::GetMaxManaAttribute());
	BindAttributeChange(UHunterAttributeSet::GetMaxEffectiveManaAttribute());
	BindAttributeChange(UHunterAttributeSet::GetStaminaAttribute());
	BindAttributeChange(UHunterAttributeSet::GetMaxStaminaAttribute());
	BindAttributeChange(UHunterAttributeSet::GetMaxEffectiveStaminaAttribute());
	BindAttributeChange(UHunterAttributeSet::GetArcaneShieldAttribute());
	BindAttributeChange(UHunterAttributeSet::GetMaxArcaneShieldAttribute());
	BindAttributeChange(UHunterAttributeSet::GetMaxEffectiveArcaneShieldAttribute());

	UE_LOG(LogTagManager, Verbose, TEXT("BindAttributeChangeDelegates: bound resource delegates for owner %s."), *GetNameSafe(GetOwner()));
}

void UTagManager::UnbindAttributeChangeDelegates()
{
	if (!ASC || AttributeDelegateBindings.Num() == 0)
	{
		AttributeDelegateBindings.Reset();
		return;
	}

	for (const FTagAttributeDelegateBinding& Binding : AttributeDelegateBindings)
	{
		if (Binding.Attribute.IsValid() && Binding.Handle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Binding.Attribute).Remove(Binding.Handle);
		}
	}

	AttributeDelegateBindings.Reset();
}

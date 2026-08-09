#include "Tags/Components/TagManager.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Character/PHBaseCharacter.h"
#include "GameFramework/Character.h"
#include "Tags/PHGameplayTags.h"
#include "Tags/Debug/TagDebugManager.h"

DEFINE_LOG_CATEGORY(LogTagManager);

namespace TagManagerPrivate
{
	constexpr float LowResourceThreshold = 0.35f;
	constexpr float MovementStartSpeedThresholdSq = 400.0f;
	constexpr float MovementStopSpeedThresholdSq = 25.0f;
	constexpr float ConditionRefreshInterval = 0.1f;

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
	if (ConditionRefreshAccumulator >= TagManagerPrivate::ConditionRefreshInterval)
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

	if (ASC)
	{
		UnbindAttributeChangeDelegates();
	}

	ASC = InASC;

#if UE_BUILD_SHIPPING
	SetComponentTickEnabled(ASC != nullptr);
#else
	SetComponentTickEnabled(ASC != nullptr || DebugManager.bEnableDebug);
#endif

	UE_LOG(LogTagManager, Verbose, TEXT("Initialized TagManager for owner %s with ASC %s."), *GetNameSafe(GetOwner()), *GetNameSafe(ASC));

	ApplyPendingStates();
	BindAttributeChangeDelegates();
	RefreshBaseConditionTags();
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

	if (ASC->HasMatchingGameplayTag(Tag))
	{
		return;
	}

	ASC->AddLooseGameplayTag(Tag);
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

	const int32 ExistingCount = ASC->GetTagCount(Tag);
	if (ExistingCount <= 0)
	{
		return;
	}

	ASC->RemoveLooseGameplayTag(Tag, ExistingCount);
	UE_LOG(LogTagManager, VeryVerbose, TEXT("Removed tag %s from owner %s. ClearedCount=%d"), *Tag.ToString(), *GetNameSafe(GetOwner()), ExistingCount);
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
	if (!ASC)
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

		SetTagState(Tags.Condition_OnFullHealth, ComputeFullResourceState(Health, MaxHealth));
		SetTagState(Tags.Condition_OnLowHealth, ComputeLowResourceState(Health, MaxHealth));
		SetTagState(Tags.Condition_OnFullMana, ComputeFullResourceState(Mana, MaxMana));
		SetTagState(Tags.Condition_OnLowMana, ComputeLowResourceState(Mana, MaxMana));
		SetTagState(Tags.Condition_OnFullStamina, ComputeFullResourceState(Stamina, MaxStamina));
		SetTagState(Tags.Condition_OnLowStamina, ComputeLowResourceState(Stamina, MaxStamina));
		SetTagState(Tags.Condition_OnFullArcaneShield, ComputeFullResourceState(ArcaneShield, MaxArcaneShield));
		SetTagState(Tags.Condition_OnLowArcaneShield, ComputeLowResourceState(ArcaneShield, MaxArcaneShield));
	}

	const bool bMoving = ComputeMovementConditionState(CharacterOwner);
	SetTagState(Tags.Condition_WhileMoving, bMoving);
	SetTagState(Tags.Condition_WhileStationary, !bMoving);

	const bool bActivelySprinting = HunterCharacter
		? TagManagerPrivate::IsActivelySprinting(HunterCharacter, bMoving)
		: HasTag(Tags.Condition_Sprinting);
	SetTagState(Tags.Condition_Sprinting, bActivelySprinting);

	const bool bInCombat = HasTag(Tags.Condition_InCombat)
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
	const bool bMoving = !bHasMovementConditionState
		? SpeedSq > TagManagerPrivate::MovementStartSpeedThresholdSq
		: (bLastMovementConditionMoving
			? SpeedSq > TagManagerPrivate::MovementStopSpeedThresholdSq
			: SpeedSq > TagManagerPrivate::MovementStartSpeedThresholdSq);

	bHasMovementConditionState = true;
	bLastMovementConditionMoving = bMoving;
	return bMoving;
}

bool UTagManager::ComputeLowResourceState(const float CurrentValue, const float MaxValue) const
{
	return MaxValue > 0.0f && (CurrentValue / MaxValue) <= TagManagerPrivate::LowResourceThreshold;
}

bool UTagManager::ComputeFullResourceState(const float CurrentValue, const float MaxValue) const
{
	return MaxValue > 0.0f && CurrentValue >= (MaxValue - KINDA_SMALL_NUMBER);
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

	SetComponentTickEnabled(bEnable || ASC != nullptr);

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
	if (!ASC)
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
		: HasTag(Tags.Condition_Sprinting);
	SetTagState(Tags.Condition_Sprinting, bActivelySprinting);

	const bool bInCombat = HasTag(Tags.Condition_TakingDamage)
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

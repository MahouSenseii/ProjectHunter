#include "Character/Component/TagManager.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Character/PHBaseCharacter.h"
#include "GameFramework/Character.h"
#include "PHGameplayTags.h"

DEFINE_LOG_CATEGORY(LogTagManager);

namespace TagManagerPrivate
{
	constexpr float LowResourceThreshold = 0.35f;
	constexpr float MovementSpeedThresholdSq = 25.f;

	// N-06 FIX: Refresh interval in seconds. Attribute-based conditions are driven by
	// GAS delegates (zero cost at rest). The tick now only re-checks movement/sprint
	// state at this cadence rather than every frame.
	constexpr float ConditionRefreshInterval = 0.1f;

	float GetEffectiveMaxValue(const float EffectiveMaxValue, const float RawMaxValue)
	{
		return EffectiveMaxValue > 0.f ? EffectiveMaxValue : FMath::Max(RawMaxValue, 0.f);
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

	if (!ASC)
	{
		if (IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner()))
		{
			Initialize(AbilitySystemInterface->GetAbilitySystemComponent());
		}
	}
}

void UTagManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	(void)TickType;
	(void)ThisTickFunction;

	if (!ASC)
	{
		return;
	}

	// OPT-TAG: Flush coalesced attribute-change refreshes (dirty flag set by
	// BindAttributeChangeDelegates callbacks). This replaces the old approach
	// where every delegate callback immediately called RefreshBaseConditionTags(),
	// which in heavy combat could fire 5-10x per frame for the same result.
	if (bBaseConditionsDirty)
	{
		bBaseConditionsDirty = false;
		RefreshBaseConditionTags();
	}

	// N-06 FIX: Only re-check movement/sprint state at a reduced cadence (100ms).
	// Attribute-based conditions (health, mana, stamina) are now event-driven via
	// GAS delegates bound in BindAttributeChangeDelegates(), so we don't need to
	// poll them every frame here.
	ConditionRefreshAccumulator += DeltaTime;
	if (ConditionRefreshAccumulator >= TagManagerPrivate::ConditionRefreshInterval)
	{
		ConditionRefreshAccumulator = 0.f;
		RefreshMovementConditionTags();
	}
}

void UTagManager::Initialize(UAbilitySystemComponent* InASC)
{
	ASC = InASC;
	SetComponentTickEnabled(ASC != nullptr);

	if (!ASC)
	{
		UE_LOG(LogTagManager, Verbose, TEXT("Initialize called without a valid ASC for owner %s."), *GetNameSafe(GetOwner()));
		return;
	}

	UE_LOG(LogTagManager, Verbose, TEXT("Initialized TagManager for owner %s with ASC %s."), *GetNameSafe(GetOwner()), *GetNameSafe(ASC));

	ApplyPendingStates();

	// N-06 FIX: Bind GAS attribute-change delegates so resource conditions update
	// reactively instead of being polled every frame.
	BindAttributeChangeDelegates();

	// Initial full refresh (establishes baseline state on first Init)
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

	if (!ASC->HasMatchingGameplayTag(Tag))
	{
		return;
	}

	ASC->RemoveLooseGameplayTag(Tag);
	UE_LOG(LogTagManager, VeryVerbose, TEXT("Removed tag %s from owner %s."), *Tag.ToString(), *GetNameSafe(GetOwner()));
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
		bPendingBaseRefresh = true;
		return;
	}

	bPendingBaseRefresh = false;

	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	const UHunterAttributeSet* Attributes = GetHunterAttributeSet();
	const APHBaseCharacter* HunterCharacter = Cast<APHBaseCharacter>(GetOwner());
	const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());

	if (Attributes)
	{
		const float Health = FMath::Max(Attributes->GetHealth(), 0.f);
		const float MaxHealth = TagManagerPrivate::GetEffectiveMaxValue(Attributes->GetMaxEffectiveHealth(), Attributes->GetMaxHealth());
		const float Mana = FMath::Max(Attributes->GetMana(), 0.f);
		const float MaxMana = TagManagerPrivate::GetEffectiveMaxValue(Attributes->GetMaxEffectiveMana(), Attributes->GetMaxMana());
		const float Stamina = FMath::Max(Attributes->GetStamina(), 0.f);
		const float MaxStamina = TagManagerPrivate::GetEffectiveMaxValue(Attributes->GetMaxEffectiveStamina(), Attributes->GetMaxStamina());
		const float ArcaneShield = FMath::Max(Attributes->GetArcaneShield(), 0.f);
		const float MaxArcaneShield = TagManagerPrivate::GetEffectiveMaxValue(Attributes->GetMaxEffectiveArcaneShield(), Attributes->GetMaxArcaneShield());

		const bool bAlive = Health > 0.f;
		SetTagState(Tags.Condition_Alive, bAlive);
		SetTagState(Tags.Condition_Dead, !bAlive);

		SetTagState(Tags.Condition_OnFullHealth, ComputeFullResourceState(Health, MaxHealth));
		SetTagState(Tags.Condition_OnLowHealth, ComputeLowResourceState(Health, MaxHealth));
		SetTagState(Tags.Condition_OnFullMana, ComputeFullResourceState(Mana, MaxMana));
		SetTagState(Tags.Condition_OnLowMana, ComputeLowResourceState(Mana, MaxMana));
		SetTagState(Tags.Condition_OnFullStamina, ComputeFullResourceState(Stamina, MaxStamina));
		SetTagState(Tags.Condition_OnLowStamina, ComputeLowResourceState(Stamina, MaxStamina));
		SetTagState(Tags.Condition_OnFullArcaneShield, ComputeFullResourceState(ArcaneShield, MaxArcaneShield));
		SetTagState(Tags.Condition_OnLowArcaneShield, ComputeLowResourceState(ArcaneShield, MaxArcaneShield));
	}

	const bool bMoving = CharacterOwner && CharacterOwner->GetVelocity().SizeSquared2D() > TagManagerPrivate::MovementSpeedThresholdSq;
	SetTagState(Tags.Condition_WhileMoving, bMoving);
	SetTagState(Tags.Condition_WhileStationary, !bMoving);

	bool bSprinting = HasTag(Tags.Condition_Sprinting);
	if (HunterCharacter)
	{
		bSprinting = HunterCharacter->GetGait() == EALSGait::Sprinting
			|| HunterCharacter->GetDesiredGait() == EALSGait::Sprinting;
	}
	SetTagState(Tags.Condition_Sprinting, bSprinting);

	// N-07 FIX: Removed the self-referential "const bool bBlocking = HasTag(...); SetTagState(..., bBlocking);"
	// pattern — reading the tag and immediately writing the same value back is a pure no-op.
	// Condition_Self_IsBlocking must be set externally by the blocking ability / combat system.

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

bool UTagManager::ComputeLowResourceState(const float CurrentValue, const float MaxValue) const
{
	return MaxValue > 0.f && (CurrentValue / MaxValue) <= TagManagerPrivate::LowResourceThreshold;
}

bool UTagManager::ComputeFullResourceState(const float CurrentValue, const float MaxValue) const
{
	return MaxValue > 0.f && CurrentValue >= (MaxValue - KINDA_SMALL_NUMBER);
}

const UHunterAttributeSet* UTagManager::GetHunterAttributeSet() const
{
	return ASC ? ASC->GetSet<UHunterAttributeSet>() : nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// N-06 FIX: Event-driven attribute refresh helpers
// ─────────────────────────────────────────────────────────────────────────────

void UTagManager::RefreshMovementConditionTags()
{
	// Called at reduced cadence (100ms) from Tick; only handles movement/sprint
	// because these require polling the character velocity / gait state.
	if (!ASC)
	{
		return;
	}

	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	const APHBaseCharacter* HunterCharacter = Cast<APHBaseCharacter>(GetOwner());
	const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());

	const bool bMoving = CharacterOwner && CharacterOwner->GetVelocity().SizeSquared2D() > TagManagerPrivate::MovementSpeedThresholdSq;
	SetTagState(Tags.Condition_WhileMoving, bMoving);
	SetTagState(Tags.Condition_WhileStationary, !bMoving);

	bool bSprinting = HasTag(Tags.Condition_Sprinting);
	if (HunterCharacter)
	{
		bSprinting = HunterCharacter->GetGait() == EALSGait::Sprinting
			|| HunterCharacter->GetDesiredGait() == EALSGait::Sprinting;
	}
	SetTagState(Tags.Condition_Sprinting, bSprinting);

	// Keep InCombat/OutOfCombat updated here too (relies on tag state, not attribute polling)
	const bool bInCombat = HasTag(Tags.Condition_TakingDamage)
		|| HasTag(Tags.Condition_DealingDamage)
		|| HasTag(Tags.Condition_RecentlyHit)
		|| HasTag(Tags.Condition_RecentlyUsedSkill);
	SetTagState(Tags.Condition_InCombat, bInCombat);
	SetTagState(Tags.Condition_OutOfCombat, !bInCombat);
}

void UTagManager::BindAttributeChangeDelegates()
{
	// N-06 FIX: Bind to GAS attribute change callbacks so resource-based condition
	// tags (low/full health, mana, stamina, arcane shield) are updated reactively
	// rather than being re-evaluated every frame in Tick.
	//
	// This uses a single lambda that calls RefreshBaseConditionTags() on any of
	// the tracked attribute changes.  The ASC throttles these internally: they only
	// fire when the value actually changes.

	if (!ASC)
	{
		return;
	}

	// OPT-TAG: Instead of calling RefreshBaseConditionTags() directly from each
	// delegate (which can fire many times per frame during combat), set a dirty
	// flag. The tick flushes it once per frame — same result, fraction of the cost.
	auto OnResourceChanged = [this](const FOnAttributeChangeData& Data)
	{
		bBaseConditionsDirty = true;
	};

	const UHunterAttributeSet* AttrSet = GetHunterAttributeSet();
	if (!AttrSet)
	{
		return;
	}

	// Health
	ASC->GetGameplayAttributeValueChangeDelegate(UHunterAttributeSet::GetHealthAttribute()).AddLambda(OnResourceChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UHunterAttributeSet::GetMaxHealthAttribute()).AddLambda(OnResourceChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UHunterAttributeSet::GetMaxEffectiveHealthAttribute()).AddLambda(OnResourceChanged);

	// Mana
	ASC->GetGameplayAttributeValueChangeDelegate(UHunterAttributeSet::GetManaAttribute()).AddLambda(OnResourceChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UHunterAttributeSet::GetMaxManaAttribute()).AddLambda(OnResourceChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UHunterAttributeSet::GetMaxEffectiveManaAttribute()).AddLambda(OnResourceChanged);

	// Stamina
	ASC->GetGameplayAttributeValueChangeDelegate(UHunterAttributeSet::GetStaminaAttribute()).AddLambda(OnResourceChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UHunterAttributeSet::GetMaxStaminaAttribute()).AddLambda(OnResourceChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UHunterAttributeSet::GetMaxEffectiveStaminaAttribute()).AddLambda(OnResourceChanged);

	// Arcane Shield
	ASC->GetGameplayAttributeValueChangeDelegate(UHunterAttributeSet::GetArcaneShieldAttribute()).AddLambda(OnResourceChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UHunterAttributeSet::GetMaxArcaneShieldAttribute()).AddLambda(OnResourceChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UHunterAttributeSet::GetMaxEffectiveArcaneShieldAttribute()).AddLambda(OnResourceChanged);

	UE_LOG(LogTagManager, Verbose, TEXT("BindAttributeChangeDelegates: bound resource delegates for owner %s"), *GetNameSafe(GetOwner()));
}

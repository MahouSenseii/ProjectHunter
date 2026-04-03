#include "Character/Component/TagManager.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Character/PHBaseCharacter.h"
#include "Character/Component/TagDebugManager.h"
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
		// ── Duplicate-component guard ─────────────────────────────────────────
		// PHBaseCharacter creates a TagManager via CreateDefaultSubobject and
		// initializes it from InitializeAbilitySystem() BEFORE Super::BeginPlay().
		// If this BeginPlay is firing with a null ASC it usually means a second
		// TagManager was added in Blueprint's Components panel on top of the C++
		// one.  Detect that case: if the owner already has an initialized TagManager
		// that is NOT this instance, we're the duplicate — bail and warn.
		{
			TArray<UTagManager*> AllManagers;
			if (GetOwner())
			{
				GetOwner()->GetComponents<UTagManager>(AllManagers);
			}

			bool bAnotherIsInitialized = false;
			for (UTagManager* Other : AllManagers)
			{
				if (Other != this && Other && Other->IsInitialized())
				{
					bAnotherIsInitialized = true;
					break;
				}
			}

			if (bAnotherIsInitialized)
			{
				UE_LOG(LogTagManager, Warning,
					TEXT("TagManager::BeginPlay: '%s' has multiple TagManager components "
					     "and this one (%s) is NOT the initialized instance. "
					     "Open '%s' in the Blueprint Editor → Components panel and remove "
					     "the extra TagManager — only the C++ one (from PHBaseCharacter) "
					     "should exist."),
					*GetNameSafe(GetOwner()),
					*GetName(),
					*GetNameSafe(GetOwner()->GetClass()));
				// Disable tick and bail — this instance should never run
				SetComponentTickEnabled(false);
				return;
			}
		}


		if (IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner()))
		{
			UAbilitySystemComponent* OwnerASC = AbilitySystemInterface->GetAbilitySystemComponent();
			if (OwnerASC)
			{
				Initialize(OwnerASC);
			}
			else
			{
				UE_LOG(LogTagManager, Verbose,
					TEXT("TagManager::BeginPlay: '%s' ASC not available yet "
					     "(PlayerState-based ASC or deferred init). "
					     "Tags will initialize via PossessedBy / OnRep_PlayerState."),
					*GetNameSafe(GetOwner()));
			}
		}
	}

#if !UE_BUILD_SHIPPING
	// If bEnableDebug is set in the Details panel before play-in-editor,
	// make sure tick is running so the "No ASC" diagnostic is visible even
	// when Initialize() was not yet called with a valid ASC.
	if (DebugManager.bEnableDebug)
	{
		SetComponentTickEnabled(true);
	}
#endif
}

void UTagManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	(void)TickType;
	(void)ThisTickFunction;

#if !UE_BUILD_SHIPPING
	// Draw BEFORE the ASC null check so the panel shows a "No ASC" diagnostic
	// even when Initialize() hasn't been called yet with a valid ASC.
	// This lets developers see immediately whether the TagManager is wired up.
	if (DebugManager.bEnableDebug)
	{
		DebugManager.DrawDebug(this, this);
	}
#endif

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
	if (InASC == NULL)
	{
		return;
	}
	ASC = InASC;

	// Enable tick when ASC is valid for normal operation.
	// Also keep tick enabled when debug is active so the panel can display a
	// "No ASC available" diagnostic even before the ASC is wired up — this
	// makes misconfigured characters immediately visible instead of silent.
#if UE_BUILD_SHIPPING
	SetComponentTickEnabled(ASC != nullptr);
#else
	SetComponentTickEnabled(ASC != nullptr || DebugManager.bEnableDebug);
#endif

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
// Debug helpers
// ─────────────────────────────────────────────────────────────────────────────

void UTagManager::SetTagDebugEnabled(bool bEnable)
{
#if UE_BUILD_SHIPPING
	DebugManager.bEnableDebug = false;
	(void)bEnable;
#else
	const bool bWasEnabled = DebugManager.bEnableDebug;
	DebugManager.bEnableDebug = bEnable;

	// Keep tick alive whenever debug is on, even when ASC hasn't been set yet.
	// When debug is turned off, fall back to ASC-driven tick control.
	SetComponentTickEnabled(bEnable || ASC != nullptr);

	// One extra DrawDebug pass with bEnableDebug=false clears stale messages.
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
	if (!ASC)
	{
		return;
	}


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

	UE_LOG(LogTagManager, Verbose,
		TEXT("BindAttributeChangeDelegates: bound resource delegates for owner %s (Health/Mana/Stamina/ArcaneShield x3 each = 12 bindings)."),
		*GetNameSafe(GetOwner()));
}
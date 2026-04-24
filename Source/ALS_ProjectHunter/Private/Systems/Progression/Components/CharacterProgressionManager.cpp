// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Progression/Components/CharacterProgressionManager.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/PHBaseCharacter.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogCharacterProgressionManager);

namespace CharacterProgressionManagerPrivate
{
	static void TrySyncPlayerLevelAttribute(UAbilitySystemComponent* ASC, int32 Level)
	{
		if (!ASC)
		{
			return;
		}

		const UHunterAttributeSet* AttributeSet = ASC->GetSet<UHunterAttributeSet>();
		if (!AttributeSet)
		{
			UE_LOG(
				LogCharacterProgressionManager,
				Verbose,
				TEXT("TrySyncPlayerLevelAttribute: Skipping level sync because the live HunterAttributeSet is not registered on ASC=%s yet"),
				*GetNameSafe(ASC));
			return;
		}

		ASC->SetNumericAttributeBase(
			UHunterAttributeSet::GetPlayerLevelAttribute(),
			static_cast<float>(FMath::Max(Level, 1)));
	}
}

UCharacterProgressionManager::UCharacterProgressionManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCharacterProgressionManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCharacterProgressionManager, Level);
	DOREPLIFETIME(UCharacterProgressionManager, CurrentXP);
	DOREPLIFETIME(UCharacterProgressionManager, UnspentStatPoints);
	DOREPLIFETIME(UCharacterProgressionManager, TotalStatPoints);
	DOREPLIFETIME(UCharacterProgressionManager, UnspentSkillPoints);
	DOREPLIFETIME_CONDITION(UCharacterProgressionManager, SpentStatPoints, COND_OwnerOnly);
}

void UCharacterProgressionManager::BeginPlay()
{
	Super::BeginPlay();

	// Cache references
	CachedASC = GetAbilitySystemComponent();
	CachedAttributeSet = GetAttributeSet();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		CharacterProgressionManagerPrivate::TrySyncPlayerLevelAttribute(CachedASC.Get(), Level);
	}

	// Rebuild the fast-lookup cache from the replicated array
	// (handles save-game loads and listen-server clients)
	RebuildSpentStatPointsCache();

	// Calculate initial XP requirement
	XPToNextLevel = GetXPForLevel(Level + 1);
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* XP CALCULATION FUNCTIONS */
/* ═══════════════════════════════════════════════════════════════════════ */

void UCharacterProgressionManager::AwardExperienceFromKill(APHBaseCharacter* KilledCharacter)
{
	if (!KilledCharacter)
	{
		UE_LOG(LogCharacterProgressionManager, Warning, TEXT("AwardExperienceFromKill: KilledCharacter is null"));
		return;
	}

	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogCharacterProgressionManager, Warning, TEXT("AwardExperienceFromKill: Called on client"));
		return;
	}

	// 1. Get base XP from killed character
	int64 BaseXP = KilledCharacter->GetXPReward();

	// 2. Get player's XP modifiers from AttributeSet
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	if (!AttrSet)
	{
		UE_LOG(LogCharacterProgressionManager, Warning, TEXT("AwardExperienceFromKill: No AttributeSet found"));
		AwardExperience(BaseXP); // Award without bonuses
		return;
	}

	float GlobalXP = AttrSet->GetGlobalXPGain();           // Party/event bonuses
	float LocalXP = AttrSet->GetLocalXPGain();             // Item bonuses
	// N-03 FIX: XPGainMultiplier attribute defaults to 0.0f in the AttributeSet, which would
	// zero all XP. Clamp to >= 1.0f so an un-initialized attribute has a neutral effect.
	float MoreXP = FMath::Max(AttrSet->GetXPGainMultiplier(), 1.0f); // Multiplicative bonuses
	float Penalty = AttrSet->GetXPPenalty();               // Penalties (e.g. death / curse penalties)

	// 3. Calculate level difference penalty
	// Positive value = player is higher level than the enemy => reduced XP
	// Negative value = enemy is higher level => no penalty (full XP)
	int32 LevelDiff = Level - KilledCharacter->GetCharacterLevel();
	float LevelPenalty = CalculateLevelPenalty(LevelDiff);

	// 4. Apply PoE2-style formula
	// Increased (additive): 1 + (Global + Local) / 100
	float IncreasedMultiplier = 1.0f + (GlobalXP + LocalXP) / 100.0f;

	// Final multiplier: Increased × More × Penalty × LevelPenalty
	float FinalMultiplier = IncreasedMultiplier * MoreXP * Penalty * LevelPenalty;

	// Calculate final XP
	int64 FinalXP = FMath::RoundToInt64(BaseXP * FinalMultiplier);
	FinalXP = FMath::Max(FinalXP, 1LL); // Minimum 1 XP

	// 5. Award XP
	CurrentXP += FinalXP;
	CheckForLevelUp();

	// 6. Log for debugging
	UE_LOG(LogCharacterProgressionManager, Log, TEXT("XP Awarded: %lld (Base: %lld, Increased: %.2fx, More: %.2fx, Penalty: %.2fx, Level Penalty: %.2fx)"),
		FinalXP, BaseXP, IncreasedMultiplier, MoreXP, Penalty, LevelPenalty);

	// 7. Broadcast event
	OnXPGained.Broadcast(FinalXP, BaseXP, FinalMultiplier);
}

void UCharacterProgressionManager::AwardExperience(int64 Amount)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	if (Amount <= 0)
	{
		return;
	}

	// Get XP bonuses (but no level penalty)
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	float GlobalXP = AttrSet ? AttrSet->GetGlobalXPGain() : 0.0f;
	float LocalXP  = AttrSet ? AttrSet->GetLocalXPGain()  : 0.0f;
	// N-03 FIX: Clamp MoreXP >= 1.0f — attribute defaults to 0.0f which would zero all XP
	float MoreXP   = FMath::Max(AttrSet ? AttrSet->GetXPGainMultiplier() : 1.0f, 1.0f);
	// BUG FIX: XPPenalty (death penalty, curses, etc.) must apply to ALL XP awards,
	// not just kill XP. AwardExperienceFromKill already multiplied by Penalty; this
	// direct-award path was skipping it, making penalties irrelevant for quest/event XP.
	// XPPenalty defaults to 1.0f (neutral), so this has zero effect when no penalty is active.
	float Penalty  = FMath::Max(AttrSet ? AttrSet->GetXPPenalty() : 1.0f, 0.0f);

	// Calculate final XP
	float IncreasedMultiplier = 1.0f + (GlobalXP + LocalXP) / 100.0f;
	float FinalMultiplier = IncreasedMultiplier * MoreXP * Penalty;

	int64 FinalXP = FMath::RoundToInt64(Amount * FinalMultiplier);
	FinalXP = FMath::Max(FinalXP, 1LL);

	// Award XP
	CurrentXP += FinalXP;
	CheckForLevelUp();

	// Broadcast event
	OnXPGained.Broadcast(FinalXP, Amount, FinalMultiplier);
}

float UCharacterProgressionManager::CalculateLevelPenalty(int32 LevelDifference) const
{
	// PoE2-style level penalty curve
	if (LevelDifference <= -5)
	{
		// Enemy 5+ levels higher: No penalty (100%)
		return 1.0f;
	}
	else if (LevelDifference <= 5)
	{
		// Within ±5 levels: No penalty (100%)
		return 1.0f;
	}
	else if (LevelDifference <= 10)
	{
		// 6-10 levels lower: 20% penalty
		return 0.8f;
	}
	else if (LevelDifference <= 20)
	{
		// 11-20 levels lower: 50% penalty
		return 0.5f;
	}
	else if (LevelDifference <= 30)
	{
		// 21-30 levels lower: 75% penalty
		return 0.25f;
	}
	else
	{
		// 31+ levels lower: 95% penalty
		return 0.05f;
	}
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* LEVELING FUNCTIONS */
/* ═══════════════════════════════════════════════════════════════════════ */

void UCharacterProgressionManager::LevelUp()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	if (Level >= MaxLevel)
	{
		UE_LOG(LogCharacterProgressionManager, Warning, TEXT("LevelUp: Already at max level (%d)"), MaxLevel);
		return;
	}

	Level++;
	OnLevelUpInternal();

	UE_LOG(LogCharacterProgressionManager, Log, TEXT("Level Up! New Level: %d"), Level);
}

void UCharacterProgressionManager::CheckForLevelUp()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	// Check if we've reached the XP threshold
	while (CurrentXP >= XPToNextLevel && Level < MaxLevel)
	{
		// Deduct XP for this level
		CurrentXP -= XPToNextLevel;

		// Level up
		Level++;
		OnLevelUpInternal();

		// Calculate next level requirement
		XPToNextLevel = GetXPForLevel(Level + 1);

		UE_LOG(LogCharacterProgressionManager, Log, TEXT("Level Up! New Level: %d, XP to next: %lld"), Level, XPToNextLevel);
	}

	// If at max level, cap XP
	if (Level >= MaxLevel)
	{
		CurrentXP = 0;
		XPToNextLevel = 0;
	}
}

int64 UCharacterProgressionManager::GetXPForLevel(int32 TargetLevel) const
{
	if (TargetLevel <= 1)
	{
		return 0;
	}

	return CalculateXPForLevel(TargetLevel);
}

float UCharacterProgressionManager::GetXPProgressPercent() const
{
	if (XPToNextLevel == 0)
	{
		return 1.0f; // Max level
	}

	return static_cast<float>(CurrentXP) / static_cast<float>(XPToNextLevel);
}

float UCharacterProgressionManager::GetTotalXPGainPercent() const
{
	UHunterAttributeSet* AttrSet = GetAttributeSet();
	if (!AttrSet)
	{
		return 0.0f;
	}

	return AttrSet->GetGlobalXPGain() + AttrSet->GetLocalXPGain();
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* STAT POINT FUNCTIONS */
/* ═══════════════════════════════════════════════════════════════════════ */

bool UCharacterProgressionManager::SpendStatPoint(FName AttributeName)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogCharacterProgressionManager, Warning, TEXT("SpendStatPoint: Called on client"));
		return false;
	}

	// Check if we have unspent points
	if (UnspentStatPoints <= 0)
	{
		UE_LOG(LogCharacterProgressionManager, Warning, TEXT("SpendStatPoint: No unspent stat points"));
		return false;
	}

	// Validate attribute name
	if (AttributeName.IsNone())
	{
		UE_LOG(LogCharacterProgressionManager, Warning, TEXT("SpendStatPoint: Invalid attribute name"));
		return false;
	}

	if (!ApplyStatPointToAttribute(AttributeName))
	{
		UE_LOG(LogCharacterProgressionManager, Warning,
			TEXT("SpendStatPoint: Could not apply stat point to '%s'; point was not spent."),
			*AttributeName.ToString());
		return false;
	}

	// Deduct stat point
	UnspentStatPoints--;

	// --- Update replicated TArray ---
	bool bFound = false;
	for (FStatPointSpending& Spending : SpentStatPoints)
	{
		if (Spending.AttributeName == AttributeName)
		{
			Spending.PointsSpent++;
			bFound = true;
			break;
		}
	}
	if (!bFound)
	{
		SpentStatPoints.Add(FStatPointSpending(AttributeName, 1));
	}

	// --- Update fast-lookup TMap cache ---
	int32& CachedCount = SpentStatPointsCache.FindOrAdd(AttributeName, 0);
	CachedCount++;

	// Broadcast event
	OnStatPointSpent.Broadcast(AttributeName, UnspentStatPoints);

	UE_LOG(LogCharacterProgressionManager, Log, TEXT("Stat Point Spent: %s (Remaining: %d)"), *AttributeName.ToString(), UnspentStatPoints);

	return true;
}

bool UCharacterProgressionManager::ResetStatPoints(int32 Cost)
{
	if (!GetOwner()->HasAuthority())
	{
		return false;
	}

	// TODO: Check gold/currency for cost
	// if (PlayerCurrency < Cost) return false;

	// Remove all stat points from attributes
	for (const FStatPointSpending& Spending : SpentStatPoints)
	{
		RemoveStatPointFromAttribute(Spending.AttributeName, Spending.PointsSpent);
	}

	// Refund all stat points
	UnspentStatPoints = TotalStatPoints;
	SpentStatPoints.Empty();
	SpentStatPointsCache.Empty();

	UE_LOG(LogCharacterProgressionManager, Log, TEXT("Stat Points Reset! Refunded: %d points"), TotalStatPoints);

	return true;
}

int32 UCharacterProgressionManager::GetStatPointsSpentOn(FName AttributeName) const
{
	// O(1) lookup via the fast-lookup cache (rebuilt from replicated array on BeginPlay/OnRep)
	if (const int32* Found = SpentStatPointsCache.Find(AttributeName))
	{
		return *Found;
	}
	return 0;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* INTERNAL FUNCTIONS */
/* ═══════════════════════════════════════════════════════════════════════ */

int64 UCharacterProgressionManager::CalculateXPForLevel(int32 TargetLevel) const
{
	// Exponential curve: BaseXP * Level^Exponent
	// Example: 5 * 50^1.3 = ~800 XP for level 50
	float XP = BaseXPPerLevel * FMath::Pow(static_cast<float>(TargetLevel), XPScalingExponent);
	return FMath::RoundToInt64(XP);
}

void UCharacterProgressionManager::OnLevelUpInternal()
{
	// Award stat points
	int32 StatPointsAwarded = StatPointsPerLevel;
	UnspentStatPoints += StatPointsAwarded;
	TotalStatPoints += StatPointsAwarded;

	// Award skill points
	int32 SkillPointsAwarded = SkillPointsPerLevel;
	UnspentSkillPoints += SkillPointsAwarded;

	// Update XP requirement
	XPToNextLevel = GetXPForLevel(Level + 1);

	CharacterProgressionManagerPrivate::TrySyncPlayerLevelAttribute(GetAbilitySystemComponent(), Level);

	// Broadcast event
	OnLevelUp.Broadcast(Level, StatPointsAwarded, SkillPointsAwarded);
}

// ─────────────────────────────────────────────────────────────────────────────
// Q-2: Single authoritative attribute-name → FGameplayAttribute mapping.
//       Referenced by both ApplyStatPointToAttribute and RemoveStatPointFromAttribute
//       so adding a new primary stat only requires editing this one function.
// ─────────────────────────────────────────────────────────────────────────────
FGameplayAttribute UCharacterProgressionManager::GetAttributeForStatName(FName StatName)
{
	if (StatName == "Strength")     return UHunterAttributeSet::GetStrengthAttribute();
	if (StatName == "Intelligence") return UHunterAttributeSet::GetIntelligenceAttribute();
	if (StatName == "Dexterity")    return UHunterAttributeSet::GetDexterityAttribute();
	if (StatName == "Endurance")    return UHunterAttributeSet::GetEnduranceAttribute();
	if (StatName == "Affliction")   return UHunterAttributeSet::GetAfflictionAttribute();
	if (StatName == "Luck")         return UHunterAttributeSet::GetLuckAttribute();
	if (StatName == "Covenant")     return UHunterAttributeSet::GetCovenantAttribute();

	return FGameplayAttribute{};
}

// ─────────────────────────────────────────────────────────────────────────────
// C-2 / C-3 fix: use a pre-configured Blueprint GE class (no runtime NewObject).
//   StatPointGEClasses["Strength"] should point to a Blueprint GE that is
//   Infinite with a single +1 Additive modifier on the Strength attribute.
//   We store every FActiveGameplayEffectHandle so respec can remove them cleanly.
// ─────────────────────────────────────────────────────────────────────────────
bool UCharacterProgressionManager::ApplyStatPointToAttribute(FName AttributeName)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogCharacterProgressionManager, Error, TEXT("ApplyStatPointToAttribute: ASC is null"));
		return false;
	}

	// Validate the attribute name maps to a known FGameplayAttribute
	const FGameplayAttribute Attribute = GetAttributeForStatName(AttributeName);
	if (!Attribute.IsValid())
	{
		UE_LOG(LogCharacterProgressionManager, Error,
			TEXT("ApplyStatPointToAttribute: Unknown primary attribute '%s'"), *AttributeName.ToString());
		return false;
	}

	// Look up the designer-configured GE class for this attribute
	const TSubclassOf<UGameplayEffect>* GEClassPtr = StatPointGEClasses.Find(AttributeName);
	if (!GEClassPtr || !(*GEClassPtr))
	{
		UE_LOG(LogCharacterProgressionManager, Warning,
			TEXT("ApplyStatPointToAttribute: No GE class configured for attribute '%s'. "
				 "Add an entry to StatPointGEClasses in the Blueprint defaults."),
			*AttributeName.ToString());
		return false;
	}

	// Apply the Infinite GE; store its handle for later removal (respec)
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(GetOwner());

	const FActiveGameplayEffectHandle Handle =
		ASC->ApplyGameplayEffectToSelf((*GEClassPtr)->GetDefaultObject<UGameplayEffect>(),
									   1.0f, Context);

	if (Handle.IsValid())
	{
		StatPointGEHandles.FindOrAdd(AttributeName).Add(Handle);
		UE_LOG(LogCharacterProgressionManager, Log,
			TEXT("ApplyStatPointToAttribute: Applied +1 to '%s' (handle %s)"),
			*AttributeName.ToString(), *Handle.ToString());
		return true;
	}
	else
	{
		UE_LOG(LogCharacterProgressionManager, Error,
			TEXT("ApplyStatPointToAttribute: GE application failed for '%s'"), *AttributeName.ToString());
	}

	return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// C-2 fix: remove exactly PointsToRemove stored handles — no counter-GE stacking.
// ─────────────────────────────────────────────────────────────────────────────
void UCharacterProgressionManager::RemoveStatPointFromAttribute(FName AttributeName, int32 PointsToRemove)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	TArray<FActiveGameplayEffectHandle>* Handles = StatPointGEHandles.Find(AttributeName);
	if (!Handles || Handles->Num() == 0)
	{
		// No handles tracked — nothing to remove (e.g. first session without saved handles)
		UE_LOG(LogCharacterProgressionManager, Warning,
			TEXT("RemoveStatPointFromAttribute: No tracked GE handles for '%s'. "
				 "Attribute will not be adjusted."), *AttributeName.ToString());
		return;
	}

	const int32 ToRemove = FMath::Min(PointsToRemove, Handles->Num());
	for (int32 i = 0; i < ToRemove; ++i)
	{
		const FActiveGameplayEffectHandle Handle = Handles->Pop(EAllowShrinking::No);
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	UE_LOG(LogCharacterProgressionManager, Log,
		TEXT("RemoveStatPointFromAttribute: Removed %d point(s) from '%s'"),
		ToRemove, *AttributeName.ToString());
}

// ─────────────────────────────────────────────────────────────────────────────
// P-4: Rebuild fast-lookup TMap from the replicated TArray.
// ─────────────────────────────────────────────────────────────────────────────
void UCharacterProgressionManager::RebuildSpentStatPointsCache()
{
	SpentStatPointsCache.Empty(SpentStatPoints.Num());
	for (const FStatPointSpending& Entry : SpentStatPoints)
	{
		SpentStatPointsCache.Add(Entry.AttributeName, Entry.PointsSpent);
	}
}

UAbilitySystemComponent* UCharacterProgressionManager::GetAbilitySystemComponent() const
{
	if (CachedASC)
	{
		return CachedASC;
	}

	if (AActor* Owner = GetOwner())
	{
		return Owner->FindComponentByClass<UAbilitySystemComponent>();
	}

	return nullptr;
}

UHunterAttributeSet* UCharacterProgressionManager::GetAttributeSet() const
{
	if (CachedAttributeSet)
	{
		return CachedAttributeSet;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		return const_cast<UHunterAttributeSet*>(ASC->GetSet<UHunterAttributeSet>());
	}

	return nullptr;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* REPLICATION CALLBACKS */
/* ═══════════════════════════════════════════════════════════════════════ */

void UCharacterProgressionManager::OnRep_Level(int32 OldLevel)
{
	// Update XP requirement for UI
	XPToNextLevel = GetXPForLevel(Level + 1);

	UE_LOG(LogCharacterProgressionManager, Log, TEXT("OnRep_Level: %d -> %d"), OldLevel, Level);

	// Can trigger UI updates here
}

void UCharacterProgressionManager::OnRep_CurrentXP(int64 OldXP)
{
	UE_LOG(LogCharacterProgressionManager, Log, TEXT("OnRep_CurrentXP: %lld -> %lld (Progress: %.1f%%)"),
		OldXP, CurrentXP, GetXPProgressPercent() * 100.0f);

	// Can trigger UI updates here
}

void UCharacterProgressionManager::OnRep_SpentStatPoints()
{
	// SpentStatPoints (TArray) has been replicated to this client.
	// Rebuild the fast-lookup TMap cache so GetStatPointsSpentOn() stays O(1).
	RebuildSpentStatPointsCache();

	UE_LOG(LogCharacterProgressionManager, Log,
		TEXT("OnRep_SpentStatPoints: cache rebuilt (%d entries)"), SpentStatPoints.Num());

	// Can trigger UI updates (stat allocation screen refresh) here
}

#include "Progression/Components/CharacterProgressionManager.h"
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

	CachedASC = GetAbilitySystemComponent();
	CachedAttributeSet = GetAttributeSet();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		CharacterProgressionManagerPrivate::TrySyncPlayerLevelAttribute(CachedASC.Get(), Level);
	}

	RebuildSpentStatPointsCache();

	XPToNextLevel = GetXPForLevel(Level + 1);
}

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

	int64 BaseXP = KilledCharacter->GetXPReward();

	UHunterAttributeSet* AttrSet = GetAttributeSet();
	if (!AttrSet)
	{
		UE_LOG(LogCharacterProgressionManager, Warning, TEXT("AwardExperienceFromKill: No AttributeSet found"));
		AwardExperience(BaseXP);
		return;
	}

	float GlobalXP = AttrSet->GetGlobalXPGain();
	float LocalXP = AttrSet->GetLocalXPGain();
	// XPGainMultiplier defaults to 0.0f in the AttributeSet; clamp to >= 1.0f for neutral effect.
	float MoreXP = FMath::Max(AttrSet->GetXPGainMultiplier(), 1.0f);
	float Penalty = AttrSet->GetXPPenalty();

	int32 LevelDiff = Level - KilledCharacter->GetCharacterLevel();
	float LevelPenalty = CalculateLevelPenalty(LevelDiff);

	float IncreasedMultiplier = 1.0f + (GlobalXP + LocalXP) / 100.0f;
	float FinalMultiplier = IncreasedMultiplier * MoreXP * Penalty * LevelPenalty;

	int64 FinalXP = FMath::RoundToInt64(BaseXP * FinalMultiplier);
	FinalXP = FMath::Max(FinalXP, 1LL);

	CurrentXP += FinalXP;
	CheckForLevelUp();

	UE_LOG(LogCharacterProgressionManager, Log, TEXT("XP Awarded: %lld (Base: %lld, Increased: %.2fx, More: %.2fx, Penalty: %.2fx, Level Penalty: %.2fx)"),
		FinalXP, BaseXP, IncreasedMultiplier, MoreXP, Penalty, LevelPenalty);

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

	UHunterAttributeSet* AttrSet = GetAttributeSet();
	float GlobalXP = AttrSet ? AttrSet->GetGlobalXPGain() : 0.0f;
	float LocalXP  = AttrSet ? AttrSet->GetLocalXPGain()  : 0.0f;
	float MoreXP   = FMath::Max(AttrSet ? AttrSet->GetXPGainMultiplier() : 1.0f, 1.0f);
	// XPPenalty must apply to all XP awards (quest/event XP previously skipped it).
	// Defaults to 1.0f so there is no effect when no penalty is active.
	float Penalty  = FMath::Max(AttrSet ? AttrSet->GetXPPenalty() : 1.0f, 0.0f);

	float IncreasedMultiplier = 1.0f + (GlobalXP + LocalXP) / 100.0f;
	float FinalMultiplier = IncreasedMultiplier * MoreXP * Penalty;

	int64 FinalXP = FMath::RoundToInt64(Amount * FinalMultiplier);
	FinalXP = FMath::Max(FinalXP, 1LL);

	CurrentXP += FinalXP;
	CheckForLevelUp();

	OnXPGained.Broadcast(FinalXP, Amount, FinalMultiplier);
}

float UCharacterProgressionManager::CalculateLevelPenalty(int32 LevelDifference) const
{
	if (LevelDifference <= 5)  { return 1.0f;  }  // within ±5 levels or enemy is higher
	if (LevelDifference <= 10) { return 0.8f;  }  // 6-10 levels lower: 20% penalty
	if (LevelDifference <= 20) { return 0.5f;  }  // 11-20 levels lower: 50% penalty
	if (LevelDifference <= 30) { return 0.25f; }  // 21-30 levels lower: 75% penalty
	return 0.05f;                                  // 31+ levels lower: 95% penalty
}

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

	while (CurrentXP >= XPToNextLevel && Level < MaxLevel)
	{
		CurrentXP -= XPToNextLevel;
		Level++;
		OnLevelUpInternal();
		XPToNextLevel = GetXPForLevel(Level + 1);

		UE_LOG(LogCharacterProgressionManager, Log, TEXT("Level Up! New Level: %d, XP to next: %lld"), Level, XPToNextLevel);
	}

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

bool UCharacterProgressionManager::SpendStatPoint(FName AttributeName)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogCharacterProgressionManager, Warning, TEXT("SpendStatPoint: Called on client"));
		return false;
	}

	if (UnspentStatPoints <= 0)
	{
		UE_LOG(LogCharacterProgressionManager, Warning, TEXT("SpendStatPoint: No unspent stat points"));
		return false;
	}

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

	UnspentStatPoints--;

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

	int32& CachedCount = SpentStatPointsCache.FindOrAdd(AttributeName, 0);
	CachedCount++;

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

	for (const FStatPointSpending& Spending : SpentStatPoints)
	{
		RemoveStatPointFromAttribute(Spending.AttributeName, Spending.PointsSpent);
	}

	UnspentStatPoints = TotalStatPoints;
	SpentStatPoints.Empty();
	SpentStatPointsCache.Empty();

	UE_LOG(LogCharacterProgressionManager, Log, TEXT("Stat Points Reset! Refunded: %d points"), TotalStatPoints);

	return true;
}

int32 UCharacterProgressionManager::GetStatPointsSpentOn(FName AttributeName) const
{
	if (const int32* Found = SpentStatPointsCache.Find(AttributeName))
	{
		return *Found;
	}
	return 0;
}

int64 UCharacterProgressionManager::CalculateXPForLevel(int32 TargetLevel) const
{
	float XP = BaseXPPerLevel * FMath::Pow(static_cast<float>(TargetLevel), XPScalingExponent);
	return FMath::RoundToInt64(XP);
}

void UCharacterProgressionManager::OnLevelUpInternal()
{
	int32 StatPointsAwarded = StatPointsPerLevel;
	UnspentStatPoints += StatPointsAwarded;
	TotalStatPoints += StatPointsAwarded;

	int32 SkillPointsAwarded = SkillPointsPerLevel;
	UnspentSkillPoints += SkillPointsAwarded;

	XPToNextLevel = GetXPForLevel(Level + 1);

	CharacterProgressionManagerPrivate::TrySyncPlayerLevelAttribute(GetAbilitySystemComponent(), Level);

	OnLevelUp.Broadcast(Level, StatPointsAwarded, SkillPointsAwarded);
}

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

bool UCharacterProgressionManager::ApplyStatPointToAttribute(FName AttributeName)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogCharacterProgressionManager, Error, TEXT("ApplyStatPointToAttribute: ASC is null"));
		return false;
	}

	const FGameplayAttribute Attribute = GetAttributeForStatName(AttributeName);
	if (!Attribute.IsValid())
	{
		UE_LOG(LogCharacterProgressionManager, Error,
			TEXT("ApplyStatPointToAttribute: Unknown primary attribute '%s'"), *AttributeName.ToString());
		return false;
	}

	const TSubclassOf<UGameplayEffect>* GEClassPtr = StatPointGEClasses.Find(AttributeName);
	if (!GEClassPtr || !(*GEClassPtr))
	{
		UE_LOG(LogCharacterProgressionManager, Warning,
			TEXT("ApplyStatPointToAttribute: No GE class configured for attribute '%s'. "
				 "Add an entry to StatPointGEClasses in the Blueprint defaults."),
			*AttributeName.ToString());
		return false;
	}

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

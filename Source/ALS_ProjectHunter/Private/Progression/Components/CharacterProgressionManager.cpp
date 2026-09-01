#include "Progression/Components/CharacterProgressionManager.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "Character/PHBaseCharacter.h"
#include "Curves/CurveFloat.h"
#include "Net/UnrealNetwork.h"
#include "Progression/Helpers/ProgressionAbilityHelper.h"
#include "Progression/Helpers/ProgressionStatPointHelper.h"
#include "Stats/Components/StatsManager.h"
#include "Stats/Data/BaseStatsData.h"
#include "Progression/Library/FunctionLibraries/ProgressionFunctionLibrary.h"

DEFINE_LOG_CATEGORY(LogCharacterProgressionManager);

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
	DOREPLIFETIME(UCharacterProgressionManager, UnspentPassivePoints);
	DOREPLIFETIME(UCharacterProgressionManager, TotalPassivePoints);
	DOREPLIFETIME_CONDITION(UCharacterProgressionManager, SpentStatPoints, COND_OwnerOnly);
}

void UCharacterProgressionManager::BeginPlay()
{
	Super::BeginPlay();

	CachedASC = GetAbilitySystemComponent();
	CachedAttributeSet = GetAttributeSet();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SeedStartingLevelFromStatsData();
	}

	RebuildSpentStatPointsCache();

	XPToNextLevel = IsAtMaxLevel() ? 0 : GetXPForLevel(Level + 1);
}

void UCharacterProgressionManager::SeedStartingLevelFromStatsData()
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	const int32 PreviousLevel = Level;
	const int64 PreviousXPToNextLevel = XPToNextLevel;
	if (bSeedStartingLevelFromStatsData && !bHasSeededStartingLevel)
	{
		const UStatsManager* Stats = Owner->FindComponentByClass<UStatsManager>();
		const UBaseStatsData* Data = Stats ? Stats->GetStatsDataAsset() : nullptr;
		if (Data)
		{
			float AuthoredLevel = 0.0f;
			if (Data->GetStatValue(TEXT("PlayerLevel"), AuthoredLevel))
			{
				const int32 SeededLevel = HasLevelCap()
				? FMath::Clamp(FMath::RoundToInt(AuthoredLevel), MinLevel, MaxLevel)
				: FMath::Max(FMath::RoundToInt(AuthoredLevel), MinLevel);
				if (SeededLevel != Level)
				{
					UE_LOG(LogCharacterProgressionManager, Log,
						TEXT("SeedStartingLevelFromStatsData: %s starting at level %d from %s (component default was %d)."),
						*GetNameSafe(Owner), SeededLevel, *GetNameSafe(Data), Level);
				}
				Level = SeededLevel;
			}
			else
			{
				UE_LOG(LogCharacterProgressionManager, Warning,
					TEXT("SeedStartingLevelFromStatsData: %s does not author PlayerLevel, so %s keeps its component "
					     "default of %d. Tick Override Value on PlayerLevel to control the starting level from data."),
					*GetNameSafe(Data), *GetNameSafe(Owner), Level);
			}
			bHasSeededStartingLevel = true;
		}
		else
		{
			UE_LOG(LogCharacterProgressionManager, Verbose,
				TEXT("SeedStartingLevelFromStatsData: %s has no stats data yet (StatsManager=%s); will retry after stats init."),
				*GetNameSafe(Owner), *GetNameSafe(Stats));
		}
	}

	// Stats can initialize after BeginPlay or again after a level-up. Keep both
	// consumers current even when data seeding is disabled or already complete.
	RefreshLevelState();
	if (PreviousLevel != Level || PreviousXPToNextLevel != XPToNextLevel)
	{
		OnProgressionChanged.Broadcast();
	}
}

void UCharacterProgressionManager::RefreshLevelState()
{
	XPToNextLevel = IsAtMaxLevel() ? 0 : GetXPForLevel(Level + 1);
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		FProgressionAbilityHelper::TrySyncPlayerLevelAttribute(GetAbilitySystemComponent(), Level);
	}
}

void UCharacterProgressionManager::AwardExperienceFromKill(APHBaseCharacter* KilledCharacter)
{
	if (!KilledCharacter)
	{
		UE_LOG(LogCharacterProgressionManager, Warning, TEXT("AwardExperienceFromKill: KilledCharacter is null"));
		return;
	}

	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		UE_LOG(LogCharacterProgressionManager, Warning, TEXT("AwardExperienceFromKill: Called without authority"));
		return;
	}

	const int64 BaseXP = KilledCharacter->GetXPReward();

	UHunterAttributeSet* AttrSet = GetAttributeSet();
	if (!AttrSet)
	{
		UE_LOG(LogCharacterProgressionManager, Warning, TEXT("AwardExperienceFromKill: No AttributeSet found"));
		AwardExperience(BaseXP);
		return;
	}

	const float GlobalXP = AttrSet->GetGlobalXPGain();
	const float LocalXP = AttrSet->GetLocalXPGain();
	const float MoreXP = AttrSet->GetXPGainMultiplier();
	const float Penalty = AttrSet->GetXPPenalty();
	const int32 LevelDiff = Level - KilledCharacter->GetCharacterLevel();
	const float LevelPenalty = UProgressionFunctionLibrary::CalculateLevelPenalty(LevelDiff);
	const float IncreasedMultiplier = 1.0f + (GlobalXP + LocalXP) / 100.0f;
	const float FinalMultiplier = UProgressionFunctionLibrary::CalculateXPMultiplier(
		GlobalXP,
		LocalXP,
		MoreXP,
		Penalty,
		LevelPenalty);
	const int64 FinalXP = UProgressionFunctionLibrary::CalculateFinalXP(BaseXP, FinalMultiplier);

	CurrentXP += FinalXP;
	CheckForLevelUp();

	UE_LOG(
		LogCharacterProgressionManager,
		Log,
		TEXT("XP Awarded: %lld (Base: %lld, Increased: %.2fx, More: %.2fx, Penalty: %.2fx, Level Penalty: %.2fx)"),
		FinalXP,
		BaseXP,
		IncreasedMultiplier,
		FMath::Max(MoreXP, 0.01f),
		Penalty,
		LevelPenalty);

	OnXPGained.Broadcast(FinalXP, BaseXP, FinalMultiplier);
	OnProgressionChanged.Broadcast();
}

void UCharacterProgressionManager::AwardExperience(const int64 Amount)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (Amount <= 0)
	{
		return;
	}

	UHunterAttributeSet* AttrSet = GetAttributeSet();
	const float GlobalXP = AttrSet ? AttrSet->GetGlobalXPGain() : 0.0f;
	const float LocalXP = AttrSet ? AttrSet->GetLocalXPGain() : 0.0f;
	const float MoreXP = AttrSet ? AttrSet->GetXPGainMultiplier() : 1.0f;
	const float Penalty = FMath::Max(AttrSet ? AttrSet->GetXPPenalty() : 1.0f, 0.0f);
	const float FinalMultiplier = UProgressionFunctionLibrary::CalculateXPMultiplier(
		GlobalXP,
		LocalXP,
		MoreXP,
		Penalty);
	const int64 FinalXP = UProgressionFunctionLibrary::CalculateFinalXP(Amount, FinalMultiplier);

	CurrentXP += FinalXP;
	CheckForLevelUp();

	OnXPGained.Broadcast(FinalXP, Amount, FinalMultiplier);
	OnProgressionChanged.Broadcast();
}

float UCharacterProgressionManager::CalculateLevelPenalty(const int32 LevelDifference) const
{
	return UProgressionFunctionLibrary::CalculateLevelPenalty(LevelDifference);
}

void UCharacterProgressionManager::LevelUp()
{
#if UE_BUILD_SHIPPING
	UE_LOG(LogCharacterProgressionManager, Warning, TEXT("DebugGrantLevel is disabled in shipping builds."));
	return;
#else
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (IsAtMaxLevel())
	{
		UE_LOG(LogCharacterProgressionManager, Warning, TEXT("LevelUp: Already at max level (%d)"), MaxLevel);
		return;
	}

	Level++;
	OnLevelUpInternal();

	UE_LOG(LogCharacterProgressionManager, Log, TEXT("Level Up! New Level: %d"), Level);
#endif
}

void UCharacterProgressionManager::CheckForLevelUp()
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	// XPToNextLevel guards this loop as much as the cap does: with no cap, a curve that returns
	// zero or less for the next level would spin here forever instead of levelling once.
	while (XPToNextLevel > 0 && CurrentXP >= XPToNextLevel && !IsAtMaxLevel())
	{
		CurrentXP -= XPToNextLevel;
		Level++;
		OnLevelUpInternal();
		XPToNextLevel = GetXPForLevel(Level + 1);

		UE_LOG(LogCharacterProgressionManager, Log, TEXT("Level Up! New Level: %d, XP to next: %lld"), Level, XPToNextLevel);
	}

	if (IsAtMaxLevel())
	{
		CurrentXP = 0;
		XPToNextLevel = 0;
	}
}

int64 UCharacterProgressionManager::GetXPForLevel(const int32 TargetLevel) const
{
	if (XPRequirementCurve)
	{
		return FMath::Max<int64>(1, FMath::RoundToInt64(XPRequirementCurve->GetFloatValue(TargetLevel)));
	}
	return UProgressionFunctionLibrary::GetXPForLevel(TargetLevel, BaseXPPerLevel, XPScalingExponent);
}

float UCharacterProgressionManager::GetXPProgressPercent() const
{
	return UProgressionFunctionLibrary::GetXPProgressPercent(CurrentXP, XPToNextLevel);
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

bool UCharacterProgressionManager::SpendStatPoint(const FName AttributeName)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		UE_LOG(LogCharacterProgressionManager, Warning, TEXT("SpendStatPoint: Called without authority"));
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
		UE_LOG(
			LogCharacterProgressionManager,
			Warning,
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
	OnProgressionChanged.Broadcast();

	UE_LOG(LogCharacterProgressionManager, Log, TEXT("Stat Point Spent: %s (Remaining: %d)"), *AttributeName.ToString(), UnspentStatPoints);

	return true;
}

bool UCharacterProgressionManager::ResetStatPoints(const int32 Cost)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return false;
	}

	if (SpentStatPoints.IsEmpty())
	{
		return true;
	}

	if (Cost < 0 || !SpendRespecCurrency(Cost))
	{
		UE_LOG(LogCharacterProgressionManager, Warning,
			TEXT("ResetStatPoints: Could not spend respec cost %d; reset cancelled."), Cost);
		return false;
	}

	for (const FStatPointSpending& Spending : SpentStatPoints)
	{
		RemoveStatPointFromAttribute(Spending.AttributeName, Spending.PointsSpent);
	}

	UnspentStatPoints = TotalStatPoints;
	SpentStatPoints.Empty();
	SpentStatPointsCache.Empty();
	OnProgressionChanged.Broadcast();

	UE_LOG(LogCharacterProgressionManager, Log, TEXT("Stat Points Reset! Refunded: %d points"), TotalStatPoints);

	return true;
}

bool UCharacterProgressionManager::SpendRespecCurrency_Implementation(const int32 Cost)
{
	return Cost == 0;
}

bool UCharacterProgressionManager::SpendSkillPoints(const int32 Amount)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || Amount <= 0 || UnspentSkillPoints < Amount)
	{
		return false;
	}

	UnspentSkillPoints -= Amount;
	OnProgressionChanged.Broadcast();
	return true;
}

bool UCharacterProgressionManager::SpendPassivePoints(const int32 Amount)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || Amount <= 0 || UnspentPassivePoints < Amount)
	{
		return false;
	}

	UnspentPassivePoints -= Amount;
	OnProgressionChanged.Broadcast();
	return true;
}

int32 UCharacterProgressionManager::GetStatPointsSpentOn(const FName AttributeName) const
{
	if (const int32* Found = SpentStatPointsCache.Find(AttributeName))
	{
		return *Found;
	}

	return 0;
}

void UCharacterProgressionManager::OnLevelUpInternal()
{
	// A delayed stats asset must not replace levels earned before it arrived.
	bHasSeededStartingLevel = true;

	const int32 StatPointsAwarded = StatPointsPerLevel;
	UnspentStatPoints += StatPointsAwarded;
	TotalStatPoints += StatPointsAwarded;

	const int32 SkillPointsAwarded = SkillPointsPerLevel;
	UnspentSkillPoints += SkillPointsAwarded;

	// Not carried on OnLevelUp: that delegate is a Blueprint contract with three parameters and
	// widening it would break every existing binding. Listeners read the counter, or take
	// OnProgressionChanged.
	UnspentPassivePoints += PassivePointsPerLevel;
	TotalPassivePoints += PassivePointsPerLevel;

	RefreshLevelState();

	OnLevelUp.Broadcast(Level, StatPointsAwarded, SkillPointsAwarded);
	OnProgressionChanged.Broadcast();
}

bool UCharacterProgressionManager::ApplyStatPointToAttribute(const FName AttributeName)
{
	return FProgressionStatPointHelper::ApplyStatPointToAttribute(*this, AttributeName);
}

void UCharacterProgressionManager::RemoveStatPointFromAttribute(const FName AttributeName, const int32 PointsToRemove)
{
	FProgressionStatPointHelper::RemoveStatPointFromAttribute(*this, AttributeName, PointsToRemove);
}

void UCharacterProgressionManager::RebuildSpentStatPointsCache()
{
	FProgressionStatPointHelper::RebuildSpentStatPointsCache(*this);
}

UAbilitySystemComponent* UCharacterProgressionManager::GetAbilitySystemComponent() const
{
	return FProgressionAbilityHelper::GetAbilitySystemComponent(*this);
}

UHunterAttributeSet* UCharacterProgressionManager::GetAttributeSet() const
{
	return FProgressionAbilityHelper::GetAttributeSet(*this);
}

void UCharacterProgressionManager::OnRep_Level(const int32 OldLevel)
{
	XPToNextLevel = IsAtMaxLevel() ? 0 : GetXPForLevel(Level + 1);
	if (Level > OldLevel)
	{
		const int32 LevelsGained = Level - OldLevel;
		OnLevelUp.Broadcast(Level, LevelsGained * StatPointsPerLevel, LevelsGained * SkillPointsPerLevel);
	}
	OnProgressionChanged.Broadcast();

	UE_LOG(LogCharacterProgressionManager, Log, TEXT("OnRep_Level: %d -> %d"), OldLevel, Level);
}

void UCharacterProgressionManager::OnRep_CurrentXP(const int64 OldXP)
{
	OnProgressionChanged.Broadcast();
	UE_LOG(
		LogCharacterProgressionManager,
		Log,
		TEXT("OnRep_CurrentXP: %lld -> %lld (Progress: %.1f%%)"),
		OldXP,
		CurrentXP,
		GetXPProgressPercent() * 100.0f);
}

void UCharacterProgressionManager::OnRep_SpentStatPoints()
{
	RebuildSpentStatPointsCache();
	OnProgressionChanged.Broadcast();

	UE_LOG(LogCharacterProgressionManager, Log, TEXT("OnRep_SpentStatPoints: cache rebuilt (%d entries)"), SpentStatPoints.Num());
}

void UCharacterProgressionManager::OnRep_ProgressionValue()
{
	OnProgressionChanged.Broadcast();
}

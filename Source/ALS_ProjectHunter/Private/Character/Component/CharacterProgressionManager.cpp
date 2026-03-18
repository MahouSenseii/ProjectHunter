// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Component/CharacterProgressionManager.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/PHBaseCharacter.h"  
#include "GameplayEffect.h"
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
	DOREPLIFETIME(UCharacterProgressionManager, SpentStatPoints);
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
	int64 BaseXP = 0; //KilledCharacter->GetXPReward();

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
	float MoreXP = AttrSet->GetXPGainMultiplier();         // Multiplicative bonuses
	float Penalty = AttrSet->GetXPPenalty();               // Penalties

	// 3. Calculate level difference penalty
	int32 LevelDiff = Level; //- KilledCharacter->GetLevel();
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
	float LocalXP = AttrSet ? AttrSet->GetLocalXPGain() : 0.0f;
	float MoreXP = AttrSet ? AttrSet->GetXPGainMultiplier() : 1.0f;

	// Calculate final XP
	float IncreasedMultiplier = 1.0f + (GlobalXP + LocalXP) / 100.0f;
	float FinalMultiplier = IncreasedMultiplier * MoreXP;

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

	// Deduct stat point
	UnspentStatPoints--;

	// Track spent points in TArray
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

	// If not found, add new entry
	if (!bFound)
	{
		SpentStatPoints.Add(FStatPointSpending(AttributeName, 1));
	}

	// Apply to AttributeSet
	ApplyStatPointToAttribute(AttributeName);

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

	UE_LOG(LogCharacterProgressionManager, Log, TEXT("Stat Points Reset! Refunded: %d points"), TotalStatPoints);

	return true;
}

int32 UCharacterProgressionManager::GetStatPointsSpentOn(FName AttributeName) const
{
	// Search TArray for attribute
	for (const FStatPointSpending& Spending : SpentStatPoints)
	{
		if (Spending.AttributeName == AttributeName)
		{
			return Spending.PointsSpent;
		}
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

void UCharacterProgressionManager::ApplyStatPointToAttribute(FName AttributeName)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	UHunterAttributeSet* AttrSet = GetAttributeSet();

	if (!ASC || !AttrSet)
	{
		UE_LOG(LogCharacterProgressionManager, Error, TEXT("ApplyStatPointToAttribute: ASC or AttributeSet is null"));
		return;
	}

	// Get the attribute to modify
	FGameplayAttribute Attribute = FGameplayAttribute();

	// Map attribute names to FGameplayAttribute
	if (AttributeName == "Strength")
	{
		Attribute = UHunterAttributeSet::GetStrengthAttribute();
	}
	else if (AttributeName == "Intelligence")
	{
		Attribute = UHunterAttributeSet::GetIntelligenceAttribute();
	}
	else if (AttributeName == "Dexterity")
	{
		Attribute = UHunterAttributeSet::GetDexterityAttribute();
	}
	else if (AttributeName == "Endurance")
	{
		Attribute = UHunterAttributeSet::GetEnduranceAttribute();
	}
	else if (AttributeName == "Affliction")
	{
		Attribute = UHunterAttributeSet::GetAfflictionAttribute();
	}
	else if (AttributeName == "Luck")
	{
		Attribute = UHunterAttributeSet::GetLuckAttribute();
	}
	else if (AttributeName == "Covenant")
	{
		Attribute = UHunterAttributeSet::GetCovenantAttribute();
	}
	else
	{
		UE_LOG(LogCharacterProgressionManager, Error, TEXT("ApplyStatPointToAttribute: Unknown attribute '%s'"), *AttributeName.ToString());
		return;
	}

	// Create GameplayEffect to add +1 to the attribute
	UGameplayEffect* GE_StatPoint = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("GE_StatPoint")));
	GE_StatPoint->DurationPolicy = EGameplayEffectDurationType::Infinite;

	// Add modifier
	int32 ModifierIndex = GE_StatPoint->Modifiers.Num();
	GE_StatPoint->Modifiers.SetNum(ModifierIndex + 1);
	FGameplayModifierInfo& ModifierInfo = GE_StatPoint->Modifiers[ModifierIndex];
	ModifierInfo.ModifierMagnitude = FScalableFloat(1.0f); // +1
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;
	ModifierInfo.Attribute = Attribute;

	// Apply effect
	ASC->ApplyGameplayEffectToSelf(GE_StatPoint, 1.0f, ASC->MakeEffectContext());

	UE_LOG(LogCharacterProgressionManager, Log, TEXT("Applied +1 to %s"), *AttributeName.ToString());
}

void UCharacterProgressionManager::RemoveStatPointFromAttribute(FName AttributeName, int32 PointsToRemove)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// Get the attribute
	FGameplayAttribute Attribute = FGameplayAttribute();

	if (AttributeName == "Strength")
	{
		Attribute = UHunterAttributeSet::GetStrengthAttribute();
	}
	else if (AttributeName == "Intelligence")
	{
		Attribute = UHunterAttributeSet::GetIntelligenceAttribute();
	}
	else if (AttributeName == "Dexterity")
	{
		Attribute = UHunterAttributeSet::GetDexterityAttribute();
	}
	else if (AttributeName == "Endurance")
	{
		Attribute = UHunterAttributeSet::GetEnduranceAttribute();
	}
	else if (AttributeName == "Affliction")
	{
		Attribute = UHunterAttributeSet::GetAfflictionAttribute();
	}
	else if (AttributeName == "Luck")
	{
		Attribute = UHunterAttributeSet::GetLuckAttribute();
	}
	else if (AttributeName == "Covenant")
	{
		Attribute = UHunterAttributeSet::GetCovenantAttribute();
	}
	else
	{
		return;
	}

	// Create GameplayEffect to remove points
	UGameplayEffect* GE_RemoveStatPoint = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("GE_RemoveStatPoint")));
	GE_RemoveStatPoint->DurationPolicy = EGameplayEffectDurationType::Infinite;

	// Add modifier (negative)
	int32 ModifierIndex = GE_RemoveStatPoint->Modifiers.Num();
	GE_RemoveStatPoint->Modifiers.SetNum(ModifierIndex + 1);
	FGameplayModifierInfo& ModifierInfo = GE_RemoveStatPoint->Modifiers[ModifierIndex];
	ModifierInfo.ModifierMagnitude = FScalableFloat(-static_cast<float>(PointsToRemove));
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;
	ModifierInfo.Attribute = Attribute;

	// Apply effect
	ASC->ApplyGameplayEffectToSelf(GE_RemoveStatPoint, 1.0f, ASC->MakeEffectContext());

	UE_LOG(LogCharacterProgressionManager, Log, TEXT("Removed %d points from %s"), PointsToRemove, *AttributeName.ToString());
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

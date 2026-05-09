// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "Progression/Library/ProgressionStructs.h"
#include "CharacterProgressionManager.generated.h"

// Forward declarations
class UAbilitySystemComponent;
class UHunterAttributeSet;
class AHunterBaseCharacter;
struct FGameplayAttribute;

DECLARE_LOG_CATEGORY_EXTERN(LogCharacterProgressionManager, Log, All);

// FStatPointSpending is defined in Progression/Library/ProgressionStructs.h

/**
 * Delegate for XP gain events
 * @param FinalXP - Total XP awarded after all bonuses
 * @param BaseXP - Base XP before multipliers
 * @param TotalMultiplier - Combined multiplier applied
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnXPGained, int64, FinalXP, int64, BaseXP, float, TotalMultiplier);

/**
 * Delegate for level up events
 * @param NewLevel - The new level reached
 * @param StatPointsAwarded - Number of stat points gained
 * @param SkillPointsAwarded - Number of skill points gained
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLevelUp, int32, NewLevel, int32, StatPointsAwarded, int32, SkillPointsAwarded);

/**
 * Delegate for stat point spending
 * @param AttributeName - Name of attribute increased
 * @param RemainingPoints - Stat points remaining
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatPointSpent, FName, AttributeName, int32, RemainingPoints);

/**
 * Character Progression Manager
 * Manages experience, leveling, and stat point allocation
 * Separate from AttributeSet for clean separation of concerns
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UCharacterProgressionManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UCharacterProgressionManager();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* EXPERIENCE & LEVELING */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Current character level (1-100) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Level, Category = "Progression")
	int32 Level = 1;  // N-01 FIX: was 0; Level 0 breaks all level-gated calculations at spawn

	/** Current experience points */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentXP, Category = "Progression")
	int64 CurrentXP = 0;

	/** Experience required for next level (cached) */
	UPROPERTY(BlueprintReadOnly, Category = "Progression")
	int64 XPToNextLevel = 100;

	/** Maximum level cap */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression")
	int32 MaxLevel = 100;

	/** Base XP for level 1 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|XP Curve")
	float BaseXPPerLevel = 5.0f;

	/** XP scaling exponent (higher = steeper curve) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|XP Curve")
	float XPScalingExponent = 1.3f;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* STAT POINTS */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Unspent stat points (for primary attributes) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Progression|Stats")
	int32 UnspentStatPoints = 0;

	/** Total stat points earned */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Progression|Stats")
	int32 TotalStatPoints = 0;

	/** Stat points awarded per level */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Stats")
	int32 StatPointsPerLevel = 2;

	/**
	 * Maps each primary attribute name to the Gameplay Effect class used when a stat point
	 * is spent on it.  Each GE should be configured as Infinite with a single +1 Additive
	 * modifier on the corresponding attribute (set via ScalableFloat or SetByCaller).
	 *
	 * Example entries: "Strength" -> GE_StatPoint_Strength, "Intelligence" -> GE_StatPoint_Intelligence
	 *
	 * Exposed to Blueprint defaults so designers can wire up GEs without touching C++.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Progression|StatPoints",
		meta = (DisplayName = "Stat Point GE Classes (Per Attribute)"))
	TMap<FName, TSubclassOf<UGameplayEffect>> StatPointGEClasses;

	/** Track stat points spent per attribute (for respec) - Uses TArray for replication support */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SpentStatPoints, Category = "Progression|Stats")
	TArray<FStatPointSpending> SpentStatPoints;

	/**
	 * Fast-lookup cache of SpentStatPoints (TMap equivalent, not replicated).
	 * Rebuilt from SpentStatPoints on BeginPlay and OnRep.
	 */
	UPROPERTY(Transient)
	TMap<FName, int32> SpentStatPointsCache;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* SKILL POINTS (Optional) */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Unspent skill points (for ability tree) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Progression|Skills")
	int32 UnspentSkillPoints = 0;

	/** Skill points awarded per level */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Skills")
	int32 SkillPointsPerLevel = 1;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* XP CALCULATION FUNCTIONS */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/**
	 * Award experience points from killing a character
	 * Applies all bonuses and penalties automatically
	 * @param KilledCharacter - The character that was killed (player or NPC)
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression", BlueprintAuthorityOnly)
	void AwardExperienceFromKill(APHBaseCharacter* KilledCharacter);

	/**
	 * Award raw experience points (without enemy context)
	 * Applies item bonuses but no level penalties
	 * @param Amount - Base XP amount
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression", BlueprintAuthorityOnly)
	void AwardExperience(int64 Amount);

	/**
	 * Calculate level difference penalty
	 * @param LevelDifference - (Player Level - Enemy Level)
	 * @return Penalty multiplier (0.0 - 1.0)
	 */
	UFUNCTION(BlueprintPure, Category = "Progression")
	float CalculateLevelPenalty(int32 LevelDifference) const;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* LEVELING FUNCTIONS */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/**
	 * Force level up the character
	 * Used for quest rewards, admin commands, etc.
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression", BlueprintAuthorityOnly)
	void LevelUp();

	/**
	 * Check if player has enough XP to level up
	 * Automatically levels up if threshold reached
	 * N-02 FIX: Added BlueprintAuthorityOnly — clients must not trigger level-up logic directly
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression", BlueprintAuthorityOnly)
	void CheckForLevelUp();

	/**
	 * Get XP required for a specific level
	 * @param TargetLevel - Level to calculate XP for
	 * @return XP required to reach that level
	 */
	UFUNCTION(BlueprintPure, Category = "Progression")
	int64 GetXPForLevel(int32 TargetLevel) const;

	/**
	 * Get current XP progress as percentage (0.0 - 1.0)
	 */
	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetXPProgressPercent() const;

	/**
	 * Get total XP gained percentage from bonuses
	 * @return Total percentage bonus (e.g., 60.0 for +60%)
	 */
	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetTotalXPGainPercent() const;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* STAT POINT FUNCTIONS */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/**
	 * Spend a stat point on a primary attribute
	 * @param AttributeName - Name of attribute (Strength, Dexterity, etc.)
	 * @return Success
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression", BlueprintAuthorityOnly)
	bool SpendStatPoint(FName AttributeName);

	/**
	 * Reset all stat points (respec)
	 * @param Cost - Gold cost for respec (0 = free)
	 * @return Success
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression", BlueprintAuthorityOnly)
	bool ResetStatPoints(int32 Cost = 0);

	/**
	 * Get total stat points spent on a specific attribute
	 */
	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetStatPointsSpentOn(FName AttributeName) const;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* EVENTS */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Called when character gains XP */
	UPROPERTY(BlueprintAssignable, Category = "Progression|Events")
	FOnXPGained OnXPGained;

	/** Called when character levels up */
	UPROPERTY(BlueprintAssignable, Category = "Progression|Events")
	FOnLevelUp OnLevelUp;

	/** Called when stat point is spent */
	UPROPERTY(BlueprintAssignable, Category = "Progression|Events")
	FOnStatPointSpent OnStatPointSpent;

protected:
	/* ═══════════════════════════════════════════════════════════════════════ */
	/* INTERNAL FUNCTIONS */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Calculate XP needed for target level (exponential curve) */
	int64 CalculateXPForLevel(int32 TargetLevel) const;

	/** Handle level up rewards (stat points, skill points, etc.) */
	void OnLevelUpInternal();

	/**
	 * Maps attribute names to all active GE handles applied for stat-point spending.
	 * Server-only — not replicated.  Handles are removed on respec.
	 */
	TMap<FName, TArray<FActiveGameplayEffectHandle>> StatPointGEHandles;

	/** Rebuild SpentStatPointsCache from the replicated SpentStatPoints array */
	void RebuildSpentStatPointsCache();

	/**
	 * Maps a primary-attribute name to its FGameplayAttribute accessor.
	 * Returns an invalid FGameplayAttribute{} for unknown names.
	 */
	static FGameplayAttribute GetAttributeForStatName(FName StatName);

	/** Apply stat point to attribute via GAS */
	bool ApplyStatPointToAttribute(FName AttributeName);

	/** Remove stat point from attribute via GAS (for respec) */
	void RemoveStatPointFromAttribute(FName AttributeName, int32 PointsToRemove);

	/** Get AbilitySystemComponent from owner */
	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	/** Get HunterAttributeSet from ASC */
	UHunterAttributeSet* GetAttributeSet() const;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* REPLICATION CALLBACKS */
	/* ═══════════════════════════════════════════════════════════════════════ */

	UFUNCTION()
	void OnRep_Level(int32 OldLevel);

	UFUNCTION()
	void OnRep_CurrentXP(int64 OldXP);

	UFUNCTION()
	void OnRep_SpentStatPoints();

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* CACHED REFERENCES */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Cached AbilitySystemComponent */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;

	/** Cached HunterAttributeSet */
	UPROPERTY()
	TObjectPtr<UHunterAttributeSet> CachedAttributeSet;
};

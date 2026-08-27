#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Progression/Library/Structs/ProgressionStructs.h"
#include "CharacterProgressionManager.generated.h"

class APHBaseCharacter;
class FProgressionAbilityHelper;
class FProgressionStatPointHelper;
class UAbilitySystemComponent;
class UCurveFloat;
class UHunterAttributeSet;

DECLARE_LOG_CATEGORY_EXTERN(LogCharacterProgressionManager, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnXPGained, int64, FinalXP, int64, BaseXP, float, TotalMultiplier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLevelUp, int32, NewLevel, int32, StatPointsAwarded, int32, SkillPointsAwarded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatPointSpent, FName, AttributeName, int32, RemainingPoints);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnProgressionChanged);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UCharacterProgressionManager : public UActorComponent
{
	GENERATED_BODY()

	friend class FProgressionAbilityHelper;
	friend class FProgressionStatPointHelper;

public:
	UCharacterProgressionManager();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Level, Category = "Progression")
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentXP, Category = "Progression")
	int64 CurrentXP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Progression")
	int64 XPToNextLevel = 100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression")
	int32 MaxLevel = 100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|XP Curve")
	float BaseXPPerLevel = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|XP Curve")
	float XPScalingExponent = 1.3f;

	/** Optional XP cost curve keyed by target level. Overrides the formula when assigned. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|XP Curve")
	TObjectPtr<UCurveFloat> XPRequirementCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ProgressionValue, Category = "Progression|Stats")
	int32 UnspentStatPoints = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ProgressionValue, Category = "Progression|Stats")
	int32 TotalStatPoints = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Stats")
	int32 StatPointsPerLevel = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Progression|StatPoints",
		meta = (DisplayName = "Stat Point GE Classes (Per Attribute)"))
	TMap<FName, TSubclassOf<UGameplayEffect>> StatPointGEClasses;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SpentStatPoints, Category = "Progression|Stats")
	TArray<FStatPointSpending> SpentStatPoints;

	UPROPERTY(Transient)
	TMap<FName, int32> SpentStatPointsCache;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ProgressionValue, Category = "Progression|Skills")
	int32 UnspentSkillPoints = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Skills")
	int32 SkillPointsPerLevel = 1;

	UFUNCTION(BlueprintCallable, Category = "Progression", BlueprintAuthorityOnly)
	void AwardExperienceFromKill(APHBaseCharacter* KilledCharacter);

	UFUNCTION(BlueprintCallable, Category = "Progression", BlueprintAuthorityOnly)
	void AwardExperience(int64 Amount);

	UFUNCTION(BlueprintPure, Category = "Progression")
	float CalculateLevelPenalty(int32 LevelDifference) const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Debug", BlueprintAuthorityOnly,
		meta = (DevelopmentOnly, DisplayName = "Debug Grant Level"))
	void LevelUp();

	UFUNCTION(BlueprintCallable, Category = "Progression", BlueprintAuthorityOnly)
	void CheckForLevelUp();

	UFUNCTION(BlueprintPure, Category = "Progression")
	int64 GetXPForLevel(int32 TargetLevel) const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetXPProgressPercent() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetTotalXPGainPercent() const;

	UFUNCTION(BlueprintCallable, Category = "Progression", BlueprintAuthorityOnly)
	bool SpendStatPoint(FName AttributeName);

	UFUNCTION(BlueprintCallable, Category = "Progression", BlueprintAuthorityOnly)
	bool ResetStatPoints(int32 Cost = 0);

	/** Override in Blueprint to atomically spend the requested respec currency. Default only permits free respecs. */
	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "Progression|Respec")
	bool SpendRespecCurrency(int32 Cost);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Progression|Skills")
	bool SpendSkillPoints(int32 Amount = 1);

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetStatPointsSpentOn(FName AttributeName) const;

	UPROPERTY(BlueprintAssignable, Category = "Progression|Events")
	FOnXPGained OnXPGained;

	UPROPERTY(BlueprintAssignable, Category = "Progression|Events")
	FOnLevelUp OnLevelUp;

	UPROPERTY(BlueprintAssignable, Category = "Progression|Events")
	FOnStatPointSpent OnStatPointSpent;

	/** UI-friendly notification fired on server mutations and replicated client updates. */
	UPROPERTY(BlueprintAssignable, Category = "Progression|Events")
	FOnProgressionChanged OnProgressionChanged;

protected:
	void OnLevelUpInternal();
	void RebuildSpentStatPointsCache();

	bool ApplyStatPointToAttribute(FName AttributeName);
	void RemoveStatPointFromAttribute(FName AttributeName, int32 PointsToRemove);

	UAbilitySystemComponent* GetAbilitySystemComponent() const;
	UHunterAttributeSet* GetAttributeSet() const;

	UFUNCTION()
	void OnRep_Level(int32 OldLevel);

	UFUNCTION()
	void OnRep_CurrentXP(int64 OldXP);

	UFUNCTION()
	void OnRep_SpentStatPoints();

	UFUNCTION()
	void OnRep_ProgressionValue();

	TMap<FName, TArray<FActiveGameplayEffectHandle>> StatPointGEHandles;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;

	UPROPERTY()
	TObjectPtr<UHunterAttributeSet> CachedAttributeSet;
};

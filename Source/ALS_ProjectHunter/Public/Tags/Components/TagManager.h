#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Tags/Debug/TagDebugManager.h"
#include "Tags/Library/Structs/TagStructs.h"
#include "TagManager.generated.h"

class ACharacter;
class UAbilitySystemComponent;
class UHunterAttributeSet;

DECLARE_LOG_CATEGORY_EXTERN(LogTagManager, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeadStateChanged, bool, bDead);

UCLASS(ClassGroup = (Managers), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UTagManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UTagManager();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Tags")
	void Initialize(UAbilitySystemComponent* InASC);

	UFUNCTION(BlueprintCallable, Category = "Tags")
	void AddTag(const FGameplayTag& Tag);

	UFUNCTION(BlueprintCallable, Category = "Tags")
	void RemoveTag(const FGameplayTag& Tag);

	UFUNCTION(BlueprintCallable, Category = "Tags")
	void SetTagState(const FGameplayTag& Tag, bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Tags|Conditions")
	void SetDeadState(bool bDead);

	/**
	 * Fires whenever SetDeadState is called, with the state it was set to.
	 *
	 * A death marked only as a tag is invisible to everything that needs to know one happened -
	 * kill counts, encounter progress, loot. The owning character listens to this so a Blueprint
	 * that marks the tag is reporting the death whether or not it also remembers to call
	 * NotifyDeath.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Tags|Conditions")
	FOnDeadStateChanged OnDeadStateChanged;

	UFUNCTION(BlueprintPure, Category = "Tags")
	bool HasTag(const FGameplayTag& Tag) const;

	UFUNCTION(BlueprintPure, Category = "Tags")
	bool HasAnyTags(const FGameplayTagContainer& Tags) const;

	UFUNCTION(BlueprintPure, Category = "Tags")
	bool HasAllTags(const FGameplayTagContainer& Tags) const;

	UFUNCTION(BlueprintCallable, Category = "Tags")
	void RefreshBaseConditionTags();

	UFUNCTION(BlueprintCallable, Category = "Tags|Debug")
	void PrintActiveTags() const;

	UFUNCTION(BlueprintCallable, Category = "Tags|Debug")
	void SetTagDebugEnabled(bool bEnable);

	UFUNCTION(BlueprintPure, Category = "Tags|Debug")
	bool GetOwnedTags(FGameplayTagContainer& OutTags) const;

	UFUNCTION(BlueprintPure, Category = "Tags")
	bool IsInitialized() const { return ASC != nullptr; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Conditions")
	FTagConditionThresholds ConditionThresholds;

	UPROPERTY(EditAnywhere, Category = "Debug")
	FTagDebugManager DebugManager;

private:
	void ApplyPendingStates();
	void ClearManagedTagStates(UAbilitySystemComponent* TargetASC);
	bool HasExternalTagSource(const FGameplayTag& Tag) const;
	bool HasPendingEnabledTag(const FGameplayTag& Tag) const;
	bool ComputeMovementConditionState(const ACharacter* CharacterOwner);
	const UHunterAttributeSet* GetHunterAttributeSet() const;
	void RefreshResourceConditionTags(
		const FGameplayTag& LowTag,
		const FGameplayTag& FullTag,
		float CurrentValue,
		float MaxValue);
	void RefreshMovementConditionTags();
	void BindAttributeChangeDelegates();
	void UnbindAttributeChangeDelegates();

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASC;

	TArray<FTagAttributeDelegateBinding> AttributeDelegateBindings;
	TSet<FGameplayTag> ManagedLooseTags;
	TMap<FGameplayTag, bool> PendingTagStates;
	bool bBaseConditionsDirty = false;
	bool bHasMovementConditionState = false;
	bool bLastMovementConditionMoving = false;
	float ConditionRefreshAccumulator = 0.0f;
};

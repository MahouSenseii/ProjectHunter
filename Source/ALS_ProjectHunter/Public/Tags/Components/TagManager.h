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

	UPROPERTY(EditAnywhere, Category = "Debug")
	FTagDebugManager DebugManager;

private:
	void ApplyPendingStates();
	bool HasPendingEnabledTag(const FGameplayTag& Tag) const;
	bool ComputeMovementConditionState(const ACharacter* CharacterOwner);
	bool ComputeLowResourceState(float CurrentValue, float MaxValue) const;
	bool ComputeFullResourceState(float CurrentValue, float MaxValue) const;
	const UHunterAttributeSet* GetHunterAttributeSet() const;
	void RefreshMovementConditionTags();
	void BindAttributeChangeDelegates();
	void UnbindAttributeChangeDelegates();

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASC;

	TArray<FTagAttributeDelegateBinding> AttributeDelegateBindings;
	TMap<FGameplayTag, bool> PendingTagStates;
	bool bBaseConditionsDirty = false;
	bool bHasMovementConditionState = false;
	bool bLastMovementConditionMoving = false;
	float ConditionRefreshAccumulator = 0.0f;
};

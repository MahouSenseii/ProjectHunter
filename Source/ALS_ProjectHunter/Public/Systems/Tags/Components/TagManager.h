#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Systems/Tags/Debug/TagDebugManager.h"
#include "TagManager.generated.h"

class UAbilitySystemComponent;
class UHunterAttributeSet;

struct FTagAttributeDelegateBinding
{
	FGameplayAttribute Attribute;
	FDelegateHandle Handle;
};

DECLARE_LOG_CATEGORY_EXTERN(LogTagManager, Log, All);

UCLASS(ClassGroup=(Managers), meta=(BlueprintSpawnableComponent))
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

	/**
	 * Enable or disable the on-screen tag debug panel at runtime.
	 * Mirrors UStatsManager::SetStatsDebugEnabled — clears stale messages when
	 * turned off and forces an immediate redraw on the first re-enable.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tags|Debug")
	void SetTagDebugEnabled(bool bEnable);

	/**
	 * Returns the full set of gameplay tags currently owned by the ASC.
	 * Returns false (and leaves OutTags unchanged) when the ASC is not ready.
	 * Used by FTagDebugManager; prefer HasTag / HasAnyTags for game logic.
	 */
	UFUNCTION(BlueprintPure, Category = "Tags|Debug")
	bool GetOwnedTags(FGameplayTagContainer& OutTags) const;

	/**
	 * True once Initialize() has been called with a valid ASC.
	 * Use this to detect whether this specific instance is the authoritative one
	 * (e.g., to guard against duplicate Blueprint-added components).
	 */
	UFUNCTION(BlueprintPure, Category = "Tags")
	bool IsInitialized() const { return ASC != nullptr; }

	/** On-screen tag-state visualiser.  Toggle bEnableDebug in the Details panel. */
	UPROPERTY(EditAnywhere, Category = "Debug")
	FTagDebugManager DebugManager;

private:
	void ApplyPendingStates();
	bool HasPendingEnabledTag(const FGameplayTag& Tag) const;
	bool ComputeLowResourceState(float CurrentValue, float MaxValue) const;
	bool ComputeFullResourceState(float CurrentValue, float MaxValue) const;
	const UHunterAttributeSet* GetHunterAttributeSet() const;

	/** Refresh ONLY movement/sprint tags (called by tick at reduced cadence). */
	void RefreshMovementConditionTags();

	/** Bind GAS attribute-change delegates so resource-based tags update reactively.
	 *  N-06 FIX: Replaces per-frame attribute polling with event-driven updates. */
	void BindAttributeChangeDelegates();
	void UnbindAttributeChangeDelegates();

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASC;

	TArray<FTagAttributeDelegateBinding> AttributeDelegateBindings;

	TMap<FGameplayTag, bool> PendingTagStates;
	bool bPendingBaseRefresh = false;

	// OPT-TAG: Dirty flag for coalescing multiple attribute-change callbacks
	// into a single RefreshBaseConditionTags() call per tick. During heavy combat,
	// Health/Mana/Stamina can change many times in a single frame — the delegate
	// sets this flag and the tick performs the actual refresh once.
	bool bBaseConditionsDirty = false;

	// N-06 FIX: accumulator for reduced-rate movement/sprint tick
	float ConditionRefreshAccumulator = 0.f;
};

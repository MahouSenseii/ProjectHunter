// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "PHPassiveTreeComponent.generated.h"

class UAbilitySystemComponent;
class UCharacterProgressionManager;
class UGameplayEffect;
class UPHPassiveTreeDataAsset;
struct FPHPassiveNodeDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPassiveTreeChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPassiveAllocationRejected, FName, NodeID, FText, Reason);

/**
 * Owns passive-node allocations and their permanent GAS effects. Progression remains the owner of
 * passive-point currency; this component performs the validated point-spend + effect transaction.
 */
UCLASS(ClassGroup = (Progression), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UPHPassiveTreeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPHPassiveTreeComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Optional per-character graph. Empty uses UPHPassiveTreeSettings::DefaultTree. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive Tree")
	TSoftObjectPtr<UPHPassiveTreeDataAsset> TreeDataOverride;

	/** Stable node IDs are replicated only to the owning client; GAS effects replicate normally. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AllocatedNodeIDs, Category = "Passive Tree")
	TArray<FName> AllocatedNodeIDs;

	/**
	 * The node this Hunter woke up on, granted free. Rolled once and then persisted as an ID rather
	 * than re-derived from the seed, so adding nodes to the tree later never moves an existing
	 * character's origin.
	 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_RandomStartNodeID, Category = "Passive Tree|Random Start")
	FName RandomStartNodeID;

	/** Zero rolls an unpredictable origin. Any other value makes the roll reproducible. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Passive Tree|Random Start")
	int32 RandomStartSeed = 0;

	/** Clear this when a character-creation flow owns the roll and wants to pass its own seed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive Tree|Random Start")
	bool bAutoRollRandomStartOnBeginPlay = true;

	UPROPERTY(BlueprintAssignable, Category = "Passive Tree|Events")
	FOnPassiveTreeChanged OnPassiveTreeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Passive Tree|Events")
	FOnPassiveAllocationRejected OnPassiveAllocationRejected;

	UFUNCTION(BlueprintPure, Category = "Passive Tree")
	UPHPassiveTreeDataAsset* GetTreeData() const;

	UFUNCTION(BlueprintPure, Category = "Passive Tree")
	bool IsNodeAllocated(FName NodeID) const;

	/** Explains locked, duplicate, invalid-data, and insufficient-point states for the UI. */
	UFUNCTION(BlueprintPure, Category = "Passive Tree")
	bool CanAllocateNode(FName NodeID, FText& OutReason) const;

	/** Routes a local request to authority. Returns false only when the local request is immediately rejected. */
	UFUNCTION(BlueprintCallable, Category = "Passive Tree")
	bool RequestAllocateNode(FName NodeID);

	/** Authority-only operation used by tests and non-player callers. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Passive Tree")
	bool AllocateNode(FName NodeID);

	UFUNCTION(BlueprintPure, Category = "Passive Tree|Random Start")
	bool IsRandomStartNode(FName NodeID) const;

	/**
	 * Rolls this character's opening node and grants it free. Returns the chosen node, or NAME_None
	 * when random starts are off or nothing is eligible. Does nothing if an origin already exists, so
	 * it is safe to call after a save restore.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Passive Tree|Random Start")
	FName RollRandomStart(int32 ParentSeed);

	/** Grants a specific origin free. Used by the roll and by save restore. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Passive Tree|Random Start")
	bool SetRandomStart(FName NodeID);

	/** Restores already-paid allocations without spending points. Invalid or disconnected IDs are ignored. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Passive Tree|Persistence")
	void RestoreAllocatedNodes(const TArray<FName>& SavedNodeIDs);

	/** Restore including the rolled origin. Saving the origin separately is what stops a reload re-rolling it. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Passive Tree|Persistence")
	void RestorePassiveState(const TArray<FName>& SavedNodeIDs, FName SavedRandomStartNodeID);

	/** Rebuilds the owned infinite effects after ASC initialization or a save restore. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Passive Tree")
	void RefreshPassiveEffects();

protected:
	UFUNCTION(Server, Reliable)
	void ServerAllocateNode(FName NodeID);

	UFUNCTION()
	void OnRep_AllocatedNodeIDs();

	UFUNCTION()
	void OnRep_RandomStartNodeID();

private:
	UCharacterProgressionManager* GetProgression() const;
	UAbilitySystemComponent* GetAbilitySystem() const;
	bool ApplyNodeEffect(const FPHPassiveNodeDefinition& Node, FActiveGameplayEffectHandle& OutHandle);
	void ClearPassiveEffects();
	void Reject(FName NodeID, const FText& Reason);

	/** Adds a node to the owned set and applies its effect without touching passive points. */
	bool GrantNodeWithoutCost(const FPHPassiveNodeDefinition& Node);

	/** False once a rolled origin has displaced the authored one, per the tree's random start rule. */
	bool AllowsUnconnectedStarts(const UPHPassiveTreeDataAsset& Tree) const;

	/** Mirrors AllocatedNodeIDs. The array stays authoritative because it is what replicates and saves. */
	void RebuildAllocatedLookup();

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UGameplayEffect>> RuntimeEffects;

	/**
	 * Resolved once instead of per query. The menu asks about every visible node on each rebuild,
	 * and both a soft-pointer resolve and FindComponentByClass are far too expensive at that rate.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UPHPassiveTreeDataAsset> CachedTree;

	mutable TWeakObjectPtr<UCharacterProgressionManager> CachedProgression;

	TSet<FName> AllocatedLookup;

	TMap<FName, FActiveGameplayEffectHandle> ActiveEffectHandles;
};

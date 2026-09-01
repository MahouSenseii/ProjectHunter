// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "PHPassiveTreeDataAsset.generated.h"

UENUM(BlueprintType)
enum class EPHPassiveNodeSize : uint8
{
	Small UMETA(DisplayName = "Small"),
	Major UMETA(DisplayName = "Major")
};

/**
 * How a node's listed connections gate it.
 *
 * RequireAny is what a Path-of-Exile-shaped graph needs: a node opens as soon as one neighbour is
 * allocated, so clusters can be approached from either side and the graph may contain loops.
 * RequireAll turns the same list into a directed prerequisite chain, which forbids loops.
 */
UENUM(BlueprintType)
enum class EPHPassiveConnectionRule : uint8
{
	RequireAny UMETA(DisplayName = "Require Any Connection"),
	RequireAll UMETA(DisplayName = "Require All Connections")
};

/** One permanent GAS modifier granted while its passive node is allocated. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHPassiveModifier
{
	GENERATED_BODY()

	/** Attribute registry tag, for example Attributes.Primary.Strength. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive", meta = (Categories = "Attributes"))
	FGameplayTag AttributeTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	TEnumAsByte<EGameplayModOp::Type> Operation = EGameplayModOp::Additive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	float Magnitude = 0.0f;
};

/**
 * Where a fresh Hunter wakes up on the graph.
 *
 * Rolling the opening node is what keeps two characters of the same level from being the same build:
 * the roll decides which corner of the tree is cheap to reach, not just which stat you opened with.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHPassiveRandomStartRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	bool bEnabled = true;

	/** Only nodes of this size are drawn. Small keeps the roll an opening, not a handed-out milestone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	EPHPassiveNodeSize EligibleSize = EPHPassiveNodeSize::Small;

	/** Nodes that must never be rolled - too strong for turn one, or reserved for a path's identity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive", meta = (GetOptions = "GetAllNodeIDOptions"))
	TArray<FName> ExcludedNodeIDs;

	/**
	 * While a roll is active, authored nodes with no connections stop being free entries, so the
	 * rolled node is the character's only origin. Turn this off to make the roll a bonus sitting on
	 * top of the authored starting node instead of replacing it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	bool bReplacesAuthoredStarts = true;
};

/** A stable, editable node in the Project Hunter passive graph. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHPassiveNodeDefinition
{
	GENERATED_BODY()

	/** Stable save identity. Renaming this requires a save migration. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	FName NodeID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	EPHPassiveNodeSize NodeSize = EPHPassiveNodeSize::Small;

	/** Graph-space position. Editing this moves the node without changing gameplay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	FVector2D Position = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive", meta = (ClampMin = "1", UIMin = "1"))
	int32 PointCost = 1;

	/**
	 * Neighbours this node hangs off. Empty means this is a starting node.
	 * The owning asset's ConnectionRule decides whether one or all of them must be allocated first.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive", meta = (GetOptions = "GetAllNodeIDOptions"))
	TArray<FName> RequiredNodeIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	TArray<FPHPassiveModifier> Modifiers;

	/** Optional aliases used by the menu search. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	TArray<FString> SearchKeywords;
};

/**
 * Designer-owned passive graph. Adding, removing, connecting, renaming, or repositioning nodes is
 * data authoring; the runtime owner and menu do not need a code change.
 *
 * Node lookup is indexed rather than linear because the menu resolves every connection and every
 * node state on each rebuild - at a few thousand nodes a linear FindNode makes that quadratic.
 */
UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UPHPassiveTreeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	FName TreeID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	EPHPassiveConnectionRule ConnectionRule = EPHPassiveConnectionRule::RequireAny;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	FPHPassiveRandomStartRule RandomStart;

	/** TitleProperty keeps the array readable in the details panel once this grows past a screenful. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive", meta = (TitleProperty = "NodeID"))
	TArray<FPHPassiveNodeDefinition> Nodes;

	const FPHPassiveNodeDefinition* FindNode(FName NodeID) const;

	/** INDEX_NONE when the ID is unknown. */
	int32 FindNodeIndex(FName NodeID) const;

	/**
	 * Every node joined to this one, in either direction. Connections are authored one-way - a child
	 * lists its parent - but a passive graph is undirected: standing on a leaf has to open the branch
	 * above it, or a random start would be marooned on its own node.
	 */
	const TArray<FName>* FindNeighbours(FName NodeID) const;

	/**
	 * Applies ConnectionRule to one node.
	 *
	 * bAllowUnconnectedAsStart is how a random start displaces the authored one: pass false and a node
	 * that lists no connections stops being a free entry and has to be reached like anything else.
	 */
	bool AreConnectionsSatisfied(
		const FPHPassiveNodeDefinition& Node,
		TFunctionRef<bool(FName)> IsAllocated,
		bool bAllowUnconnectedAsStart = true) const;

	/** Nodes the random start may draw, sorted by ID so one seed always yields one node. */
	void GatherRandomStartCandidates(TArray<FName>& OutCandidates) const;

	/** NAME_None when random starts are disabled or nothing is eligible. */
	FName PickRandomStart(int32 Seed) const;

	/** Case-insensitive name, description, ID, and keyword search. */
	UFUNCTION(BlueprintPure, Category = "Passive")
	bool DoesNodeMatchSearch(const FPHPassiveNodeDefinition& Node, const FString& SearchText) const;

	/** Feeds the RequiredNodeIDs dropdown in the details panel. */
	UFUNCTION()
	TArray<FString> GetAllNodeIDOptions() const;

	/** Returns authoring errors without mutating the asset. */
	bool ValidateTree(TArray<FText>& OutErrors) const;

	/** Drops the ID index. Call after mutating Nodes outside the editor property system. */
	void InvalidateNodeIndex() const { bNodeIndexDirty = true; }

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

private:
	void RebuildNodeIndex() const;

	/** Both are derived from Nodes, so they are rebuilt on demand rather than serialised. */
	mutable TMap<FName, int32> NodeIndexByID;
	mutable TMap<FName, TArray<FName>> NeighboursByID;
	mutable bool bNodeIndexDirty = true;
};

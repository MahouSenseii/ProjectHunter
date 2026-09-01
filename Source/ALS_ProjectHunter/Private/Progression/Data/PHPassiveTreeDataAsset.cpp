#include "Progression/Data/PHPassiveTreeDataAsset.h"

#include "Misc/DataValidation.h"
#include "Tags/PHGameplayTags.h"

void UPHPassiveTreeDataAsset::RebuildNodeIndex() const
{
	NodeIndexByID.Reset();
	NodeIndexByID.Reserve(Nodes.Num());
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		// A duplicate ID is an authoring error IsDataValid reports; first-wins keeps lookup total.
		NodeIndexByID.FindOrAdd(Nodes[Index].NodeID, Index);
	}

	// Each authored connection is recorded from both ends, which is what makes the graph undirected.
	NeighboursByID.Reset();
	for (const FPHPassiveNodeDefinition& Node : Nodes)
	{
		for (const FName ConnectedID : Node.RequiredNodeIDs)
		{
			if (ConnectedID.IsNone() || ConnectedID == Node.NodeID || !NodeIndexByID.Contains(ConnectedID))
			{
				continue;
			}
			NeighboursByID.FindOrAdd(Node.NodeID).AddUnique(ConnectedID);
			NeighboursByID.FindOrAdd(ConnectedID).AddUnique(Node.NodeID);
		}
	}

	bNodeIndexDirty = false;
}

const TArray<FName>* UPHPassiveTreeDataAsset::FindNeighbours(const FName NodeID) const
{
	if (bNodeIndexDirty)
	{
		RebuildNodeIndex();
	}
	return NeighboursByID.Find(NodeID);
}

int32 UPHPassiveTreeDataAsset::FindNodeIndex(const FName NodeID) const
{
	if (bNodeIndexDirty)
	{
		RebuildNodeIndex();
	}
	const int32* Found = NodeIndexByID.Find(NodeID);
	return Found ? *Found : INDEX_NONE;
}

const FPHPassiveNodeDefinition* UPHPassiveTreeDataAsset::FindNode(const FName NodeID) const
{
	const int32 Index = FindNodeIndex(NodeID);
	return Nodes.IsValidIndex(Index) ? &Nodes[Index] : nullptr;
}

bool UPHPassiveTreeDataAsset::AreConnectionsSatisfied(
	const FPHPassiveNodeDefinition& Node,
	TFunctionRef<bool(FName)> IsAllocated,
	const bool bAllowUnconnectedAsStart) const
{
	// Listing no connections is the authoring convention for "this is a root". It stays the test for
	// rootness even under the undirected rule below, where a root usually does have neighbours.
	if (Node.RequiredNodeIDs.IsEmpty() && bAllowUnconnectedAsStart)
	{
		return true;
	}

	if (ConnectionRule == EPHPassiveConnectionRule::RequireAll)
	{
		if (Node.RequiredNodeIDs.IsEmpty())
		{
			return false;
		}
		for (const FName RequiredID : Node.RequiredNodeIDs)
		{
			if (!IsAllocated(RequiredID))
			{
				return false;
			}
		}
		return true;
	}

	const TArray<FName>* Neighbours = FindNeighbours(Node.NodeID);
	if (!Neighbours)
	{
		return false;
	}
	for (const FName NeighbourID : *Neighbours)
	{
		if (IsAllocated(NeighbourID))
		{
			return true;
		}
	}
	return false;
}

void UPHPassiveTreeDataAsset::GatherRandomStartCandidates(TArray<FName>& OutCandidates) const
{
	OutCandidates.Reset();
	if (!RandomStart.bEnabled)
	{
		return;
	}

	for (const FPHPassiveNodeDefinition& Node : Nodes)
	{
		if (Node.NodeID.IsNone() ||
			Node.NodeSize != RandomStart.EligibleSize ||
			Node.Modifiers.IsEmpty() ||
			RandomStart.ExcludedNodeIDs.Contains(Node.NodeID))
		{
			continue;
		}
		OutCandidates.Add(Node.NodeID);
	}

	// Sorted by ID, not array order, so reordering the Nodes array in the editor does not silently
	// reshuffle which node every existing seed maps to.
	OutCandidates.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
}

FName UPHPassiveTreeDataAsset::PickRandomStart(const int32 Seed) const
{
	TArray<FName> Candidates;
	GatherRandomStartCandidates(Candidates);
	if (Candidates.IsEmpty())
	{
		return NAME_None;
	}

	const FRandomStream Stream(Seed);
	return Candidates[Stream.RandRange(0, Candidates.Num() - 1)];
}

bool UPHPassiveTreeDataAsset::DoesNodeMatchSearch(
	const FPHPassiveNodeDefinition& Node,
	const FString& SearchText) const
{
	const FString Needle = SearchText.TrimStartAndEnd().ToLower();
	if (Needle.IsEmpty())
	{
		return true;
	}

	if (Node.NodeID.ToString().ToLower().Contains(Needle) ||
		Node.DisplayName.ToString().ToLower().Contains(Needle) ||
		Node.Description.ToString().ToLower().Contains(Needle))
	{
		return true;
	}

	return Node.SearchKeywords.ContainsByPredicate([&Needle](const FString& Keyword)
	{
		return Keyword.ToLower().Contains(Needle);
	});
}

TArray<FString> UPHPassiveTreeDataAsset::GetAllNodeIDOptions() const
{
	TArray<FString> Options;
	Options.Reserve(Nodes.Num());
	for (const FPHPassiveNodeDefinition& Node : Nodes)
	{
		if (!Node.NodeID.IsNone())
		{
			Options.AddUnique(Node.NodeID.ToString());
		}
	}
	Options.Sort();
	return Options;
}

void UPHPassiveTreeDataAsset::PostLoad()
{
	Super::PostLoad();
	bNodeIndexDirty = true;
}

#if WITH_EDITOR
void UPHPassiveTreeDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// Any edit can renumber or rename nodes, so the index is dropped wholesale rather than patched.
	bNodeIndexDirty = true;
}
#endif

bool UPHPassiveTreeDataAsset::ValidateTree(TArray<FText>& OutErrors) const
{
	OutErrors.Reset();
	if (TreeID.IsNone())
	{
		OutErrors.Add(NSLOCTEXT("PHPassiveTree", "MissingTreeID", "Tree ID must be set."));
	}
	if (Nodes.IsEmpty())
	{
		OutErrors.Add(NSLOCTEXT("PHPassiveTree", "EmptyTree", "The passive tree contains no nodes."));
		return false;
	}

	TSet<FName> NodeIDs;
	bool bHasStartNode = false;
	for (const FPHPassiveNodeDefinition& Node : Nodes)
	{
		if (Node.NodeID.IsNone())
		{
			OutErrors.Add(NSLOCTEXT("PHPassiveTree", "MissingNodeID", "Every passive node needs a stable Node ID."));
			continue;
		}
		if (NodeIDs.Contains(Node.NodeID))
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("PHPassiveTree", "DuplicateNodeID", "Node ID '{0}' is duplicated."),
				FText::FromName(Node.NodeID)));
		}
		NodeIDs.Add(Node.NodeID);
		bHasStartNode |= Node.RequiredNodeIDs.IsEmpty();

		if (Node.DisplayName.IsEmpty())
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("PHPassiveTree", "MissingDisplayName", "Node '{0}' has no display name."),
				FText::FromName(Node.NodeID)));
		}
		if (Node.PointCost <= 0)
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("PHPassiveTree", "InvalidCost", "Node '{0}' must cost at least one point."),
				FText::FromName(Node.NodeID)));
		}
		if (!FMath::IsFinite(Node.Position.X) || !FMath::IsFinite(Node.Position.Y))
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("PHPassiveTree", "InvalidPosition", "Node '{0}' has a non-finite graph position."),
				FText::FromName(Node.NodeID)));
		}
		if (Node.Modifiers.IsEmpty())
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("PHPassiveTree", "NoModifiers", "Node '{0}' grants no modifiers."),
				FText::FromName(Node.NodeID)));
		}
		for (const FPHPassiveModifier& Modifier : Node.Modifiers)
		{
			if (!Modifier.AttributeTag.IsValid() ||
				!FPHGameplayTags::GetAttributeFromTag(Modifier.AttributeTag).IsValid())
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("PHPassiveTree", "UnknownAttribute", "Node '{0}' uses an unresolved attribute tag."),
					FText::FromName(Node.NodeID)));
			}
			if (!FMath::IsFinite(Modifier.Magnitude))
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("PHPassiveTree", "InvalidMagnitude", "Node '{0}' has a non-finite modifier magnitude."),
					FText::FromName(Node.NodeID)));
			}
		}
	}

	if (!bHasStartNode)
	{
		OutErrors.Add(NSLOCTEXT("PHPassiveTree", "NoStart", "The tree needs at least one node with no connections."));
	}

	for (const FPHPassiveNodeDefinition& Node : Nodes)
	{
		for (const FName RequiredID : Node.RequiredNodeIDs)
		{
			if (RequiredID == Node.NodeID)
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("PHPassiveTree", "SelfDependency", "Node '{0}' connects to itself."),
					FText::FromName(Node.NodeID)));
			}
			else if (!NodeIDs.Contains(RequiredID))
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("PHPassiveTree", "MissingDependency", "Node '{0}' connects to missing node '{1}'."),
					FText::FromName(Node.NodeID), FText::FromName(RequiredID)));
			}
		}
	}

	if (ConnectionRule == EPHPassiveConnectionRule::RequireAll)
	{
		// Under RequireAll the connection list is a directed prerequisite chain, so a cycle would
		// leave every node inside it permanently unreachable.
		TSet<FName> Visiting;
		TSet<FName> Visited;
		TFunction<bool(FName)> Visit = [&](const FName NodeID)
		{
			if (Visiting.Contains(NodeID))
			{
				return false;
			}
			if (Visited.Contains(NodeID))
			{
				return true;
			}
			Visiting.Add(NodeID);
			if (const FPHPassiveNodeDefinition* Node = FindNode(NodeID))
			{
				for (const FName RequiredID : Node->RequiredNodeIDs)
				{
					if (NodeIDs.Contains(RequiredID) && !Visit(RequiredID))
					{
						return false;
					}
				}
			}
			Visiting.Remove(NodeID);
			Visited.Add(NodeID);
			return true;
		};

		for (const FName NodeID : NodeIDs)
		{
			if (!Visit(NodeID))
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("PHPassiveTree", "Cycle", "The prerequisite chain containing '{0}' has a cycle."),
					FText::FromName(NodeID)));
				break;
			}
		}
	}
	else
	{
		// Under RequireAny loops are the point, so the useful check is connectivity: the undirected
		// graph must be one piece. Anything else is an island no origin can grow into - and with a
		// random start the origin could be any eligible node, so "one piece" is the only safe shape.
		FName SeedID = NAME_None;
		for (const FPHPassiveNodeDefinition& Node : Nodes)
		{
			if (!Node.NodeID.IsNone())
			{
				SeedID = Node.NodeID;
				break;
			}
		}

		TSet<FName> Connected;
		if (!SeedID.IsNone())
		{
			TArray<FName> Frontier{SeedID};
			Connected.Add(SeedID);
			while (!Frontier.IsEmpty())
			{
				const FName Current = Frontier.Pop(EAllowShrinking::No);
				if (const TArray<FName>* Neighbours = FindNeighbours(Current))
				{
					for (const FName NeighbourID : *Neighbours)
					{
						if (!Connected.Contains(NeighbourID))
						{
							Connected.Add(NeighbourID);
							Frontier.Add(NeighbourID);
						}
					}
				}
			}
		}

		for (const FPHPassiveNodeDefinition& Node : Nodes)
		{
			if (!Node.NodeID.IsNone() && !Connected.Contains(Node.NodeID))
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("PHPassiveTree", "Unreachable", "Node '{0}' sits on an island disconnected from the rest of the tree."),
					FText::FromName(Node.NodeID)));
			}
		}
	}

	if (RandomStart.bEnabled)
	{
		TArray<FName> Candidates;
		GatherRandomStartCandidates(Candidates);
		if (Candidates.IsEmpty())
		{
			OutErrors.Add(NSLOCTEXT("PHPassiveTree", "NoRandomStart",
				"Random starts are enabled but no node matches the eligible size and exclusions."));
		}
		for (const FName ExcludedID : RandomStart.ExcludedNodeIDs)
		{
			if (!ExcludedID.IsNone() && !NodeIDs.Contains(ExcludedID))
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("PHPassiveTree", "UnknownExclusion", "Random start excludes missing node '{0}'."),
					FText::FromName(ExcludedID)));
			}
		}
	}

	return OutErrors.IsEmpty();
}

#if WITH_EDITOR
EDataValidationResult UPHPassiveTreeDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	TArray<FText> Errors;
	if (!ValidateTree(Errors))
	{
		for (const FText& Error : Errors)
		{
			Context.AddError(Error);
		}
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif

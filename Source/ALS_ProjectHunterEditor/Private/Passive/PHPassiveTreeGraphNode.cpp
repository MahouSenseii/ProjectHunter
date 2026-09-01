// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "Passive/PHPassiveTreeGraphNode.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"

const FName UPHPassiveTreeGraphNode::ParentPinName(TEXT("Parents"));
const FName UPHPassiveTreeGraphNode::ChildPinName(TEXT("Children"));
const FName UPHPassiveTreeGraphNode::PinCategory(TEXT("PassiveLink"));

void UPHPassiveTreeGraphNode::AllocateDefaultPins()
{
	// Both pins accept any number of links. Direction here is authoring bookkeeping only - a graph
	// set to RequireAny reads every connection both ways at runtime.
	CreatePin(EGPD_Input, PinCategory, ParentPinName);
	CreatePin(EGPD_Output, PinCategory, ChildPinName);
}

FText UPHPassiveTreeGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (!Definition.DisplayName.IsEmpty())
	{
		return Definition.DisplayName;
	}
	return Definition.NodeID.IsNone()
		? NSLOCTEXT("PHPassiveTreeGraph", "UnnamedNode", "Unnamed Passive")
		: FText::FromName(Definition.NodeID);
}

FLinearColor UPHPassiveTreeGraphNode::GetNodeTitleColor() const
{
	// Milestones read at a glance against the small nodes that outnumber them.
	return Definition.NodeSize == EPHPassiveNodeSize::Major
		? FLinearColor(0.35f, 0.18f, 0.55f)
		: FLinearColor(0.06f, 0.26f, 0.45f);
}

FText UPHPassiveTreeGraphNode::GetTooltipText() const
{
	return FText::Format(
		NSLOCTEXT("PHPassiveTreeGraph", "NodeTooltip", "{0}\nID: {1}\nCost: {2}\n\n{3}"),
		GetNodeTitle(ENodeTitleType::FullTitle),
		FText::FromName(Definition.NodeID),
		FText::AsNumber(Definition.PointCost),
		Definition.Description);
}

UEdGraphPin* UPHPassiveTreeGraphNode::GetParentPin() const
{
	return FindPin(ParentPinName, EGPD_Input);
}

UEdGraphPin* UPHPassiveTreeGraphNode::GetChildPin() const
{
	return FindPin(ChildPinName, EGPD_Output);
}

FVector2D UPHPassiveTreeGraphNode::GetFootprint() const
{
	// Mirrors SPHPassiveTreeGraph::RebuildGeometry. A node that hangs off nothing is a starting node
	// and the runtime draws it largest.
	if (Definition.RequiredNodeIDs.IsEmpty())
	{
		return FVector2D(250.0, 100.0);
	}
	return Definition.NodeSize == EPHPassiveNodeSize::Major
		? FVector2D(230.0, 92.0)
		: FVector2D(180.0, 74.0);
}

void UPHPassiveTreeGraphNode::ApplyGraphPosition()
{
	const FVector2D TopLeft = Definition.Position - GetFootprint() * 0.5;
	NodePosX = FMath::RoundToInt(TopLeft.X);
	NodePosY = FMath::RoundToInt(TopLeft.Y);
}

void UPHPassiveTreeGraphNode::SyncDefinitionFromGraph()
{
	const FVector2D FootprintBefore = GetFootprint();
	CaptureConnections();
	const FVector2D FootprintAfter = GetFootprint();

	// Wiring a root node stops it being a root, which shrinks it. Nudging the top-left by half the
	// change keeps the node visually where the designer put it instead of letting it drift when the
	// footprint changes under it.
	const FVector2D Correction = (FootprintBefore - FootprintAfter) * 0.5;
	NodePosX += FMath::RoundToInt(Correction.X);
	NodePosY += FMath::RoundToInt(Correction.Y);

	Definition.Position = FVector2D(NodePosX, NodePosY) + FootprintAfter * 0.5;
}

FName UPHPassiveTreeGraphNode::MakeUniqueNodeID(const UEdGraph* Graph)
{
	if (!Graph)
	{
		return NAME_None;
	}

	TSet<FName> Used;
	for (const UEdGraphNode* Node : Graph->Nodes)
	{
		if (const UPHPassiveTreeGraphNode* PassiveNode = Cast<UPHPassiveTreeGraphNode>(Node))
		{
			Used.Add(PassiveNode->Definition.NodeID);
		}
	}

	for (int32 Suffix = 1; Suffix < TNumericLimits<int32>::Max(); ++Suffix)
	{
		const FName Candidate(*FString::Printf(TEXT("NewPassive_%d"), Suffix));
		if (!Used.Contains(Candidate))
		{
			return Candidate;
		}
	}
	return NAME_None;
}

void UPHPassiveTreeGraphNode::CaptureConnections()
{
	Definition.RequiredNodeIDs.Reset();
	const UEdGraphPin* ParentPin = GetParentPin();
	if (!ParentPin)
	{
		return;
	}

	for (const UEdGraphPin* LinkedPin : ParentPin->LinkedTo)
	{
		const UPHPassiveTreeGraphNode* ParentNode =
			LinkedPin ? Cast<UPHPassiveTreeGraphNode>(LinkedPin->GetOwningNode()) : nullptr;
		if (ParentNode && !ParentNode->Definition.NodeID.IsNone())
		{
			Definition.RequiredNodeIDs.AddUnique(ParentNode->Definition.NodeID);
		}
	}
}

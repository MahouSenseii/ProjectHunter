// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "Passive/PHPassiveTreeGraphSchema.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Passive/PHPassiveTreeGraphNode.h"
#include "Framework/Commands/GenericCommands.h"
#include "ScopedTransaction.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"

#define LOCTEXT_NAMESPACE "PHPassiveTreeGraphSchema"

UEdGraphNode* FPHPassiveTreeSchemaAction_NewNode::PerformAction(
	UEdGraph* ParentGraph,
	UEdGraphPin* FromPin,
	const FVector2f& Location,
	const bool bSelectNewNode)
{
	if (!ParentGraph)
	{
		return nullptr;
	}

	const FScopedTransaction Transaction(LOCTEXT("AddPassiveNode", "Add Passive Node"));
	ParentGraph->Modify();

	UPHPassiveTreeGraphNode* NewNode = NewObject<UPHPassiveTreeGraphNode>(ParentGraph);
	NewNode->SetFlags(RF_Transactional);
	NewNode->CreateNewGuid();
	NewNode->NodePosX = FMath::RoundToInt(Location.X);
	NewNode->NodePosY = FMath::RoundToInt(Location.Y);
	NewNode->Definition.NodeID = UPHPassiveTreeGraphNode::MakeUniqueNodeID(ParentGraph);
	NewNode->Definition.DisplayName = FText::FromName(NewNode->Definition.NodeID);
	NewNode->Definition.NodeSize = Size;
	NewNode->AllocateDefaultPins();
	NewNode->SyncDefinitionFromGraph();
	ParentGraph->AddNode(NewNode, true, bSelectNewNode);

	// Dragging off a pin and releasing on empty canvas should finish the connection it started.
	if (FromPin)
	{
		UEdGraphPin* TargetPin = FromPin->Direction == EGPD_Output
			? NewNode->GetParentPin()
			: NewNode->GetChildPin();
		if (TargetPin)
		{
			FromPin->MakeLinkTo(TargetPin);
		}
	}

	ParentGraph->NotifyGraphChanged();
	return NewNode;
}

void UPHPassiveTreeGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const TSharedPtr<FPHPassiveTreeSchemaAction_NewNode> AddSmall = MakeShared<FPHPassiveTreeSchemaAction_NewNode>(
		LOCTEXT("PassiveCategory", "Passive"),
		LOCTEXT("AddSmall", "Add Small Passive"),
		LOCTEXT("AddSmallTooltip", "A small node. Only these are eligible for a character's random start."),
		0);
	AddSmall->Size = EPHPassiveNodeSize::Small;
	ContextMenuBuilder.AddAction(AddSmall);

	const TSharedPtr<FPHPassiveTreeSchemaAction_NewNode> AddMajor = MakeShared<FPHPassiveTreeSchemaAction_NewNode>(
		LOCTEXT("PassiveCategory", "Passive"),
		LOCTEXT("AddMajor", "Add Major Passive"),
		LOCTEXT("AddMajorTooltip", "A milestone node. Drawn larger and never rolled as a random start."),
		0);
	AddMajor->Size = EPHPassiveNodeSize::Major;
	ContextMenuBuilder.AddAction(AddMajor);
}

const FPinConnectionResponse UPHPassiveTreeGraphSchema::CanCreateConnection(
	const UEdGraphPin* A,
	const UEdGraphPin* B) const
{
	if (!A || !B)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("BadPin", "Invalid pin."));
	}
	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW,
			LOCTEXT("SelfLink", "A passive cannot connect to itself."));
	}
	if (A->Direction == B->Direction)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW,
			LOCTEXT("SameDirection", "Connect a Children pin to a Parents pin."));
	}
	if (A->LinkedTo.Contains(B))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW,
			LOCTEXT("Duplicate", "These passives are already connected."));
	}

	// Many-to-many with no cycle check on purpose: loops are how a cluster gets approached from more
	// than one direction, which is the shape this tree is aiming for.
	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("Connect", "Connect these passives."));
}

FLinearColor UPHPassiveTreeGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FLinearColor(0.27f, 0.77f, 0.72f);
}

void UPHPassiveTreeGraphSchema::GetContextMenuActions(
	UToolMenu* Menu,
	UGraphNodeContextMenuContext* Context) const
{
	if (Context && Context->Node)
	{
		FToolMenuSection& Section = Menu->AddSection(
			TEXT("PHPassiveNodeActions"), LOCTEXT("NodeActions", "Passive Node"));
		Section.AddMenuEntry(FGenericCommands::Get().Delete);
		Section.AddMenuEntry(FGenericCommands::Get().Duplicate);
	}

	Super::GetContextMenuActions(Menu, Context);
}

#undef LOCTEXT_NAMESPACE

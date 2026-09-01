// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "Passive/PHPassiveTreeGraphFactories.h"

#include "Passive/PHPassiveTreeGraphNode.h"
#include "Passive/PHPassiveTreeGraphSchema.h"
#include "Passive/SPHPassiveTreeGraphNodeWidget.h"
#include "Rendering/DrawElements.h"
#include "SNodePanel.h"
#include "Styling/AppStyle.h"
#include "UI/Library/PHUIStyle.h"

TSharedPtr<SGraphNode> FPHPassiveTreeNodeFactory::CreateNode(UEdGraphNode* Node) const
{
	if (UPHPassiveTreeGraphNode* PassiveNode = Cast<UPHPassiveTreeGraphNode>(Node))
	{
		return SNew(SPHPassiveTreeGraphNodeWidget, PassiveNode);
	}
	return nullptr;
}

FPHPassiveTreeConnectionDrawingPolicy::FPHPassiveTreeConnectionDrawingPolicy(
	const int32 InBackLayerID,
	const int32 InFrontLayerID,
	const float InZoomFactor,
	const FSlateRect& InClippingRect,
	FSlateWindowElementList& InDrawElements)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements)
{
	// No arrowheads. Under RequireAny a connection opens in both directions, so an arrow would assert
	// a one-way relationship the runtime does not have.
	ArrowImage = nullptr;
	ArrowRadius = FVector2f::ZeroVector;
}

void FPHPassiveTreeConnectionDrawingPolicy::Draw(
	TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries,
	FArrangedChildren& ArrangedNodes)
{
	// #1E3C69 in linear: the System window's field colour. Held just short of opaque so the editor's
	// own grid still reads faintly through it, which is what the in-game panel's grid looks like.
	static const FLinearColor Backdrop(0.012f, 0.045f, 0.143f, 0.92f);

	// One layer below the wires: above the shared grey background, below every node.
	const int32 BackdropLayer = WireLayerID - 1;
	const FVector2f PanelTopLeft(ClippingRect.Left, ClippingRect.Top);
	const FVector2f PanelSize(ClippingRect.GetSize());
	const FPaintGeometry PanelGeometry(PanelTopLeft, PanelSize, 1.0f);

	FSlateDrawElement::MakeBox(
		DrawElementsList, BackdropLayer, PanelGeometry,
		FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Backdrop);

	// The backdrop hides the editor's own grid, so the System window's grid is redrawn on top of it -
	// same 90-unit spacing and tint the runtime uses.
	//
	// The graph-to-screen transform is recovered from an arranged node rather than tracked: the policy
	// is handed a zoom factor but no pan offset, and a node's screen position paired with its graph
	// coordinates supplies the missing half. Without that the grid would not pan with the tree.
	static const FLinearColor GridColor(0.20f, 0.55f, 0.88f, 0.12f);
	constexpr float GridSpacing = 90.0f;

	const UEdGraphNode* ReferenceNode = nullptr;
	FVector2f ReferenceScreenPosition = FVector2f::ZeroVector;
	for (int32 Index = 0; Index < ArrangedNodes.Num(); ++Index)
	{
		const TSharedRef<SNodePanel::SNode> NodeWidget =
			StaticCastSharedRef<SNodePanel::SNode>(ArrangedNodes[Index].Widget);
		if (const UEdGraphNode* Node = Cast<UEdGraphNode>(NodeWidget->GetObjectBeingDisplayed()))
		{
			ReferenceNode = Node;
			ReferenceScreenPosition = FVector2f(ArrangedNodes[Index].Geometry.AbsolutePosition);
			break;
		}
	}

	const float Step = GridSpacing * ZoomFactor;
	if (ReferenceNode && Step >= 6.0f)
	{
		const FVector2f GraphOrigin = ReferenceScreenPosition -
			FVector2f(ReferenceNode->NodePosX, ReferenceNode->NodePosY) * ZoomFactor - PanelTopLeft;

		for (float X = FMath::Fmod(GraphOrigin.X, Step); X < PanelSize.X; X += Step)
		{
			FSlateDrawElement::MakeLines(
				DrawElementsList, BackdropLayer, PanelGeometry,
				{FVector2D(X, 0.0f), FVector2D(X, PanelSize.Y)},
				ESlateDrawEffect::None, GridColor, true, 1.0f);
		}
		for (float Y = FMath::Fmod(GraphOrigin.Y, Step); Y < PanelSize.Y; Y += Step)
		{
			FSlateDrawElement::MakeLines(
				DrawElementsList, BackdropLayer, PanelGeometry,
				{FVector2D(0.0f, Y), FVector2D(PanelSize.X, Y)},
				ESlateDrawEffect::None, GridColor, true, 1.0f);
		}
	}

	// Wires run centre to centre rather than pin to pin.
	//
	// In game a connector is just a segment between two node centres, trimmed at the silhouette.
	// Routing pin to pin instead sends the wire looping around the node whenever a child sits to the
	// left of its parent - most of the Vanguard branch does - and the graph stops resembling the tree
	// it is authoring. Wires paint below nodes, so the node bodies trim these for free.
	//
	// This replaces the base Draw rather than adding to it, which costs wire hover-highlighting. Links
	// are still broken from a pin's own context menu, and in a passive tree the interaction that
	// matters is with nodes.
	TMap<const UEdGraphNode*, FVector2f> Centres;
	Centres.Reserve(ArrangedNodes.Num());
	for (int32 Index = 0; Index < ArrangedNodes.Num(); ++Index)
	{
		const TSharedRef<SNodePanel::SNode> NodeWidget =
			StaticCastSharedRef<SNodePanel::SNode>(ArrangedNodes[Index].Widget);
		if (const UEdGraphNode* Node = Cast<UEdGraphNode>(NodeWidget->GetObjectBeingDisplayed()))
		{
			Centres.Add(Node, FVector2f(
				ArrangedNodes[Index].Geometry.GetAbsolutePositionAtCoordinates(FVector2D(0.5, 0.5))));
		}
	}

	const float WireThickness = FMath::Max(1.5f, 2.0f * ZoomFactor);
	for (const TPair<const UEdGraphNode*, FVector2f>& Entry : Centres)
	{
		// Walking parent pins only means each connection is drawn exactly once.
		const UPHPassiveTreeGraphNode* Node = Cast<UPHPassiveTreeGraphNode>(Entry.Key);
		const UEdGraphPin* ParentPin = Node ? Node->GetParentPin() : nullptr;
		if (!ParentPin)
		{
			continue;
		}

		for (const UEdGraphPin* LinkedPin : ParentPin->LinkedTo)
		{
			const UEdGraphNode* Parent = LinkedPin ? LinkedPin->GetOwningNode() : nullptr;
			const FVector2f* ParentCentre = Parent ? Centres.Find(Parent) : nullptr;
			if (!ParentCentre)
			{
				continue;
			}
			FSlateDrawElement::MakeLines(
				DrawElementsList, WireLayerID, PanelGeometry,
				{FVector2D(Entry.Value - PanelTopLeft), FVector2D(*ParentCentre - PanelTopLeft)},
				ESlateDrawEffect::None, PHUIStyle::HeaderTeal, true, WireThickness);
		}
	}
}

void FPHPassiveTreeConnectionDrawingPolicy::DetermineWiringStyle(
	UEdGraphPin* OutputPin,
	UEdGraphPin* InputPin,
	FConnectionParams& Params)
{
	FConnectionDrawingPolicy::DetermineWiringStyle(OutputPin, InputPin, Params);
	Params.WireColor = PHUIStyle::HeaderTeal;
	Params.WireThickness = 2.0f;
	// Direction is authoring bookkeeping under RequireAny, so an arrowhead would imply a one-way
	// relationship the runtime does not have.
	Params.bDrawBubbles = false;
}

FVector2f FPHPassiveTreeConnectionDrawingPolicy::ComputeSplineTangent(
	const FVector2f& Start,
	const FVector2f& End) const
{
	// A cubic whose control points lie on the straight line between the endpoints draws as that line.
	return End - Start;
}

FConnectionDrawingPolicy* FPHPassiveTreeConnectionFactory::CreateConnectionPolicy(
	const UEdGraphSchema* Schema,
	const int32 InBackLayerID,
	const int32 InFrontLayerID,
	const float ZoomFactor,
	const FSlateRect& InClippingRect,
	FSlateWindowElementList& InDrawElements,
	UEdGraph* InGraphObj) const
{
	if (Schema && Schema->IsA<UPHPassiveTreeGraphSchema>())
	{
		return new FPHPassiveTreeConnectionDrawingPolicy(
			InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements);
	}
	return nullptr;
}

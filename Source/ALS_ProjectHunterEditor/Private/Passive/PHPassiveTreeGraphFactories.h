// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "ConnectionDrawingPolicy.h"
#include "EdGraphUtilities.h"

/** Hands passive nodes their System-window visual instead of the default graph node body. */
struct FPHPassiveTreeNodeFactory : public FGraphPanelNodeFactory
{
	virtual TSharedPtr<SGraphNode> CreateNode(UEdGraphNode* Node) const override;
};

/**
 * Straight connectors instead of the default Blueprint spline.
 *
 * The in-game tree draws every connection as a straight segment between two node borders, so a curved
 * wire here would misrepresent which nodes look adjacent - the thing this editor exists to get right.
 */
class FPHPassiveTreeConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
public:
	FPHPassiveTreeConnectionDrawingPolicy(
		int32 InBackLayerID,
		int32 InFrontLayerID,
		float InZoomFactor,
		const FSlateRect& InClippingRect,
		FSlateWindowElementList& InDrawElements);

	/**
	 * Also paints the canvas backdrop.
	 *
	 * The graph panel's own background brush comes from UEditorStyleSettings and is shared by every
	 * Blueprint and material graph in the project, so it cannot be recoloured for one asset. This
	 * override is the one hook that runs above that background and below the nodes, which makes it the
	 * only place a single graph can own its own field colour.
	 */
	virtual void Draw(
		TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries,
		FArrangedChildren& ArrangedNodes) override;

	virtual void DetermineWiringStyle(
		UEdGraphPin* OutputPin,
		UEdGraphPin* InputPin,
		FConnectionParams& Params) override;

	virtual FVector2f ComputeSplineTangent(const FVector2f& Start, const FVector2f& End) const override;
};

/** Routes the passive schema to the straight-line policy. */
struct FPHPassiveTreeConnectionFactory : public FGraphPanelPinConnectionFactory
{
	virtual FConnectionDrawingPolicy* CreateConnectionPolicy(
		const UEdGraphSchema* Schema,
		int32 InBackLayerID,
		int32 InFrontLayerID,
		float ZoomFactor,
		const FSlateRect& InClippingRect,
		FSlateWindowElementList& InDrawElements,
		UEdGraph* InGraphObj) const override;
};

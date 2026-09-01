// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

class UPHPassiveTreeGraphNode;

/**
 * Draws a passive node in the graph editor the way the System window draws it in game: the same
 * chamfered panel art, the same azure/teal palette, the same Small/Major footprints, and the name and
 * point cost laid out the same way.
 *
 * The sizes are deliberately the exact pixel sizes the runtime widget uses, and graph coordinates are
 * the same coordinates the runtime reads, so at 100% zoom the editor canvas is a true preview of the
 * tree rather than an abstract diagram of it. Getting a cluster to look right here means it looks
 * right in game.
 */
class SPHPassiveTreeGraphNodeWidget : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SPHPassiveTreeGraphNodeWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UPHPassiveTreeGraphNode* InNode);

	virtual void UpdateGraphNode() override;

	/**
	 * No drop shadow. SGraphNode draws a rounded-rect shadow sized to the node's bounding box, which
	 * on a chamfered hexagon shows up as a lighter rectangle poking out behind the silhouette - and
	 * the runtime tree has nothing like it.
	 */
	virtual const FSlateBrush* GetShadowBrush(bool bSelected) const override;
	virtual void CreatePinWidgets() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;

private:
	FVector2D GetNodeFootprint() const;
	FText GetTitleText() const;
	FText GetCostText() const;
	FSlateColor GetFillColor() const;
	FSlateColor GetFrameColor() const;

	TWeakObjectPtr<UPHPassiveTreeGraphNode> PassiveNode;
	FSlateBrush FillBrush;
	FSlateBrush FrameBrush;
};

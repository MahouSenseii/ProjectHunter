// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "Progression/Data/PHPassiveTreeDataAsset.h"
#include "PHPassiveTreeGraphNode.generated.h"

/**
 * One passive node as it appears in the graph editor.
 *
 * The graph is a view over UPHPassiveTreeDataAsset::Nodes, not a second copy of the truth. Two fields
 * of Definition are owned by the graph rather than by the details panel and are overwritten on every
 * sync: Position comes from where the node sits on the canvas, and RequiredNodeIDs comes from what is
 * wired into the parent pin. Everything else - name, cost, size, modifiers - is edited as normal.
 */
UCLASS()
class UPHPassiveTreeGraphNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Passive Node", meta = (ShowOnlyInnerProperties))
	FPHPassiveNodeDefinition Definition;

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual bool CanUserDeleteNode() const override { return true; }
	virtual bool CanDuplicateNode() const override { return true; }

	/** Nodes this one hangs off. Wiring into this pin is what writes RequiredNodeIDs. */
	UEdGraphPin* GetParentPin() const;

	/** Nodes that hang off this one. */
	UEdGraphPin* GetChildPin() const;

	/**
	 * The size the runtime draws this node at, which is also what the editor draws it at.
	 *
	 * This is not cosmetic: Definition.Position is the node's *centre* (the runtime derives its
	 * top-left from it), while NodePosX/Y is a top-left. Converting between them needs the footprint,
	 * and because roots, Major and Small nodes have different footprints, getting this wrong shifts
	 * each node by a different amount and silently distorts the authored layout.
	 */
	FVector2D GetFootprint() const;

	/** Places the node on the canvas from authored data when the editor opens. Centre -> top-left. */
	void ApplyGraphPosition();

	/**
	 * Writes the canvas back into Definition: connections from the parent pin, then position as a
	 * centre. Order matters, because wiring a node can change whether it is a root and therefore how
	 * big it is.
	 */
	void SyncDefinitionFromGraph();

	/** Node IDs are the save key, so a new or pasted node must land on one the graph is not using. */
	static FName MakeUniqueNodeID(const UEdGraph* Graph);

	static const FName ParentPinName;
	static const FName ChildPinName;
	static const FName PinCategory;

private:
	/** Rebuilds Definition.RequiredNodeIDs from whatever is currently wired into the parent pin. */
	void CaptureConnections();
};

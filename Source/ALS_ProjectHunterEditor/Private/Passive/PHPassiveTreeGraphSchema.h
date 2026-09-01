// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "Progression/Data/PHPassiveTreeDataAsset.h"
#include "PHPassiveTreeGraphSchema.generated.h"

/** Right-click "Add Small / Major Passive" on the canvas. */
USTRUCT()
struct FPHPassiveTreeSchemaAction_NewNode : public FEdGraphSchemaAction
{
	GENERATED_BODY()

	FPHPassiveTreeSchemaAction_NewNode() = default;

	FPHPassiveTreeSchemaAction_NewNode(
		FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
	{
	}

	static FName StaticGetTypeId()
	{
		static FName Type(TEXT("FPHPassiveTreeSchemaAction_NewNode"));
		return Type;
	}
	virtual FName GetTypeId() const override { return StaticGetTypeId(); }

	UPROPERTY()
	EPHPassiveNodeSize Size = EPHPassiveNodeSize::Small;

	virtual UEdGraphNode* PerformAction(
		UEdGraph* ParentGraph,
		UEdGraphPin* FromPin,
		const FVector2f& Location,
		bool bSelectNewNode = true) override;
};

/**
 * Connection rules for the passive graph.
 *
 * Deliberately permissive: a node may have many parents and many children, and loops are legal,
 * because a Path-of-Exile-shaped tree is full of clusters reachable from more than one side. The only
 * things refused are self-links and duplicates.
 */
UCLASS()
class UPHPassiveTreeGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

public:
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
	virtual void GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;
};

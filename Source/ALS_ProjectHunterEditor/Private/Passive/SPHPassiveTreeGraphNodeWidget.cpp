// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "Passive/SPHPassiveTreeGraphNodeWidget.h"

#include "Engine/Texture2D.h"
#include "Passive/PHPassiveTreeGraphNode.h"
#include "SGraphPanel.h"
#include "SGraphPin.h"
#include "Styling/CoreStyle.h"
#include "Styling/StyleDefaults.h"
#include "UI/Library/PHUIStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SPHPassiveTreeGraphNodeWidget"

namespace
{
	/**
	 * The tint the runtime uses for a node that is ready to take. Both sizes share it, exactly as they
	 * do in game - a Major node is distinguished by its footprint and its MAJOR label, not by colour.
	 */
	const FLinearColor NodeFill(0.10f, 0.42f, 0.78f, 1.0f);

	/**
	 * A bare connection point. The runtime tree has no pin furniture - connectors simply meet the node
	 * silhouette - so these sit on the hexagon's left and right points and show no label.
	 */
	class SPHPassiveTreePin final : public SGraphPin
	{
	public:
		SLATE_BEGIN_ARGS(SPHPassiveTreePin) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, UEdGraphPin* InPin)
		{
			SGraphPin::Construct(SGraphPin::FArguments().SideToSideMargin(0.0f), InPin);
			bShowLabel = false;
		}

	protected:
		virtual FSlateColor GetPinColor() const override
		{
			return PHUIStyle::HeaderTeal;
		}
	};
}

void SPHPassiveTreeGraphNodeWidget::Construct(const FArguments& InArgs, UPHPassiveTreeGraphNode* InNode)
{
	GraphNode = InNode;
	PassiveNode = InNode;

	// The same 9-slice setup the System window uses, so the chamfer reads identically at any size.
	FillBrush.DrawAs = ESlateBrushDrawType::Box;
	FillBrush.Margin = FMargin(0.1875f);
	FillBrush.Tiling = ESlateBrushTileType::NoTile;
	FillBrush.ImageType = ESlateBrushImageType::FullColor;
	FillBrush.SetResourceObject(LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/ProjectHunter/UI/Widgets/Menus/System/T_SystemPanel_Fill.T_SystemPanel_Fill")));

	FrameBrush.DrawAs = ESlateBrushDrawType::Box;
	FrameBrush.Margin = FMargin(0.1875f);
	FrameBrush.Tiling = ESlateBrushTileType::NoTile;
	FrameBrush.ImageType = ESlateBrushImageType::FullColor;
	FrameBrush.SetResourceObject(LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/ProjectHunter/UI/Widgets/Menus/System/T_SystemPanel_Frame.T_SystemPanel_Frame")));

	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

const FSlateBrush* SPHPassiveTreeGraphNodeWidget::GetShadowBrush(bool bSelected) const
{
	return FStyleDefaults::GetNoBrush();
}

FVector2D SPHPassiveTreeGraphNodeWidget::GetNodeFootprint() const
{
	// Deferred to the node so the size drawn here and the size used to convert Definition.Position
	// between a centre and a top-left can never disagree.
	const UPHPassiveTreeGraphNode* Node = PassiveNode.Get();
	return Node ? Node->GetFootprint() : FVector2D(180.0, 74.0);
}

FText SPHPassiveTreeGraphNodeWidget::GetTitleText() const
{
	const UPHPassiveTreeGraphNode* Node = PassiveNode.Get();
	if (!Node)
	{
		return FText::GetEmpty();
	}
	return FText::FromString(Node->GetNodeTitle(ENodeTitleType::ListView).ToString().ToUpper());
}

FText SPHPassiveTreeGraphNodeWidget::GetCostText() const
{
	const UPHPassiveTreeGraphNode* Node = PassiveNode.Get();
	if (!Node)
	{
		return FText::GetEmpty();
	}
	return Node->Definition.NodeSize == EPHPassiveNodeSize::Major
		? FText::Format(LOCTEXT("MajorCost", "MAJOR  •  {0} PT"), FText::AsNumber(Node->Definition.PointCost))
		: FText::Format(LOCTEXT("SmallCost", "{0} PT"), FText::AsNumber(Node->Definition.PointCost));
}

FSlateColor SPHPassiveTreeGraphNodeWidget::GetFillColor() const
{
	return NodeFill;
}

FSlateColor SPHPassiveTreeGraphNodeWidget::GetFrameColor() const
{
	// Selection is the one piece of editor state worth showing on the node itself.
	if (GraphNode && GetOwnerPanel().IsValid() && GetOwnerPanel()->SelectionManager.IsNodeSelected(GraphNode))
	{
		return PHUIStyle::GradeA;
	}
	return PHUIStyle::HeaderTeal;
}

void SPHPassiveTreeGraphNodeWidget::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	const FVector2D Footprint = GetNodeFootprint();
	ContentScale.Bind(this, &SGraphNode::GetContentScale);

	GetOrAddSlot(ENodeZone::Center)
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SBox)
		.WidthOverride(Footprint.X)
		.HeightOverride(Footprint.Y)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(&FillBrush)
				.ColorAndOpacity(this, &SPHPassiveTreeGraphNodeWidget::GetFillColor)
			]
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(&FrameBrush)
				.ColorAndOpacity(this, &SPHPassiveTreeGraphNodeWidget::GetFrameColor)
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(FMargin(26.0f, 4.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SPHPassiveTreeGraphNodeWidget::GetTitleText)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(PHUIStyle::TextPrimary)
					.Justification(ETextJustify::Center)
					.WrapTextAt(Footprint.X - 34.0f)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(this, &SPHPassiveTreeGraphNodeWidget::GetCostText)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					.ColorAndOpacity(PHUIStyle::TextDim)
				]
			]
			// The pins sit on the hexagon's left and right points, which is where the runtime trims
			// its connectors to, so a wire in the editor lands where the connector lands in game.
			+ SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SAssignNew(LeftNodeBox, SVerticalBox)
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			[
				SAssignNew(RightNodeBox, SVerticalBox)
			]
		]
	];

	CreatePinWidgets();
}

void SPHPassiveTreeGraphNodeWidget::CreatePinWidgets()
{
	const UPHPassiveTreeGraphNode* Node = PassiveNode.Get();
	if (!Node)
	{
		return;
	}

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && !Pin->bHidden)
		{
			AddPin(SNew(SPHPassiveTreePin, Pin));
		}
	}
}

void SPHPassiveTreeGraphNodeWidget::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	const UEdGraphPin* Pin = PinToAdd->GetPinObj();
	const bool bIsInput = Pin && Pin->Direction == EGPD_Input;

	const TSharedPtr<SVerticalBox> TargetBox = bIsInput ? LeftNodeBox : RightNodeBox;
	if (TargetBox.IsValid())
	{
		TargetBox->AddSlot()
			.AutoHeight()
			.HAlign(bIsInput ? HAlign_Left : HAlign_Right)
			.VAlign(VAlign_Center)
			[
				PinToAdd
			];
	}

	if (bIsInput)
	{
		InputPins.Add(PinToAdd);
	}
	else
	{
		OutputPins.Add(PinToAdd);
	}
}

#undef LOCTEXT_NAMESPACE

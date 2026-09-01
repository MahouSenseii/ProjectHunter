#include "UI/Menu/Widgets/PHPassiveTreeMenuPageWidget.h"

#include "Character/PHBaseCharacter.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Reply.h"
#include "Math/Box2D.h"
#include "Progression/Components/CharacterProgressionManager.h"
#include "Progression/Components/PHPassiveTreeComponent.h"
#include "Progression/Data/PHPassiveTreeDataAsset.h"
#include "Styling/CoreStyle.h"
#include "UI/Library/PHUIStyle.h"
#include "UI/Menu/Helpers/MenuRowBuilder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FLinearColor GraphBackground(0.006f, 0.035f, 0.11f, 0.72f);
	const FLinearColor GraphGrid(0.20f, 0.55f, 0.88f, 0.12f);
	const FLinearColor NodeAllocatedFill = PHUIStyle::HeaderTeal;
	const FLinearColor NodeAvailableFill(0.10f, 0.42f, 0.78f, 1.0f);
	const FLinearColor NodeLockedFill(0.020f, 0.075f, 0.150f, 0.90f);
	const FLinearColor SearchHighlight = PHUIStyle::GradeA;

	/** Edge tints, brightest for a path the Hunter actually owns. */
	const FLinearColor EdgeAllocated(1.0f, 1.0f, 1.0f, 0.95f);
	const FLinearColor EdgeReachable = PHUIStyle::HeaderTeal;
	const FLinearColor EdgeLocked(0.25f, 0.45f, 0.65f, 0.32f);

	constexpr float MinZoom = 0.10f;
	constexpr float MaxZoom = 2.00f;

	/** Below these the labels are unreadable anyway, and skipping them is what keeps a huge tree cheap. */
	constexpr float TitleVisibleZoom = 0.45f;
	constexpr float CostVisibleZoom = 0.62f;

	FPaintGeometry PaintAt(const FGeometry& Geometry, const FVector2D Position, const FVector2D Size)
	{
		return Geometry.ToPaintGeometry(
			FVector2f(static_cast<float>(Size.X), static_cast<float>(Size.Y)),
			FSlateLayoutTransform(FVector2f(static_cast<float>(Position.X), static_cast<float>(Position.Y))));
	}

	/**
	 * Fraction of a node's width taken by the chamfer at each end, matching the 9-slice margin of the
	 * System panel textures. It is what turns the node's box into the hexagon the player actually sees.
	 */
	constexpr double NodeChamferRatio = 0.1875;

	/**
	 * Trims a centre-to-centre segment back to the two node borders so a connector reads as joining
	 * two nodes instead of running underneath them.
	 *
	 * The node silhouette is a hexagon: a box with both ends chamfered to a point at mid-height. For a
	 * convex shape around the origin the exit distance along a unit direction D is the smallest
	 * d / (N . D) over the edges facing D. Here that collapses to two closed forms - the flat caps give
	 * HalfHeight / |D.y|, and all four slanted edges give the same
	 * HalfWidth * HalfHeight / (HalfHeight * |D.x| + Chamfer * |D.y|) - so no per-edge loop is needed.
	 * Trimming to the box instead would leave a visible gap on every diagonal, where the box corner
	 * sits well outside the hexagon.
	 *
	 * Returns false when the trimmed span is empty, which happens when two nodes overlap.
	 */
	bool TrimSegmentToNodeBorders(
		const FVector2D FromCentre,
		const FVector2D FromHalfSize,
		const FVector2D ToCentre,
		const FVector2D ToHalfSize,
		const float Gap,
		FVector2D& OutStart,
		FVector2D& OutEnd)
	{
		const FVector2D Delta = ToCentre - FromCentre;
		const double Length = Delta.Size();
		if (Length <= UE_KINDA_SMALL_NUMBER)
		{
			return false;
		}

		const FVector2D Direction = Delta / Length;
		auto BorderDistance = [&Direction](const FVector2D& HalfSize)
		{
			const double AbsX = FMath::Abs(Direction.X);
			const double AbsY = FMath::Abs(Direction.Y);
			const double Chamfer = HalfSize.X * 2.0 * NodeChamferRatio;

			double Distance = TNumericLimits<double>::Max();
			const double SlantDenominator = HalfSize.Y * AbsX + Chamfer * AbsY;
			if (SlantDenominator > UE_KINDA_SMALL_NUMBER)
			{
				Distance = HalfSize.X * HalfSize.Y / SlantDenominator;
			}
			if (AbsY > UE_KINDA_SMALL_NUMBER)
			{
				Distance = FMath::Min(Distance, HalfSize.Y / AbsY);
			}
			return Distance;
		};

		const double StartDistance = BorderDistance(FromHalfSize) + Gap;
		const double EndDistance = Length - BorderDistance(ToHalfSize) - Gap;
		if (EndDistance <= StartDistance)
		{
			return false;
		}

		OutStart = FromCentre + Direction * StartDistance;
		OutEnd = FromCentre + Direction * EndDistance;
		return true;
	}

	/**
	 * The node silhouette as a closed polyline, using the same chamfer the connector trim assumes.
	 *
	 * Needed because the node brushes are full-colour textures: a brush tint multiplies against the
	 * texture's own colour, so a panel brush can never be made to read as amber. MakeLines takes the
	 * colour directly, which is the only way to draw a marker in a hue the panel art does not contain.
	 */
	TArray<FVector2D> MakeNodeOutline(const FVector2D Centre, const FVector2D HalfSize)
	{
		const double A = HalfSize.X;
		const double B = HalfSize.Y;
		const double C = A * 2.0 * NodeChamferRatio;
		return {
			Centre + FVector2D(-A, 0.0),
			Centre + FVector2D(-A + C, -B),
			Centre + FVector2D(A - C, -B),
			Centre + FVector2D(A, 0.0),
			Centre + FVector2D(A - C, B),
			Centre + FVector2D(-A + C, B),
			Centre + FVector2D(-A, 0.0)
		};
	}

	const FButtonStyle& SystemButtonStyle()
	{
		static const FButtonStyle Style = []
		{
			FButtonStyle Result = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
			Result.Normal.TintColor = FSlateColor(FLinearColor(PHUIStyle::AzureDeep.R, PHUIStyle::AzureDeep.G, PHUIStyle::AzureDeep.B, 0.85f));
			Result.Hovered.TintColor = FSlateColor(PHUIStyle::Azure);
			Result.Pressed.TintColor = FSlateColor(PHUIStyle::HeaderTeal);
			return Result;
		}();
		return Style;
	}
}

/**
 * Pannable, zoomable passive graph.
 *
 * Everything the paint pass needs is precomputed into flat arrays: node geometry and edges when the
 * tree asset changes, allocation state when the character's allocations or points change. OnPaint
 * then only culls and draws. That split is what lets the same widget carry a tree with thousands of
 * nodes - the old shape resolved every connection and re-asked the component about every node on
 * every frame, which is quadratic in node count.
 */
class SPHPassiveTreeGraph final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SPHPassiveTreeGraph) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UPHPassiveTreeMenuPageWidget>, OwnerWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args)
	{
		OwnerWidget = Args._OwnerWidget;
		NodeFillBrush.DrawAs = ESlateBrushDrawType::Box;
		NodeFillBrush.Margin = FMargin(0.1875f);
		NodeFillBrush.Tiling = ESlateBrushTileType::NoTile;
		NodeFillBrush.ImageType = ESlateBrushImageType::FullColor;
		NodeFillBrush.SetResourceObject(LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/ProjectHunter/UI/Widgets/Menus/System/T_SystemPanel_Fill.T_SystemPanel_Fill")));
		NodeFrameBrush.DrawAs = ESlateBrushDrawType::Box;
		NodeFrameBrush.Margin = FMargin(0.1875f);
		NodeFrameBrush.Tiling = ESlateBrushTileType::NoTile;
		NodeFrameBrush.ImageType = ESlateBrushImageType::FullColor;
		NodeFrameBrush.SetResourceObject(LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/ProjectHunter/UI/Widgets/Menus/System/T_SystemPanel_Frame.T_SystemPanel_Frame")));
		SetCanTick(false);
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(1100.0f, 650.0f);
	}

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		const int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		const bool bParentEnabled) const override
	{
		const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
		CachedLocalSize = LocalSize;

		const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
		OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry));
		FSlateDrawElement::MakeBox(
			OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), WhiteBrush,
			ESlateDrawEffect::None, GraphBackground);

		const float GridStep = 90.0f * Zoom;
		if (GridStep >= 22.0f)
		{
			const FVector2D Center = LocalSize * 0.5f + PanOffset;
			const float StartX = FMath::Fmod(Center.X, GridStep);
			const float StartY = FMath::Fmod(Center.Y, GridStep);
			for (float X = StartX; X < LocalSize.X; X += GridStep)
			{
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1,
					AllottedGeometry.ToPaintGeometry(), {FVector2D(X, 0.0f), FVector2D(X, LocalSize.Y)},
					ESlateDrawEffect::None, GraphGrid, true, 1.0f);
			}
			for (float Y = StartY; Y < LocalSize.Y; Y += GridStep)
			{
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1,
					AllottedGeometry.ToPaintGeometry(), {FVector2D(0.0f, Y), FVector2D(LocalSize.X, Y)},
					ESlateDrawEffect::None, GraphGrid, true, 1.0f);
			}
		}

		if (NodeViews.IsEmpty())
		{
			FSlateDrawElement::MakeText(
				OutDrawElements, LayerId + 2,
				PaintAt(AllottedGeometry, FVector2D(28.0f, 28.0f), LocalSize - FVector2D(56.0f, 56.0f)),
				TEXT("NO PASSIVE TREE CONFIGURED"), PHMenuRowBuilder::MenuFont(18),
				ESlateDrawEffect::None, PHUIStyle::TextDim);
			OutDrawElements.PopClip();
			return LayerId + 2;
		}

		// One local-space rect covers culling for both passes. Padding it by the widest node keeps a
		// node whose centre is off-screen but whose body is not from popping out at the edges.
		const FSlateRect ViewRect(
			-MaxNodeHalfSize.X * Zoom, -MaxNodeHalfSize.Y * Zoom,
			LocalSize.X + MaxNodeHalfSize.X * Zoom, LocalSize.Y + MaxNodeHalfSize.Y * Zoom);

		const int32 EdgeLayer = LayerId + 2;
		const float EdgeGap = 3.0f * Zoom;
		for (const FPassiveEdge& Edge : Edges)
		{
			const FPassiveNodeView& From = NodeViews[Edge.FromIndex];
			const FPassiveNodeView& To = NodeViews[Edge.ToIndex];
			const FVector2D FromCentre = GraphToLocal(From.Position, LocalSize);
			const FVector2D ToCentre = GraphToLocal(To.Position, LocalSize);
			if (!DoesSegmentTouchView(FromCentre, ToCentre, ViewRect))
			{
				continue;
			}

			FVector2D Start;
			FVector2D End;
			if (!TrimSegmentToNodeBorders(
				FromCentre, From.HalfSize * Zoom, ToCentre, To.HalfSize * Zoom, EdgeGap, Start, End))
			{
				continue;
			}

			const bool bBothAllocated = From.bAllocated && To.bAllocated;
			const bool bOneAllocated = From.bAllocated || To.bAllocated;
			const FLinearColor Tint = bBothAllocated ? EdgeAllocated : (bOneAllocated ? EdgeReachable : EdgeLocked);
			const float Thickness = (bBothAllocated ? 4.0f : (bOneAllocated ? 2.6f : 1.8f)) * FMath::Max(Zoom, 0.5f);
			FSlateDrawElement::MakeLines(
				OutDrawElements, bBothAllocated ? EdgeLayer + 1 : EdgeLayer,
				AllottedGeometry.ToPaintGeometry(), {Start, End},
				ESlateDrawEffect::None, Tint, true, Thickness);
		}

		const bool bSearching = !SearchText.IsEmpty();
		const bool bShowTitles = Zoom >= TitleVisibleZoom;
		const bool bShowCosts = Zoom >= CostVisibleZoom;
		const FSlateBrush* FillBrush = NodeFillBrush.GetResourceObject() ? &NodeFillBrush : WhiteBrush;
		const FSlateBrush* FrameBrush = NodeFrameBrush.GetResourceObject() ? &NodeFrameBrush : WhiteBrush;
		const TSharedRef<FSlateFontMeasure> FontMeasure =
			FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

		for (const FPassiveNodeView& View : NodeViews)
		{
			const FVector2D Size = View.HalfSize * 2.0f * Zoom;
			const FVector2D Centre = GraphToLocal(View.Position, LocalSize);
			const FVector2D TopLeft = Centre - Size * 0.5f;
			if (Centre.X < ViewRect.Left || Centre.X > ViewRect.Right ||
				Centre.Y < ViewRect.Top || Centre.Y > ViewRect.Bottom)
			{
				continue;
			}

			// A search dims everything it did not match so the hits stand out in a crowded graph.
			const float SearchFade = (bSearching && !View.bSearchMatch) ? 0.25f : 1.0f;
			const bool bHovered = HoveredNodeID == View.NodeID;

			// Allocated nodes paint above available ones, which paint above locked ones, so an owned
			// path stays legible where clusters overlap.
			const int32 StateLayer = LayerId + 4 + (View.bAllocated ? 8 : (View.bAvailable ? 4 : 0));

			FLinearColor Fill = View.bAllocated ? NodeAllocatedFill
				: (View.bAvailable ? NodeAvailableFill : NodeLockedFill);
			FLinearColor Border = View.bAllocated ? FLinearColor::White
				: (View.bAvailable ? PHUIStyle::HeaderTeal : FLinearColor(PHUIStyle::TextDim.R, PHUIStyle::TextDim.G, PHUIStyle::TextDim.B, 0.32f));
			if (bHovered)
			{
				Border = FLinearColor::White;
			}
			if (View.bSearchMatch && bSearching)
			{
				Border = SearchHighlight;
			}
			Fill.A *= SearchFade;
			Border.A *= SearchFade;

			// The halo is what makes an owned node obvious at a glance and at low zoom, where the
			// fill colour alone is only a few pixels of difference.
			if (View.bAllocated || (View.bSearchMatch && bSearching))
			{
				// The origin's halo is wider so it still reads as special once zoomed out past the
				// point where its outline is legible.
				const float Halo = FMath::Max(3.0f, (View.bOrigin ? 11.0f : 7.0f) * Zoom);
				const FLinearColor HaloTint = View.bAllocated
					? FLinearColor(NodeAllocatedFill.R, NodeAllocatedFill.G, NodeAllocatedFill.B, 0.45f * SearchFade)
					: FLinearColor(SearchHighlight.R, SearchHighlight.G, SearchHighlight.B, 0.45f);
				FSlateDrawElement::MakeBox(
					OutDrawElements, StateLayer,
					PaintAt(AllottedGeometry, TopLeft - FVector2D(Halo), Size + FVector2D(Halo * 2.0f)),
					FrameBrush, ESlateDrawEffect::None, HaloTint);
			}

			FSlateDrawElement::MakeBox(
				OutDrawElements, StateLayer + 1,
				PaintAt(AllottedGeometry, TopLeft, Size), FillBrush, ESlateDrawEffect::None, Fill);
			FSlateDrawElement::MakeBox(
				OutDrawElements, StateLayer + 2,
				PaintAt(AllottedGeometry, TopLeft, Size), FrameBrush, ESlateDrawEffect::None, Border);

			// A second, non-colour cue for the allocated state: an inset second ring, so an owned node
			// reads as double-outlined at any zoom and for a player who cannot separate the two blues.
			// It reuses the frame brush rather than a corner pip because the node art is a chamfered
			// hexagon - anything drawn in the bounding box corners lands on transparent pixels.
			if (View.bAllocated)
			{
				const float Inset = FMath::Max(2.0f, 5.0f * Zoom);
				if (View.bOrigin)
				{
					// The rolled origin is the one node the player did not choose, so it gets the only
					// amber in the graph. Drawn as lines rather than a brush because the panel textures
					// are full-colour and a brush tint only multiplies against them - see MakeNodeOutline.
					// It sits inside the frame, where the white ring would be, so connectors that stop
					// on the node border never cross it.
					FSlateDrawElement::MakeLines(
						OutDrawElements, StateLayer + 3, AllottedGeometry.ToPaintGeometry(),
						MakeNodeOutline(Centre, View.HalfSize * Zoom - FVector2D(Inset)),
						ESlateDrawEffect::None,
						FLinearColor(PHUIStyle::GradeA.R, PHUIStyle::GradeA.G, PHUIStyle::GradeA.B, SearchFade),
						true, FMath::Max(2.0f, 3.0f * Zoom));
				}
				else
				{
					FSlateDrawElement::MakeBox(
						OutDrawElements, StateLayer + 3,
						PaintAt(AllottedGeometry, TopLeft + FVector2D(Inset), Size - FVector2D(Inset * 2.0f)),
						FrameBrush, ESlateDrawEffect::None,
						FLinearColor(1.0f, 1.0f, 1.0f, 0.9f * SearchFade));
				}
			}

			if (!bShowTitles)
			{
				continue;
			}

			const FLinearColor TextTint = View.bAllocated || View.bAvailable
				? FLinearColor(PHUIStyle::TextPrimary.R, PHUIStyle::TextPrimary.G, PHUIStyle::TextPrimary.B, SearchFade)
				: FLinearColor(PHUIStyle::TextDim.R, PHUIStyle::TextDim.G, PHUIStyle::TextDim.B, 0.75f * SearchFade);

			const int32 TitleFontSize = FMath::Max(6, FMath::RoundToInt(View.FittedTitleSize * Zoom));
			const FSlateFontInfo TitleFont = PHMenuRowBuilder::MenuFont(TitleFontSize, View.TitleTypeface);
			const FVector2D TitleSize = FontMeasure->Measure(View.Label, TitleFont);
			const FVector2D TitlePosition(
				Centre.X - TitleSize.X * 0.5f,
				TopLeft.Y + Size.Y * 0.43f - TitleSize.Y * 0.5f);
			FSlateDrawElement::MakeText(
				OutDrawElements, StateLayer + 3,
				PaintAt(AllottedGeometry, TitlePosition, TitleSize + FVector2D(2.0f)),
				View.Label, TitleFont, ESlateDrawEffect::None, TextTint);

			if (!bShowCosts)
			{
				continue;
			}

			const FSlateFontInfo CostFont = PHMenuRowBuilder::MenuFont(
				FMath::Max(6, FMath::RoundToInt(View.CostFontSize * Zoom)), View.TitleTypeface);
			const FVector2D CostSize = FontMeasure->Measure(View.CostLabel, CostFont);
			const FVector2D CostPosition(
				Centre.X - CostSize.X * 0.5f,
				TopLeft.Y + Size.Y * 0.72f - CostSize.Y * 0.5f);
			FSlateDrawElement::MakeText(
				OutDrawElements, StateLayer + 3,
				PaintAt(AllottedGeometry, CostPosition, CostSize + FVector2D(2.0f)),
				View.CostLabel, CostFont, ESlateDrawEffect::None, TextTint);
		}

		OutDrawElements.PopClip();
		return LayerId + 16;
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		const FKey Button = MouseEvent.GetEffectingButton();
		const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		if (Button == EKeys::LeftMouseButton)
		{
			PressedNodeID = FindNodeAt(Local, MyGeometry.GetLocalSize());
			LastPointer = Local;
			PressOrigin = Local;
			bPanning = PressedNodeID.IsNone();
			return FReply::Handled().CaptureMouse(AsShared());
		}
		if (Button == EKeys::RightMouseButton || Button == EKeys::MiddleMouseButton)
		{
			bPanning = true;
			LastPointer = Local;
			PressOrigin = Local;
			return FReply::Handled().CaptureMouse(AsShared());
		}
		return FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (!HasMouseCapture())
		{
			return FReply::Unhandled();
		}

		const FName ReleasedNode = PressedNodeID;
		PressedNodeID = NAME_None;
		const bool bWasPanning = bPanning;
		bPanning = false;
		if (!bWasPanning && !ReleasedNode.IsNone())
		{
			if (UPHPassiveTreeMenuPageWidget* Owner = OwnerWidget.Get())
			{
				Owner->RequestAllocateNode(ReleasedNode);
			}
		}
		return FReply::Handled().ReleaseMouseCapture();
	}

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		if (HasMouseCapture())
		{
			// A drag that starts on a node still pans once it passes the slop threshold, so the
			// player never has to hunt for empty space to grab in a densely packed tree.
			if (!bPanning && !PressedNodeID.IsNone() &&
				FVector2D::Distance(Local, PressOrigin) > 5.0)
			{
				bPanning = true;
				PressedNodeID = NAME_None;
			}
			if (bPanning)
			{
				PanOffset += Local - LastPointer;
				LastPointer = Local;
				ClampPan();
				Invalidate(EInvalidateWidgetReason::Paint);
				return FReply::Handled();
			}
		}

		const FName NewHoveredNodeID = FindNodeAt(Local, MyGeometry.GetLocalSize());
		if (HoveredNodeID != NewHoveredNodeID)
		{
			HoveredNodeID = NewHoveredNodeID;
			Invalidate(EInvalidateWidgetReason::Paint);
			if (UPHPassiveTreeMenuPageWidget* Owner = OwnerWidget.Get())
			{
				const UPHPassiveTreeDataAsset* Tree = Owner->GetTreeData();
				Owner->ShowNodeDetails(Tree ? Tree->FindNode(HoveredNodeID) : nullptr);
			}
		}
		return FReply::Unhandled();
	}

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override
	{
		SLeafWidget::OnMouseLeave(MouseEvent);
		if (!HasMouseCapture())
		{
			HoveredNodeID = NAME_None;
			Invalidate(EInvalidateWidgetReason::Paint);
			if (UPHPassiveTreeMenuPageWidget* Owner = OwnerWidget.Get())
			{
				Owner->ShowNodeDetails(nullptr);
			}
		}
	}

	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		// Multiplicative so each notch covers the same visual step at both ends of a wide zoom range.
		SetZoomAround(Zoom * FMath::Pow(1.12f, MouseEvent.GetWheelDelta()), Local, MyGeometry.GetLocalSize());
		return FReply::Handled();
	}

	void SetSearchText(const FString& NewSearch)
	{
		SearchText = NewSearch.TrimStartAndEnd();
		const UPHPassiveTreeMenuPageWidget* Owner = OwnerWidget.Get();
		const UPHPassiveTreeDataAsset* Tree = Owner ? Owner->GetTreeData() : nullptr;
		SearchMatchCount = 0;

		int32 FirstMatchIndex = INDEX_NONE;
		for (int32 Index = 0; Index < NodeViews.Num(); ++Index)
		{
			const bool bMatch = Tree && !SearchText.IsEmpty() &&
				Tree->Nodes.IsValidIndex(Index) && Tree->DoesNodeMatchSearch(Tree->Nodes[Index], SearchText);
			NodeViews[Index].bSearchMatch = bMatch;
			if (bMatch)
			{
				FirstMatchIndex = FirstMatchIndex == INDEX_NONE ? Index : FirstMatchIndex;
				++SearchMatchCount;
			}
		}

		if (FirstMatchIndex != INDEX_NONE)
		{
			PanOffset = -NodeViews[FirstMatchIndex].Position * Zoom;
			ClampPan();
		}
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	int32 GetSearchMatchCount() const { return SearchMatchCount; }
	float GetZoom() const { return Zoom; }
	int32 GetAllocatedCount() const { return AllocatedCount; }
	int32 GetNodeCount() const { return NodeViews.Num(); }

	void ChangeZoom(const float Delta)
	{
		SetZoomAround(Zoom + Delta, CachedLocalSize * 0.5f, CachedLocalSize);
	}

	/** Frames the whole graph rather than snapping to the origin, which a large tree can be far from. */
	void ResetView()
	{
		Zoom = 0.85f;
		PanOffset = FVector2D::ZeroVector;
		if (GraphBounds.bIsValid && CachedLocalSize.X > 0.0 && CachedLocalSize.Y > 0.0)
		{
			const FVector2D Extent = GraphBounds.GetSize() + MaxNodeHalfSize * 2.0f + FVector2D(80.0f);
			const double FitZoom = FMath::Min(CachedLocalSize.X / Extent.X, CachedLocalSize.Y / Extent.Y);
			Zoom = FMath::Clamp(static_cast<float>(FitZoom), MinZoom, 1.0f);
			PanOffset = -GraphBounds.GetCenter() * Zoom;
		}
		ClampPan();
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	/** Called whenever allocations or points change; rebuilds only what those affect. */
	void Refresh()
	{
		const UPHPassiveTreeMenuPageWidget* Owner = OwnerWidget.Get();
		const UPHPassiveTreeDataAsset* Tree = Owner ? Owner->GetTreeData() : nullptr;
		if (Tree != BuiltForTree.Get())
		{
			RebuildGeometry(Tree);
		}
		RebuildState();
		Invalidate(EInvalidateWidgetReason::Paint);
	}

private:
	struct FPassiveNodeView
	{
		FName NodeID = NAME_None;
		FVector2D Position = FVector2D::ZeroVector;
		FVector2D HalfSize = FVector2D::ZeroVector;
		FString Label;
		FString CostLabel;
		FName TitleTypeface = FName(TEXT("SemiBold"));
		int32 FittedTitleSize = 11;
		int32 CostFontSize = 8;
		bool bAllocated = false;
		bool bAvailable = false;
		bool bOrigin = false;
		bool bSearchMatch = false;
	};

	struct FPassiveEdge
	{
		int32 FromIndex = INDEX_NONE;
		int32 ToIndex = INDEX_NONE;
	};

	/** Tree-shaped data: positions, sizes, labels, fitted font sizes, and the deduplicated edge list. */
	void RebuildGeometry(const UPHPassiveTreeDataAsset* Tree)
	{
		NodeViews.Reset();
		Edges.Reset();
		GraphBounds.Init();
		MaxNodeHalfSize = FVector2D::ZeroVector;
		BuiltForTree = Tree;
		if (!Tree)
		{
			return;
		}

		const TSharedRef<FSlateFontMeasure> FontMeasure =
			FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

		NodeViews.Reserve(Tree->Nodes.Num());
		for (const FPHPassiveNodeDefinition& Node : Tree->Nodes)
		{
			const bool bRoot = Node.RequiredNodeIDs.IsEmpty();
			const bool bMajor = Node.NodeSize == EPHPassiveNodeSize::Major;
			const FVector2D Unscaled = bRoot ? FVector2D(250.0f, 100.0f)
				: (bMajor ? FVector2D(230.0f, 92.0f) : FVector2D(180.0f, 74.0f));

			FPassiveNodeView View;
			View.NodeID = Node.NodeID;
			View.Position = Node.Position;
			View.HalfSize = Unscaled * 0.5f;
			View.Label = Node.DisplayName.ToString().ToUpper();
			View.CostLabel = bMajor
				? FString::Printf(TEXT("MAJOR  •  %d PT"), Node.PointCost)
				: FString::Printf(TEXT("%d PT"), Node.PointCost);
			View.TitleTypeface = bMajor ? FName(TEXT("Bold")) : FName(TEXT("SemiBold"));
			View.CostFontSize = bMajor ? 9 : 8;

			// Shrink-to-fit is resolved once here at unit scale rather than per node per frame; the
			// paint pass just multiplies the result by the current zoom.
			int32 TitleSize = bMajor ? 14 : 11;
			const double Budget = Unscaled.X - 24.0;
			while (TitleSize > 6 && FontMeasure->Measure(View.Label, PHMenuRowBuilder::MenuFont(TitleSize, View.TitleTypeface)).X > Budget)
			{
				--TitleSize;
			}
			View.FittedTitleSize = TitleSize;

			MaxNodeHalfSize.X = FMath::Max(MaxNodeHalfSize.X, View.HalfSize.X);
			MaxNodeHalfSize.Y = FMath::Max(MaxNodeHalfSize.Y, View.HalfSize.Y);
			if (FMath::IsFinite(Node.Position.X) && FMath::IsFinite(Node.Position.Y))
			{
				GraphBounds += Node.Position;
			}
			NodeViews.Add(MoveTemp(View));
		}

		// A pair listed from both ends is one connector, not two overlapping ones.
		TSet<uint64> SeenPairs;
		for (int32 Index = 0; Index < Tree->Nodes.Num(); ++Index)
		{
			for (const FName ConnectedID : Tree->Nodes[Index].RequiredNodeIDs)
			{
				const int32 OtherIndex = Tree->FindNodeIndex(ConnectedID);
				if (OtherIndex == INDEX_NONE || OtherIndex == Index)
				{
					continue;
				}
				const uint64 Key = (static_cast<uint64>(FMath::Min(Index, OtherIndex)) << 32) |
					static_cast<uint32>(FMath::Max(Index, OtherIndex));
				if (SeenPairs.Contains(Key))
				{
					continue;
				}
				SeenPairs.Add(Key);
				Edges.Add({OtherIndex, Index});
			}
		}
	}

	/** Character-shaped data: which nodes are owned and which are currently affordable. */
	void RebuildState()
	{
		AllocatedCount = 0;
		const UPHPassiveTreeMenuPageWidget* Owner = OwnerWidget.Get();
		if (!Owner)
		{
			for (FPassiveNodeView& View : NodeViews)
			{
				View.bAllocated = false;
				View.bAvailable = false;
				View.bOrigin = false;
			}
			return;
		}

		FText UnusedReason;
		for (FPassiveNodeView& View : NodeViews)
		{
			View.bAllocated = Owner->IsNodeAllocated(View.NodeID);
			View.bAvailable = !View.bAllocated && Owner->CanAllocateNode(View.NodeID, UnusedReason);
			View.bOrigin = Owner->IsRandomStartNode(View.NodeID);
			AllocatedCount += View.bAllocated ? 1 : 0;
		}
	}

	FVector2D GraphToLocal(const FVector2D GraphPosition, const FVector2D LocalSize) const
	{
		return LocalSize * 0.5f + PanOffset + GraphPosition * Zoom;
	}

	static bool DoesSegmentTouchView(const FVector2D A, const FVector2D B, const FSlateRect& View)
	{
		// Bounding-box reject only. An edge whose box straddles the view but whose line does not is
		// cheap to draw and clipped by Slate anyway.
		return FMath::Max(A.X, B.X) >= View.Left && FMath::Min(A.X, B.X) <= View.Right &&
			FMath::Max(A.Y, B.Y) >= View.Top && FMath::Min(A.Y, B.Y) <= View.Bottom;
	}

	FName FindNodeAt(const FVector2D LocalPosition, const FVector2D LocalSize) const
	{
		// Reverse order so the topmost node under the cursor wins, matching the paint order.
		for (int32 Index = NodeViews.Num() - 1; Index >= 0; --Index)
		{
			const FPassiveNodeView& View = NodeViews[Index];
			const FVector2D HalfSize = View.HalfSize * Zoom;
			const FVector2D Delta = LocalPosition - GraphToLocal(View.Position, LocalSize);
			if (FMath::Abs(Delta.X) <= HalfSize.X && FMath::Abs(Delta.Y) <= HalfSize.Y)
			{
				return View.NodeID;
			}
		}
		return NAME_None;
	}

	void SetZoomAround(const float NewZoom, const FVector2D LocalPivot, const FVector2D LocalSize)
	{
		const float Clamped = FMath::Clamp(NewZoom, MinZoom, MaxZoom);
		const FVector2D Center = LocalSize * 0.5f;
		const FVector2D GraphUnderPointer = (LocalPivot - Center - PanOffset) / Zoom;
		Zoom = Clamped;
		PanOffset = LocalPivot - Center - GraphUnderPointer * Zoom;
		ClampPan();
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	/** Keeps at least a margin of the graph on screen so the view cannot be lost in empty space. */
	void ClampPan()
	{
		if (!GraphBounds.bIsValid || CachedLocalSize.X <= 0.0 || CachedLocalSize.Y <= 0.0)
		{
			return;
		}

		const FVector2D Margin = CachedLocalSize * 0.35f;
		const FVector2D Center = CachedLocalSize * 0.5f;
		const FVector2D ScaledMin = GraphBounds.Min * Zoom;
		const FVector2D ScaledMax = GraphBounds.Max * Zoom;
		const FVector2D LowerBound = Margin - Center - ScaledMax;
		const FVector2D UpperBound = CachedLocalSize - Margin - Center - ScaledMin;

		PanOffset.X = FMath::Clamp(PanOffset.X, FMath::Min(LowerBound.X, UpperBound.X), FMath::Max(LowerBound.X, UpperBound.X));
		PanOffset.Y = FMath::Clamp(PanOffset.Y, FMath::Min(LowerBound.Y, UpperBound.Y), FMath::Max(LowerBound.Y, UpperBound.Y));
	}

	TWeakObjectPtr<UPHPassiveTreeMenuPageWidget> OwnerWidget;
	TWeakObjectPtr<const UPHPassiveTreeDataAsset> BuiltForTree;
	FSlateBrush NodeFillBrush;
	FSlateBrush NodeFrameBrush;

	TArray<FPassiveNodeView> NodeViews;
	TArray<FPassiveEdge> Edges;
	FBox2D GraphBounds = FBox2D(ForceInit);
	FVector2D MaxNodeHalfSize = FVector2D::ZeroVector;
	int32 AllocatedCount = 0;

	FVector2D PanOffset = FVector2D::ZeroVector;
	FVector2D LastPointer = FVector2D::ZeroVector;
	FVector2D PressOrigin = FVector2D::ZeroVector;
	mutable FVector2D CachedLocalSize = FVector2D(1100.0f, 650.0f);
	float Zoom = 0.85f;
	bool bPanning = false;
	FName PressedNodeID = NAME_None;
	FName HoveredNodeID = NAME_None;
	FString SearchText;
	int32 SearchMatchCount = 0;
};

TSharedRef<SWidget> UPHPassiveTreeMenuPageWidget::RebuildWidget()
{
	const FButtonStyle& ButtonStyle = SystemButtonStyle();

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(22.0f, 14.0f, 22.0f, 8.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("PHPassiveTree", "PageTitle", "HUNTER PATHS"))
				.Font(PHMenuRowBuilder::MenuFont(24, TEXT("Bold")))
				.ColorAndOpacity(PHUIStyle::HeaderTeal)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(22.0f, 0.0f).VAlign(VAlign_Center)
			[
				SNew(SSearchBox)
				.HintText(NSLOCTEXT("PHPassiveTree", "SearchHint", "Search perks, paths, or attributes"))
				.OnTextChanged_UObject(this, &UPHPassiveTreeMenuPageWidget::SetSearchText)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f).VAlign(VAlign_Center)
			[
				SAssignNew(SearchCountText, STextBlock)
				.Font(PHMenuRowBuilder::MenuFont(12))
				.ColorAndOpacity(PHUIStyle::TextDim)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(12.0f, 0.0f).VAlign(VAlign_Center)
			[
				SAssignNew(AllocatedCountText, STextBlock)
				.Font(PHMenuRowBuilder::MenuFont(13, TEXT("Bold")))
				.ColorAndOpacity(PHUIStyle::HeaderTeal)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(12.0f, 0.0f).VAlign(VAlign_Center)
			[
				SAssignNew(PointsText, STextBlock)
				.Font(PHMenuRowBuilder::MenuFont(15, TEXT("Bold")))
				.ColorAndOpacity(PHUIStyle::TextPrimary)
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(22.0f, 4.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(PHUIStyle::AzureDeep.R, PHUIStyle::AzureDeep.G, PHUIStyle::AzureDeep.B, 0.75f))
			.Padding(2.0f)
			[
				SAssignNew(GraphWidget, SPHPassiveTreeGraph)
				.OwnerWidget(TWeakObjectPtr<UPHPassiveTreeMenuPageWidget>(this))
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(22.0f, 6.0f, 22.0f, 14.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(DetailTitleText, STextBlock)
					.Text(NSLOCTEXT("PHPassiveTree", "Instructions", "DRAG TO SCROLL  •  MOUSE WHEEL OR BUTTONS TO ZOOM  •  CLICK TO ALLOCATE"))
					.Font(PHMenuRowBuilder::MenuFont(12, TEXT("Bold")))
					.ColorAndOpacity(PHUIStyle::TextPrimary)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
				[
					SAssignNew(DetailBodyText, STextBlock)
					.Text(NSLOCTEXT("PHPassiveTree", "InstructionsBody", "Owned nodes are teal and double-ringed. Bordered nodes are ready to take."))
					.Font(PHMenuRowBuilder::MenuFont(11))
					.ColorAndOpacity(PHUIStyle::TextDim)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
				[
					SAssignNew(StatusText, STextBlock)
					.Font(PHMenuRowBuilder::MenuFont(11))
					.ColorAndOpacity(PHUIStyle::GradeA)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6.0f).VAlign(VAlign_Center)
			[
				SNew(SButton).ButtonStyle(&ButtonStyle).OnClicked_UObject(this, &UPHPassiveTreeMenuPageWidget::ZoomOut)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("−"))).Font(PHMenuRowBuilder::MenuFont(18)).ColorAndOpacity(PHUIStyle::TextPrimary)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f).VAlign(VAlign_Center)
			[
				SAssignNew(ZoomText, STextBlock).Font(PHMenuRowBuilder::MenuFont(12)).ColorAndOpacity(PHUIStyle::TextPrimary)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6.0f).VAlign(VAlign_Center)
			[
				SNew(SButton).ButtonStyle(&ButtonStyle).OnClicked_UObject(this, &UPHPassiveTreeMenuPageWidget::ZoomIn)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("+"))).Font(PHMenuRowBuilder::MenuFont(18)).ColorAndOpacity(PHUIStyle::TextPrimary)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6.0f).VAlign(VAlign_Center)
			[
				SNew(SButton).ButtonStyle(&ButtonStyle).OnClicked_UObject(this, &UPHPassiveTreeMenuPageWidget::ResetView)
				[
					SNew(STextBlock).Text(NSLOCTEXT("PHPassiveTree", "Reset", "FIT")).Font(PHMenuRowBuilder::MenuFont(11)).ColorAndOpacity(PHUIStyle::TextPrimary)
				]
			]
		];
}

void UPHPassiveTreeMenuPageWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	Super::NativeInitializeForCharacter(Character);
	PassiveTree = Character ? Character->GetPassiveTreeComponent() : nullptr;
	Progression = Character ? Character->GetProgressionManager() : nullptr;

	if (PassiveTree.IsValid())
	{
		PassiveTree->OnPassiveTreeChanged.AddUniqueDynamic(this, &UPHPassiveTreeMenuPageWidget::HandleTreeChanged);
		PassiveTree->OnPassiveAllocationRejected.AddUniqueDynamic(this, &UPHPassiveTreeMenuPageWidget::HandleAllocationRejected);
	}
	if (Progression.IsValid())
	{
		Progression->OnProgressionChanged.AddUniqueDynamic(this, &UPHPassiveTreeMenuPageWidget::HandleProgressionChanged);
	}
	RefreshView();
}

void UPHPassiveTreeMenuPageWidget::NativeReleaseCharacter()
{
	if (PassiveTree.IsValid())
	{
		PassiveTree->OnPassiveTreeChanged.RemoveDynamic(this, &UPHPassiveTreeMenuPageWidget::HandleTreeChanged);
		PassiveTree->OnPassiveAllocationRejected.RemoveDynamic(this, &UPHPassiveTreeMenuPageWidget::HandleAllocationRejected);
	}
	if (Progression.IsValid())
	{
		Progression->OnProgressionChanged.RemoveDynamic(this, &UPHPassiveTreeMenuPageWidget::HandleProgressionChanged);
	}
	PassiveTree.Reset();
	Progression.Reset();
	Super::NativeReleaseCharacter();
}

UPHPassiveTreeDataAsset* UPHPassiveTreeMenuPageWidget::GetTreeData() const
{
	return PassiveTree.IsValid() ? PassiveTree->GetTreeData() : nullptr;
}

bool UPHPassiveTreeMenuPageWidget::IsNodeAllocated(const FName NodeID) const
{
	return PassiveTree.IsValid() && PassiveTree->IsNodeAllocated(NodeID);
}

bool UPHPassiveTreeMenuPageWidget::IsRandomStartNode(const FName NodeID) const
{
	return PassiveTree.IsValid() && PassiveTree->IsRandomStartNode(NodeID);
}

bool UPHPassiveTreeMenuPageWidget::CanAllocateNode(const FName NodeID, FText& OutReason) const
{
	if (!PassiveTree.IsValid())
	{
		OutReason = NSLOCTEXT("PHPassiveTree", "NoBoundTree", "No character passive tree is bound.");
		return false;
	}
	return PassiveTree->CanAllocateNode(NodeID, OutReason);
}

void UPHPassiveTreeMenuPageWidget::RequestAllocateNode(const FName NodeID)
{
	if (!PassiveTree.IsValid())
	{
		return;
	}
	const UPHPassiveTreeDataAsset* Tree = GetTreeData();
	const FPHPassiveNodeDefinition* Node = Tree ? Tree->FindNode(NodeID) : nullptr;
	if (PassiveTree->RequestAllocateNode(NodeID))
	{
		if (StatusText.IsValid())
		{
			StatusText->SetText(FText::Format(NSLOCTEXT("PHPassiveTree", "Activated", "ACTIVATED: {0}"),
				Node ? Node->DisplayName : FText::FromName(NodeID)));
		}
	}
}

void UPHPassiveTreeMenuPageWidget::ShowNodeDetails(const FPHPassiveNodeDefinition* Node)
{
	if (!DetailTitleText.IsValid() || !DetailBodyText.IsValid())
	{
		return;
	}
	if (!Node)
	{
		DetailTitleText->SetText(NSLOCTEXT("PHPassiveTree", "Instructions", "DRAG TO SCROLL  •  MOUSE WHEEL OR BUTTONS TO ZOOM  •  CLICK TO ALLOCATE"));
		DetailBodyText->SetText(NSLOCTEXT("PHPassiveTree", "InstructionsBody", "Owned nodes are teal and double-ringed. Bordered nodes are ready to take."));
		return;
	}

	DetailTitleText->SetText(FText::Format(
		NSLOCTEXT("PHPassiveTree", "NodeTitle", "{0}  •  {1} POINT(S)"), Node->DisplayName, FText::AsNumber(Node->PointCost)));
	FText Reason;
	const bool bCanAllocate = CanAllocateNode(Node->NodeID, Reason);
	const FText State = IsRandomStartNode(Node->NodeID)
		? NSLOCTEXT("PHPassiveTree", "OriginState", "ORIGIN - GRANTED AT AWAKENING")
		: (IsNodeAllocated(Node->NodeID)
			? NSLOCTEXT("PHPassiveTree", "ActiveState", "ALLOCATED")
			: (bCanAllocate ? NSLOCTEXT("PHPassiveTree", "ReadyState", "READY") : Reason));
	DetailBodyText->SetText(FText::Format(
		NSLOCTEXT("PHPassiveTree", "NodeBody", "{0}  [{1}]"), Node->Description, State));
}

void UPHPassiveTreeMenuPageWidget::HandleTreeChanged()
{
	RefreshView();
}

void UPHPassiveTreeMenuPageWidget::HandleAllocationRejected(const FName, const FText Reason)
{
	if (StatusText.IsValid())
	{
		StatusText->SetText(Reason);
	}
}

void UPHPassiveTreeMenuPageWidget::HandleProgressionChanged()
{
	RefreshView();
}

void UPHPassiveTreeMenuPageWidget::RefreshView()
{
	if (GraphWidget.IsValid())
	{
		GraphWidget->Refresh();
	}
	if (PointsText.IsValid())
	{
		const int32 Points = Progression.IsValid() ? Progression->UnspentPassivePoints : 0;
		PointsText->SetText(FText::Format(
			NSLOCTEXT("PHPassiveTree", "Points", "PASSIVE POINTS  {0}"), FText::AsNumber(Points)));
	}
	if (AllocatedCountText.IsValid())
	{
		const int32 Allocated = GraphWidget.IsValid() ? GraphWidget->GetAllocatedCount() : 0;
		const int32 Total = GraphWidget.IsValid() ? GraphWidget->GetNodeCount() : 0;
		AllocatedCountText->SetText(FText::Format(
			NSLOCTEXT("PHPassiveTree", "Allocated", "ALLOCATED  {0} / {1}"),
			FText::AsNumber(Allocated), FText::AsNumber(Total)));
	}
	if (ZoomText.IsValid())
	{
		ZoomText->SetText(FText::AsPercent(GraphWidget.IsValid() ? GraphWidget->GetZoom() : 1.0f));
	}
}

void UPHPassiveTreeMenuPageWidget::SetSearchText(const FText& Text)
{
	if (GraphWidget.IsValid())
	{
		GraphWidget->SetSearchText(Text.ToString());
	}
	if (SearchCountText.IsValid())
	{
		const int32 Count = GraphWidget.IsValid() ? GraphWidget->GetSearchMatchCount() : 0;
		SearchCountText->SetText(Text.IsEmpty()
			? FText::GetEmpty()
			: FText::Format(NSLOCTEXT("PHPassiveTree", "SearchCount", "{0} FOUND"), FText::AsNumber(Count)));
	}
}

FReply UPHPassiveTreeMenuPageWidget::ZoomIn()
{
	if (GraphWidget.IsValid())
	{
		GraphWidget->ChangeZoom(0.10f);
	}
	RefreshView();
	return FReply::Handled();
}

FReply UPHPassiveTreeMenuPageWidget::ZoomOut()
{
	if (GraphWidget.IsValid())
	{
		GraphWidget->ChangeZoom(-0.10f);
	}
	RefreshView();
	return FReply::Handled();
}

FReply UPHPassiveTreeMenuPageWidget::ResetView()
{
	if (GraphWidget.IsValid())
	{
		GraphWidget->ResetView();
	}
	RefreshView();
	return FReply::Handled();
}

#include "Generation/Generators/PHCaveGenerator.h"

#include "Generation/PHGenerationTags.h"

namespace PHCaveGeneratorPrivate
{
	/** Matches the dungeon strategy's synchronous ceiling, so neither can outgrow the planner. */
	constexpr int32 MaxCaveTiles = 65536;

	/** Row-major index into a Width x Height grid. */
	FORCEINLINE int32 At(const int32 X, const int32 Y, const int32 Width)
	{
		return Y * Width + X;
	}
}

UPHCaveGenerator::UPHCaveGenerator()
{
	// 1: first cellular-automata cave. Its seeds have no earlier behaviour to replay.
	GenerationVersion = 1;
}

bool UPHCaveGenerator::BuildLayout(const FPHLayoutRequest& Request, FRandomStream& Stream,
	FPHGeneratedLayout& OutLayout, TArray<FPHGenerationIssue>& OutIssues)
{
	using namespace PHCaveGeneratorPrivate;

	auto Refuse = [&OutIssues](FString Message)
	{
		AddIssue(OutIssues, EPHGenerationIssueCode::InvalidRequest, INDEX_NONE, MoveTemp(Message));
		return false;
	};

	const double Grid = Request.GridSize;
	const int32 Width = ModulesDown(Request.AreaSize.X, Grid);
	const int32 Height = ModulesDown(Request.AreaSize.Y, Grid);

	// Two borders plus something between them, or there is no cave to carve.
	const int32 MinimumSpan = BorderTiles * 2 + 3;
	if (Width == INDEX_NONE || Height == INDEX_NONE || Width < MinimumSpan || Height < MinimumSpan)
	{
		return Refuse(FString::Printf(
			TEXT("A cave needs at least %d tiles on each axis for its %d-tile border; the area gives %d x %d."),
			MinimumSpan, BorderTiles, Width, Height));
	}

	const int64 CellCount = static_cast<int64>(Width) * Height;
	if (CellCount > MaxCaveTiles)
	{
		return Refuse(FString::Printf(
			TEXT("A %d x %d cave grid is %lld tiles, beyond the %d this synchronous strategy supports."),
			Width, Height, CellCount, MaxCaveTiles));
	}

	// True is rock. Seeded before smoothing, and the border is forced solid so the cave can never
	// open onto the void at the edge of its own envelope.
	TArray<bool> Rock;
	Rock.SetNumUninitialized(static_cast<int32>(CellCount));
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const bool bBorder = X < BorderTiles || Y < BorderTiles
				|| X >= Width - BorderTiles || Y >= Height - BorderTiles;
			// Drawn for every cell including the border, so the border width cannot shift the
			// sequence and change the cave that a seed produces.
			const bool bSeeded = Stream.RandRange(1, 100) <= InitialRockPercent;
			Rock[At(X, Y, Width)] = bBorder || bSeeded;
		}
	}

	// Each pass asks every tile to become whatever most of its neighbours already are. Repeated,
	// this erodes noise into rounded chambers: isolated rock dissolves and isolated gaps fill in.
	// Read from a snapshot so every tile in a pass sees the same previous state.
	//
	// The early passes carry a second rule as well. Majority smoothing alone can erode a seed into
	// one large open field - measurably so: without this, seeds 6, 8, 16, 21 and 24 filled 90-93%
	// of their bounding box and read as rooms again. A tile with almost no rock anywhere in its 5x5
	// neighbourhood becomes rock, which drops pillars into open ground and breaks it into chambers.
	// Later passes drop the rule so those seeds get rounded off rather than left speckled.
	TArray<bool> Previous;
	const int32 TotalPasses = OpenFieldPasses + SmoothingPasses;
	for (int32 Pass = 0; Pass < TotalPasses; ++Pass)
	{
		const bool bBreakOpenGround = (Pass < OpenFieldPasses);
		Previous = Rock;
		for (int32 Y = BorderTiles; Y < Height - BorderTiles; ++Y)
		{
			for (int32 X = BorderTiles; X < Width - BorderTiles; ++X)
			{
				int32 RockNeighbours = 0;
				for (int32 OffsetY = -1; OffsetY <= 1; ++OffsetY)
				{
					for (int32 OffsetX = -1; OffsetX <= 1; ++OffsetX)
					{
						if (OffsetX == 0 && OffsetY == 0)
						{
							continue;
						}
						RockNeighbours += Previous[At(X + OffsetX, Y + OffsetY, Width)] ? 1 : 0;
					}
				}

				bool bRock = (RockNeighbours >= RockNeighbourThreshold);

				if (bBreakOpenGround && !bRock)
				{
					// Out-of-grid counts as rock, matching the solid border, so a tile near the
					// edge is never mistaken for open ground.
					int32 WideRock = 0;
					for (int32 OffsetY = -2; OffsetY <= 2; ++OffsetY)
					{
						for (int32 OffsetX = -2; OffsetX <= 2; ++OffsetX)
						{
							if (OffsetX == 0 && OffsetY == 0)
							{
								continue;
							}
							const int32 SampleX = X + OffsetX;
							const int32 SampleY = Y + OffsetY;
							const bool bOutside = SampleX < 0 || SampleY < 0
								|| SampleX >= Width || SampleY >= Height;
							WideRock += (bOutside || Previous[At(SampleX, SampleY, Width)]) ? 1 : 0;
						}
					}
					bRock = (WideRock <= WideOpenThreshold);
				}

				Rock[At(X, Y, Width)] = bRock;
			}
		}
	}

	// Smoothing readily leaves chambers with no way between them. Rather than tunnel between them -
	// which reintroduces the straight corridors this strategy exists to avoid - the largest
	// component is kept and the rest returned to rock. What survives is walkable by construction.
	TArray<int32> Component;
	Component.Init(INDEX_NONE, static_cast<int32>(CellCount));
	TArray<FIntPoint> Best;
	TArray<FIntPoint> Pending;

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const int32 Index = At(X, Y, Width);
			if (Rock[Index] || Component[Index] != INDEX_NONE)
			{
				continue;
			}

			TArray<FIntPoint> Current;
			Pending.Reset();
			Pending.Add(FIntPoint(X, Y));
			Component[Index] = 0;

			for (int32 Cursor = 0; Cursor < Pending.Num(); ++Cursor)
			{
				const FIntPoint Tile = Pending[Cursor];
				Current.Add(Tile);

				const FIntPoint Neighbours[] = {
					FIntPoint(Tile.X + 1, Tile.Y), FIntPoint(Tile.X - 1, Tile.Y),
					FIntPoint(Tile.X, Tile.Y + 1), FIntPoint(Tile.X, Tile.Y - 1) };
				for (const FIntPoint& Next : Neighbours)
				{
					if (Next.X < 0 || Next.Y < 0 || Next.X >= Width || Next.Y >= Height)
					{
						continue;
					}
					const int32 NextIndex = At(Next.X, Next.Y, Width);
					if (!Rock[NextIndex] && Component[NextIndex] == INDEX_NONE)
					{
						Component[NextIndex] = 0;
						Pending.Add(Next);
					}
				}
			}

			if (Current.Num() > Best.Num())
			{
				Best = MoveTemp(Current);
			}
		}
	}

	const int32 MinimumFloor = FMath::Max(1,
		static_cast<int32>((CellCount * MinimumFloorPercent) / 100));
	if (Best.Num() < MinimumFloor)
	{
		return Refuse(FString::Printf(
			TEXT("The largest chamber is %d tiles, under the %d this cave requires. Lower ")
			TEXT("InitialRockPercent or MinimumFloorPercent, or raise SmoothingPasses."),
			Best.Num(), MinimumFloor));
	}

	// Sorted so the mask is byte-for-byte reproducible; flood fill order is an implementation
	// detail and must not leak into the published layout.
	Best.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return (A.X != B.X) ? (A.X < B.X) : (A.Y < B.Y);
	});

	OutLayout.FloorMask = Best;
	OutLayout.MaskTileSize = Grid;

	// The envelope is the whole griddable area rather than the cave's own extent, so region bounds,
	// navigation and encounter volumes all agree with the tile coordinates in the mask - which are
	// absolute, not relative to wherever the cave happens to sit.
	const double CaveHeight = FMath::Max(1, ModulesDown(Request.RegionHeight, Grid)) * Grid;
	OutLayout.Bounds = FBox(
		FVector(0.0, 0.0, 0.0),
		FVector(Width * Grid, Height * Grid, CaveHeight));

	FPHGeneratedRegion& Region = OutLayout.Regions.AddDefaulted_GetRef();
	Region.RegionID = 0;
	Region.Bounds = OutLayout.Bounds;

	// The endpoints are the two floor tiles furthest apart, so a cave is crossed rather than
	// stepped over. Measured from an extreme tile rather than over every pair, which would be
	// quadratic in the tile count.
	const FIntPoint First = Best[0];
	auto FurthestFrom = [&Best](const FIntPoint& From)
	{
		FIntPoint Found = From;
		double Best2 = -1.0;
		for (const FIntPoint& Tile : Best)
		{
			const double Distance = FVector2D::DistSquared(
				FVector2D(Tile.X, Tile.Y), FVector2D(From.X, From.Y));
			if (Distance > Best2)
			{
				Best2 = Distance;
				Found = Tile;
			}
		}
		return Found;
	};
	const FIntPoint StartTile = FurthestFrom(First);
	const FIntPoint ExitTile = FurthestFrom(StartTile);

	auto CentreOf = [Grid](const FIntPoint& Tile)
	{
		return FVector((Tile.X + 0.5) * Grid, (Tile.Y + 0.5) * Grid, 0.0);
	};

	FPHGeneratedAnchor& Start = OutLayout.Anchors.AddDefaulted_GetRef();
	Start.AnchorID = 0;
	Start.RegionID = 0;
	Start.Transform = FTransform(CentreOf(StartTile));
	Start.SemanticTag = PHGenerationTags::Anchor_PlayerStart.GetTag();

	FPHGeneratedAnchor& Exit = OutLayout.Anchors.AddDefaulted_GetRef();
	Exit.AnchorID = 1;
	Exit.RegionID = 0;
	Exit.Transform = FTransform(CentreOf(ExitTile));
	Exit.SemanticTag = PHGenerationTags::Anchor_Exit.GetTag();

	OutLayout.PlayerStartAnchorID = 0;
	OutLayout.ExitAnchorID = 1;

	// Rule anchors are seated on actual cave floor, never on rock, and keep the same endpoint
	// clearance the dungeon strategy uses so a chest cannot land on the pose the player arrives on.
	if (!Request.AnchorRules.IsEmpty())
	{
		TArray<FIntPoint> Slots = Best;
		for (int32 Index = Slots.Num() - 1; Index > 0; --Index)
		{
			Slots.Swap(Index, Stream.RandRange(0, Index));
		}

		const double ClearanceSquared = FMath::Square(
			FMath::Max(0.0, Request.EndpointClearanceModules));
		Slots.RemoveAll([&](const FIntPoint& Tile)
		{
			return FVector2D::DistSquared(FVector2D(Tile.X, Tile.Y),
					FVector2D(StartTile.X, StartTile.Y)) < ClearanceSquared
				|| FVector2D::DistSquared(FVector2D(Tile.X, Tile.Y),
					FVector2D(ExitTile.X, ExitTile.Y)) < ClearanceSquared;
		});

		int32 NextAnchorID = OutLayout.Anchors.Num();
		int32 NextSlot = 0;
		TMap<FGameplayTag, int32> PlacedTotals;

		for (const FPHAnchorRule& Rule : Request.AnchorRules)
		{
			// A single-region cave is both the entry and the exit region, so a rule barred from
			// either has nowhere in this layout to go.
			if (!Rule.bAllowInStartRegion || !Rule.bAllowInExitRegion)
			{
				continue;
			}

			// Drawn before the cap so a full budget does not shift later draws.
			int32 Wanted = Stream.RandRange(Rule.MinPerRegion, Rule.MaxPerRegion);
			if (Rule.MaxTotal > 0)
			{
				Wanted = FMath::Min(Wanted, Rule.MaxTotal - PlacedTotals.FindRef(Rule.SemanticTag));
			}

			for (int32 Placed = 0; Placed < Wanted && NextSlot < Slots.Num(); ++Placed)
			{
				FPHGeneratedAnchor& Anchor = OutLayout.Anchors.AddDefaulted_GetRef();
				Anchor.AnchorID = NextAnchorID++;
				Anchor.RegionID = 0;
				Anchor.Transform = FTransform(CentreOf(Slots[NextSlot++]));
				Anchor.SemanticTag = Rule.SemanticTag;
				PlacedTotals.FindOrAdd(Rule.SemanticTag)++;
			}
		}
	}

	return true;
}

bool UPHCaveGenerator::ValidateStrategyConstraints(const FPHLayoutRequest& Request,
	const FPHGeneratedLayout& Layout, TArray<FPHGenerationIssue>& OutIssues) const
{
	const int32 IssuesBefore = OutIssues.Num();

	if (Layout.FloorMask.IsEmpty())
	{
		AddIssue(OutIssues, EPHGenerationIssueCode::EmptyRegions, INDEX_NONE,
			TEXT("A cave layout must publish a floor mask; its regions are only envelopes."));
		return false;
	}

	if (!FMath::IsNearlyEqual(Layout.MaskTileSize, Request.GridSize))
	{
		AddIssue(OutIssues, EPHGenerationIssueCode::RegionOffGrid, INDEX_NONE,
			FString::Printf(TEXT("The cave mask is on a %f tile but the request asked for %f."),
				Layout.MaskTileSize, Request.GridSize));
	}

	// Connectivity is the property the largest-component step exists to guarantee, so it is proved
	// on the published mask rather than assumed from the algorithm that produced it.
	const TSet<FIntPoint> Floor(Layout.FloorMask);
	TSet<FIntPoint> Reached;
	TArray<FIntPoint> Pending;
	Reached.Add(Layout.FloorMask[0]);
	Pending.Add(Layout.FloorMask[0]);
	for (int32 Cursor = 0; Cursor < Pending.Num(); ++Cursor)
	{
		const FIntPoint Tile = Pending[Cursor];
		const FIntPoint Neighbours[] = {
			FIntPoint(Tile.X + 1, Tile.Y), FIntPoint(Tile.X - 1, Tile.Y),
			FIntPoint(Tile.X, Tile.Y + 1), FIntPoint(Tile.X, Tile.Y - 1) };
		for (const FIntPoint& Next : Neighbours)
		{
			if (Floor.Contains(Next) && !Reached.Contains(Next))
			{
				Reached.Add(Next);
				Pending.Add(Next);
			}
		}
	}
	if (Reached.Num() != Floor.Num())
	{
		AddIssue(OutIssues, EPHGenerationIssueCode::InvalidRequest, INDEX_NONE,
			FString::Printf(TEXT("The cave mask has %d tiles but only %d are reachable."),
				Floor.Num(), Reached.Num()));
	}

	// Every anchor must stand on cave floor. An anchor inside rock is a chest, an enemy or the
	// player embedded in geometry.
	for (int32 Index = 0; Index < Layout.Anchors.Num(); ++Index)
	{
		const FVector Location = Layout.Anchors[Index].Transform.GetLocation();
		const FIntPoint Tile(
			FMath::FloorToInt32(Location.X / Request.GridSize),
			FMath::FloorToInt32(Location.Y / Request.GridSize));
		if (!Floor.Contains(Tile))
		{
			AddIssue(OutIssues, EPHGenerationIssueCode::InvalidRequest, Index,
				FString::Printf(TEXT("Cave anchor %d sits at tile (%d,%d), which is solid rock."),
					Layout.Anchors[Index].AnchorID, Tile.X, Tile.Y));
		}
	}

	return OutIssues.Num() == IssuesBefore;
}

#include "Generation/Library/FunctionLibraries/PHBlockoutPlanLibrary.h"

#include "Generation/PHGenerationTags.h"
#include "Tower/Library/FunctionLibraries/RunSeedFunctionLibrary.h"

namespace PHBlockoutPlanPrivate
{
	// This synchronous constructor is bounded independently of future streamed generators.
	constexpr int32 MaxFloorTiles = 65536;
	constexpr int64 MaxRasterVisits = 1000000;
	constexpr int32 MaxPlacements = 1000000;
	constexpr int64 MaxLightDistanceChecks = 16000000;

	void AddIssue(TArray<FPHGenerationIssue>& Issues, const EPHGenerationIssueCode Code,
		const int32 ElementIndex, FString Message)
	{
		FPHGenerationIssue& Issue = Issues.AddDefaulted_GetRef();
		Issue.Code = Code;
		Issue.ElementIndex = ElementIndex;
		Issue.Message = MoveTemp(Message);
	}

	bool IsWholeTiles(const double Value, const double TileSize, int32& OutTiles)
	{
		const double Tiles = Value / TileSize;
		if (!FMath::IsFinite(Tiles) || Tiles <= static_cast<double>(MIN_int32)
			|| Tiles >= static_cast<double>(MAX_int32))
		{
			return false;
		}
		OutTiles = FMath::RoundToInt32(Tiles);
		return FMath::IsNearlyEqual(Tiles, static_cast<double>(OutTiles), UE_KINDA_SMALL_NUMBER);
	}

	/** Min inclusive, Max exclusive. */
	struct FTileRect
	{
		int32 MinX = 0;
		int32 MinY = 0;
		int32 MaxX = 0;
		int32 MaxY = 0;
	};

	/**
	 * Adds one corridor cell plus its perpendicular width, centred on the path.
	 *
	 * The span is narrowed to whatever fits inside the envelope rather than allowed to reach past
	 * it: nothing may be built outside Layout.Bounds, because everything downstream - navigation
	 * sizing, region ownership during decoration, encounter volumes - reads those bounds as the
	 * truth about where the floor is. It is only refused when the narrowing would take the passage
	 * below the width the caller said it needed, so an authored minimum still fails loudly rather
	 * than being quietly downgraded.
	 */
	bool AddSpan(const FIntPoint& Centre, const FIntPoint& Perpendicular, const int32 Width,
		const int32 MinWidth, const FTileRect& Envelope, int64& RasterVisits,
		TSet<FIntPoint>& Tiles, TSet<FIntPoint>& CorridorOnly)
	{
		if (Centre.X < Envelope.MinX || Centre.X >= Envelope.MaxX
			|| Centre.Y < Envelope.MinY || Centre.Y >= Envelope.MaxY)
		{
			return false;
		}

		// Room on each side of the centre along the axis the span widens on.
		const bool bAlongX = (Perpendicular.X != 0);
		const int32 At = bAlongX ? Centre.X : Centre.Y;
		const int32 Low = bAlongX ? Envelope.MinX : Envelope.MinY;
		const int32 High = bAlongX ? Envelope.MaxX : Envelope.MaxY;

		const int32 Back = FMath::Min((Width - 1) / 2, At - Low);
		const int32 Forward = FMath::Min(Width - 1 - (Width - 1) / 2, High - 1 - At);

		const int32 Fitted = Back + Forward + 1;
		if (Fitted < MinWidth || RasterVisits + Fitted > MaxRasterVisits)
		{
			return false;
		}

		RasterVisits += Fitted;
		for (int32 Offset = -Back; Offset <= Forward; ++Offset)
		{
			const FIntPoint Tile = Centre + Perpendicular * Offset;
			if (!Tiles.Contains(Tile))
			{
				CorridorOnly.Add(Tile);
			}
			Tiles.Add(Tile);
			if (Tiles.Num() > MaxFloorTiles)
			{
				return false;
			}
		}
		return true;
	}

	/** True when the two regions share a run of tiles on one axis, so a straight run can join them. */
	bool SharesFrontage(const FTileRect& A, const FTileRect& B)
	{
		return (FMath::Min(A.MaxY, B.MaxY) > FMath::Max(A.MinY, B.MinY))
			|| (FMath::Min(A.MaxX, B.MaxX) > FMath::Max(A.MinX, B.MinX));
	}

	/**
	 * A straight corridor along one axis, filling only the gap between the two regions. Used
	 * whenever the regions share frontage: a room joined to its neighbour by a short square
	 * passage reads as a floor plan, where an L-shaped run between centres reads as a trench cut
	 * across whatever happened to lie between them.
	 *
	 * The lane is a row (or column) inside the shared frontage, and the span is placed so its whole
	 * width lands within that frontage, so both mouths open onto floor the regions already own.
	 */
	bool AddStraightCorridor(const FTileRect& A, const FTileRect& B, const int32 Width,
		const int32 MinWidth, const int32 LaneRoll, const FTileRect& Envelope, int64& RasterVisits,
		TSet<FIntPoint>& Tiles, TSet<FIntPoint>& CorridorOnly)
	{
		const bool bAlongX = (FMath::Min(A.MaxY, B.MaxY) > FMath::Max(A.MinY, B.MinY));

		const int32 LaneLow = bAlongX ? FMath::Max(A.MinY, B.MinY) : FMath::Max(A.MinX, B.MinX);
		const int32 LaneHigh = bAlongX ? FMath::Min(A.MaxY, B.MaxY) : FMath::Min(A.MaxX, B.MaxX);

		// The lane is chosen so the span's own width lands inside the frontage, rather than centred
		// anywhere in it and left to reach past the rooms. A passage whose mouth opens onto wall is
		// the visible symptom; building outside Layout.Bounds is the contract breach underneath.
		const int32 Frontage = LaneHigh - LaneLow;
		const int32 Fitted = FMath::Min(Width, Frontage);
		if (Fitted < MinWidth)
		{
			return false;
		}

		const int32 Back = (Fitted - 1) / 2;
		const int32 Forward = Fitted - 1 - Back;
		const int32 FirstLane = LaneLow + Back;
		const int32 LaneChoices = FMath::Max(1, (LaneHigh - 1 - Forward) - FirstLane + 1);
		const int32 Lane = FirstLane + LaneRoll % LaneChoices;

		// Regions never overlap, so on the axis they do not share, the gap is unambiguous: it runs
		// from the nearer far edge to the further near edge, and is empty when they already touch.
		const int32 GapLow = bAlongX ? FMath::Min(A.MaxX, B.MaxX) : FMath::Min(A.MaxY, B.MaxY);
		const int32 GapHigh = bAlongX ? FMath::Max(A.MinX, B.MinX) : FMath::Max(A.MinY, B.MinY);

		const FIntPoint Perpendicular = bAlongX ? FIntPoint(0, 1) : FIntPoint(1, 0);
		for (int32 Step = GapLow; Step < GapHigh; ++Step)
		{
			if (!AddSpan(bAlongX ? FIntPoint(Step, Lane) : FIntPoint(Lane, Step),
				Perpendicular, Fitted, MinWidth, Envelope, RasterVisits, Tiles, CorridorOnly))
			{
				return false;
			}
		}
		return true;
	}

	/**
	 * An L-shaped corridor of the given width. Turning X-first or Y-first moves which side the
	 * corner sits on, which is what stops every route reading as the same passage.
	 */
	bool AddCorridorTiles(const FIntPoint& From, const FIntPoint& To, const int32 Width,
		const int32 MinWidth, const bool bXFirst, const FTileRect& Envelope, int64& RasterVisits,
		TSet<FIntPoint>& Tiles, TSet<FIntPoint>& CorridorOnly)
	{
		FIntPoint Current = From;

		auto RunX = [&]()
		{
			const int32 Step = (To.X > Current.X) ? 1 : -1;
			while (Current.X != To.X)
			{
				Current.X += Step;
				if (!AddSpan(Current, FIntPoint(0, 1), Width, MinWidth, Envelope, RasterVisits, Tiles, CorridorOnly))
				{
					return false;
				}
			}
			return true;
		};

		auto RunY = [&]()
		{
			const int32 Step = (To.Y > Current.Y) ? 1 : -1;
			while (Current.Y != To.Y)
			{
				Current.Y += Step;
				if (!AddSpan(Current, FIntPoint(1, 0), Width, MinWidth, Envelope, RasterVisits, Tiles, CorridorOnly))
				{
					return false;
				}
			}
			return true;
		};

		return bXFirst ? (RunX() && RunY()) : (RunY() && RunX());
	}
}

bool UPHBlockoutPlanLibrary::BuildPlan(const FPHGeneratedLayout& Layout, const double TileSize,
	FPHBlockoutPlan& OutPlan, TArray<FPHGenerationIssue>& OutIssues, const bool bIncludeCeiling,
	const int32 MinCorridorWidth, const int32 MaxCorridorWidth,
	const double WallHeight, const int32 LightSpacingTiles)
{
	using namespace PHBlockoutPlanPrivate;

	OutPlan = FPHBlockoutPlan();
	OutIssues.Reset();
	auto Refuse = [&](FString Message)
	{
		OutPlan = FPHBlockoutPlan();
		AddIssue(OutIssues, EPHGenerationIssueCode::InvalidRequest, INDEX_NONE, MoveTemp(Message));
		return false;
	};

	if (!FMath::IsFinite(TileSize) || TileSize <= 0.0)
	{
		return Refuse(FString::Printf(TEXT("TileSize must be finite and positive, got %f."), TileSize));
	}
	if (!FMath::IsFinite(WallHeight))
	{
		return Refuse(TEXT("WallHeight must be finite."));
	}

	if (Layout.Regions.IsEmpty())
	{
		AddIssue(OutIssues, EPHGenerationIssueCode::EmptyRegions, INDEX_NONE,
			TEXT("A blockout needs at least one region."));
		return false;
	}

	if (MinCorridorWidth < 1 || MaxCorridorWidth < MinCorridorWidth || MaxCorridorWidth > MaxFloorTiles)
	{
		AddIssue(OutIssues, EPHGenerationIssueCode::InvalidRequest, INDEX_NONE,
			FString::Printf(TEXT("Corridor width range %d..%d is invalid."),
				MinCorridorWidth, MaxCorridorWidth));
		return false;
	}

	const double FloorZ = Layout.Bounds.Min.Z;
	const double Course = (WallHeight > 0.0) ? WallHeight : TileSize;
	const double MinimumX = Layout.Bounds.Min.X / TileSize;
	const double MinimumY = Layout.Bounds.Min.Y / TileSize;
	const double MaximumX = Layout.Bounds.Max.X / TileSize;
	const double MaximumY = Layout.Bounds.Max.Y / TileSize;
	if (!Layout.Bounds.IsValid || Layout.Bounds.Min.ContainsNaN() || Layout.Bounds.Max.ContainsNaN()
		|| !FMath::IsFinite(MinimumX) || !FMath::IsFinite(MinimumY)
		|| !FMath::IsFinite(MaximumX) || !FMath::IsFinite(MaximumY)
		|| MinimumX <= static_cast<double>(MIN_int32) + 1.0 || MinimumY <= static_cast<double>(MIN_int32) + 1.0
		|| MaximumX >= static_cast<double>(MAX_int32) - 1.0 || MaximumY >= static_cast<double>(MAX_int32) - 1.0
		|| MaximumX <= MinimumX || MaximumY <= MinimumY || Layout.Bounds.Max.Z < FloorZ)
	{
		return Refuse(TEXT("The blockout envelope must have finite ordered bounds within tile coordinate range."));
	}
	const FTileRect Envelope{
		FMath::CeilToInt32(MinimumX - UE_KINDA_SMALL_NUMBER),
		FMath::CeilToInt32(MinimumY - UE_KINDA_SMALL_NUMBER),
		FMath::FloorToInt32(MaximumX + UE_KINDA_SMALL_NUMBER),
		FMath::FloorToInt32(MaximumY + UE_KINDA_SMALL_NUMBER) };

	int64 RasterVisits = 0;

	// Regions are converted to whole tiles up front; a partial tile has no piece that fills it.
	TSet<FIntPoint> FloorTiles;
	TMap<FIntPoint, double> TopByTile;
	double ShortestRegionTop = TNumericLimits<double>::Max();
	TMap<int32, FIntPoint> CentreByRegionID;
	TMap<int32, FTileRect> RectByRegionID;

	for (int32 Index = 0; Index < Layout.Regions.Num(); ++Index)
	{
		const FPHGeneratedRegion& Region = Layout.Regions[Index];
		int32 MinTileX = 0, MinTileY = 0, SizeTileX = 0, SizeTileY = 0;

		const bool bAligned =
			Region.Bounds.IsValid
			&& IsWholeTiles(Region.Bounds.Min.X, TileSize, MinTileX)
			&& IsWholeTiles(Region.Bounds.Min.Y, TileSize, MinTileY)
			&& IsWholeTiles(Region.Bounds.Max.X - Region.Bounds.Min.X, TileSize, SizeTileX)
			&& IsWholeTiles(Region.Bounds.Max.Y - Region.Bounds.Min.Y, TileSize, SizeTileY);

		if (!bAligned || SizeTileX <= 0 || SizeTileY <= 0)
		{
			AddIssue(OutIssues, EPHGenerationIssueCode::RegionOffGrid, Index,
				FString::Printf(
					TEXT("Region %d does not sit on whole %f-unit tiles; use a matching layout GridSize."),
					Region.RegionID, TileSize));
			continue;
		}

		const double RegionTop = Region.Bounds.Max.Z;
		int32 RegionCourses = 0;
		if (!FMath::IsFinite(Region.Bounds.Min.Z) || !FMath::IsNearlyEqual(Region.Bounds.Min.Z, FloorZ)
			|| !IsWholeTiles(RegionTop - FloorZ, Course, RegionCourses) || RegionCourses < 0)
		{
			return Refuse(FString::Printf(TEXT("Region %d must share the floor plane and fit whole wall courses."), Region.RegionID));
		}
		const int64 RegionTiles = static_cast<int64>(SizeTileX) * SizeTileY;
		if (RegionTiles > MaxFloorTiles || RasterVisits + RegionTiles > MaxRasterVisits
			|| MinTileX < Envelope.MinX || MinTileY < Envelope.MinY
			|| static_cast<int64>(MinTileX) + SizeTileX > Envelope.MaxX
			|| static_cast<int64>(MinTileY) + SizeTileY > Envelope.MaxY)
		{
			return Refuse(FString::Printf(TEXT("Region %d exceeds the blockout envelope or tile budget."), Region.RegionID));
		}
		RasterVisits += RegionTiles;
		ShortestRegionTop = FMath::Min(ShortestRegionTop, RegionTop);

		for (int32 X = 0; X < SizeTileX; ++X)
		{
			for (int32 Y = 0; Y < SizeTileY; ++Y)
			{
				const FIntPoint Tile(MinTileX + X, MinTileY + Y);
				FloorTiles.Add(Tile);
				if (FloorTiles.Num() > MaxFloorTiles)
				{
					return Refuse(TEXT("The blockout exceeds its 65536 floor-tile budget."));
				}
				// Overlapping regions keep the taller roof, so no wall is left with a gap above it.
				double& Stored = TopByTile.FindOrAdd(Tile, RegionTop);
				Stored = FMath::Max(Stored, RegionTop);
			}
		}

		const FIntPoint Centre(MinTileX + SizeTileX / 2, MinTileY + SizeTileY / 2);
		CentreByRegionID.Add(Region.RegionID, Centre);
		RectByRegionID.Add(Region.RegionID,
			FTileRect{ MinTileX, MinTileY, MinTileX + SizeTileX, MinTileY + SizeTileY });
	}

	if (!OutIssues.IsEmpty())
	{
		return false;
	}

	// Corridors realise the logical connections. Without them a validated layout builds as
	// disconnected rooms: reachable on the graph, unwalkable in the level.
	// Its own labelled branch off the layout seed: corridor variety must never shift the draws
	// that decide rooms, encounters or loot.
	FRandomStream CorridorStream(
		URunSeedFunctionLibrary::DeriveSeed(Layout.Seed, FName(TEXT("Corridor")), 0));

	TSet<FIntPoint> CorridorTiles;
	for (const FPHGeneratedConnection& Connection : Layout.Connections)
	{
		// Drawn for every connection, resolved or not, so the sequence stays stable.
		const int32 Width = CorridorStream.RandRange(MinCorridorWidth, MaxCorridorWidth);
		const bool bXFirst = CorridorStream.FRand() < 0.5f;
		const int32 LaneRoll = CorridorStream.RandRange(0, MAX_int16);

		const FTileRect* FromRect = RectByRegionID.Find(Connection.FromRegionID);
		const FTileRect* ToRect = RectByRegionID.Find(Connection.ToRegionID);
		if (!FromRect || !ToRect)
		{
			continue;
		}

		// Two regions that share frontage get a short square passage. Only a diagonal pair, which
		// no straight run can join, falls back to a dog-leg through the two centres.
		const bool bFits = SharesFrontage(*FromRect, *ToRect)
			? AddStraightCorridor(*FromRect, *ToRect, Width, MinCorridorWidth, LaneRoll, Envelope, RasterVisits, FloorTiles, CorridorTiles)
			: AddCorridorTiles(CentreByRegionID[Connection.FromRegionID], CentreByRegionID[Connection.ToRegionID],
				Width, MinCorridorWidth, bXFirst, Envelope, RasterVisits, FloorTiles, CorridorTiles);
		if (!bFits)
		{
			// A loop is an enrichment on top of a spanning tree, so one that will not fit is
			// dropped and the floor still builds fully connected. A required connection has no
			// such slack: losing it would leave rooms reachable on the graph and walled off in
			// the level, which is the failure corridors exist to prevent.
			if (Connection.bOptional)
			{
				continue;
			}
			return Refuse(FString::Printf(TEXT("Corridor %d exceeds the blockout envelope or tile/work budget."), Connection.ConnectionID));
		}
	}

	OutPlan.Placements.Reserve(FloorTiles.Num() * 3);

	// Deterministic output order: TSet iteration order is not guaranteed stable across runs.
	TArray<FIntPoint> OrderedTiles = FloorTiles.Array();
	OrderedTiles.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return (A.X != B.X) ? (A.X < B.X) : (A.Y < B.Y);
	});

	// Corridors take the shortest room height, so rooms rise above the passages joining them.
	const double CorridorTop =
		(ShortestRegionTop < TNumericLimits<double>::Max()) ? ShortestRegionTop : FloorZ + TileSize;

	auto TopOf = [&TopByTile, CorridorTop](const FIntPoint& Tile)
	{
		const double* Found = TopByTile.Find(Tile);
		return Found ? *Found : CorridorTop;
	};

	for (const FIntPoint& Tile : OrderedTiles)
	{
		const double Top = TopOf(Tile);
		if (OutPlan.Placements.Num() + (bIncludeCeiling ? 2 : 1) > MaxPlacements)
		{
			return Refuse(TEXT("The blockout exceeds its 1000000 placement budget."));
		}

		FPHPiecePlacement& Floor = OutPlan.Placements.AddDefaulted_GetRef();
		Floor.PieceTag = PHGenerationTags::Piece_Floor.GetTag();
		Floor.Transform = FTransform(FVector(Tile.X * TileSize, Tile.Y * TileSize, FloorZ));

		if (bIncludeCeiling)
		{
			FPHPiecePlacement& Ceiling = OutPlan.Placements.AddDefaulted_GetRef();
			Ceiling.PieceTag = PHGenerationTags::Piece_Ceiling.GetTag();
			Ceiling.Transform = FTransform(FVector(Tile.X * TileSize, Tile.Y * TileSize, Top));
			++OutPlan.CeilingCount;
		}

		// A wall goes on every edge whose neighbour is not floor, which leaves an opening exactly
		// where a corridor meets a room without needing to reason about doorways separately.
		// Where a taller region abuts a shorter one, the courses above the neighbour are still
		// needed or the room would be open above head height.
		struct FEdge { FIntPoint Offset; FVector Origin; double Yaw; };
		const FEdge Edges[] = {
			{ FIntPoint(0, -1), FVector(Tile.X * TileSize, Tile.Y * TileSize, 0.0), 0.0 },
			{ FIntPoint(0,  1), FVector(Tile.X * TileSize, (Tile.Y + 1) * TileSize, 0.0), 0.0 },
			{ FIntPoint(-1, 0), FVector(Tile.X * TileSize, Tile.Y * TileSize, 0.0), 90.0 },
			{ FIntPoint( 1, 0), FVector((Tile.X + 1) * TileSize, Tile.Y * TileSize, 0.0), 90.0 },
		};

		for (const FEdge& Edge : Edges)
		{
			const FIntPoint Neighbour = Tile + Edge.Offset;
			const bool bOpenEdge = !FloorTiles.Contains(Neighbour);
			const double Base = bOpenEdge ? FloorZ : TopOf(Neighbour);
			if (Base >= Top - UE_KINDA_SMALL_NUMBER)
			{
				continue;
			}

			const int32 Courses = FMath::Max(1, FMath::RoundToInt32((Top - Base) / Course));
			if (Courses > MaxPlacements - OutPlan.Placements.Num())
			{
				return Refuse(TEXT("The blockout exceeds its 1000000 placement budget."));
			}
			for (int32 CourseIndex = 0; CourseIndex < Courses; ++CourseIndex)
			{
				FPHPiecePlacement& Wall = OutPlan.Placements.AddDefaulted_GetRef();
				Wall.PieceTag = PHGenerationTags::Piece_Wall_Straight.GetTag();
				FVector Origin = Edge.Origin;
				Origin.Z = Base + CourseIndex * Course;
				Wall.Transform = FTransform(FRotator(0.0, Edge.Yaw, 0.0), Origin);
				++OutPlan.WallCount;
			}
		}
	}

	// Lighting is a coverage problem, not a per-room decoration. One light per room leaves a large
	// room lit in the middle and black at the edges, and taking every Nth tile from a sorted
	// corridor array walks the map in column order rather than along any passage, so lights clump
	// in some places and vanish in others. Both were visible in play: black room corners next to an
	// over-lit corridor.
	//
	// Farthest-point sampling instead: seed near the middle of the floor, then repeatedly light the
	// tile furthest from every light so far, until nothing is further than the spacing. That is
	// evenly spread by construction and gives a property worth testing - no floor tile is further
	// than LightSpacingTiles from a light - which "one per room" could never promise.
	if (LightSpacingTiles > 0 && !OrderedTiles.IsEmpty())
	{
		FVector2D Centroid = FVector2D::ZeroVector;
		for (const FIntPoint& Tile : OrderedTiles)
		{
			Centroid += FVector2D(Tile.X, Tile.Y);
		}
		Centroid /= static_cast<double>(OrderedTiles.Num());

		TArray<double> NearestLightSquared;
		NearestLightSquared.Init(TNumericLimits<double>::Max(), OrderedTiles.Num());

		auto PlaceLightAt = [&](const int32 TileIndex)
		{
			const FIntPoint& Chosen = OrderedTiles[TileIndex];
			// The ceiling mounting plane, matching where the ceiling piece is
			// placed. It used to sit a quarter course below that, which hung
			// both the fixture and its light a metre under the ceiling.
			// A light source that must clear the ceiling geometry is the
			// floor actor's concern, not the plan's.
			OutPlan.LightPoses.Add(FTransform(FVector(
				(Chosen.X + 0.5) * TileSize, (Chosen.Y + 0.5) * TileSize,
				TopOf(Chosen))));

			for (int32 Index = 0; Index < OrderedTiles.Num(); ++Index)
			{
				NearestLightSquared[Index] = FMath::Min(NearestLightSquared[Index],
					FVector2D::DistSquared(FVector2D(OrderedTiles[Index].X, OrderedTiles[Index].Y),
						FVector2D(Chosen.X, Chosen.Y)));
			}
		};

		// Strict comparison keeps the first tile on ties, so the seed is the same every run.
		int32 SeedIndex = 0;
		double BestToCentre = TNumericLimits<double>::Max();
		for (int32 Index = 0; Index < OrderedTiles.Num(); ++Index)
		{
			const double Distance =
				FVector2D::DistSquared(FVector2D(OrderedTiles[Index].X, OrderedTiles[Index].Y), Centroid);
			if (Distance < BestToCentre)
			{
				BestToCentre = Distance;
				SeedIndex = Index;
			}
		}
		PlaceLightAt(SeedIndex);

		const double SpacingSquared =
			static_cast<double>(LightSpacingTiles) * static_cast<double>(LightSpacingTiles);

		// Bounded by the tile count: every pass lights a tile that was not lit before, so this
		// cannot run longer than there are tiles even if the distance maths were wrong.
		for (int32 Pass = 1; Pass < OrderedTiles.Num(); ++Pass)
		{
			int32 FurthestIndex = INDEX_NONE;
			double FurthestSquared = SpacingSquared;
			for (int32 Index = 0; Index < OrderedTiles.Num(); ++Index)
			{
				if (NearestLightSquared[Index] > FurthestSquared)
				{
					FurthestSquared = NearestLightSquared[Index];
					FurthestIndex = Index;
				}
			}

			if (FurthestIndex == INDEX_NONE)
			{
				break;
			}
			if (static_cast<int64>(Pass + 1) * OrderedTiles.Num() * 2 > MaxLightDistanceChecks)
			{
				return Refuse(TEXT("The blockout exceeds its 16000000 lighting-distance-check budget; increase light spacing or reduce the floor."));
			}
			PlaceLightAt(FurthestIndex);
		}
	}

	OutPlan.FloorTileCount = OrderedTiles.Num();
	OutPlan.CorridorTileCount = CorridorTiles.Num();
	OutPlan.TileSize = TileSize;
	OutPlan.FloorTiles = OrderedTiles;

	// Sorted for the same reason placements are: set order is not stable between runs.
	OutPlan.CorridorTiles = CorridorTiles.Array();
	OutPlan.CorridorTiles.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return (A.X != B.X) ? (A.X < B.X) : (A.Y < B.Y);
	});

	auto FindAnchor = [&Layout](const int32 AnchorID) -> const FPHGeneratedAnchor*
	{
		return Layout.Anchors.FindByPredicate(
			[AnchorID](const FPHGeneratedAnchor& Anchor) { return Anchor.AnchorID == AnchorID; });
	};

	// Endpoints are snapped to the centre of a tile that was actually built, then lifted to the
	// floor plane. A region centre is not a safe pose on its own: an even-sized room centres on
	// the seam between two tiles, and a corridor lane can put the nearest built floor a tile away
	// from where the logical anchor sits. Anything spawned on an unsnapped anchor - a PlayerStart
	// above all - can end up straddling geometry or off the floor entirely.
	auto SnapToBuiltTile = [&OrderedTiles, &FloorTiles, TileSize, FloorZ](
		const FVector& Location, FIntPoint& OutTile)
	{
		const FIntPoint Raw(FMath::FloorToInt32(Location.X / TileSize),
			FMath::FloorToInt32(Location.Y / TileSize));
		OutTile = Raw;

		if (!FloorTiles.Contains(Raw))
		{
			double BestDistance = TNumericLimits<double>::Max();
			for (const FIntPoint& Tile : OrderedTiles)
			{
				const double Distance = FVector2D::DistSquared(
					FVector2D(Tile.X, Tile.Y), FVector2D(Raw.X, Raw.Y));
				if (Distance < BestDistance)
				{
					BestDistance = Distance;
					OutTile = Tile;
				}
			}
		}

		return FTransform(FVector(
			(OutTile.X + 0.5) * TileSize, (OutTile.Y + 0.5) * TileSize, FloorZ));
	};

	if (const FPHGeneratedAnchor* Start = FindAnchor(Layout.PlayerStartAnchorID))
	{
		OutPlan.PlayerStart = SnapToBuiltTile(Start->Transform.GetLocation(), OutPlan.PlayerStartTile);
	}
	if (const FPHGeneratedAnchor* Exit = FindAnchor(Layout.ExitAnchorID))
	{
		OutPlan.Exit = SnapToBuiltTile(Exit->Transform.GetLocation(), OutPlan.ExitTile);
	}

	return true;
}

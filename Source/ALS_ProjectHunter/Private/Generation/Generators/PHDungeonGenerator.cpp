#include "Generation/Generators/PHDungeonGenerator.h"

#include "Generation/PHGenerationTags.h"

namespace PHDungeonGeneratorPrivate
{
	/** Rooms are scattered or grown one at a time; this caps a pathological request. */
	constexpr int32 MaxRooms = 64;

	/** At most 2 * 64 * 1024 placement attempts, including the growth fallback (default: 32). */
	constexpr int32 MaxPlacementRetries = 1024;

	/** Bounds total shuffle work and the largest int32 slot buffer to 1 MiB, before any allocation. */
	constexpr int64 MaxAnchorSlotWork = 262144;

	/**
	 * A room in whole modules. Min is inclusive and Max exclusive, so two rooms sharing an edge
	 * value do not overlap — the same convention the tile grid uses, which keeps the placement
	 * maths and the later blockout in agreement about what "adjacent" means.
	 */
	struct FRoomRect
	{
		int32 MinX = 0;
		int32 MinY = 0;
		int32 MaxX = 0;
		int32 MaxY = 0;

		int32 Width() const { return MaxX - MinX; }
		int32 Height() const { return MaxY - MinY; }
	};

	bool Overlaps(const FRoomRect& A, const FRoomRect& B, const int32 Margin)
	{
		return static_cast<int64>(A.MinX) - Margin < B.MaxX && B.MinX < static_cast<int64>(A.MaxX) + Margin
			&& static_cast<int64>(A.MinY) - Margin < B.MaxY && B.MinY < static_cast<int64>(A.MaxY) + Margin;
	}

	/**
	 * Rooms are drawn in whole modules and only multiplied into units at the end, so every
	 * edge lands exactly on the grid. Modular blockout walls and floors tile a room only when
	 * its edges are module multiples, and no later construction pass can recover alignment
	 * that logical generation threw away.
	 */
	FBox ToBox(const FRoomRect& Rect, const int32 Stacks, const double Grid, const int32 HeightModules)
	{
		return FBox(
			FVector(Rect.MinX * Grid, Rect.MinY * Grid, 0.0),
			FVector(Rect.MaxX * Grid, Rect.MaxY * Grid, HeightModules * Grid * Stacks));
	}

	/** Breadth-first hop count from the start room; unreachable rooms keep INDEX_NONE. */
	TArray<int32> HopCountsFromStart(const TArray<TArray<int32>>& Neighbours)
	{
		TArray<int32> Hops;
		Hops.Init(INDEX_NONE, Neighbours.Num());
		if (Neighbours.IsEmpty())
		{
			return Hops;
		}

		TArray<int32> Pending;
		Pending.Reserve(Neighbours.Num());
		Pending.Add(0);
		Hops[0] = 0;

		for (int32 PendingIndex = 0; PendingIndex < Pending.Num(); ++PendingIndex)
		{
			const int32 Current = Pending[PendingIndex];
			for (const int32 Next : Neighbours[Current])
			{
				if (Hops[Next] == INDEX_NONE)
				{
					Hops[Next] = Hops[Current] + 1;
					Pending.Add(Next);
				}
			}
		}
		return Hops;
	}
}

UPHDungeonGenerator::UPHDungeonGenerator()
{
	// 3: regions grow off one another by default and loops are distance limited, so version 2
	// seeds no longer replay.
	GenerationVersion = 3;
}

bool UPHDungeonGenerator::BuildLayout(const FPHLayoutRequest& Request, FRandomStream& Stream,
	FPHGeneratedLayout& OutLayout, TArray<FPHGenerationIssue>& OutIssues)
{
	using namespace PHDungeonGeneratorPrivate;
	auto Refuse = [&OutIssues](FString Message)
	{
		AddIssue(OutIssues, EPHGenerationIssueCode::InvalidRequest, INDEX_NONE, MoveTemp(Message));
		return false;
	};

	// These capacities belong to this strategy, not the generator-neutral request contract.
	// Refuse before drawing so a cap never silently lowers the authored minimum or truncates work.
	if (Request.MinRegionCount > MaxRooms)
	{
		return Refuse(FString::Printf(TEXT("Dungeon strategy supports at most %d regions; the requested minimum is %d."),
			MaxRooms, Request.MinRegionCount));
	}
	if (Request.MaxPlacementAttempts > MaxPlacementRetries)
	{
		return Refuse(FString::Printf(TEXT("Dungeon placement permits at most %d retries per region, got %d."),
			MaxPlacementRetries, Request.MaxPlacementAttempts));
	}

	const double Grid = Request.GridSize;
	const int32 AreaModulesX = ModulesDown(Request.AreaSize.X, Grid);
	const int32 AreaModulesY = ModulesDown(Request.AreaSize.Y, Grid);
	const int32 HeightModules = FMath::Max(1, ModulesDown(Request.RegionHeight, Grid));

	// Rounded up to a whole module so the gap a corridor has to bridge is itself tileable, and
	// never zero: rooms placed flush would read as one space with a seam through it.
	const int32 SpacingModules =
		FMath::Max(1, ModulesUp(Request.RegionSpacing, Grid));

	// A maximum wider than the area is capped rather than refused: the area is the binding
	// limit, and ValidateRequest has already proved the minimum fits.
	const int32 MinSizeX = ModulesUp(Request.MinRegionSize.X, Grid);
	const int32 MinSizeY = ModulesUp(Request.MinRegionSize.Y, Grid);
	const int32 MaxSizeX = FMath::Min(ModulesDown(Request.MaxRegionSize.X, Grid), AreaModulesX);
	const int32 MaxSizeY = FMath::Min(ModulesDown(Request.MaxRegionSize.Y, Grid), AreaModulesY);
	const bool bGrow = (Request.RegionPlacement == EPHRegionPlacement::Growth);

	int32 MaxGap = SpacingModules;
	if (bGrow)
	{
		const int64 RequestedMaxGap = static_cast<int64>(SpacingModules) + Request.ExtraCorridorModules;
		if (RequestedMaxGap > MAX_int32)
		{
			return Refuse(TEXT("Dungeon spacing plus ExtraCorridorModules exceeds the supported 32-bit growth gap."));
		}
		MaxGap = static_cast<int32>(RequestedMaxGap);
	}

	// FBox centre calculation adds its planar corners. Even finite authored extents must not
	// overflow when converted into world-space bounds and centre poses.
	if (!FMath::IsFinite(AreaModulesX * Grid * 2.0) || !FMath::IsFinite(AreaModulesY * Grid * 2.0))
	{
		return Refuse(TEXT("Dungeon planar bounds are too large to compute finite world-space centres."));
	}

	const int32 TargetRooms = FMath::Min(
		Stream.RandRange(Request.MinRegionCount, Request.MaxRegionCount), MaxRooms);

	TArray<FRoomRect> Rooms;
	TArray<int32> Stacks;
	TArray<int32> ParentOf;
	Rooms.Reserve(TargetRooms);
	Stacks.Reserve(TargetRooms);
	ParentOf.Reserve(TargetRooms);

	auto DrawStacks = [&Stream, &Request]()
	{
		// Guarded so a single-stack request draws nothing extra and its seeds replay unchanged.
		return (Request.MaxHeightStacks > 1) ? Stream.RandRange(1, Request.MaxHeightStacks) : 1;
	};

	auto Collides = [&Rooms](const FRoomRect& Candidate, const int32 Margin)
	{
		for (const FRoomRect& Existing : Rooms)
		{
			if (Overlaps(Candidate, Existing, Margin))
			{
				return true;
			}
		}
		return false;
	};

	// Free placement anywhere in the area. Used for the first room, for Scatter layouts, and as
	// the fallback that keeps the requested room count honest when growth runs out of frontage.
	auto TryScatter = [&](FRoomRect& OutRect, int32& OutStacks)
	{
		const int32 SizeX = Stream.RandRange(MinSizeX, MaxSizeX);
		const int32 SizeY = Stream.RandRange(MinSizeY, MaxSizeY);

		// Sampling the min corner from the remaining span keeps the room inside the area.
		OutRect.MinX = Stream.RandRange(0, AreaModulesX - SizeX);
		OutRect.MinY = Stream.RandRange(0, AreaModulesY - SizeY);
		OutRect.MaxX = OutRect.MinX + SizeX;
		OutRect.MaxY = OutRect.MinY + SizeY;
		OutStacks = DrawStacks();

		return !Collides(OutRect, SpacingModules);
	};

	// Grows one room off an already-placed one. Every draw is taken before the first early-out,
	// so a rejected attempt advances the stream exactly as far as an accepted one and the seed
	// sequence stays predictable however the attempt resolves.
	auto TryGrow = [&](FRoomRect& OutRect, int32& OutStacks, int32& OutParent)
	{
		const int32 Window = FMath::Min(Rooms.Num(), Request.GrowthFrontier);
		OutParent = Rooms.Num() - 1 - Stream.RandRange(0, Window - 1);

		const int32 SizeX = Stream.RandRange(MinSizeX, MaxSizeX);
		const int32 SizeY = Stream.RandRange(MinSizeY, MaxSizeY);
		const int32 Side = Stream.RandRange(0, 3);
		const int32 Gap = Stream.RandRange(SpacingModules, MaxGap);
		const int32 SlideRoll = Stream.RandRange(0, MAX_int16);

		const FRoomRect Parent = Rooms[OutParent];

		// Frontage is what makes a short straight corridor possible: the two rooms must share a
		// run of modules on the axis they are not separated along. Two is preferred so a wide
		// corridor still meets the wall squarely, but a thin room may only afford one.
		const bool bAlongX = (Side < 2);
		const int32 SharedSpan = bAlongX ? SizeY : SizeX;
		const int32 ParentSpan = bAlongX ? Parent.Height() : Parent.Width();
		const int32 MinFrontage = FMath::Min3(2, SharedSpan, ParentSpan);

		const int64 SlideLow = static_cast<int64>(bAlongX ? Parent.MinY : Parent.MinX) - SharedSpan + MinFrontage;
		const int64 SlideHigh = static_cast<int64>(bAlongX ? Parent.MaxY : Parent.MaxX) - MinFrontage;
		const int64 Slide = SlideLow + SlideRoll % FMath::Max<int64>(1, SlideHigh - SlideLow + 1);

		// A rejected candidate can lie beyond int32 even though the area and each input fit.
		// Keep intermediate arithmetic wide and narrow only after proving the candidate fits.
		int64 CandidateMinX = 0;
		int64 CandidateMinY = 0;
		switch (Side)
		{
		case 0: CandidateMinX = static_cast<int64>(Parent.MaxX) + Gap; CandidateMinY = Slide; break;
		case 1: CandidateMinX = static_cast<int64>(Parent.MinX) - Gap - SizeX; CandidateMinY = Slide; break;
		case 2: CandidateMinX = Slide; CandidateMinY = static_cast<int64>(Parent.MaxY) + Gap; break;
		default: CandidateMinX = Slide; CandidateMinY = static_cast<int64>(Parent.MinY) - Gap - SizeY; break;
		}

		const int64 CandidateMaxX = CandidateMinX + SizeX;
		const int64 CandidateMaxY = CandidateMinY + SizeY;
		OutStacks = DrawStacks();

		if (CandidateMinX < 0 || CandidateMinY < 0
			|| CandidateMaxX > AreaModulesX || CandidateMaxY > AreaModulesY)
		{
			return false;
		}
		OutRect.MinX = static_cast<int32>(CandidateMinX);
		OutRect.MinY = static_cast<int32>(CandidateMinY);
		OutRect.MaxX = static_cast<int32>(CandidateMaxX);
		OutRect.MaxY = static_cast<int32>(CandidateMaxY);

		// The parent needs no special case: the drawn gap is never smaller than the spacing, so
		// the same margin test that keeps other rooms clear already passes against it.
		return !Collides(OutRect, SpacingModules);
	};

	for (int32 RoomIndex = 0; RoomIndex < TargetRooms; ++RoomIndex)
	{
		FRoomRect Rect;
		int32 RoomStacks = 1;
		int32 Parent = INDEX_NONE;
		bool bPlaced = false;

		if (bGrow && !Rooms.IsEmpty())
		{
			for (int32 Attempt = 0; Attempt < Request.MaxPlacementAttempts && !bPlaced; ++Attempt)
			{
				bPlaced = TryGrow(Rect, RoomStacks, Parent);
			}
		}

		// Falling back rather than dropping the room: the requested count is a promise, and a
		// floor short of rooms is a worse outcome than one room that did not find frontage.
		if (!bPlaced)
		{
			Parent = INDEX_NONE;
			for (int32 Attempt = 0; Attempt < Request.MaxPlacementAttempts && !bPlaced; ++Attempt)
			{
				bPlaced = TryScatter(Rect, RoomStacks);
			}
		}

		if (bPlaced)
		{
			Rooms.Add(Rect);
			Stacks.Add(RoomStacks);
			ParentOf.Add(Parent);
		}
	}

	if (Rooms.Num() < Request.MinRegionCount)
	{
		AddIssue(OutIssues, EPHGenerationIssueCode::UnplaceableRegions, INDEX_NONE,
			FString::Printf(
				TEXT("Placed %d of a required %d regions in area %s; the request is too dense."),
				Rooms.Num(), Request.MinRegionCount, *Request.AreaSize.ToString()));
		return false;
	}

	for (int32 RoomIndex = 0; RoomIndex < Rooms.Num(); ++RoomIndex)
	{
		FPHGeneratedRegion& Region = OutLayout.Regions.AddDefaulted_GetRef();
		Region.RegionID = RoomIndex;
		Region.Bounds = ToBox(Rooms[RoomIndex], Stacks[RoomIndex], Grid, HeightModules);
		OutLayout.Bounds += Region.Bounds;
	}

	auto CentreOf = [&Rooms](const int32 Index)
	{
		return FVector2D(
			(static_cast<double>(Rooms[Index].MinX) + Rooms[Index].MaxX) * 0.5,
			(static_cast<double>(Rooms[Index].MinY) + Rooms[Index].MaxY) * 0.5);
	};

	TArray<TArray<int32>> Neighbours;
	Neighbours.SetNum(Rooms.Num());

	auto AddConnection = [&OutLayout, &Neighbours](const int32 From, const int32 To, const bool bOptional = false)
	{
		FPHGeneratedConnection& Connection = OutLayout.Connections.AddDefaulted_GetRef();
		Connection.ConnectionID = OutLayout.Connections.Num() - 1;
		Connection.FromRegionID = From;
		Connection.ToRegionID = To;
		Connection.bBidirectional = true;
		// Marked so construction may drop a loop it cannot physically build without losing the
		// floor. Every connection added above this point is a spanning-tree edge and required.
		Connection.bOptional = bOptional;
		Neighbours[From].Add(To);
		Neighbours[To].Add(From);
	};

	if (bGrow)
	{
		// The growth tree is already a spanning tree: every room but the first was placed against
		// an earlier one, so linking each to its parent connects the floor with the shortest
		// corridors available. A room that fell back to a free draw has no parent and takes the
		// nearest earlier room instead, which is the best link available after the fact.
		for (int32 RoomIndex = 1; RoomIndex < Rooms.Num(); ++RoomIndex)
		{
			int32 Parent = ParentOf[RoomIndex];
			if (Parent == INDEX_NONE)
			{
				double BestDistance = TNumericLimits<double>::Max();
				for (int32 Candidate = 0; Candidate < RoomIndex; ++Candidate)
				{
					// Strict less-than keeps the first candidate on ties, so equal spacing
					// resolves the same way every run.
					const double Distance =
						FVector2D::DistSquared(CentreOf(RoomIndex), CentreOf(Candidate));
					if (Distance < BestDistance)
					{
						BestDistance = Distance;
						Parent = Candidate;
					}
				}
			}
			AddConnection(Parent, RoomIndex);
		}
	}
	else
	{
		// Nearest-neighbour spanning tree over room centres. Growing from a single room
		// guarantees one connected component, which is what structural validation requires.
		TArray<bool> bLinked;
		bLinked.Init(false, Rooms.Num());
		bLinked[0] = true;
		TArray<int32> LinkedRooms;
		LinkedRooms.Reserve(Rooms.Num());
		LinkedRooms.Add(0);

		while (LinkedRooms.Num() < Rooms.Num())
		{
			double BestDistance = TNumericLimits<double>::Max();
			int32 BestFrom = INDEX_NONE;
			int32 BestTo = INDEX_NONE;

			for (const int32 LinkedRoom : LinkedRooms)
			{
				for (int32 Candidate = 0; Candidate < Rooms.Num(); ++Candidate)
				{
					if (bLinked[Candidate])
					{
						continue;
					}

					const double Distance = FVector2D::DistSquared(CentreOf(LinkedRoom), CentreOf(Candidate));
					if (Distance < BestDistance)
					{
						BestDistance = Distance;
						BestFrom = LinkedRoom;
						BestTo = Candidate;
					}
				}
			}

			if (BestTo == INDEX_NONE)
			{
				break;
			}

			AddConnection(BestFrom, BestTo);
			bLinked[BestTo] = true;
			LinkedRooms.Add(BestTo);
		}
	}

	// Optional loops on top of the tree. Rolled for every pair in a fixed order so the
	// draw sequence stays stable even when a pair is skipped. Distant pairs are excluded
	// because a loop across the floor builds as a corridor cutting through everything
	// between its ends, which erases the room structure the loop was meant to enrich.
	const double MaxLoopModules = (Request.MaxLoopDistance > 0.0)
		? Request.MaxLoopDistance / Grid : TNumericLimits<double>::Max();
	const double MaxLoopModulesSquared = (MaxLoopModules < TNumericLimits<double>::Max())
		? MaxLoopModules * MaxLoopModules : TNumericLimits<double>::Max();

	for (int32 From = 0; From < Rooms.Num(); ++From)
	{
		for (int32 To = From + 1; To < Rooms.Num(); ++To)
		{
			if (Stream.FRand() >= Request.LoopChance)
			{
				continue;
			}
			if (Neighbours[From].Contains(To))
			{
				continue;
			}
			if (FVector2D::DistSquared(CentreOf(From), CentreOf(To)) > MaxLoopModulesSquared)
			{
				continue;
			}
			AddConnection(From, To, true);
		}
	}

	// The exit goes as deep into the floor as the connection graph allows. Hops decide it so a
	// sprawling floor still ends somewhere that takes several rooms to reach; distance breaks
	// ties so two equally deep rooms resolve to the one that is actually further away.
	const TArray<int32> Hops = HopCountsFromStart(Neighbours);
	int32 ExitRoom = 0;
	int32 FurthestHops = 0;
	double FurthestDistance = 0.0;
	for (int32 RoomIndex = 0; RoomIndex < Rooms.Num(); ++RoomIndex)
	{
		const double Distance = FVector2D::DistSquared(CentreOf(0), CentreOf(RoomIndex));
		if (Hops[RoomIndex] > FurthestHops
			|| (Hops[RoomIndex] == FurthestHops && Distance > FurthestDistance))
		{
			FurthestHops = Hops[RoomIndex];
			FurthestDistance = Distance;
			ExitRoom = RoomIndex;
		}
	}

	FPHGeneratedAnchor& StartAnchor = OutLayout.Anchors.AddDefaulted_GetRef();
	StartAnchor.AnchorID = 0;
	StartAnchor.RegionID = 0;
	StartAnchor.Transform = FTransform(OutLayout.Regions[0].Bounds.GetCenter());
	StartAnchor.SemanticTag = PHGenerationTags::Anchor_PlayerStart.GetTag();

	// A single-room layout still needs two distinct poses; offset toward the max corner
	// while staying inside the room so the anchor passes its bounds check.
	const FVector ExitLocation = (ExitRoom == 0)
		? FMath::Lerp(OutLayout.Regions[0].Bounds.GetCenter(), OutLayout.Regions[0].Bounds.Max, 0.5)
		: OutLayout.Regions[ExitRoom].Bounds.GetCenter();

	FPHGeneratedAnchor& ExitAnchor = OutLayout.Anchors.AddDefaulted_GetRef();
	ExitAnchor.AnchorID = 1;
	ExitAnchor.RegionID = ExitRoom;
	ExitAnchor.Transform = FTransform(ExitLocation);
	ExitAnchor.SemanticTag = PHGenerationTags::Anchor_Exit.GetTag();

	OutLayout.PlayerStartAnchorID = StartAnchor.AnchorID;
	OutLayout.ExitAnchorID = ExitAnchor.AnchorID;

	// Guarded so a layout without authored rules draws nothing extra and a seed replays
	// identically whether or not the anchor feature exists.
	if (!Request.AnchorRules.IsEmpty())
	{
		TArray<FBox> RoomBoxes;
		RoomBoxes.Reserve(OutLayout.Regions.Num());
		for (const FPHGeneratedRegion& Region : OutLayout.Regions)
		{
			RoomBoxes.Add(Region.Bounds);
		}
		if (!PlaceRuleAnchors(Request, Stream, RoomBoxes, ExitRoom, OutLayout, OutIssues))
		{
			return false;
		}
	}
	return true;
}

bool UPHDungeonGenerator::PlaceRuleAnchors(const FPHLayoutRequest& Request, FRandomStream& Stream,
	const TArray<FBox>& Rooms, const int32 ExitRoom, FPHGeneratedLayout& OutLayout,
	TArray<FPHGenerationIssue>& OutIssues) const
{
	using namespace PHDungeonGeneratorPrivate;
	const double Grid = Request.GridSize;

	// Preflight actual rooms before allocating or drawing anything. A product such as
	// 50000 * 50000 otherwise wraps int32 before Reserve sees it; smaller products can
	// still allocate and shuffle unreasonable buffers. No rules means this pass is never called.
	TArray<FIntPoint> InteriorDimensions;
	InteriorDimensions.Reserve(Rooms.Num());
	int64 TotalSlots = 0;
	for (int32 RoomIndex = 0; RoomIndex < Rooms.Num(); ++RoomIndex)
	{
		const FVector Size = Rooms[RoomIndex].GetSize();
		const int32 WidthModules = ModulesDown(Size.X, Grid);
		const int32 HeightModules = ModulesDown(Size.Y, Grid);
		if (WidthModules < 1 || HeightModules < 1)
		{
			AddIssue(OutIssues, EPHGenerationIssueCode::InvalidRequest, RoomIndex,
				TEXT("Dungeon anchor placement requires representable positive room footprints."));
			return false;
		}
		const FIntPoint Slots(FMath::Max(0, WidthModules - 2), FMath::Max(0, HeightModules - 2));
		const int64 SlotCount = static_cast<int64>(Slots.X) * Slots.Y;
		if (SlotCount > MaxAnchorSlotWork - TotalSlots)
		{
			AddIssue(OutIssues, EPHGenerationIssueCode::InvalidRequest, RoomIndex,
				FString::Printf(
					TEXT("Dungeon anchor placement exceeds its %lld-slot work budget at region %d (%lld slots). Reduce room footprints or remove anchor rules."),
					MaxAnchorSlotWork, RoomIndex, SlotCount));
			return false;
		}
		TotalSlots += SlotCount;
		InteriorDimensions.Add(Slots);
	}

	int32 NextAnchorID = OutLayout.Anchors.Num();
	TMap<FGameplayTag, int32> PlacedTotals;

	for (int32 RoomIndex = 0; RoomIndex < Rooms.Num(); ++RoomIndex)
	{
		const FBox& Room = Rooms[RoomIndex];

		// One module is reserved along each edge so an anchor never sits in a wall.
		const int32 SlotsX = InteriorDimensions[RoomIndex].X;
		const int32 SlotsY = InteriorDimensions[RoomIndex].Y;
		if (SlotsX <= 0 || SlotsY <= 0)
		{
			continue;
		}

		const int32 SlotCount = static_cast<int32>(static_cast<int64>(SlotsX) * SlotsY);
		TArray<int32> Slots;
		Slots.Reserve(SlotCount);
		for (int32 Slot = 0; Slot < SlotCount; ++Slot)
		{
			Slots.Add(Slot);
		}

		// Shuffled once per room so different kinds interleave instead of clustering by rule order.
		for (int32 Index = Slots.Num() - 1; Index > 0; --Index)
		{
			Slots.Swap(Index, Stream.RandRange(0, Index));
		}

		const bool bIsStartRoom = (RoomIndex == 0);
		const bool bIsExitRoom = (RoomIndex == ExitRoom);
		int32 NextSlot = 0;

		for (const FPHAnchorRule& Rule : Request.AnchorRules)
		{
			if ((bIsStartRoom && !Rule.bAllowInStartRegion)
				|| (bIsExitRoom && !Rule.bAllowInExitRegion))
			{
				continue;
			}

			// Drawn before the cap is applied so a full budget does not shift later draws.
			int32 Wanted = Stream.RandRange(Rule.MinPerRegion, Rule.MaxPerRegion);
			if (Rule.MaxTotal > 0)
			{
				Wanted = FMath::Min(Wanted, Rule.MaxTotal - PlacedTotals.FindRef(Rule.SemanticTag));
			}

			for (int32 Placed = 0; Placed < Wanted && NextSlot < Slots.Num(); ++Placed)
			{
				const int32 Slot = Slots[NextSlot++];
				const FVector Location(
					Room.Min.X + (1 + (Slot % SlotsX) + 0.5) * Grid,
					Room.Min.Y + (1 + (Slot / SlotsX) + 0.5) * Grid,
					Room.Min.Z);

				FPHGeneratedAnchor& Anchor = OutLayout.Anchors.AddDefaulted_GetRef();
				Anchor.AnchorID = NextAnchorID++;
				Anchor.RegionID = RoomIndex;
				Anchor.Transform = FTransform(Location);
				Anchor.SemanticTag = Rule.SemanticTag;

				PlacedTotals.FindOrAdd(Rule.SemanticTag)++;
			}
		}
	}
	return true;
}

bool UPHDungeonGenerator::ValidateStrategyConstraints(const FPHLayoutRequest& Request,
	const FPHGeneratedLayout& Layout, TArray<FPHGenerationIssue>& OutIssues) const
{
	// ValidateLayout permits overlapping envelopes, so rooms-and-links proves its own
	// separation here. Two rooms sharing space would read as one space in play.
	// The ceiling of the tallest permitted region, not of a single storey: with height stacks a
	// room may legitimately rise several RegionHeight steps above the floor.
	const FBox Area(FVector::ZeroVector,
		FVector(Request.AreaSize.X, Request.AreaSize.Y,
			Request.RegionHeight * FMath::Max(1, Request.MaxHeightStacks)));

	// Grid alignment is checked on the finished layout, not merely trusted from placement,
	// because a room off the module grid cannot be tiled by modular blockout meshes.
	auto IsOnGrid = [Grid = Request.GridSize](const double Value)
	{
		return FMath::IsNearlyZero(FMath::Fmod(FMath::Abs(Value), Grid), UE_KINDA_SMALL_NUMBER)
			|| FMath::IsNearlyEqual(FMath::Fmod(FMath::Abs(Value), Grid), Grid, UE_KINDA_SMALL_NUMBER);
	};

	const int32 IssuesBefore = OutIssues.Num();
	for (int32 Index = 0; Index < Layout.Regions.Num(); ++Index)
	{
		const FPHGeneratedRegion& Region = Layout.Regions[Index];
		if (!Area.IsInsideOrOn(Region.Bounds))
		{
			AddIssue(OutIssues, EPHGenerationIssueCode::RegionOutsideLayout, Index,
				FString::Printf(TEXT("Region %d escapes the requested area."), Region.RegionID));
		}

		const FVector Corners[] = { Region.Bounds.Min, Region.Bounds.Max };
		for (const FVector& Corner : Corners)
		{
			if (!IsOnGrid(Corner.X) || !IsOnGrid(Corner.Y) || !IsOnGrid(Corner.Z))
			{
				AddIssue(OutIssues, EPHGenerationIssueCode::RegionOffGrid, Index,
					FString::Printf(TEXT("Region %d corner %s is off the %f-unit module grid."),
						Region.RegionID, *Corner.ToString(), Request.GridSize));
				break;
			}
		}

		for (int32 Other = Index + 1; Other < Layout.Regions.Num(); ++Other)
		{
			if (Region.Bounds.Intersect(Layout.Regions[Other].Bounds))
			{
				AddIssue(OutIssues, EPHGenerationIssueCode::OverlappingRegions, Index,
					FString::Printf(TEXT("Regions %d and %d overlap."),
						Region.RegionID, Layout.Regions[Other].RegionID));
			}
		}
	}

	return OutIssues.Num() == IssuesBefore;
}

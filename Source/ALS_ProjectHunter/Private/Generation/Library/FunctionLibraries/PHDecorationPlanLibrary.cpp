#include "Generation/Library/FunctionLibraries/PHDecorationPlanLibrary.h"

#include "GameplayTagsManager.h"
#include "Generation/PHGenerationTags.h"

namespace PHDecorationPlanPrivate
{
	constexpr int32 MaxDecorationTiles = 65536;
	constexpr int64 MaxRegionTileChecks = 8000000;
	constexpr int64 MaxClusterChecks = 8000000;

	void AddIssue(TArray<FPHGenerationIssue>& Issues, const int32 ElementIndex, FString Message)
	{
		FPHGenerationIssue& Issue = Issues.AddDefaulted_GetRef();
		Issue.Code = EPHGenerationIssueCode::InvalidRequest;
		Issue.ElementIndex = ElementIndex;
		Issue.Message = MoveTemp(Message);
	}

	/** Registry membership before hierarchical matching; see REVIEW-PH-20260829-02. */
	bool IsRegisteredPropTag(const FGameplayTag& Tag)
	{
		const TSharedPtr<FGameplayTagNode> Node = UGameplayTagsManager::Get().FindTagNode(Tag);
		return Node.IsValid() && Node->GetCompleteTag() == Tag
			&& Node->GetSingleTagContainer().HasTag(PHGenerationTags::Prop);
	}

	/**
	 * How much likelier a prop is at the heart of a clump than a full radius away. Constants rather
	 * than authored fields: they set how a clump reads, not what is in it, and every extra knob on
	 * a content struct is one more thing to get wrong per rule. The stray weight is deliberately
	 * nonzero so a room keeps a few odd items outside its piles.
	 */
	constexpr float ClusterPeakWeight = 2.75f;
	constexpr float ClusterStrayWeight = 0.15f;

	/** Highest chance any one tile may reach, so the middle of a clump stays a pile with gaps. */
	constexpr float MaxTileWeight = 0.75f;

	/** Both the candidate and every previously placed prop keep their own clearance. */
	bool IsCrowded(const FIntPoint& Tile, const TMap<FIntPoint, int32>& Occupied,
		const int32 Spacing, const int32 LargestPreviousSpacing)
	{
		if (Spacing == 0 && LargestPreviousSpacing == 0)
		{
			return Occupied.Contains(Tile);
		}

		// Small radii retain constant-size hash probes; large radii visit only actual props.
		const int64 Diameter = static_cast<int64>(Spacing) * 2 + 1;
		if (Spacing >= LargestPreviousSpacing && Diameter <= 65535
			&& Diameter * Diameter <= Occupied.Num())
		{
			for (int64 X = -static_cast<int64>(Spacing); X <= Spacing; ++X)
			{
				for (int64 Y = -static_cast<int64>(Spacing); Y <= Spacing; ++Y)
				{
					const int64 NeighbourX = static_cast<int64>(Tile.X) + X;
					const int64 NeighbourY = static_cast<int64>(Tile.Y) + Y;
					if (NeighbourX >= MIN_int32 && NeighbourX <= MAX_int32
						&& NeighbourY >= MIN_int32 && NeighbourY <= MAX_int32
						&& Occupied.Contains(FIntPoint(static_cast<int32>(NeighbourX), static_cast<int32>(NeighbourY))))
					{
						return true;
					}
				}
			}
			return false;
		}

		for (const TPair<FIntPoint, int32>& Previous : Occupied)
		{
			const int64 Required = FMath::Max(Spacing, Previous.Value);
			if (FMath::Abs(static_cast<int64>(Tile.X) - Previous.Key.X) <= Required
				&& FMath::Abs(static_cast<int64>(Tile.Y) - Previous.Key.Y) <= Required)
			{
				return true;
			}
		}
		return false;
	}
}

bool UPHDecorationPlanLibrary::BuildDecorationPlan(const FPHGeneratedLayout& Layout,
	const FPHBlockoutPlan& Blockout, const TArray<FPHPropRule>& Rules, const int32 DecorationSeed,
	FPHDecorationPlan& OutPlan, TArray<FPHGenerationIssue>& OutIssues)
{
	using namespace PHDecorationPlanPrivate;

	OutPlan = FPHDecorationPlan();
	OutIssues.Reset();

	for (int32 Index = 0; Index < Rules.Num(); ++Index)
	{
		const FPHPropRule& Rule = Rules[Index];
		if (Rule.PropTag == PHGenerationTags::Prop.GetTag() || !IsRegisteredPropTag(Rule.PropTag))
		{
			AddIssue(OutIssues, Index,
				FString::Printf(TEXT("Prop rule %d tag '%s' must be a registered descendant of Prop."),
					Index, *Rule.PropTag.ToString()));
		}

		if (Rule.MaxTotal < 0)
		{
			AddIssue(OutIssues, Index,
				FString::Printf(TEXT("Prop rule %d has a negative MaxTotal."), Index));
		}

		if (!FMath::IsFinite(Rule.ChancePerTile) || Rule.ChancePerTile < 0.0f || Rule.ChancePerTile > 1.0f
			|| !FMath::IsFinite(Rule.YawJitter) || Rule.YawJitter < 0.0f || Rule.YawJitter > 180.0f
			|| !FMath::IsFinite(Rule.CornerBias) || Rule.CornerBias < 0.0f || Rule.CornerBias > 4.0f
			|| !FMath::IsFinite(Rule.RegionDensityJitter) || Rule.RegionDensityJitter < 0.0f || Rule.RegionDensityJitter > 1.0f
			|| Rule.ClustersPerRegion < 0 || Rule.ClusterRadiusTiles < 1 || Rule.MinSpacingTiles < 0)
		{
			AddIssue(OutIssues, Index,
				FString::Printf(TEXT("Prop rule %d has nonfinite or out-of-range scalar settings."), Index));
		}
		if (Rule.Placement != EPHPropPlacement::AgainstWall && Rule.Placement != EPHPropPlacement::OpenFloor
			&& Rule.Placement != EPHPropPlacement::Anywhere)
		{
			AddIssue(OutIssues, Index, FString::Printf(TEXT("Prop rule %d has an unknown placement mode."), Index));
		}
	}

	if (!OutIssues.IsEmpty())
	{
		return false;
	}

	if (!FMath::IsFinite(Blockout.TileSize) || Blockout.TileSize <= 0.0)
	{
		AddIssue(OutIssues, INDEX_NONE, TEXT("The blockout carries no usable tile size."));
		return false;
	}

	if (Rules.IsEmpty() || Blockout.FloorTiles.IsEmpty())
	{
		return true;
	}
	if (Blockout.FloorTiles.Num() > MaxDecorationTiles || Layout.Regions.Num() > 4096
		|| static_cast<int64>(Blockout.FloorTiles.Num()) * Layout.Regions.Num() > MaxRegionTileChecks
		|| static_cast<int64>(Blockout.FloorTiles.Num()) * Rules.Num() > MaxRegionTileChecks)
	{
		AddIssue(OutIssues, INDEX_NONE, TEXT("The decoration plan exceeds its tile or rule/region work budget."));
		return false;
	}

	const double TileSize = Blockout.TileSize;
	auto CanConvertPosition = [TileSize](const FVector& Location)
	{
		const double X = Location.X / TileSize;
		const double Y = Location.Y / TileSize;
		return !Location.ContainsNaN() && FMath::IsFinite(X) && FMath::IsFinite(Y)
			&& X > static_cast<double>(MIN_int32) + 1.0 && Y > static_cast<double>(MIN_int32) + 1.0
			&& X < static_cast<double>(MAX_int32) - 1.0 && Y < static_cast<double>(MAX_int32) - 1.0;
	};
	if (!FMath::IsFinite(Layout.Bounds.Min.Z) || !CanConvertPosition(Blockout.PlayerStart.GetLocation())
		|| !CanConvertPosition(Blockout.Exit.GetLocation()))
	{
		AddIssue(OutIssues, INDEX_NONE, TEXT("Decoration endpoints and floor height must be finite and within tile coordinate range."));
		return false;
	}
	for (const FIntPoint& Tile : Blockout.FloorTiles)
	{
		if (Tile.X == MIN_int32 || Tile.X == MAX_int32 || Tile.Y == MIN_int32 || Tile.Y == MAX_int32
			|| !FMath::IsFinite(Tile.X * TileSize) || !FMath::IsFinite(Tile.Y * TileSize)
			|| !FMath::IsFinite((Tile.X + 1.0) * TileSize) || !FMath::IsFinite((Tile.Y + 1.0) * TileSize))
		{
			AddIssue(OutIssues, INDEX_NONE, TEXT("Decoration tiles need finite world coordinates and room for neighbouring-tile queries."));
			return false;
		}
	}
	const TSet<FIntPoint> FloorTiles(Blockout.FloorTiles);
	const TSet<FIntPoint> CorridorTiles(Blockout.CorridorTiles);

	auto ToTile = [TileSize](const FVector& Location)
	{
		return FIntPoint(FMath::FloorToInt32(Location.X / TileSize),
			FMath::FloorToInt32(Location.Y / TileSize));
	};

	// Endpoints and anchors are reserved before anything is scattered: a barrel on the player
	// start or buried in a chest anchor is worse than a slightly emptier room.
	TSet<FIntPoint> Reserved;
	Reserved.Add(ToTile(Blockout.PlayerStart.GetLocation()));
	Reserved.Add(ToTile(Blockout.Exit.GetLocation()));

	TSet<FIntPoint> AnchorTiles;
	for (const FPHGeneratedAnchor& Anchor : Layout.Anchors)
	{
		if (!CanConvertPosition(Anchor.Transform.GetLocation()))
		{
			AddIssue(OutIssues, INDEX_NONE, TEXT("A decoration anchor is outside finite tile coordinate range."));
			return false;
		}
		AnchorTiles.Add(ToTile(Anchor.Transform.GetLocation()));
	}

	// Tiles are grouped by the region that owns them, with corridor tiles in a bucket of their own,
	// so density and clustering are decided per room. A floor dressed as one undivided surface is
	// what makes every room carry the same average clutter.
	TMap<FIntPoint, int32> RegionOfTile;
	for (int32 Index = 0; Index < Layout.Regions.Num(); ++Index)
	{
		const FBox& Bounds = Layout.Regions[Index].Bounds;
		if (!Bounds.IsValid || !CanConvertPosition(Bounds.Min) || !CanConvertPosition(Bounds.Max)
			|| Bounds.Max.X < Bounds.Min.X || Bounds.Max.Y < Bounds.Min.Y)
		{
			AddIssue(OutIssues, Index, TEXT("A decoration region has invalid or out-of-range bounds."));
			return false;
		}
		const FIntPoint Min(FMath::RoundToInt32(Bounds.Min.X / TileSize),
			FMath::RoundToInt32(Bounds.Min.Y / TileSize));
		const FIntPoint Max(FMath::RoundToInt32(Bounds.Max.X / TileSize),
			FMath::RoundToInt32(Bounds.Max.Y / TileSize));

		// Only built tiles need owners; a large region envelope must not allocate an empty raster.
		for (const FIntPoint& Tile : Blockout.FloorTiles)
		{
			if (Tile.X >= Min.X && Tile.X < Max.X && Tile.Y >= Min.Y && Tile.Y < Max.Y)
			{
				RegionOfTile.Add(Tile, Index);
			}
		}
	}

	// Keep layout array order, then the corridor bucket, to preserve the draw sequence.
	TArray<TArray<FIntPoint>> Buckets;
	Buckets.SetNum(Layout.Regions.Num() + 1);
	for (const FIntPoint& Tile : Blockout.FloorTiles)
	{
		const int32* Owner = RegionOfTile.Find(Tile);
		Buckets[Owner ? *Owner : Layout.Regions.Num()].Add(Tile);
	}

	FRandomStream Stream(DecorationSeed);
	TMap<FIntPoint, int32> Occupied;
	int32 LargestPreviousSpacing = 0;
	int64 ClusterChecks = 0;
	const double FloorZ = Layout.Bounds.Min.Z;

	for (int32 RuleIndex = 0; RuleIndex < Rules.Num(); ++RuleIndex)
	{
		const FPHPropRule& Rule = Rules[RuleIndex];
		int32 Placed = 0;

		for (const TArray<FIntPoint>& Bucket : Buckets)
		{
			// Eligibility is settled before any draw so the number of rolls a bucket consumes does
			// not depend on what earlier rules happened to occupy.
			TArray<FIntPoint> Eligible;
			TArray<int32> OpenSideCount;
			TArray<FIntPoint> WallDirection;
			Eligible.Reserve(Bucket.Num());

			for (const FIntPoint& Tile : Bucket)
			{
				const FIntPoint Neighbours[] = {
					Tile + FIntPoint(1, 0), Tile + FIntPoint(-1, 0),
					Tile + FIntPoint(0, 1), Tile + FIntPoint(0, -1) };

				int32 OpenSides = 0;
				FIntPoint LastOpenSide(0, 0);
				for (const FIntPoint& Next : Neighbours)
				{
					if (!FloorTiles.Contains(Next))
					{
						++OpenSides;
						LastOpenSide = Next - Tile;
					}
				}

				const bool bEligiblePlacement =
					(Rule.Placement == EPHPropPlacement::Anywhere)
					|| (Rule.Placement == EPHPropPlacement::AgainstWall && OpenSides > 0)
					|| (Rule.Placement == EPHPropPlacement::OpenFloor && OpenSides == 0);

				if (!bEligiblePlacement
					|| Reserved.Contains(Tile)
					|| (Rule.bAvoidAnchors && AnchorTiles.Contains(Tile))
					|| (Rule.bAvoidCorridors && CorridorTiles.Contains(Tile)))
				{
					continue;
				}

				Eligible.Add(Tile);
				OpenSideCount.Add(OpenSides);
				WallDirection.Add(LastOpenSide);
			}

			if (Eligible.IsEmpty())
			{
				continue;
			}

			// One draw per bucket decides how dressed this room is, before any tile is considered.
			const float Jitter = FMath::Clamp(Rule.RegionDensityJitter, 0.0f, 1.0f);
			const float Density = Stream.FRandRange(1.0f - Jitter, 1.0f + Jitter);

			const int32 ClusterRadius = FMath::Max(1, Rule.ClusterRadiusTiles);

			// Clumps that cover the whole room are not clumps. A radius-1 clump already reaches nine
			// tiles and a room here is often sixteen, so the authored count is capped to leave most
			// of the room outside any clump; without it the rule blankets the room and reads exactly
			// like the even scatter it was meant to replace.
			const double ClusterDiameter = 2.0 * ClusterRadius + 1.0;
			const double ClusterFootprint = ClusterDiameter * ClusterDiameter;
			const int32 UsefulClusters = FMath::Max(1, FMath::FloorToInt32(Eligible.Num() / (2.0 * ClusterFootprint)));
			const int32 ClusterCount = FMath::Min(Rule.ClustersPerRegion, UsefulClusters);
			ClusterChecks += static_cast<int64>(Eligible.Num()) * ClusterCount;
			if (ClusterChecks > MaxClusterChecks)
			{
				OutPlan = FPHDecorationPlan();
				AddIssue(OutIssues, INDEX_NONE, TEXT("The decoration plan exceeds its cluster-distance-check budget."));
				return false;
			}

			// Clump centres are drawn from the eligible tiles themselves, so a clump always lands
			// somewhere the prop could actually go rather than in the middle of a wall.
			TArray<FIntPoint> ClusterCentres;
			for (int32 Cluster = 0; Cluster < ClusterCount; ++Cluster)
			{
				ClusterCentres.Add(Eligible[Stream.RandRange(0, Eligible.Num() - 1)]);
			}

			// Weights are built for the whole bucket before anything is rolled, so they can be
			// normalised back to the authored chance. Clustering has to decide *where* props go,
			// not how many there are: left unnormalised, a rule that concentrates weight into a few
			// tiles and thins the rest empties the floor, which is a worse fault than the even
			// sprinkle it was meant to fix.
			TArray<float> Weights;
			Weights.Reserve(Eligible.Num());
			double WeightTotal = 0.0;

			for (int32 Index = 0; Index < Eligible.Num(); ++Index)
			{
				const FIntPoint& Tile = Eligible[Index];
				float Weight = 1.0f;

				if (OpenSideCount[Index] >= 2)
				{
					Weight *= 1.0f + FMath::Max(0.0f, Rule.CornerBias);
				}

				if (!ClusterCentres.IsEmpty())
				{
					int64 Nearest = TNumericLimits<int64>::Max();
					for (const FIntPoint& Centre : ClusterCentres)
					{
						Nearest = FMath::Min(Nearest,
							FMath::Max(FMath::Abs(static_cast<int64>(Tile.X) - Centre.X),
								FMath::Abs(static_cast<int64>(Tile.Y) - Centre.Y)));
					}

					// Dense at the heart of a clump, thin but not empty away from it, so a room
					// still has a few strays rather than a hard edge around each pile.
					//
					// Divided by Radius + 1, not Radius. Dividing by the radius puts the stray
					// weight on the tiles *at* the radius, so a radius of one leaves a single
					// raised tile ringed by suppressed ones - which scatters props further apart
					// than no clustering at all. Measured: props touching a neighbour fell from
					// 48% to 39% against the even scatter it was supposed to beat.
					const float Falloff = FMath::Clamp(
						static_cast<float>(Nearest) / (static_cast<float>(ClusterRadius) + 1.0f), 0.0f, 1.0f);
					Weight *= FMath::Lerp(ClusterPeakWeight, ClusterStrayWeight, Falloff);
				}

				Weights.Add(Weight);
				WeightTotal += Weight;
			}

			// Mean weight of one keeps the expected count at ChancePerTile per eligible tile,
			// whatever shape the weighting took.
			const float Normalise = (WeightTotal > 0.0)
				? static_cast<float>(Eligible.Num() / WeightTotal) : 1.0f;

			for (int32 Index = 0; Index < Eligible.Num(); ++Index)
			{
				if (Rule.MaxTotal > 0 && Placed >= Rule.MaxTotal)
				{
					break;
				}

				const FIntPoint& Tile = Eligible[Index];

				// A roll is taken for every eligible tile, used or not, so occupancy left by
				// earlier rules cannot shift this rule's draw sequence.
				//
				// Capped below certainty on purpose. Normalising can push the heart of a clump past
				// a probability of one, and a tile that always fills turns a pile into a solid block
				// of identical meshes - which looks no more placed by hand than the even sprinkle.
				const float Weight = FMath::Min(MaxTileWeight,
					Rule.ChancePerTile * Density * Weights[Index] * Normalise);
				const bool bRolled = Stream.FRand() < Weight;
				const double Yaw = (Rule.YawJitter > 0.0f)
					? Stream.FRandRange(-Rule.YawJitter, Rule.YawJitter)
					: 0.0;

				if (!bRolled || Occupied.Contains(Tile))
				{
					continue;
				}

				if (IsCrowded(Tile, Occupied, Rule.MinSpacingTiles, LargestPreviousSpacing))
				{
					continue;
				}

				// Wall-hugging props sit toward the wall they were chosen for; everything else
				// centres on its tile.
				FVector Location(
					(Tile.X + 0.5) * TileSize,
					(Tile.Y + 0.5) * TileSize,
					FloorZ);

				if (Rule.Placement == EPHPropPlacement::AgainstWall)
				{
					Location.X += WallDirection[Index].X * TileSize * 0.3;
					Location.Y += WallDirection[Index].Y * TileSize * 0.3;
				}

				FPHPiecePlacement& Placement = OutPlan.Placements.AddDefaulted_GetRef();
				Placement.PieceTag = Rule.PropTag;
				Placement.Transform = FTransform(FRotator(0.0, Yaw, 0.0), Location);
				Placement.SourceRuleIndex = RuleIndex;

				Occupied.Add(Tile, Rule.MinSpacingTiles);
				LargestPreviousSpacing = FMath::Max(LargestPreviousSpacing, Rule.MinSpacingTiles);
				++Placed;
				++OutPlan.PropCount;
			}
		}
	}

	return true;
}

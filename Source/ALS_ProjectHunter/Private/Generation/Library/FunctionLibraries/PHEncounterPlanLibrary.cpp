#include "Generation/Library/FunctionLibraries/PHEncounterPlanLibrary.h"

#include "Generation/PHGenerationTags.h"

namespace PHEncounterPlanPrivate
{
	void AddIssue(TArray<FPHGenerationIssue>& Issues, const EPHGenerationIssueCode Code,
		const int32 ElementIndex, FString Message)
	{
		FPHGenerationIssue& Issue = Issues.AddDefaulted_GetRef();
		Issue.Code = Code;
		Issue.ElementIndex = ElementIndex;
		Issue.Message = MoveTemp(Message);
	}

	/** Enemy-bearing anchor kinds. Elite and Boss are separate roots, not Enemy descendants. */
	bool IsEnemyAnchor(const FGameplayTag& Tag)
	{
		return Tag.MatchesTag(PHGenerationTags::Anchor_Enemy)
			|| Tag.MatchesTag(PHGenerationTags::Anchor_Elite)
			|| Tag.MatchesTag(PHGenerationTags::Anchor_Boss);
	}
}

bool UPHEncounterPlanLibrary::BuildEncounterPlan(const FPHGeneratedLayout& Layout, const double Inset,
	FPHEncounterPlan& OutPlan, TArray<FPHGenerationIssue>& OutIssues)
{
	using namespace PHEncounterPlanPrivate;

	OutPlan = FPHEncounterPlan();
	OutIssues.Reset();

	if (!FMath::IsFinite(Inset) || Inset < 0.0)
	{
		AddIssue(OutIssues, EPHGenerationIssueCode::InvalidRequest, INDEX_NONE,
			FString::Printf(TEXT("Inset must be finite and nonnegative, got %f."), Inset));
		return false;
	}

	TMap<int32, int32> RegionIndexByID;
	RegionIndexByID.Reserve(Layout.Regions.Num());
	for (int32 Index = 0; Index < Layout.Regions.Num(); ++Index)
	{
		RegionIndexByID.Add(Layout.Regions[Index].RegionID, Index);
	}

	int32 StartRegionID = INDEX_NONE;
	if (const FPHGeneratedAnchor* Start = Layout.Anchors.FindByPredicate(
		[&Layout](const FPHGeneratedAnchor& Anchor)
		{
			return Anchor.AnchorID == Layout.PlayerStartAnchorID;
		}))
	{
		StartRegionID = Start->RegionID;
	}

	// Region order, not anchor order, so the plan is stable and readable.
	TMap<int32, int32> PlacementIndexByRegionID;

	for (const FPHGeneratedAnchor& Anchor : Layout.Anchors)
	{
		if (!IsEnemyAnchor(Anchor.SemanticTag))
		{
			continue;
		}

		const int32* RegionIndex = RegionIndexByID.Find(Anchor.RegionID);
		if (!RegionIndex)
		{
			AddIssue(OutIssues, EPHGenerationIssueCode::MissingAnchorRegion, INDEX_NONE,
				FString::Printf(TEXT("Enemy anchor %d references missing region %d."),
					Anchor.AnchorID, Anchor.RegionID));
			continue;
		}

		int32* Existing = PlacementIndexByRegionID.Find(Anchor.RegionID);
		if (!Existing)
		{
			const FPHGeneratedRegion& Region = Layout.Regions[*RegionIndex];

			FPHEncounterPlacement& Placement = OutPlan.Placements.AddDefaulted_GetRef();
			Placement.RegionID = Region.RegionID;
			Placement.bIsStartRegion = (Region.RegionID == StartRegionID);

			// Inset only on XY: the vertical span is the room's own height, and shrinking it
			// would push candidates off the floor the encounter owner projects onto.
			FBox Bounds = Region.Bounds;
			const FVector Size = Bounds.GetSize();
			if (Size.X > Inset * 2.0 && Size.Y > Inset * 2.0)
			{
				Bounds.Min.X += Inset;
				Bounds.Min.Y += Inset;
				Bounds.Max.X -= Inset;
				Bounds.Max.Y -= Inset;
			}
			Placement.SpawnBounds = Bounds;

			PlacementIndexByRegionID.Add(Region.RegionID, OutPlan.Placements.Num() - 1);
			Existing = PlacementIndexByRegionID.Find(Region.RegionID);
		}

		FPHEncounterPlacement& Placement = OutPlan.Placements[*Existing];
		++Placement.EnemyCount;
		Placement.EnemyKinds.AddTag(Anchor.SemanticTag);
		++OutPlan.TotalEnemyCount;
	}

	return OutIssues.IsEmpty();
}

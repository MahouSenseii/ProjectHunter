#include "Generation/Library/FunctionLibraries/PHGenerationValidationLibrary.h"

#include "GameplayTagsManager.h"
#include "Generation/PHGenerationTags.h"

namespace PHGenerationValidationPrivate
{
	void AddIssue(TArray<FPHGenerationIssue>& Issues, const EPHGenerationIssueCode Code,
		const int32 ElementIndex, FString Message)
	{
		FPHGenerationIssue& Issue = Issues.AddDefaulted_GetRef();
		Issue.Code = Code;
		Issue.ElementIndex = ElementIndex;
		Issue.Message = MoveTemp(Message);
	}

	bool IsUsableBounds(const FBox& Bounds)
	{
		// FBox::IsValid alone does not reject inverted or non-finite bounds.
		return Bounds.IsValid && !Bounds.ContainsNaN()
			&& Bounds.Min.X <= Bounds.Max.X
			&& Bounds.Min.Y <= Bounds.Max.Y
			&& Bounds.Min.Z <= Bounds.Max.Z
			&& (Bounds.Min.X < Bounds.Max.X || Bounds.Min.Y < Bounds.Max.Y || Bounds.Min.Z < Bounds.Max.Z);
	}

	bool MatchesRegisteredTag(const FGameplayTag& Tag, const FGameplayTag& ParentTag)
	{
		// A retained, removed tag can be nonempty; MatchesTag would ensure on it.
		const TSharedPtr<FGameplayTagNode> TagNode = UGameplayTagsManager::Get().FindTagNode(Tag);
		return TagNode.IsValid() && TagNode->GetCompleteTag() == Tag
			&& TagNode->GetSingleTagContainer().HasTag(ParentTag);
	}

	void RegisterID(const int32 ID, const int32 ElementIndex,
		const EPHGenerationIssueCode InvalidCode, const EPHGenerationIssueCode DuplicateCode,
		TMap<int32, int32>& IndexByID, TArray<FPHGenerationIssue>& Issues)
	{
		if (ID < 0)
		{
			AddIssue(Issues, InvalidCode, ElementIndex,
				FString::Printf(TEXT("ID %d must be nonnegative."), ID));
		}
		else if (IndexByID.Contains(ID))
		{
			AddIssue(Issues, DuplicateCode, ElementIndex,
				FString::Printf(TEXT("ID %d is already used in this collection."), ID));
		}
		else
		{
			IndexByID.Add(ID, ElementIndex);
		}
	}
}

bool UPHGenerationValidationLibrary::ValidateLayout(
	const FPHGeneratedLayout& Layout, TArray<FPHGenerationIssue>& OutIssues)
{
	using namespace PHGenerationValidationPrivate;
	OutIssues.Reset();

	if (Layout.GenerationVersion <= 0)
	{
		AddIssue(OutIssues, EPHGenerationIssueCode::InvalidGenerationVersion, INDEX_NONE,
			TEXT("GenerationVersion must be positive."));
	}

	const bool bHasValidLayoutBounds = IsUsableBounds(Layout.Bounds);
	if (!bHasValidLayoutBounds)
	{
		AddIssue(OutIssues, EPHGenerationIssueCode::InvalidLayoutBounds, INDEX_NONE,
			TEXT("Layout bounds must be initialized, finite, ordered, and larger than a point."));
	}

	if (Layout.Regions.IsEmpty())
	{
		AddIssue(OutIssues, EPHGenerationIssueCode::EmptyRegions, INDEX_NONE,
			TEXT("A layout must contain at least one traversable region."));
	}

	TMap<int32, int32> RegionIndexByID;
	RegionIndexByID.Reserve(Layout.Regions.Num());
	for (int32 Index = 0; Index < Layout.Regions.Num(); ++Index)
	{
		const FPHGeneratedRegion& Region = Layout.Regions[Index];
		RegisterID(Region.RegionID, Index, EPHGenerationIssueCode::InvalidRegionID,
			EPHGenerationIssueCode::DuplicateRegionID, RegionIndexByID, OutIssues);

		if (!IsUsableBounds(Region.Bounds))
		{
			AddIssue(OutIssues, EPHGenerationIssueCode::InvalidRegionBounds, Index,
				TEXT("Region bounds must be initialized, finite, ordered, and larger than a point."));
		}
		else if (bHasValidLayoutBounds && !Layout.Bounds.IsInsideOrOn(Region.Bounds))
		{
			AddIssue(OutIssues, EPHGenerationIssueCode::RegionOutsideLayout, Index,
				FString::Printf(TEXT("Region %d lies outside the layout bounds."), Region.RegionID));
		}
	}

	TMap<int32, int32> ConnectionIndexByID;
	ConnectionIndexByID.Reserve(Layout.Connections.Num());
	for (int32 Index = 0; Index < Layout.Connections.Num(); ++Index)
	{
		const FPHGeneratedConnection& Connection = Layout.Connections[Index];
		RegisterID(Connection.ConnectionID, Index, EPHGenerationIssueCode::InvalidConnectionID,
			EPHGenerationIssueCode::DuplicateConnectionID, ConnectionIndexByID, OutIssues);

		if (!RegionIndexByID.Contains(Connection.FromRegionID) || !RegionIndexByID.Contains(Connection.ToRegionID))
		{
			AddIssue(OutIssues, EPHGenerationIssueCode::MissingConnectionRegion, Index,
				FString::Printf(TEXT("Connection %d references a missing region (%d -> %d)."),
					Connection.ConnectionID, Connection.FromRegionID, Connection.ToRegionID));
		}
		else if (Connection.FromRegionID == Connection.ToRegionID)
		{
			AddIssue(OutIssues, EPHGenerationIssueCode::SelfConnection, Index,
				FString::Printf(TEXT("Connection %d must join two different regions."), Connection.ConnectionID));
		}
	}

	TMap<int32, int32> AnchorIndexByID;
	AnchorIndexByID.Reserve(Layout.Anchors.Num());
	for (int32 Index = 0; Index < Layout.Anchors.Num(); ++Index)
	{
		const FPHGeneratedAnchor& Anchor = Layout.Anchors[Index];
		RegisterID(Anchor.AnchorID, Index, EPHGenerationIssueCode::InvalidAnchorID,
			EPHGenerationIssueCode::DuplicateAnchorID, AnchorIndexByID, OutIssues);

		const int32* RegionIndex = RegionIndexByID.Find(Anchor.RegionID);
		if (!RegionIndex)
		{
			AddIssue(OutIssues, EPHGenerationIssueCode::MissingAnchorRegion, Index,
				FString::Printf(TEXT("Anchor %d references missing region %d."), Anchor.AnchorID, Anchor.RegionID));
		}

		const bool bHasValidTransform = Anchor.Transform.IsValid()
			&& Anchor.Transform.GetScale3D().Equals(FVector::OneVector);
		if (!bHasValidTransform)
		{
			AddIssue(OutIssues, EPHGenerationIssueCode::InvalidAnchorTransform, Index,
				TEXT("Anchor pose must be finite with normalized rotation and unit scale."));
		}
		else if (RegionIndex && IsUsableBounds(Layout.Regions[*RegionIndex].Bounds)
			&& !Layout.Regions[*RegionIndex].Bounds.IsInsideOrOn(Anchor.Transform.GetLocation()))
		{
			AddIssue(OutIssues, EPHGenerationIssueCode::AnchorOutsideRegion, Index,
				FString::Printf(TEXT("Anchor %d lies outside its region %d."), Anchor.AnchorID, Anchor.RegionID));
		}

		if (Anchor.SemanticTag == PHGenerationTags::Anchor.GetTag()
			|| !MatchesRegisteredTag(Anchor.SemanticTag, PHGenerationTags::Anchor))
		{
			AddIssue(OutIssues, EPHGenerationIssueCode::InvalidAnchorTag, Index,
				TEXT("Anchor semantic tag must be a registered descendant of Anchor."));
		}
	}

	auto ValidateEndpoint = [&](const int32 AnchorID, const FGameplayTag ExpectedTag,
		const EPHGenerationIssueCode MissingCode, const EPHGenerationIssueCode TagCode,
		const TCHAR* EndpointName) -> int32
	{
		const int32* AnchorIndex = AnchorIndexByID.Find(AnchorID);
		if (!AnchorIndex)
		{
			AddIssue(OutIssues, MissingCode, INDEX_NONE,
				FString::Printf(TEXT("%s references missing anchor %d."), EndpointName, AnchorID));
			return INDEX_NONE;
		}

		if (!MatchesRegisteredTag(Layout.Anchors[*AnchorIndex].SemanticTag, ExpectedTag))
		{
			AddIssue(OutIssues, TagCode, *AnchorIndex,
				FString::Printf(TEXT("%s anchor %d must match %s."), EndpointName, AnchorID, *ExpectedTag.ToString()));
		}
		return *AnchorIndex;
	};

	const int32 StartAnchorIndex = ValidateEndpoint(Layout.PlayerStartAnchorID, PHGenerationTags::Anchor_PlayerStart,
		EPHGenerationIssueCode::MissingPlayerStart, EPHGenerationIssueCode::InvalidPlayerStartTag, TEXT("Player start"));
	const int32 ExitAnchorIndex = ValidateEndpoint(Layout.ExitAnchorID, PHGenerationTags::Anchor_Exit,
		EPHGenerationIssueCode::MissingExit, EPHGenerationIssueCode::InvalidExitTag, TEXT("Exit"));

	// Invalid references and duplicate IDs cannot form an unambiguous traversal graph.
	if (!OutIssues.IsEmpty())
	{
		return false;
	}

	TArray<TArray<int32>> OutgoingRegions;
	OutgoingRegions.SetNum(Layout.Regions.Num());
	for (const FPHGeneratedConnection& Connection : Layout.Connections)
	{
		const int32 FromIndex = RegionIndexByID.FindChecked(Connection.FromRegionID);
		const int32 ToIndex = RegionIndexByID.FindChecked(Connection.ToRegionID);
		OutgoingRegions[FromIndex].Add(ToIndex);
		if (Connection.bBidirectional)
		{
			OutgoingRegions[ToIndex].Add(FromIndex);
		}
	}

	const int32 StartRegionIndex = RegionIndexByID.FindChecked(Layout.Anchors[StartAnchorIndex].RegionID);
	const int32 ExitRegionIndex = RegionIndexByID.FindChecked(Layout.Anchors[ExitAnchorIndex].RegionID);
	TArray<bool> ReachableRegions;
	ReachableRegions.Init(false, Layout.Regions.Num());
	TArray<int32> PendingRegions;
	PendingRegions.Reserve(Layout.Regions.Num());
	PendingRegions.Add(StartRegionIndex);
	ReachableRegions[StartRegionIndex] = true;

	for (int32 PendingIndex = 0; PendingIndex < PendingRegions.Num(); ++PendingIndex)
	{
		for (const int32 NextRegionIndex : OutgoingRegions[PendingRegions[PendingIndex]])
		{
			if (!ReachableRegions[NextRegionIndex])
			{
				ReachableRegions[NextRegionIndex] = true;
				PendingRegions.Add(NextRegionIndex);
			}
		}
	}

	for (int32 Index = 0; Index < Layout.Regions.Num(); ++Index)
	{
		if (!ReachableRegions[Index])
		{
			AddIssue(OutIssues, EPHGenerationIssueCode::UnreachableRegion, Index,
				FString::Printf(TEXT("Region %d is unreachable from the player start."), Layout.Regions[Index].RegionID));
		}
	}
	if (!ReachableRegions[ExitRegionIndex])
	{
		AddIssue(OutIssues, EPHGenerationIssueCode::UnreachableExit, INDEX_NONE,
			FString::Printf(TEXT("Exit anchor %d is unreachable from the player start."), Layout.ExitAnchorID));
	}

	return OutIssues.IsEmpty();
}

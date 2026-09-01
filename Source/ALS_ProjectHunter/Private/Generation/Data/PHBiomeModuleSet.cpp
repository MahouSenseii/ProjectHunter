#include "Generation/Data/PHBiomeModuleSet.h"

#include "GameplayTagsManager.h"
#include "Generation/PHGenerationTags.h"

namespace PHBiomeModuleSetPrivate
{
	/**
	 * A set maps both construction pieces and decoration props: they differ in when they are
	 * placed, not in how an art pack resolves them.
	 * Registry membership is resolved before hierarchical matching; see REVIEW-PH-20260829-02.
	 */
	bool IsRegisteredContentTag(const FGameplayTag& Tag)
	{
		const TSharedPtr<FGameplayTagNode> Node = UGameplayTagsManager::Get().FindTagNode(Tag);
		if (!Node.IsValid() || Node->GetCompleteTag() != Tag)
		{
			return false;
		}

		const FGameplayTagContainer& Parents = Node->GetSingleTagContainer();
		return Parents.HasTag(PHGenerationTags::Piece) || Parents.HasTag(PHGenerationTags::Prop);
	}

	bool IsContentRoot(const FGameplayTag& Tag)
	{
		return Tag == PHGenerationTags::Piece.GetTag() || Tag == PHGenerationTags::Prop.GetTag();
	}

	bool IsWholeModule(const double Value, const double GridSize)
	{
		const double Modules = Value / GridSize;
		return FMath::IsFinite(Modules)
			&& FMath::IsNearlyEqual(Modules, FMath::RoundToDouble(Modules), UE_KINDA_SMALL_NUMBER);
	}

	void AddIssue(TArray<FPHGenerationIssue>& Issues, const int32 ElementIndex, FString Message)
	{
		FPHGenerationIssue& Issue = Issues.AddDefaulted_GetRef();
		Issue.Code = EPHGenerationIssueCode::InvalidModuleSet;
		Issue.ElementIndex = ElementIndex;
		Issue.Message = MoveTemp(Message);
	}
}

bool UPHBiomeModuleSet::ResolvePiece(const FGameplayTag PieceTag, FPHModuleEntry& OutEntry) const
{
	OutEntry = FPHModuleEntry();
	if (!PieceTag.IsValid())
	{
		return false;
	}

	if (const FPHModuleEntry* Exact = Modules.Find(PieceTag))
	{
		OutEntry = *Exact;
		return true;
	}

	// Walk toward the root so a kit can map Piece.Wall once instead of every variant.
	// RequestDirectParent is only meaningful for a registered tag.
	FGameplayTag Current = PieceTag;
	while (PHBiomeModuleSetPrivate::IsRegisteredContentTag(Current))
	{
		Current = Current.RequestDirectParent();
		if (!Current.IsValid())
		{
			break;
		}

		if (const FPHModuleEntry* Inherited = Modules.Find(Current))
		{
			OutEntry = *Inherited;
			return true;
		}
	}
	return false;
}

bool UPHBiomeModuleSet::ValidateModuleSet(TArray<FPHGenerationIssue>& OutIssues) const
{
	using namespace PHBiomeModuleSetPrivate;
	OutIssues.Reset();

	if (!FMath::IsFinite(GridSize) || GridSize <= 0.0)
	{
		AddIssue(OutIssues, INDEX_NONE,
			FString::Printf(TEXT("GridSize must be finite and positive, got %f."), GridSize));
		return false;
	}

	if (Modules.IsEmpty())
	{
		AddIssue(OutIssues, INDEX_NONE, TEXT("A module set must map at least one piece."));
	}

	int32 Index = 0;
	for (const TPair<FGameplayTag, FPHModuleEntry>& Pair : Modules)
	{
		const FString TagName = Pair.Key.ToString();
		const bool bRegisteredContentTag = IsRegisteredContentTag(Pair.Key);

		if (IsContentRoot(Pair.Key) || !bRegisteredContentTag)
		{
			AddIssue(OutIssues, Index,
				FString::Printf(TEXT("'%s' must be a registered descendant of Piece or Prop."),
					*TagName));
		}

		if (Pair.Value.Mesh.IsNull())
		{
			AddIssue(OutIssues, Index,
				FString::Printf(TEXT("Piece '%s' has no mesh assigned."), *TagName));
		}

		if (!FMath::IsFinite(Pair.Value.Footprint.X) || !FMath::IsFinite(Pair.Value.Footprint.Y)
			|| Pair.Value.Footprint.X <= 0.0 || Pair.Value.Footprint.Y <= 0.0)
		{
			AddIssue(OutIssues, Index,
				FString::Printf(TEXT("Piece '%s' needs a finite positive footprint, got %s."),
					*TagName, *Pair.Value.Footprint.ToString()));
		}
		else if (bRegisteredContentTag && Pair.Key.MatchesTag(PHGenerationTags::Piece)
			&& (!IsWholeModule(Pair.Value.Footprint.X, GridSize)
				|| !IsWholeModule(Pair.Value.Footprint.Y, GridSize)))
		{
			// Construction pieces tile; prop dimensions need not match the module grid.
			AddIssue(OutIssues, Index,
				FString::Printf(TEXT("Piece '%s' footprint %s is not whole %f-unit modules."),
					*TagName, *Pair.Value.Footprint.ToString(), GridSize));
		}

		if (!FMath::IsFinite(Pair.Value.Height) || Pair.Value.Height < 0.0)
		{
			AddIssue(OutIssues, Index,
				FString::Printf(TEXT("Piece '%s' needs a finite nonnegative height."), *TagName));
		}

		if (!FMath::IsFinite(Pair.Value.YawOffset))
		{
			AddIssue(OutIssues, Index,
				FString::Printf(TEXT("Piece '%s' needs a finite yaw offset."), *TagName));
		}

		++Index;
	}

	return OutIssues.IsEmpty();
}

bool UPHBiomeModuleSet::HasPieces(const TArray<FGameplayTag>& RequiredPieces,
	TArray<FGameplayTag>& OutMissing) const
{
	OutMissing.Reset();

	FPHModuleEntry Unused;
	for (const FGameplayTag& Required : RequiredPieces)
	{
		if (!ResolvePiece(Required, Unused))
		{
			OutMissing.Add(Required);
		}
	}
	return OutMissing.IsEmpty();
}

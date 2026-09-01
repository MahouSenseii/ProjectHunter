#include "Generation/Generators/PHLayoutGenerator.h"

#include "GameplayTagsManager.h"
#include "Generation/Library/FunctionLibraries/PHGenerationValidationLibrary.h"
#include "Generation/PHGenerationTags.h"

namespace PHLayoutGeneratorPrivate
{
	/** Check in floating point before narrowing; converting NaN or an out-of-range quotient is unsafe. */
	bool TryRoundModules(const double Size, const double GridSize, const bool bRoundUp, int32& OutModules)
	{
		OutModules = INDEX_NONE;
		if (!FMath::IsFinite(Size) || !FMath::IsFinite(GridSize) || GridSize <= 0.0)
		{
			return false;
		}

		const double Adjusted = Size / GridSize + (bRoundUp ? -UE_KINDA_SMALL_NUMBER : UE_KINDA_SMALL_NUMBER);
		if (!FMath::IsFinite(Adjusted))
		{
			return false;
		}

		const double Rounded = bRoundUp ? FMath::CeilToDouble(Adjusted) : FMath::FloorToDouble(Adjusted);
		if (Rounded < static_cast<double>(MIN_int32) || Rounded > static_cast<double>(MAX_int32))
		{
			return false;
		}

		OutModules = static_cast<int32>(Rounded);
		return true;
	}

	/**
	 * Registry membership is checked before hierarchical matching. A nonempty tag retained after
	 * its definition is removed passes IsValid but makes MatchesTag fire an engine ensure instead
	 * of returning an ordinary result; see REVIEW-PH-20260829-02.
	 */
	bool IsRegisteredAnchorTag(const FGameplayTag& Tag)
	{
		const TSharedPtr<FGameplayTagNode> Node = UGameplayTagsManager::Get().FindTagNode(Tag);
		return Node.IsValid() && Node->GetCompleteTag() == Tag
			&& Node->GetSingleTagContainer().HasTag(PHGenerationTags::Anchor);
	}
}

void UPHLayoutGenerator::AddIssue(TArray<FPHGenerationIssue>& Issues,
	const EPHGenerationIssueCode Code, const int32 ElementIndex, FString Message)
{
	FPHGenerationIssue& Issue = Issues.AddDefaulted_GetRef();
	Issue.Code = Code;
	Issue.ElementIndex = ElementIndex;
	Issue.Message = MoveTemp(Message);
}

int32 UPHLayoutGenerator::ModulesDown(const double Size, const double GridSize)
{
	int32 Modules;
	PHLayoutGeneratorPrivate::TryRoundModules(Size, GridSize, false, Modules);
	return Modules;
}

int32 UPHLayoutGenerator::ModulesUp(const double Size, const double GridSize)
{
	int32 Modules;
	PHLayoutGeneratorPrivate::TryRoundModules(Size, GridSize, true, Modules);
	return Modules;
}

bool UPHLayoutGenerator::ValidateRequest(const FPHLayoutRequest& Request,
	TArray<FPHGenerationIssue>& OutIssues)
{
	OutIssues.Reset();

	auto Refuse = [&OutIssues](FString Message)
	{
		AddIssue(OutIssues, EPHGenerationIssueCode::InvalidRequest, INDEX_NONE, MoveTemp(Message));
	};
	auto RequireFinite = [&Refuse](const TCHAR* Name, const double Value)
	{
		if (!FMath::IsFinite(Value))
		{
			Refuse(FString::Printf(TEXT("%s must be finite, got %g."), Name, Value));
		}
	};

	// Comparisons alone do not reject NaN. Refuse every non-finite input before module maths,
	// probability checks or height multiplication can turn it into malformed geometry.
	RequireFinite(TEXT("AreaSize.X"), Request.AreaSize.X);
	RequireFinite(TEXT("AreaSize.Y"), Request.AreaSize.Y);
	RequireFinite(TEXT("MinRegionSize.X"), Request.MinRegionSize.X);
	RequireFinite(TEXT("MinRegionSize.Y"), Request.MinRegionSize.Y);
	RequireFinite(TEXT("MaxRegionSize.X"), Request.MaxRegionSize.X);
	RequireFinite(TEXT("MaxRegionSize.Y"), Request.MaxRegionSize.Y);
	RequireFinite(TEXT("GridSize"), Request.GridSize);
	RequireFinite(TEXT("RegionHeight"), Request.RegionHeight);
	RequireFinite(TEXT("RegionSpacing"), Request.RegionSpacing);
	RequireFinite(TEXT("LoopChance"), Request.LoopChance);
	RequireFinite(TEXT("MaxLoopDistance"), Request.MaxLoopDistance);
	if (!OutIssues.IsEmpty())
	{
		return false;
	}

	if (Request.AreaSize.X <= 0.0 || Request.AreaSize.Y <= 0.0)
	{
		Refuse(FString::Printf(TEXT("AreaSize must be positive on both axes, got %s."),
			*Request.AreaSize.ToString()));
	}

	if (Request.RegionHeight <= 0.0)
	{
		Refuse(FString::Printf(TEXT("RegionHeight must be positive, got %f."), Request.RegionHeight));
	}

	if (Request.MaxHeightStacks < 1)
	{
		Refuse(FString::Printf(TEXT("MaxHeightStacks must be at least one, got %d."), Request.MaxHeightStacks));
	}

	if (Request.RegionPlacement != EPHRegionPlacement::Scatter
		&& Request.RegionPlacement != EPHRegionPlacement::Growth)
	{
		Refuse(FString::Printf(TEXT("RegionPlacement %d is not a supported enum value."),
			static_cast<int32>(Request.RegionPlacement)));
	}

	if (Request.RegionSpacing < 0.0)
	{
		Refuse(FString::Printf(TEXT("RegionSpacing cannot be negative, got %f."), Request.RegionSpacing));
	}

	if (Request.MinRegionCount < 1)
	{
		Refuse(FString::Printf(TEXT("MinRegionCount must be at least one, got %d."), Request.MinRegionCount));
	}

	if (Request.MaxRegionCount < Request.MinRegionCount)
	{
		Refuse(FString::Printf(TEXT("Region count range is inverted: %d..%d."),
			Request.MinRegionCount, Request.MaxRegionCount));
	}

	if (Request.MinRegionSize.X <= 0.0 || Request.MinRegionSize.Y <= 0.0)
	{
		Refuse(FString::Printf(TEXT("MinRegionSize must be positive on both axes, got %s."),
			*Request.MinRegionSize.ToString()));
	}

	if (Request.MaxRegionSize.X < Request.MinRegionSize.X
		|| Request.MaxRegionSize.Y < Request.MinRegionSize.Y)
	{
		Refuse(FString::Printf(TEXT("Region size range is inverted: %s..%s."),
			*Request.MinRegionSize.ToString(), *Request.MaxRegionSize.ToString()));
	}

	// Checked against the minimum: if the smallest permitted region cannot fit, no draw can.
	if (Request.MinRegionSize.X > Request.AreaSize.X || Request.MinRegionSize.Y > Request.AreaSize.Y)
	{
		Refuse(FString::Printf(TEXT("Smallest region %s does not fit inside area %s."),
			*Request.MinRegionSize.ToString(), *Request.AreaSize.ToString()));
	}

	if (Request.GridSize <= 0.0)
	{
		Refuse(FString::Printf(TEXT("GridSize must be positive, got %f."), Request.GridSize));
	}

	if (Request.LoopChance < 0.0f || Request.LoopChance > 1.0f)
	{
		Refuse(FString::Printf(TEXT("LoopChance must be a 0-1 probability, got %f."), Request.LoopChance));
	}

	if (Request.MaxPlacementAttempts < 1)
	{
		Refuse(FString::Printf(TEXT("MaxPlacementAttempts must be at least one, got %d."),
			Request.MaxPlacementAttempts));
	}

	if (Request.ExtraCorridorModules < 0)
	{
		Refuse(FString::Printf(TEXT("ExtraCorridorModules cannot be negative, got %d."),
			Request.ExtraCorridorModules));
	}

	if (Request.GrowthFrontier < 1)
	{
		Refuse(FString::Printf(TEXT("GrowthFrontier must be at least one, got %d."),
			Request.GrowthFrontier));
	}

	if (Request.MaxLoopDistance < 0.0)
	{
		Refuse(FString::Printf(TEXT("MaxLoopDistance cannot be negative, got %f."),
			Request.MaxLoopDistance));
	}

	for (int32 Index = 0; Index < Request.AnchorRules.Num(); ++Index)
	{
		const FPHAnchorRule& Rule = Request.AnchorRules[Index];

		if (Rule.SemanticTag == PHGenerationTags::Anchor.GetTag()
			|| !PHLayoutGeneratorPrivate::IsRegisteredAnchorTag(Rule.SemanticTag))
		{
			Refuse(FString::Printf(
				TEXT("Anchor rule %d tag '%s' must be a registered descendant of Anchor."),
				Index, *Rule.SemanticTag.ToString()));
		}

		if (Rule.MinPerRegion < 0 || Rule.MaxPerRegion < Rule.MinPerRegion)
		{
			Refuse(FString::Printf(TEXT("Anchor rule %d has an invalid count range %d..%d."),
				Index, Rule.MinPerRegion, Rule.MaxPerRegion));
		}
		else if (static_cast<int64>(Rule.MaxPerRegion) - Rule.MinPerRegion + 1 > MAX_int32)
		{
			// FRandomStream::RandRange forms an inclusive int32 span before it draws.
			Refuse(FString::Printf(TEXT("Anchor rule %d count range %d..%d exceeds a 32-bit random span."),
				Index, Rule.MinPerRegion, Rule.MaxPerRegion));
		}

		if (Rule.MaxTotal < 0)
		{
			Refuse(FString::Printf(TEXT("Anchor rule %d has a negative MaxTotal %d."),
				Index, Rule.MaxTotal));
		}
	}

	if (!OutIssues.IsEmpty())
	{
		return false;
	}

	// Sizes are honoured in whole modules. Check conversion bounds as well as ordering:
	// a finite positive range can exceed int32, or round to a zero-width footprint.
	auto RefuseAxis = [&Refuse, &Request](const TCHAR* Axis, const double Min,
		const double Max, const double Area)
	{
		const int32 MinModules = ModulesUp(Min, Request.GridSize);
		const int32 MaxModules = ModulesDown(Max, Request.GridSize);
		const int32 AreaModules = ModulesDown(Area, Request.GridSize);
		if (MinModules < 0 || MaxModules < 0 || AreaModules < 0)
		{
			Refuse(FString::Printf(TEXT("Sizes on %s exceed the supported 32-bit module count on grid %f."),
				Axis, Request.GridSize));
		}
		else if (MinModules < 1 || MaxModules < 1 || AreaModules < 1)
		{
			Refuse(FString::Printf(TEXT("A region and its area must span at least one whole module on %s."), Axis));
		}
		else if (MinModules > MaxModules)
		{
			Refuse(FString::Printf(TEXT("No whole %f-unit module count fits between region size %f and %f on %s."),
				Request.GridSize, Min, Max, Axis));
		}
		else if (MinModules > AreaModules)
		{
			Refuse(FString::Printf(TEXT("Smallest region needs %d modules on %s but the area holds only %d."),
				MinModules, Axis, AreaModules));
		}
	};
	RefuseAxis(TEXT("X"), Request.MinRegionSize.X, Request.MaxRegionSize.X, Request.AreaSize.X);
	RefuseAxis(TEXT("Y"), Request.MinRegionSize.Y, Request.MaxRegionSize.Y, Request.AreaSize.Y);

	const int32 HeightModules = ModulesDown(Request.RegionHeight, Request.GridSize);
	if (HeightModules < 1)
	{
		Refuse(FString::Printf(TEXT("RegionHeight %f has no representable positive module count on grid %f."),
			Request.RegionHeight, Request.GridSize));
	}
	else if (!FMath::IsFinite(Request.RegionHeight * Request.MaxHeightStacks)
		|| !FMath::IsFinite(HeightModules * Request.GridSize * Request.MaxHeightStacks))
	{
		Refuse(TEXT("The maximum stacked region height must remain finite in world units."));
	}

	if (ModulesUp(Request.RegionSpacing, Request.GridSize) < 0)
	{
		Refuse(TEXT("RegionSpacing exceeds the supported 32-bit module count."));
	}

	return OutIssues.IsEmpty();
}

bool UPHLayoutGenerator::ValidateStrategyConstraints(const FPHLayoutRequest&,
	const FPHGeneratedLayout&, TArray<FPHGenerationIssue>&) const
{
	return true;
}

bool UPHLayoutGenerator::GenerateLayout(const FPHLayoutRequest& Request,
	FPHGeneratedLayout& OutLayout, TArray<FPHGenerationIssue>& OutIssues)
{
	OutLayout = FPHGeneratedLayout();

	if (!ValidateRequest(Request, OutIssues))
	{
		return false;
	}

	FRandomStream Stream(Request.Seed);
	FPHGeneratedLayout Built;
	if (!BuildLayout(Request, Stream, Built, OutIssues))
	{
		if (OutIssues.IsEmpty())
		{
			AddIssue(OutIssues, EPHGenerationIssueCode::EmptyRegions, INDEX_NONE,
				FString::Printf(TEXT("%s could not build a layout for seed %d."),
					*GetClass()->GetName(), Request.Seed));
		}
		return false;
	}

	// Stamped here so a strategy cannot forget its own provenance.
	Built.Seed = Request.Seed;
	Built.GenerationVersion = GenerationVersion;
	Built.Tags.AppendTags(Request.Tags);

	// ValidateLayout replaces the array it is given, so structural issues are collected
	// separately and appended rather than overwriting anything already reported.
	TArray<FPHGenerationIssue> StructuralIssues;
	if (!UPHGenerationValidationLibrary::ValidateLayout(Built, StructuralIssues))
	{
		OutIssues.Append(StructuralIssues);
		return false;
	}

	if (!ValidateStrategyConstraints(Request, Built, OutIssues))
	{
		return false;
	}

	OutLayout = MoveTemp(Built);
	return true;
}

#include "Character/Components/Interaction/InteractionDebugManager.h"
#include "Character/Components/Interaction/InteractionTraceManager.h"
#include "Interactable/Components/InteractableManager.h"
#include "Components/ALSDebugComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
DEFINE_LOG_CATEGORY(LogInteractionDebugManager);

FInteractionDebugManager::FInteractionDebugManager()
	: DebugMode(EInteractionDebugMode::None)
	, bDrawTraceLines(true)
	, bDrawHitPoints(true)
	, bDrawInteractionRange(true)
	, bDrawGroundItems(true)
	, bShowDebugText(true)
	, TraceHitColor(FColor::Green)
	, TraceMissColor(FColor::Red)
	, InteractableColor(FColor::Cyan)
	, GroundItemColor(FColor::Yellow)
	, DrawDuration(0.0f)
	, DrawThickness(2.0f)
	, OwnerActor(nullptr)
	, WorldContext(nullptr)
	, CachedALSDebugComponent(nullptr)
	, TotalInteractions(0)
	, SuccessfulInteractions(0)
	, FailedInteractions(0)
	, TotalGroundItemsPickedUp(0)
	, AverageTraceTime(0.0f)
	, AverageValidationTime(0.0f)
{
}

void FInteractionDebugManager::Initialize(AActor* Owner, UWorld* World)
{
	OwnerActor = Owner;
	WorldContext = World;

	if (!OwnerActor || !WorldContext)
	{
		UE_LOG(LogInteractionDebugManager, Error, TEXT("InteractionDebugManager: Invalid initialization parameters"));
		return;
	}

	CachedALSDebugComponent = OwnerActor->FindComponentByClass<UALSDebugComponent>();
	if (CachedALSDebugComponent)
	{
		UE_LOG(LogInteractionDebugManager, Log, TEXT("InteractionDebugManager:  ALS Debug Component found - Using ALS debug toggle"));
	}
	else
	{
		UE_LOG(LogInteractionDebugManager, Log, TEXT("InteractionDebugManager: ALS Debug Component not found - Using manual toggle"));
	}

	UE_LOG(LogInteractionDebugManager, Log, TEXT("InteractionDebugManager: Initialized for %s"), *OwnerActor->GetName());
}

void FInteractionDebugManager::DrawTraceLine(FVector Start, FVector End, bool bHit)
{
	if (!ShouldShowDebugTraces() || !bDrawTraceLines || !WorldContext)
	{
		return;
	}

	FColor LineColor = bHit ? TraceHitColor : TraceMissColor;

	DrawDebugLine(
		WorldContext,
		Start,
		End,
		LineColor,
		false,
		DrawDuration,
		0,
		DrawThickness
	);
}

void FInteractionDebugManager::DrawTraceResult(
	FVector Start, FVector End, const FHitResult& HitResult, bool bHit, float TraceRadius)
{
	if (!ShouldShowDebugTraces() || !bDrawTraceLines || !WorldContext)
	{
		return;
	}

	if (!bHit)
	{
		DrawTraceLine(Start, End, false);
		return;
	}

	const FVector HitLocation = !HitResult.Location.IsNearlyZero()
		? HitResult.Location
		: HitResult.ImpactPoint;
	const FVector SafeHitLocation = HitLocation.IsNearlyZero() ? End : HitLocation;

	DrawDebugLine(
		WorldContext,
		Start,
		SafeHitLocation,
		TraceHitColor,
		false,
		DrawDuration,
		0,
		DrawThickness);

	DrawDebugLine(
		WorldContext,
		SafeHitLocation,
		End,
		FColor(70, 70, 70),
		false,
		DrawDuration,
		0,
		DrawThickness * 0.5f);
}

void FInteractionDebugManager::DrawHitPoint(FVector HitLocation, FVector HitNormal, float Radius)
{
	if (!ShouldShowDebugTraces() || !bDrawHitPoints || !WorldContext)
	{
		return;
	}

	const float DebugRadius = FMath::Clamp(Radius, 8.0f, 50.0f);

	DrawDebugSphere(
		WorldContext,
		HitLocation,
		DebugRadius,
		8,
		TraceHitColor,
		false,
		DrawDuration,
		0,
		DrawThickness
	);

	if (DebugMode == EInteractionDebugMode::Detailed || DebugMode == EInteractionDebugMode::Full)
	{
		DrawDebugDirectionalArrow(
			WorldContext,
			HitLocation,
			HitLocation + (HitNormal * 50.0f),
			10.0f,
			FColor::White,
			false,
			DrawDuration,
			0,
			DrawThickness
		);
	}
}

void FInteractionDebugManager::DrawInteractionRange(FVector Center, float Radius)
{
	if (!ShouldShowDebugTraces() || !bDrawInteractionRange || !WorldContext)
	{
		return;
	}

	DrawDebugSphere(
		WorldContext,
		Center,
		Radius,
		16,
		InteractableColor,
		false,
		DrawDuration,
		0,
		DrawThickness * 0.5f
	);
}

void FInteractionDebugManager::DrawLookAtCone(FVector Origin, FVector Forward, float MinDot, float Length)
{
	if (!ShouldShowDebugTraces() || !bDrawLookAtCone || !WorldContext)
	{
		return;
	}

	const float HalfAngleRad = FMath::Acos(FMath::Clamp(MinDot, -1.0f, 1.0f));

	DrawDebugCone(
		WorldContext,
		Origin,
		Forward,
		Length,
		HalfAngleRad,
		HalfAngleRad,
		16,
		FColor(0, 160, 255),
		false,
		DrawDuration,
		0,
		DrawThickness * 0.5f
	);
}

void FInteractionDebugManager::DrawPlayerForwardGate(FVector Origin, FVector Forward, float MinDot, float Length)
{
	if (!ShouldShowDebugTraces() || !bDrawLookAtCone || !WorldContext)
	{
		return;
	}

	const FVector SafeForward = Forward.GetSafeNormal();
	if (SafeForward.IsNearlyZero())
	{
		return;
	}

	const float HalfAngleRad = FMath::Acos(FMath::Clamp(MinDot, -1.0f, 1.0f));

	DrawDebugCone(
		WorldContext,
		Origin,
		SafeForward,
		Length,
		HalfAngleRad,
		HalfAngleRad,
		16,
		FColor(0, 255, 120),
		false,
		DrawDuration,
		0,
		DrawThickness * 0.5f
	);
}

void FInteractionDebugManager::DrawAimCandidate(FVector Location, float Dot, bool bPassedGate, bool bWinner)
{
	if (!ShouldShowDebugTraces() || !bDrawAimCandidates || !WorldContext)
	{
		return;
	}

	const FColor CandidateColor = bWinner
		? TraceHitColor                       // green: took focus
		: (bPassedGate ? FColor::Yellow       // in the cone, lost on dot
		               : FColor::Orange);     // failed the gate

	DrawDebugSphere(
		WorldContext,
		Location,
		bWinner ? 30.0f : 18.0f,
		8,
		CandidateColor,
		false,
		DrawDuration,
		0,
		DrawThickness
	);

	if (DebugMode == EInteractionDebugMode::Detailed || DebugMode == EInteractionDebugMode::Full)
	{
		DrawDebugString(
			WorldContext,
			Location + FVector(0, 0, 40.0f),
			FString::Printf(TEXT("dot %.3f%s"), Dot, bWinner ? TEXT(" ") : TEXT("")),
			nullptr,
			CandidateColor,
			DrawDuration <= 0.0f ? 0.05f : DrawDuration,
			true
		);
	}
}

void FInteractionDebugManager::DrawGroundItemSearchVolume(FVector Center, float Radius)
{
	if (!ShouldShowDebugTraces() || !bDrawGroundItemAimWindow || !WorldContext || Radius <= 0.0f)
	{
		return;
	}

	const FColor WindowColor(210, 210, 210);

	DrawDebugSphere(
		WorldContext,
		Center,
		Radius,
		16,
		WindowColor,
		false,
		DrawDuration,
		0,
		DrawThickness * 0.35f
	);

	if (DebugMode == EInteractionDebugMode::Detailed || DebugMode == EInteractionDebugMode::Full)
	{
		DrawDebugString(
			WorldContext,
			Center + FVector(0, 0, Radius + 25.0f),
			FString::Printf(
				TEXT("PLAYER INTERACTION RADIUS %.0f cm"),
				Radius),
			nullptr,
			WindowColor,
			DrawDuration <= 0.0f ? 0.05f : DrawDuration,
			true
		);
	}
}

void FInteractionDebugManager::DrawSelectionDirections(
	FVector PlayerCenter,
	FVector PlayerForward,
	FVector CameraOrigin,
	FVector CameraForward,
	float Length)
{
	if (!ShouldShowDebugTraces() || !WorldContext || Length <= 0.0f)
	{
		return;
	}

	const FVector SafePlayerForward = PlayerForward.GetSafeNormal();
	const FVector SafeCameraForward = CameraForward.GetSafeNormal();
	const FColor PlayerColor(80, 255, 120);
	const FColor CameraColor(60, 160, 255);

	DrawDebugDirectionalArrow(
		WorldContext,
		PlayerCenter,
		PlayerCenter + SafePlayerForward * Length,
		18.0f,
		PlayerColor,
		false,
		DrawDuration,
		0,
		DrawThickness);

	DrawDebugDirectionalArrow(
		WorldContext,
		CameraOrigin,
		CameraOrigin + SafeCameraForward * Length,
		18.0f,
		CameraColor,
		false,
		DrawDuration,
		0,
		DrawThickness);

	if (DebugMode == EInteractionDebugMode::Detailed || DebugMode == EInteractionDebugMode::Full)
	{
		DrawDebugString(
			WorldContext,
			PlayerCenter + SafePlayerForward * Length,
			TEXT("PLAYER FORWARD"),
			nullptr,
			PlayerColor,
			DrawDuration,
			true);
		DrawDebugString(
			WorldContext,
			CameraOrigin + SafeCameraForward * Length,
			TEXT("CAMERA FORWARD"),
			nullptr,
			CameraColor,
			DrawDuration,
			true);
	}
}

void FInteractionDebugManager::DrawRejectedGroundItemCandidate(
	FVector Location, int32 ItemID, float PlayerForwardDot)
{
	if (!ShouldShowDebugTraces() || !bDrawAimCandidates || !WorldContext)
	{
		return;
	}

	DrawDebugSphere(
		WorldContext,
		Location,
		14.0f,
		8,
		RejectedCandidateColor,
		false,
		DrawDuration,
		0,
		DrawThickness * 0.75f);

	if (DebugMode == EInteractionDebugMode::Detailed || DebugMode == EInteractionDebugMode::Full)
	{
		DrawDebugString(
			WorldContext,
			Location + FVector(0, 0, 35.0f),
			FString::Printf(
				TEXT("REJECTED ID %d | player dot %.3f"),
				ItemID,
				PlayerForwardDot),
			nullptr,
			RejectedCandidateColor,
			DrawDuration,
			true);
	}
}

void FInteractionDebugManager::DrawGroundItemCandidateStack(
	FVector TraceOrigin,
	const TArray<FGroundItemInteractionCandidate>& Candidates,
	int32 SelectedItemID,
	int32 AutomaticItemID,
	bool bManualSelectionLocked,
	float ManualLockRemaining)
{
	if (!ShouldShowDebugTraces() || !bDrawGroundItemCandidateStack || !WorldContext)
	{
		return;
	}

	const FGroundItemInteractionCandidate* SelectedCandidate = Candidates.FindByPredicate(
		[SelectedItemID](const FGroundItemInteractionCandidate& Candidate)
		{
			return Candidate.ItemID == SelectedItemID;
		});
	if (!SelectedCandidate)
	{
		return;
	}

	const FGroundItemInteractionCandidate* RunnerUpCandidate = Candidates.FindByPredicate(
		[SelectedItemID, AutomaticItemID](const FGroundItemInteractionCandidate& Candidate)
		{
			return Candidate.ItemID == AutomaticItemID && Candidate.ItemID != SelectedItemID;
		});
	if (!RunnerUpCandidate)
	{
		RunnerUpCandidate = Candidates.FindByPredicate(
			[SelectedItemID](const FGroundItemInteractionCandidate& Candidate)
			{
				return Candidate.ItemID != SelectedItemID;
			});
	}

	const FColor SelectedColor = bManualSelectionLocked ? ManualCandidateColor : TraceHitColor;
	DrawDebugSphere(
		WorldContext,
		SelectedCandidate->WorldLocation,
		30.0f,
		10,
		SelectedColor,
		false,
		DrawDuration,
		0,
		DrawThickness);
	DrawDebugLine(
		WorldContext,
		TraceOrigin,
		SelectedCandidate->WorldLocation,
		SelectedColor,
		false,
		DrawDuration,
		0,
		DrawThickness);

	if (RunnerUpCandidate)
	{
		const FColor RunnerColor(120, 120, 120);
		DrawDebugSphere(
			WorldContext,
			RunnerUpCandidate->WorldLocation,
			16.0f,
			8,
			RunnerColor,
			false,
			DrawDuration,
			0,
			DrawThickness * 0.5f);
		DrawDebugLine(
			WorldContext,
			TraceOrigin,
			RunnerUpCandidate->WorldLocation,
			RunnerColor,
			false,
			DrawDuration,
			0,
			DrawThickness * 0.35f);
	}

	if (DebugMode == EInteractionDebugMode::Detailed || DebugMode == EInteractionDebugMode::Full)
	{
		const FString SelectionState = bManualSelectionLocked
			? FString::Printf(TEXT("MANUAL %.2fs"), ManualLockRemaining)
			: TEXT("AUTOMATIC");
		DrawDebugString(
			WorldContext,
			SelectedCandidate->WorldLocation + FVector(0, 0, 55.0f),
			FString::Printf(
				TEXT("SELECTED ID %d | %s\nscore %.3f | camera %.3f, player gate %.3f, range %.0f cm (gate only), focus +%.3f"),
				SelectedCandidate->ItemID,
				*SelectionState,
				SelectedCandidate->Score,
				SelectedCandidate->CameraForwardDot,
				SelectedCandidate->PlayerForwardDot,
				SelectedCandidate->Distance,
				SelectedCandidate->FocusBonus),
			nullptr,
			SelectedColor,
			DrawDuration,
			true);

		if (RunnerUpCandidate)
		{
			DrawDebugString(
				WorldContext,
				RunnerUpCandidate->WorldLocation + FVector(0, 0, 35.0f),
				FString::Printf(
					TEXT("RUNNER-UP ID %d | score %.3f"),
					RunnerUpCandidate->ItemID,
					RunnerUpCandidate->Score),
				nullptr,
				FColor(160, 160, 160),
				DrawDuration,
				true);
		}
	}
}

void FInteractionDebugManager::DrawGroundItem(FVector ItemLocation, int32 ItemID)
{
	if (!ShouldShowDebugTraces() || !bDrawGroundItems || !WorldContext)
	{
		return;
	}

	DrawDebugCylinder(
		WorldContext,
		ItemLocation,
		ItemLocation + FVector(0, 0, 100),
		20.0f,
		12,
		GroundItemColor,
		false,
		DrawDuration,
		0,
		DrawThickness
	);

	if (DebugMode == EInteractionDebugMode::Detailed || DebugMode == EInteractionDebugMode::Full)
	{
		DrawDebugString(
			WorldContext,
			ItemLocation + FVector(0, 0, 110),
			FString::Printf(TEXT("Item ID: %d"), ItemID),
			nullptr,
			GroundItemColor,
			DrawDuration
		);
	}
}

void FInteractionDebugManager::DrawInteractableInfo(
	UInteractableManager* Interactable, float Distance, FVector TraceOrigin)
{
	if (!ShouldShowDebugTraces() || !Interactable || !WorldContext)
	{
		return;
	}

	AActor* TargetActor = Interactable->GetOwner();
	if (!TargetActor)
	{
		return;
	}

	FVector ActorLocation = TargetActor->GetActorLocation();

	DrawDebugSphere(
		WorldContext,
		ActorLocation,
		50.0f,
		8,
		InteractableColor,
		false,
		DrawDuration,
		0,
		DrawThickness
	);
	DrawDebugLine(
		WorldContext,
		TraceOrigin,
		ActorLocation,
		InteractableColor,
		false,
		DrawDuration,
		0,
		DrawThickness);

	if (DebugMode == EInteractionDebugMode::Detailed || DebugMode == EInteractionDebugMode::Full)
	{
		FString DebugInfo = FString::Printf(
			TEXT("%s\nDistance: %.1f\nType: %s"),
			*TargetActor->GetName(),
			Distance,
			*UEnum::GetValueAsString(Interactable->Config.InteractionType)
		);

		DrawDebugString(
			WorldContext,
			ActorLocation + FVector(0, 0, 100),
			DebugInfo,
			nullptr,
			InteractableColor,
			DrawDuration
		);
	}
}

void FInteractionDebugManager::DisplayInteractionState(
	UInteractableManager* Interactable,
	float Distance,
	int32 GroundItemID,
	int32 SelectionNumber,
	int32 CandidateCount,
	int32 AutomaticItemID,
	const FGroundItemInteractionCandidate* SelectedCandidate,
	bool bHasProximityCandidates,
	float EvaluationInterval,
	bool bManualSelectionLocked,
	float ManualLockRemaining)
{
	if (!bShowDebugText || !ShouldShowDebugTraces())
	{
		return;
	}

	FString DebugText;

	if (Interactable)
	{
		AActor* TargetActor = Interactable->GetOwner();
		DebugText = FString::Printf(
			TEXT("INTERACTION DECISION\n")
			TEXT("Sphere: %s | Loop: %s (%.0f Hz)\n")
			TEXT("Target: %s\n")
			TEXT("Distance: %.1f\n")
			TEXT("Type: %s\n")
			TEXT("Can Interact: %s"),
			bHasProximityCandidates ? TEXT("OCCUPIED") : TEXT("EMPTY"),
			bHasProximityCandidates ? TEXT("ACTIVE") : TEXT("IDLE DISCOVERY"),
			EvaluationInterval > KINDA_SMALL_NUMBER ? 1.0f / EvaluationInterval : 0.0f,
			TargetActor ? *TargetActor->GetName() : TEXT("NULL"),
			Distance,
			*UEnum::GetValueAsString(Interactable->Config.InteractionType),
			Interactable->Config.bCanInteract ? TEXT("YES") : TEXT("NO")
		);
	}
	else if (GroundItemID != -1)
	{
		if (SelectedCandidate)
		{
			DebugText = FString::Printf(
				TEXT("INTERACTION DECISION\n")
				TEXT("Sphere: OCCUPIED | Loop: ACTIVE (%.0f Hz)\n")
				TEXT("Selected Item: %d  (%d / %d)\n")
				TEXT("Mode: %s\n")
				TEXT("Final Score: %.3f\n")
				TEXT("Camera Dot: %.3f\n")
				TEXT("Player Gate Dot: %.3f (must be >= 0)\n")
				TEXT("Range: %.0f cm (gate only)\n")
				TEXT("Current Focus Bonus: +%.3f\n")
				TEXT("Automatic Best: %d%s"),
				EvaluationInterval > KINDA_SMALL_NUMBER ? 1.0f / EvaluationInterval : 0.0f,
				GroundItemID,
				SelectionNumber,
				CandidateCount,
				bManualSelectionLocked ? TEXT("MANUAL LOCK") : TEXT("AUTOMATIC"),
				SelectedCandidate->Score,
				SelectedCandidate->CameraForwardDot,
				SelectedCandidate->PlayerForwardDot,
				SelectedCandidate->Distance,
				SelectedCandidate->FocusBonus,
				AutomaticItemID,
				bManualSelectionLocked
					? *FString::Printf(TEXT("\nLock Remaining: %.2fs"), ManualLockRemaining)
					: TEXT(""));
		}
		else
		{
			DebugText = FString::Printf(
				TEXT("INTERACTION DECISION\n")
				TEXT("Sphere: %s | Loop: %s (%.0f Hz)\n")
				TEXT("Ground Item ID: %d\nEligible Candidates: %d"),
				bHasProximityCandidates ? TEXT("OCCUPIED") : TEXT("EMPTY"),
				bHasProximityCandidates ? TEXT("ACTIVE") : TEXT("IDLE DISCOVERY"),
				EvaluationInterval > KINDA_SMALL_NUMBER ? 1.0f / EvaluationInterval : 0.0f,
				GroundItemID,
				CandidateCount);
		}
	}
	else
	{
		DebugText = FString::Printf(
			TEXT("INTERACTION DECISION\n")
			TEXT("Sphere: %s | Loop: %s (%.0f Hz)\n")
			TEXT("Eligible Ground Items: %d\n")
			TEXT("%s"),
			bHasProximityCandidates ? TEXT("OCCUPIED") : TEXT("EMPTY"),
			bHasProximityCandidates ? TEXT("ACTIVE") : TEXT("IDLE DISCOVERY"),
			EvaluationInterval > KINDA_SMALL_NUMBER ? 1.0f / EvaluationInterval : 0.0f,
			CandidateCount,
			bHasProximityCandidates
				? TEXT("No target passed the facing gates")
				: TEXT("No interactables in player radius"));
	}

	if (GEngine)
	{
		const uint64 MessageKey = OwnerActor
			? (static_cast<uint64>(OwnerActor->GetUniqueID()) << 1) | 1ULL
			: 739001ULL;
		GEngine->AddOnScreenDebugMessage(
			MessageKey,
			FMath::Max(DrawDuration * 1.5f, 0.1f),
			bManualSelectionLocked ? ManualCandidateColor : InteractableColor,
			DebugText
		);
	}
}

void FInteractionDebugManager::DisplayPerformanceMetrics(float TraceTime, float ValidationTime)
{
	if (!bShowDebugText || DebugMode != EInteractionDebugMode::Full)
	{
		return;
	}

	AverageTraceTime = (AverageTraceTime * 0.9f) + (TraceTime * 0.1f);
	AverageValidationTime = (AverageValidationTime * 0.9f) + (ValidationTime * 0.1f);

	FString PerfText = FString::Printf(
		TEXT("PERFORMANCE\n")
		TEXT("Trace Time: %.2f ms (Avg: %.2f ms)\n")
		TEXT("Validation Time: %.2f ms (Avg: %.2f ms)"),
		TraceTime,
		AverageTraceTime,
		ValidationTime,
		AverageValidationTime
	);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			0.0f,
			FColor::Yellow,
			PerfText
		);
	}
}

void FInteractionDebugManager::LogInteraction(UInteractableManager* Interactable, bool bSuccess, const FString& Reason)
{
	TotalInteractions++;

	if (bSuccess)
	{
		SuccessfulInteractions++;
		UE_LOG(LogInteractionDebugManager, Log, TEXT("Interaction Success: %s"),
			Interactable ? *Interactable->GetOwner()->GetName() : TEXT("Unknown"));
	}
	else
	{
		FailedInteractions++;
		UE_LOG(LogInteractionDebugManager, Warning, TEXT("Interaction Failed: %s | Reason: %s"),
			Interactable ? *Interactable->GetOwner()->GetName() : TEXT("Unknown"),
			*Reason);
	}
}

void FInteractionDebugManager::LogGroundItemPickup(int32 ItemID, bool bToInventory, bool bSuccess)
{
	if (bSuccess)
	{
		TotalGroundItemsPickedUp++;
		UE_LOG(LogInteractionDebugManager, Log, TEXT("Ground Item Pickup: ID=%d | Destination=%s"),
			ItemID,
			bToInventory ? TEXT("Inventory") : TEXT("Equipment"));
	}
	else
	{
		UE_LOG(LogInteractionDebugManager, Warning, TEXT("Ground Item Pickup Failed: ID=%d"), ItemID);
	}
}

void FInteractionDebugManager::LogValidationFailure(const FString& ValidationReason, float Distance, float MaxDistance)
{
	UE_LOG(LogInteractionDebugManager, Warning, TEXT("Validation Failed: %s | Distance: %.1f / %.1f"),
		*ValidationReason,
		Distance,
		MaxDistance);
}

void FInteractionDebugManager::PrintDebugStats()
{
	float SuccessRate = TotalInteractions > 0 ?
		(static_cast<float>(SuccessfulInteractions) / TotalInteractions) * 100.0f : 0.0f;

	UE_LOG(LogInteractionDebugManager, Display, TEXT("  INTERACTION DEBUG STATISTICS"));
	UE_LOG(LogInteractionDebugManager, Display, TEXT("Total Interactions: %d"), TotalInteractions);
	UE_LOG(LogInteractionDebugManager, Display, TEXT("Successful: %d"), SuccessfulInteractions);
	UE_LOG(LogInteractionDebugManager, Display, TEXT("Failed: %d"), FailedInteractions);
	UE_LOG(LogInteractionDebugManager, Display, TEXT("Success Rate: %.1f%%"), SuccessRate);
	UE_LOG(LogInteractionDebugManager, Display, TEXT("Ground Items Picked Up: %d"), TotalGroundItemsPickedUp);
	UE_LOG(LogInteractionDebugManager, Display, TEXT("Avg Trace Time: %.2f ms"), AverageTraceTime);
	UE_LOG(LogInteractionDebugManager, Display, TEXT("Avg Validation Time: %.2f ms"), AverageValidationTime);
}

bool FInteractionDebugManager::ShouldShowDebugTraces() const
{
#if !UE_BUILD_SHIPPING
	if (CachedALSDebugComponent)
	{
		return CachedALSDebugComponent->GetShowTraces();
	}

	return DebugMode != EInteractionDebugMode::None;
#else
	return false;
#endif
}

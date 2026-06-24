#include "Character/Components/Interaction/InteractionDebugManager.h"
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
		UE_LOG(LogInteractionDebugManager, Log, TEXT("InteractionDebugManager: ✓ ALS Debug Component found - Using ALS debug toggle"));
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
			FString::Printf(TEXT("dot %.3f%s"), Dot, bWinner ? TEXT(" ★") : TEXT("")),
			nullptr,
			CandidateColor,
			DrawDuration <= 0.0f ? 0.05f : DrawDuration,
			true
		);
	}
}

void FInteractionDebugManager::DrawGroundItemAimWindow(
	FVector Origin, FVector Forward, float MinDistance, float MaxDistance, float Radius, bool bLimitedByTraceHit)
{
	if (!ShouldShowDebugTraces() || !bDrawGroundItemAimWindow || !WorldContext
		|| MaxDistance <= MinDistance || Radius <= 0.0f)
	{
		return;
	}

	const FVector SafeForward = Forward.GetSafeNormal();
	if (SafeForward.IsNearlyZero())
	{
		return;
	}

	const float WindowLength = MaxDistance - MinDistance;
	const FVector Start = Origin + SafeForward * MinDistance;
	const FVector End = Origin + SafeForward * MaxDistance;
	const FColor WindowColor = bLimitedByTraceHit
		? FColor(0, 220, 255)
		: FColor(0, 120, 255);

	DrawDebugCylinder(
		WorldContext,
		Start,
		End,
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
			Start + SafeForward * (WindowLength * 0.5f) + FVector(0, 0, Radius + 25.0f),
			FString::Printf(
				TEXT("ground aim r %.0f depth %.0f-%.0f%s"),
				Radius,
				MinDistance,
				MaxDistance,
				bLimitedByTraceHit ? TEXT(" hit-limited") : TEXT(" full")),
			nullptr,
			WindowColor,
			DrawDuration <= 0.0f ? 0.05f : DrawDuration,
			true
		);
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

void FInteractionDebugManager::DrawInteractableInfo(UInteractableManager* Interactable, float Distance)
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

	if (DebugMode == EInteractionDebugMode::Detailed || DebugMode == EInteractionDebugMode::Full)
	{
		FString DebugInfo = FString::Printf(
			TEXT("%s\nDistance: %.1f\nType: %s"),
			*TargetActor->GetName(),
			Distance,
			*UEnum::GetValueAsString(Interactable->Config.InteractionType)
		);
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.0f,
				FColor::Green,
				DebugInfo
			);
		}

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

void FInteractionDebugManager::DisplayInteractionState(UInteractableManager* Interactable, float Distance, int32 GroundItemID)
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
			TEXT("INTERACTION DEBUG\n")
			TEXT("Target: %s\n")
			TEXT("Distance: %.1f\n")
			TEXT("Type: %s\n")
			TEXT("Can Interact: %s"),
			TargetActor ? *TargetActor->GetName() : TEXT("NULL"),
			Distance,
			*UEnum::GetValueAsString(Interactable->Config.InteractionType),
			Interactable->Config.bCanInteract ? TEXT("YES") : TEXT("NO")
		);
	}
	else if (GroundItemID != -1)
	{
		DebugText = FString::Printf(
			TEXT("INTERACTION DEBUG\n")
			TEXT("Ground Item ID: %d"),
			GroundItemID
		);
	}
	else
	{
		DebugText = TEXT("INTERACTION DEBUG\nNo Target");
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			0.0f,
			InteractableColor,
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
		UE_LOG(LogInteractionDebugManager, Log, TEXT("✓ Interaction Success: %s"), 
			Interactable ? *Interactable->GetOwner()->GetName() : TEXT("Unknown"));
	}
	else
	{
		FailedInteractions++;
		UE_LOG(LogInteractionDebugManager, Warning, TEXT("✗ Interaction Failed: %s | Reason: %s"), 
			Interactable ? *Interactable->GetOwner()->GetName() : TEXT("Unknown"),
			*Reason);
	}
}

void FInteractionDebugManager::LogGroundItemPickup(int32 ItemID, bool bToInventory, bool bSuccess)
{
	if (bSuccess)
	{
		TotalGroundItemsPickedUp++;
		UE_LOG(LogInteractionDebugManager, Log, TEXT("✓ Ground Item Pickup: ID=%d | Destination=%s"), 
			ItemID,
			bToInventory ? TEXT("Inventory") : TEXT("Equipment"));
	}
	else
	{
		UE_LOG(LogInteractionDebugManager, Warning, TEXT("✗ Ground Item Pickup Failed: ID=%d"), ItemID);
	}
}

void FInteractionDebugManager::LogValidationFailure(const FString& ValidationReason, float Distance, float MaxDistance)
{
	UE_LOG(LogInteractionDebugManager, Warning, TEXT("✗ Validation Failed: %s | Distance: %.1f / %.1f"), 
		*ValidationReason,
		Distance,
		MaxDistance);
}

void FInteractionDebugManager::PrintDebugStats()
{
	float SuccessRate = TotalInteractions > 0 ? 
		(static_cast<float>(SuccessfulInteractions) / TotalInteractions) * 100.0f : 0.0f;

	UE_LOG(LogInteractionDebugManager, Display, TEXT("═══════════════════════════════════════════"));
	UE_LOG(LogInteractionDebugManager, Display, TEXT("  INTERACTION DEBUG STATISTICS"));
	UE_LOG(LogInteractionDebugManager, Display, TEXT("═══════════════════════════════════════════"));
	UE_LOG(LogInteractionDebugManager, Display, TEXT("Total Interactions: %d"), TotalInteractions);
	UE_LOG(LogInteractionDebugManager, Display, TEXT("Successful: %d"), SuccessfulInteractions);
	UE_LOG(LogInteractionDebugManager, Display, TEXT("Failed: %d"), FailedInteractions);
	UE_LOG(LogInteractionDebugManager, Display, TEXT("Success Rate: %.1f%%"), SuccessRate);
	UE_LOG(LogInteractionDebugManager, Display, TEXT("Ground Items Picked Up: %d"), TotalGroundItemsPickedUp);
	UE_LOG(LogInteractionDebugManager, Display, TEXT("Avg Trace Time: %.2f ms"), AverageTraceTime);
	UE_LOG(LogInteractionDebugManager, Display, TEXT("Avg Validation Time: %.2f ms"), AverageValidationTime);
	UE_LOG(LogInteractionDebugManager, Display, TEXT("═══════════════════════════════════════════"));
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

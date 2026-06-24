#include "Character/Components/Interaction/InteractionTraceManager.h"
#include "Interactable/Interface/Interactable.h"
#include "Interactable/Components/InteractableManager.h"
#include "Tower/Subsystems/GroundItemSubsystem.h"
#include "Item/ItemInstance.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/ALSPlayerCameraManager.h"
#include "Character/Components/Interaction/InteractionDebugManager.h"
#include "Engine/OverlapResult.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
DEFINE_LOG_CATEGORY(LogInteractionTraceManager);
FInteractionTraceManager::FInteractionTraceManager()
	: InteractionDistance(300.0f)
	  // 20Hz focus updates — 0.1 (10Hz) read as visibly steppy when sweeping
	  // the camera across interactables. Still timer-driven, still cheap.
	  , CheckFrequency(0.05f)
	  , InteractionTraceChannel(ECC_Visibility)
	  , OverlapRadius(75.0f)
	  , MinPlayerForwardDot(0.0f)
	  , CurrentFocusDotBonus(0.02f)
	  , bUseALSCameraOrigin(true)
	  , OffsetForward(0.0f)
	  , OffsetRight(0.0f)
	  , OffsetUp(60.0f)
	  , OwnerActor(nullptr)
	  , WorldContext(nullptr)
	  , CachedPlayerController(nullptr)
	  , CachedALSCameraManager(nullptr)
	  , CachedGroundItemSubsystem(nullptr)
	  , DebugManager(nullptr)
{
}

void FInteractionTraceManager::Initialize(AActor* Owner, UWorld* World)
{
	OwnerActor = Owner;
	WorldContext = World;
	
	if (!OwnerActor || !WorldContext)
	{
		UE_LOG(LogInteractionTraceManager, Error, TEXT("InteractionTraceManager: Invalid initialization parameters"));
		return;
	}

	CacheComponents();
	
	UE_LOG(LogInteractionTraceManager, Log, TEXT("InteractionTraceManager: Initialized for %s"), *OwnerActor->GetName());
}

void FInteractionTraceManager::SetDebugManager(FInteractionDebugManager* InDebugManager)
{
	DebugManager = InDebugManager;
}

TScriptInterface<IInteractable> FInteractionTraceManager::TraceForActorInteractable()
{
	TScriptInterface<IInteractable> Result;
	int32 IgnoredItemID = INDEX_NONE;
	FindBestInteractionTarget(TScriptInterface<IInteractable>(), INDEX_NONE, Result, IgnoredItemID);
	return Result;
}

void FInteractionTraceManager::FindBestInteractionTarget(
	const TScriptInterface<IInteractable>& CurrentInteractable,
	int32 CurrentItemID,
	TScriptInterface<IInteractable>& OutInteractable,
	int32& OutGroundItemID)
{
	OutInteractable = TScriptInterface<IInteractable>();
	OutGroundItemID = INDEX_NONE;

	if (!WorldContext) return;

	FVector CameraLocation;
	FRotator CameraRotation;
	if (!GetCameraViewPoint(CameraLocation, CameraRotation)) return;

	const FVector TraceStart = GetTraceStartLocation(CameraLocation, CameraRotation);
	const FVector CameraForward = CameraRotation.Vector().GetSafeNormal();
	if (CameraForward.IsNearlyZero()) return;

	const FVector TraceEnd = TraceStart + CameraForward * InteractionDistance;

	// ── 1. Line trace ────────────────────────────────────────────────────────
	FHitResult LineHit;
	const bool bHit = PerformLineTrace(TraceStart, TraceEnd, LineHit);
	const FVector SearchCenter = bHit ? LineHit.ImpactPoint : TraceEnd;

	if (DebugManager)
	{
		DebugManager->DrawTraceResult(TraceStart, TraceEnd, LineHit, bHit, 0.0f);
		if (bHit) DebugManager->DrawHitPoint(LineHit.Location, LineHit.Normal, 0.0f);
		// Overlap sphere — shows exactly what radius is active and where candidates are searched.
		DebugManager->DrawInteractionRange(SearchCenter, OverlapRadius);
	}

	// ── 2. Sphere overlap at hit point ───────────────────────────────────────
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams OverlapParams;
	OverlapParams.AddIgnoredActor(OwnerActor);

	WorldContext->OverlapMultiByChannel(
		Overlaps,
		SearchCenter,
		FQuat::Identity,
		InteractionTraceChannel,
		FCollisionShape::MakeSphere(OverlapRadius),
		OverlapParams
	);

	// ── 3. Dot-product scoring — best dot wins, no rejection threshold ───────

	// Resolve the actor that owns the currently focused interactable so we can
	// apply the same hysteresis bonus that ground items already receive.
	AActor* CurrentFocusActor = nullptr;
	if (UObject* CurrentObj = CurrentInteractable.GetObject())
	{
		if (const UInteractableManager* Comp = Cast<UInteractableManager>(CurrentObj))
		{
			CurrentFocusActor = Comp->GetOwner();
		}
		else
		{
			CurrentFocusActor = Cast<AActor>(CurrentObj);
		}
	}

	float BestDot = -1.0f;
	AActor* BestActor = nullptr;
	int32 BestItemID = INDEX_NONE;

	// Actor interactables
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OverlapActor = Overlap.GetActor();
		if (!IsActorInteractable(OverlapActor)) continue;

		float PlayerDot = 0.0f;
		if (!PassesPlayerForwardGate(OverlapActor->GetActorLocation(), PlayerDot)) continue;

		const FVector ToTarget = (OverlapActor->GetActorLocation() - TraceStart).GetSafeNormal();
		float Dot = FVector::DotProduct(CameraForward, ToTarget);

		// Hysteresis: current actor needs a worse dot to lose focus, matching
		// the same behaviour already applied to ground items below.
		if (OverlapActor == CurrentFocusActor) Dot += CurrentFocusDotBonus;

		if (Dot > BestDot)
		{
			BestDot = Dot;
			BestActor = OverlapActor;
			BestItemID = INDEX_NONE;
		}
	}

	// Ground items — scan registered locations within OverlapRadius of the hit
	if (CachedGroundItemSubsystem)
	{
		const float RadiusSq = FMath::Square(OverlapRadius);
		for (const TPair<int32, FVector>& Pair : CachedGroundItemSubsystem->GetInstanceLocations())
		{
			if (FVector::DistSquared(Pair.Value, SearchCenter) > RadiusSq) continue;

			float PlayerDot = 0.0f;
			if (!PassesPlayerForwardGate(Pair.Value, PlayerDot)) continue;

			const FVector ToItem = (Pair.Value - TraceStart).GetSafeNormal();
			float Dot = FVector::DotProduct(CameraForward, ToItem);

			// Hysteresis: current item needs a worse dot to lose focus.
			if (Pair.Key == CurrentItemID) Dot += CurrentFocusDotBonus;

			if (Dot > BestDot)
			{
				BestDot = Dot;
				BestActor = nullptr;
				BestItemID = Pair.Key;
			}
		}
	}

	// ── 4. Debug — single dot at winner ─────────────────────────────────────
	if (DebugManager)
	{
		if (BestActor)
		{
			DebugManager->DrawAimCandidate(BestActor->GetActorLocation(), BestDot, true, true);
		}
		else if (BestItemID != INDEX_NONE && CachedGroundItemSubsystem)
		{
			if (const FVector* WinnerLoc =
				CachedGroundItemSubsystem->GetInstanceLocations().Find(BestItemID))
			{
				DebugManager->DrawAimCandidate(*WinnerLoc, BestDot, true, true);
			}
		}
	}

	// ── 5. Output ────────────────────────────────────────────────────────────
	if (BestActor)
	{
		LastTraceResult = LineHit;
		OutInteractable = MakeInteractableInterface(BestActor);
	}
	else if (BestItemID != INDEX_NONE)
	{
		LastTraceResult = LineHit;
		OutGroundItemID = BestItemID;
	}
}

bool FInteractionTraceManager::PassesPlayerForwardGate(const FVector& TargetLocation, float& OutDot) const
{
	if (!OwnerActor)
	{
		OutDot = -1.0f;
		return false;
	}

	const FVector PlayerForward = OwnerActor->GetActorForwardVector().GetSafeNormal();
	if (PlayerForward.IsNearlyZero())
	{
		OutDot = -1.0f;
		return false;
	}

	const FVector ToTarget = TargetLocation - OwnerActor->GetActorLocation();
	const float Distance = ToTarget.Size();
	if (Distance <= KINDA_SMALL_NUMBER)
	{
		OutDot = 1.0f;
		return true;
	}

	OutDot = FVector::DotProduct(PlayerForward, ToTarget / Distance);
	return OutDot >= MinPlayerForwardDot;
}

TScriptInterface<IInteractable> FInteractionTraceManager::MakeInteractableInterface(AActor* Actor) const
{
	TScriptInterface<IInteractable> Result;
	if (!Actor)
	{
		return Result;
	}

	if (UInteractableManager* InteractableComp = Actor->FindComponentByClass<UInteractableManager>())
	{
		Result.SetObject(InteractableComp);
		Result.SetInterface(Cast<IInteractable>(InteractableComp));
	}
	else if (Actor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		Result.SetObject(Actor);
		Result.SetInterface(Cast<IInteractable>(Actor));
	}

	return Result;
}

UItemInstance* FInteractionTraceManager::FindNearestGroundItem(int32& OutItemID)
{
	OutItemID = -1;

	if (!CachedGroundItemSubsystem)
	{
		return nullptr;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	if (!GetCameraViewPoint(CameraLocation, CameraRotation))
	{
		return nullptr;
	}

	return CachedGroundItemSubsystem->GetNearestItem(
		CameraLocation,
		InteractionDistance,
		OutItemID
	);
}

bool FInteractionTraceManager::GetCameraViewPoint(FVector& OutLocation, FRotator& OutRotation)
{

	if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		CachedPlayerController = Cast<APlayerController>(OwnerPawn->GetController());

		if (CachedPlayerController && CachedPlayerController->PlayerCameraManager)
		{
			CachedALSCameraManager = Cast<AALSPlayerCameraManager>(
				CachedPlayerController->PlayerCameraManager);
		}
		else
		{
			CachedALSCameraManager = nullptr;
		}
	}

	if (!CachedPlayerController)
	{
		if (OwnerActor)
		{
			OutLocation = OwnerActor->GetActorLocation();
			OutRotation = OwnerActor->GetActorRotation();
			return true;
		}
		return false;
	}

	FVector RawCameraLoc;
	CachedPlayerController->GetPlayerViewPoint(RawCameraLoc, OutRotation);

	if (OwnerActor)
	{
		FVector PivotLocation = OwnerActor->GetActorLocation();

		const FRotationMatrix RotMat(OutRotation);
		OutLocation = PivotLocation
			+ RotMat.GetUnitAxis(EAxis::X) * OffsetForward
			+ RotMat.GetUnitAxis(EAxis::Y) * OffsetRight
			+ RotMat.GetUnitAxis(EAxis::Z) * OffsetUp;

		return true;
	}

	OutLocation = RawCameraLoc;
	return true;
}

FVector FInteractionTraceManager::GetTraceStart()
{
	FVector CameraLocation;
	FRotator CameraRotation;
	GetCameraViewPoint(CameraLocation, CameraRotation);
	return GetTraceStartLocation(CameraLocation, CameraRotation);
}

void FInteractionTraceManager::GetTraceOrigin(FVector& OutCameraLocation, FVector& OutCameraDirection)
{
	FRotator CameraRotation;
	GetCameraViewPoint(OutCameraLocation, CameraRotation);

	
	OutCameraDirection = CameraRotation.Vector();
}

FVector FInteractionTraceManager::GetTraceStartLocation(const FVector& CameraLocation, const FRotator& CameraRotation) const
{
	
	if (bUseALSCameraOrigin && CachedALSCameraManager)
	{
		return CachedALSCameraManager->GetCameraLocation();
	}

	return CameraLocation;
}

bool FInteractionTraceManager::IsLocallyControlled() const
{
	if (!OwnerActor)
	{
		return false;
	}

	if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		return OwnerPawn->IsLocallyControlled();
	}

	return false;
}

void FInteractionTraceManager::CacheComponents()
{
	if (!OwnerActor)
	{
		return;
	}

	if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		CachedPlayerController = Cast<APlayerController>(OwnerPawn->GetController());

		if (CachedPlayerController && CachedPlayerController->PlayerCameraManager)
		{
			CachedALSCameraManager = Cast<AALSPlayerCameraManager>(CachedPlayerController->PlayerCameraManager);
			if (CachedALSCameraManager)
			{
				UE_LOG(LogInteractionTraceManager, Log, TEXT("InteractionTraceManager: Found ALS Camera Manager"));
			}
		}
	}

	if (WorldContext)
	{
		CachedGroundItemSubsystem = WorldContext->GetSubsystem<UGroundItemSubsystem>();
		if (!CachedGroundItemSubsystem)
		{
			UE_LOG(LogInteractionTraceManager, Warning, TEXT("InteractionTraceManager: No GroundItemSubsystem found"));
		}
	}
}

bool FInteractionTraceManager::PerformLineTrace(const FVector& Start, const FVector& End, FHitResult& OutHit)
{
	if (!WorldContext)
	{
		return false;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);
	QueryParams.bTraceComplex = false;

	return WorldContext->LineTraceSingleByChannel(
		OutHit,
		Start,
		End,
		InteractionTraceChannel,
		QueryParams
	);
}

bool FInteractionTraceManager::IsActorInteractable(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	if (Actor->FindComponentByClass<UInteractableManager>())
	{
		return true;
	}

	return Actor->GetClass()->ImplementsInterface(UInteractable::StaticClass());
}

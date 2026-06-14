#include "Character/Components/Interaction/InteractionTraceManager.h"
#include "Interactable/Interface/Interactable.h"
#include "Interactable/Component/InteractableManager.h"
#include "Tower/Subsystems/GroundItemSubsystem.h"
#include "Item/ItemInstance.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/ALSPlayerCameraManager.h"
#include "Character/Components/Interaction/InteractionDebugManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
DEFINE_LOG_CATEGORY(LogInteractionTraceManager);
FInteractionTraceManager::FInteractionTraceManager()
	: InteractionDistance(300.0f)
	  // 20Hz focus updates — 0.1 (10Hz) read as visibly steppy when sweeping
	  // the camera across interactables. Still timer-driven, still cheap.
	  , CheckFrequency(0.05f)
	  , InteractionTraceChannel(ECC_Visibility)
	  , bUseALSCameraOrigin(true)
	  , OffsetForward(0.0f)
	  , OffsetRight(0.0f)
	  , OffsetUp(60.0f)
	  , OwnerActor(nullptr)
	  , WorldContext(nullptr)
	  , CachedPlayerController(nullptr)
	  , CachedALSCameraManager(nullptr)
	  , CachedGroundItemSubsystem(nullptr), DebugManager(nullptr)
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
	return FindBestActorInteractable();
}

TScriptInterface<IInteractable> FInteractionTraceManager::FindBestActorInteractable()
{
	TScriptInterface<IInteractable> Result;

	FVector CameraLocation;
	FRotator CameraRotation;
	if (!GetCameraViewPoint(CameraLocation, CameraRotation))
	{
		return Result;
	}

	const FVector TraceStart   = GetTraceStartLocation(CameraLocation, CameraRotation);
	const FVector CameraForward = CameraRotation.Vector().GetSafeNormal();
	const FVector TraceEnd     = TraceStart + CameraForward * InteractionDistance;

	FHitResult HitResult;
	const bool bHit = PerformAimTrace(TraceStart, TraceEnd, HitResult);

	if (DebugManager)
	{
		DebugManager->DrawLookAtCone(TraceStart, CameraForward, MinLookAtDot, InteractionDistance);
		DebugManager->DrawTraceLine(TraceStart, TraceEnd, bHit);

		if (OwnerActor)
		{
			DebugManager->DrawPlayerForwardGate(
				OwnerActor->GetActorLocation(),
				OwnerActor->GetActorForwardVector(),
				MinPlayerForwardDot,
				InteractionDistance);
		}

		if (bHit)
		{
			DebugManager->DrawHitPoint(HitResult.Location, HitResult.Normal);
		}
	}

	if (!bHit || !IsActorInteractable(HitResult.GetActor()))
	{
		return Result;
	}

	float CameraDot = 0.0f;
	float PlayerDot = 0.0f;
	const bool bPassedGates = PassesCameraAndPlayerGates(
		TraceStart,
		CameraForward,
		HitResult.ImpactPoint,
		MinLookAtDot,
		CameraDot,
		PlayerDot);

	if (DebugManager)
	{
		DebugManager->DrawAimCandidate(HitResult.ImpactPoint, CameraDot, bPassedGates, bPassedGates);
	}

	if (!bPassedGates)
	{
		return Result;
	}

	LastTraceResult = HitResult;
	return MakeInteractableInterface(HitResult.GetActor());
}

bool FInteractionTraceManager::PassesLookAtGate(const FVector& ViewOrigin, const FVector& Forward,
	const FVector& TargetLocation, float MinDot, float& OutDot) const
{
	const FVector ToTarget = TargetLocation - ViewOrigin;
	const float Distance = ToTarget.Size();
	if (Distance <= KINDA_SMALL_NUMBER)
	{
		// On top of it — looking "at" it by definition.
		OutDot = 1.0f;
		return true;
	}

	OutDot = FVector::DotProduct(Forward, ToTarget / Distance);
	return OutDot >= MinDot;
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

bool FInteractionTraceManager::PassesCameraAndPlayerGates(
	const FVector& ViewOrigin,
	const FVector& CameraForward,
	const FVector& TargetLocation,
	float MinCameraDot,
	float& OutCameraDot,
	float& OutPlayerDot) const
{
	const bool bPassedCamera =
		PassesLookAtGate(ViewOrigin, CameraForward, TargetLocation, MinCameraDot, OutCameraDot);
	const bool bPassedPlayer = PassesPlayerForwardGate(TargetLocation, OutPlayerDot);

	return bPassedCamera && bPassedPlayer;
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

int32 FInteractionTraceManager::FindGroundItemByTrace(int32 CurrentItemID)
{
	if (!CachedGroundItemSubsystem)
	{
		return INDEX_NONE;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	if (!GetCameraViewPoint(CameraLocation, CameraRotation))
	{
		return INDEX_NONE;
	}

	const FVector TraceStart = GetTraceStartLocation(CameraLocation, CameraRotation);
	const FVector CameraForward = CameraRotation.Vector().GetSafeNormal();
	const FVector TraceEnd = TraceStart + CameraForward * InteractionDistance;

	FHitResult HitResult;
	const bool bHit = PerformAimTrace(TraceStart, TraceEnd, HitResult);

	if (DebugManager)
	{
		DebugManager->DrawLookAtCone(TraceStart, CameraForward, MinGroundLookAtDot, InteractionDistance);
		DebugManager->DrawTraceLine(TraceStart, TraceEnd, bHit);

		if (OwnerActor)
		{
			DebugManager->DrawPlayerForwardGate(
				OwnerActor->GetActorLocation(),
				OwnerActor->GetActorForwardVector(),
				MinPlayerForwardDot,
				InteractionDistance);
		}

		if (bHit)
		{
			DebugManager->DrawHitPoint(HitResult.Location, HitResult.Normal);
		}
	}

	if (!bHit)
	{
		return INDEX_NONE;
	}

	UInstancedStaticMeshComponent* HitISM =
		Cast<UInstancedStaticMeshComponent>(HitResult.Component.Get());
	if (!HitISM)
	{
		return INDEX_NONE;
	}

	const int32 HitInstanceIndex = HitResult.Item;
	if (HitInstanceIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	const int32 HitItemID =
		CachedGroundItemSubsystem->FindItemByISMInstance(HitISM, HitInstanceIndex);
	if (HitItemID == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	const FVector* HitItemLocation =
		CachedGroundItemSubsystem->GetInstanceLocations().Find(HitItemID);
	const FVector TargetLocation = HitItemLocation ? *HitItemLocation : HitResult.ImpactPoint;
	const float HitProjectionDistance = FVector::DotProduct(TargetLocation - TraceStart, CameraForward);
	if (HitProjectionDistance <= KINDA_SMALL_NUMBER)
	{
		return INDEX_NONE;
	}

	float CameraDot = 0.0f;
	float PlayerDot = 0.0f;
	const bool bPassedGates = PassesCameraAndPlayerGates(
		TraceStart,
		CameraForward,
		TargetLocation,
		MinGroundLookAtDot,
		CameraDot,
		PlayerDot);

	if (DebugManager)
	{
		DebugManager->DrawAimCandidate(TargetLocation, CameraDot, bPassedGates, bPassedGates);
	}

	if (!bPassedGates)
	{
		return INDEX_NONE;
	}

	LastTraceResult = HitResult;
	return FindBestGroundItemFromTraceHit(CurrentItemID, HitProjectionDistance);
}

int32 FInteractionTraceManager::FindBestGroundItemFromTraceHit(int32 CurrentItemID, float HitProjectionDistance)
{
	if (!CachedGroundItemSubsystem)
	{
		return INDEX_NONE;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	if (!GetCameraViewPoint(CameraLocation, CameraRotation))
	{
		return INDEX_NONE;
	}

	const FVector TraceStart = GetTraceStartLocation(CameraLocation, CameraRotation);
	const FVector CameraForward = CameraRotation.Vector().GetSafeNormal();
	if (CameraForward.IsNearlyZero())
	{
		return INDEX_NONE;
	}

	const float AimRadius = FMath::Max(GroundItemAimRadius, TraceSphereRadius);
	const float AimRadiusSq = FMath::Square(AimRadius);
	const float InteractionDistanceSq = FMath::Square(InteractionDistance);
	const float MinProjectionLimit = FMath::Clamp(
		HitProjectionDistance - GroundItemTraceDepthTolerance,
		0.0f,
		InteractionDistance);
	const float MaxProjectionLimit = FMath::Clamp(
		HitProjectionDistance + GroundItemTraceDepthTolerance,
		0.0f,
		InteractionDistance);
	constexpr float DotTieTolerance = 0.002f;

	if (DebugManager)
	{
		DebugManager->DrawGroundItemAimWindow(
			TraceStart,
			CameraForward,
			MinProjectionLimit,
			MaxProjectionLimit,
			AimRadius,
			true);
	}

	int32 BestItemID = INDEX_NONE;
	float BestScoreDot = -1.0f;
	float BestProjectionDelta = TNumericLimits<float>::Max();
	FVector BestLocation = FVector::ZeroVector;

	// The initial trace already hit a ground item. Only nearby items in that
	// local depth window can compete, then both camera and player-forward gates
	// must pass before camera-dot scoring.
	for (const TPair<int32, FVector>& Pair : CachedGroundItemSubsystem->GetInstanceLocations())
	{
		const FVector ToItem = Pair.Value - TraceStart;
		const float DistanceSq = ToItem.SizeSquared();
		if (DistanceSq <= KINDA_SMALL_NUMBER || DistanceSq > InteractionDistanceSq)
		{
			continue;
		}

		const float ProjectionDistance = FVector::DotProduct(ToItem, CameraForward);
		if (ProjectionDistance < MinProjectionLimit || ProjectionDistance > MaxProjectionLimit)
		{
			continue;
		}

		const float LateralDistanceSq =
			FMath::Max(0.0f, DistanceSq - FMath::Square(ProjectionDistance));
		if (LateralDistanceSq > AimRadiusSq)
		{
			continue;
		}

		float CameraDot = 0.0f;
		float PlayerDot = 0.0f;
		const bool bPassedGates = PassesCameraAndPlayerGates(
			TraceStart,
			CameraForward,
			Pair.Value,
			MinGroundLookAtDot,
			CameraDot,
			PlayerDot);

		if (DebugManager)
		{
			DebugManager->DrawAimCandidate(Pair.Value, CameraDot, bPassedGates, false);
		}

		if (!bPassedGates)
		{
			continue;
		}

		float ScoreDot = CameraDot;
		if (Pair.Key == CurrentItemID)
		{
			ScoreDot += CurrentFocusDotBonus;
		}

		const float ProjectionDelta = FMath::Abs(ProjectionDistance - HitProjectionDistance);
		const bool bBetterDot = ScoreDot > BestScoreDot + KINDA_SMALL_NUMBER;
		const bool bTieButCloser =
			FMath::Abs(ScoreDot - BestScoreDot) <= DotTieTolerance
			&& ProjectionDelta < BestProjectionDelta;

		if (bBetterDot || bTieButCloser)
		{
			BestScoreDot = ScoreDot;
			BestProjectionDelta = ProjectionDelta;
			BestItemID = Pair.Key;
			BestLocation = Pair.Value;
		}
	}

	if (DebugManager && BestItemID != INDEX_NONE)
	{
		DebugManager->DrawAimCandidate(BestLocation, BestScoreDot, true, true);
	}

	return BestItemID;
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

FVector FInteractionTraceManager::GetTraceEndLocation(const FVector& CameraLocation, const FRotator& CameraRotation) const
{
	return CameraLocation + CameraRotation.Vector() * InteractionDistance;
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

bool FInteractionTraceManager::PerformAimTrace(const FVector& Start, const FVector& End, FHitResult& OutHit)
{
	if (TraceSphereRadius <= 0.0f)
	{
		return PerformLineTrace(Start, End, OutHit);
	}

	if (!WorldContext)
	{
		return false;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);
	QueryParams.bTraceComplex = false;

	return WorldContext->SweepSingleByChannel(
		OutHit,
		Start,
		End,
		FQuat::Identity,
		InteractionTraceChannel,
		FCollisionShape::MakeSphere(TraceSphereRadius),
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

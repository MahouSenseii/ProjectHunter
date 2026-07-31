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
	  // 20Hz focus updates - 0.1 (10Hz) read as visibly steppy when sweeping
	  // the camera across interactables. Still timer-driven, still cheap.
	  , CheckFrequency(0.05f)
	  , IdleCheckFrequency(0.20f)
	  , InteractionTraceChannel(ECC_Visibility)
	  , OverlapRadius(75.0f)
	  , MinCameraForwardDot(0.25f)
	  , MinPlayerForwardDot(0.0f)
	  , CurrentFocusDotBonus(0.015f)
	  , MaxGroundItemCandidates(16)
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
	TArray<FGroundItemInteractionCandidate> IgnoredGroundItemCandidates;
	bool bIgnoredHasProximityCandidates = false;
	FindBestInteractionTarget(
		TScriptInterface<IInteractable>(),
		INDEX_NONE,
		Result,
		IgnoredItemID,
		IgnoredGroundItemCandidates,
		bIgnoredHasProximityCandidates);
	return Result;
}

void FInteractionTraceManager::FindBestInteractionTarget(
	const TScriptInterface<IInteractable>& CurrentInteractable,
	int32 CurrentItemID,
	TScriptInterface<IInteractable>& OutInteractable,
	int32& OutGroundItemID,
	TArray<FGroundItemInteractionCandidate>& OutGroundItemCandidates,
	bool& bOutHasProximityCandidates)
{
	OutInteractable = TScriptInterface<IInteractable>();
	OutGroundItemID = INDEX_NONE;
	OutGroundItemCandidates.Reset();
	bOutHasProximityCandidates = false;

	if (!WorldContext) return;

	FVector CameraLocation;
	FRotator CameraRotation;
	if (!GetCameraViewPoint(CameraLocation, CameraRotation)) return;

	const FVector TraceStart = GetTraceStartLocation(CameraLocation, CameraRotation);
	const FVector CameraForward = CameraRotation.Vector().GetSafeNormal();
	if (CameraForward.IsNearlyZero()) return;

	const FVector PlayerCenter = OwnerActor ? OwnerActor->GetActorLocation() : TraceStart;
	const FVector PlayerForward = OwnerActor
		? OwnerActor->GetActorForwardVector().GetSafeNormal()
		: CameraForward;

	if (DebugManager)
	{
		DebugManager->DrawGroundItemSearchVolume(PlayerCenter, InteractionDistance);
		DebugManager->DrawSelectionDirections(
			PlayerCenter,
			PlayerForward,
			TraceStart,
			CameraForward,
			FMath::Min(InteractionDistance, 180.0f));
	}

	const float SafeInteractionDistance = FMath::Max(InteractionDistance, 1.0f);
	auto CalculateScore =
		[&](const FVector& TargetLocation, bool bIsCurrent,
			float& OutCameraDot, float& OutPlayerDot, float& OutDistance,
			float& OutScore)
		{
			const FVector PlayerToTarget = TargetLocation - PlayerCenter;
			OutDistance = PlayerToTarget.Size();
			if (OutDistance > SafeInteractionDistance)
			{
				return false;
			}

			const FVector PlayerDirection = OutDistance > KINDA_SMALL_NUMBER
				? PlayerToTarget / OutDistance
				: PlayerForward;
			OutPlayerDot = FVector::DotProduct(PlayerForward, PlayerDirection);
			if (OutPlayerDot < MinPlayerForwardDot)
			{
				return false;
			}

			const FVector CameraDirection = (TargetLocation - TraceStart).GetSafeNormal();
			OutCameraDot = CameraDirection.IsNearlyZero()
				? 1.0f
				: FVector::DotProduct(CameraForward, CameraDirection);
			if (OutCameraDot < MinCameraForwardDot)
			{
				return false;
			}

			// Player direction is only a front/back gate. Camera alignment
			// exclusively ranks every candidate that passes that gate.
			OutScore = OutCameraDot;

			if (bIsCurrent)
			{
				OutScore += CurrentFocusDotBonus;
			}

			return true;
		};

	// Actor interactables use one player-centered physics overlap.
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams OverlapParams;
	OverlapParams.AddIgnoredActor(OwnerActor);

	WorldContext->OverlapMultiByChannel(
		Overlaps,
		PlayerCenter,
		FQuat::Identity,
		InteractionTraceChannel,
		FCollisionShape::MakeSphere(SafeInteractionDistance),
		OverlapParams
	);

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

	float BestScore = -BIG_NUMBER;
	AActor* BestActor = nullptr;
	TSet<AActor*> ScoredActors;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OverlapActor = Overlap.GetActor();
		if (!IsActorInteractable(OverlapActor) || ScoredActors.Contains(OverlapActor)) continue;
		ScoredActors.Add(OverlapActor);
		bOutHasProximityCandidates = true;

		float CameraDot = -1.0f;
		float PlayerDot = -1.0f;
		float Distance = 0.0f;
		float Score = -BIG_NUMBER;
		if (!CalculateScore(
			OverlapActor->GetActorLocation(),
			OverlapActor == CurrentFocusActor,
			CameraDot,
			PlayerDot,
			Distance,
			Score))
		{
			continue;
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			BestActor = OverlapActor;
		}
	}

	// Ground items already have IDs and saved locations, so they need no
	// collision components or per-item actors.
	if (CachedGroundItemSubsystem)
	{
		TArray<int32> NearbyItemIDs;
		CachedGroundItemSubsystem->GetItemsInRadius(
			PlayerCenter,
			SafeInteractionDistance,
			NearbyItemIDs);
		bOutHasProximityCandidates |= !NearbyItemIDs.IsEmpty();

		const TMap<int32, FVector>& Locations = CachedGroundItemSubsystem->GetInstanceLocations();
		for (int32 ItemID : NearbyItemIDs)
		{
			const FVector* ItemLocation = Locations.Find(ItemID);
			if (!ItemLocation)
			{
				continue;
			}

			float CameraDot = -1.0f;
			float PlayerDot = -1.0f;
			float Distance = 0.0f;
			float Score = -BIG_NUMBER;
			if (!CalculateScore(
				*ItemLocation,
				ItemID == CurrentItemID,
				CameraDot,
				PlayerDot,
				Distance,
				Score))
			{
				continue;
			}

			FGroundItemInteractionCandidate& Candidate = OutGroundItemCandidates.AddDefaulted_GetRef();
			Candidate.ItemID = ItemID;
			Candidate.Score = Score;
			Candidate.Distance = Distance;
			Candidate.WorldLocation = *ItemLocation;
			Candidate.CameraForwardDot = CameraDot;
			Candidate.PlayerForwardDot = PlayerDot;
			Candidate.FocusBonus = ItemID == CurrentItemID ? CurrentFocusDotBonus : 0.0f;
		}

		OutGroundItemCandidates.Sort(
			[](const FGroundItemInteractionCandidate& A, const FGroundItemInteractionCandidate& B)
			{
				if (!FMath::IsNearlyEqual(A.Score, B.Score))
				{
					return A.Score > B.Score;
				}
				return A.ItemID < B.ItemID;
			});

		if (OutGroundItemCandidates.Num() > MaxGroundItemCandidates)
		{
			OutGroundItemCandidates.SetNum(MaxGroundItemCandidates, EAllowShrinking::No);
		}
	}

	int32 BestItemID = INDEX_NONE;
	if (!OutGroundItemCandidates.IsEmpty() && OutGroundItemCandidates[0].Score > BestScore)
	{
		const FGroundItemInteractionCandidate& BestItem = OutGroundItemCandidates[0];
		BestScore = BestItem.Score;
		BestActor = nullptr;
		BestItemID = BestItem.ItemID;
	}

	LastTraceResult = FHitResult();
	if (BestActor)
	{
		OutInteractable = MakeInteractableInterface(BestActor);
	}
	else if (BestItemID != INDEX_NONE)
	{
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

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
	  , NearFieldBypassRadius(150.0f)
	  , AimRadius(120.0f)
	  , DistanceWeight(0.35f)
	  , CurrentFocusScoreBonus(0.05f)
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

void FInteractionTraceManager::InvalidateCandidateCache()
{
	CachedActorCandidates.Reset();
	CachedGroundItemIDs.Reset();
	bHasGatheredCandidates = false;
}

void FInteractionTraceManager::GatherCandidates(const FVector& PlayerCenter, float SearchRadius)
{
	CachedActorCandidates.Reset();
	CachedGroundItemIDs.Reset();
	bHasGatheredCandidates = true;

	if (!WorldContext)
	{
		return;
	}

	// Actor interactables use one player-centered physics overlap.
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams OverlapParams;
	OverlapParams.AddIgnoredActor(OwnerActor);

	WorldContext->OverlapMultiByChannel(
		Overlaps,
		PlayerCenter,
		FQuat::Identity,
		InteractionTraceChannel,
		FCollisionShape::MakeSphere(SearchRadius),
		OverlapParams
	);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OverlapActor = Overlap.GetActor();
		if (IsActorInteractable(OverlapActor))
		{
			CachedActorCandidates.AddUnique(TWeakObjectPtr<AActor>(OverlapActor));
		}
	}

	// Ground items already have IDs and saved locations, so they need no
	// collision components or per-item actors.
	if (CachedGroundItemSubsystem)
	{
		CachedGroundItemSubsystem->GetItemsInRadius(PlayerCenter, SearchRadius, CachedGroundItemIDs);
	}
}

FVector FInteractionTraceManager::GetActorScoreLocation(
	const AActor* Actor, const FVector& RayOrigin, const FVector& RayDirection) const
{
	if (!Actor)
	{
		return RayOrigin;
	}

	FVector BoundsOrigin;
	FVector BoundsExtent;
	Actor->GetActorBounds(/*bOnlyCollidingComponents=*/true, BoundsOrigin, BoundsExtent);

	if (BoundsExtent.IsNearlyZero())
	{
		return Actor->GetActorLocation();
	}

	// Score a large interactable where the player is actually looking rather
	// than at its pivot, which for a chest sits on the floor at its centre.
	const float AlongRay = FMath::Max(
		FVector::DotProduct(BoundsOrigin - RayOrigin, RayDirection), 0.0f);
	const FVector NearestRayPoint = RayOrigin + RayDirection * AlongRay;

	return FBox(BoundsOrigin - BoundsExtent, BoundsOrigin + BoundsExtent)
		.GetClosestPointTo(NearestRayPoint);
}

void FInteractionTraceManager::FindBestInteractionTarget(
	const TScriptInterface<IInteractable>& CurrentInteractable,
	int32 CurrentItemID,
	TScriptInterface<IInteractable>& OutInteractable,
	int32& OutGroundItemID,
	TArray<FGroundItemInteractionCandidate>& OutGroundItemCandidates,
	bool& bOutHasProximityCandidates,
	bool bRefreshCandidates)
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

	const float SafeInteractionDistance = FMath::Max(InteractionDistance, 1.0f);

	if (bRefreshCandidates || !bHasGatheredCandidates)
	{
		GatherCandidates(PlayerCenter, SafeInteractionDistance);
	}

	// Only on the query pass: scoring runs every frame, and drawing these there
	// would queue a full set of debug shapes per rendered frame.
	if (DebugManager && bRefreshCandidates)
	{
		DebugManager->DrawGroundItemSearchVolume(PlayerCenter, InteractionDistance);
		DebugManager->DrawSelectionDirections(
			PlayerCenter,
			PlayerForward,
			TraceStart,
			CameraForward,
			FMath::Min(InteractionDistance, 180.0f));
	}

	const float SafeAimRadius = FMath::Max(AimRadius, 1.0f);

	// Returns the base score only. The current-focus bonus is applied by the
	// caller so it can never reorder the candidate list - manual cycling walks
	// that list by index, and an order that shuffled as focus moved would make
	// cycling through a stack of overlapping items skip and repeat entries.
	auto CalculateScore =
		[&](const FVector& TargetLocation,
			float& OutCameraDot, float& OutPlayerDot, float& OutDistance,
			float& OutAimOffset, float& OutScore)
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

			const FVector CameraToTarget = TargetLocation - TraceStart;
			const float CameraRange = CameraToTarget.Size();
			const float AlongRay = FVector::DotProduct(CameraForward, CameraToTarget);
			OutCameraDot = CameraRange > KINDA_SMALL_NUMBER ? AlongRay / CameraRange : 1.0f;

			// Clamping to the forward half of the ray means anything behind the
			// camera measures its offset from the camera itself, so it scores
			// badly instead of scoring perfectly for sitting on the back axis.
			OutAimOffset = (CameraToTarget - CameraForward * FMath::Max(AlongRay, 0.0f)).Size();

			// Targets the player is practically standing on skip both gates.
			// Down there the player-forward dot flips sign on a light strafe and
			// the camera dot collapses whenever camera collision pulls the camera
			// in close - gating on either makes items at the feet flicker or
			// disappear. Scoring still ranks them, so this only makes them
			// reachable, never preferred.
			if (OutDistance > NearFieldBypassRadius)
			{
				if (OutPlayerDot < MinPlayerForwardDot || OutCameraDot < MinCameraForwardDot)
				{
					return false;
				}
			}

			// Rank on perpendicular distance from the aim ray in cm, not on the
			// angle to it: a fixed angular tolerance covers almost no world space
			// near the camera and a lot of it far away, which is why targets at
			// the player's feet used to lose to targets across the room.
			OutScore = 1.0f
				- FMath::Min(OutAimOffset / SafeAimRadius, 4.0f)
				- DistanceWeight * (OutDistance / SafeInteractionDistance);

			return true;
		};

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

	float BestActorScore = -BIG_NUMBER;
	AActor* BestActor = nullptr;

	for (const TWeakObjectPtr<AActor>& WeakActor : CachedActorCandidates)
	{
		AActor* CandidateActor = WeakActor.Get();
		if (!IsValid(CandidateActor))
		{
			continue;
		}

		bOutHasProximityCandidates = true;

		float CameraDot = -1.0f;
		float PlayerDot = -1.0f;
		float Distance = 0.0f;
		float AimOffset = 0.0f;
		float Score = -BIG_NUMBER;
		if (!CalculateScore(
			GetActorScoreLocation(CandidateActor, TraceStart, CameraForward),
			CameraDot,
			PlayerDot,
			Distance,
			AimOffset,
			Score))
		{
			continue;
		}

		if (CandidateActor == CurrentFocusActor)
		{
			Score += CurrentFocusScoreBonus;
		}

		if (Score > BestActorScore)
		{
			BestActorScore = Score;
			BestActor = CandidateActor;
		}
	}

	if (CachedGroundItemSubsystem && !CachedGroundItemIDs.IsEmpty())
	{
		bOutHasProximityCandidates = true;

		const TMap<int32, FVector>& Locations = CachedGroundItemSubsystem->GetInstanceLocations();
		for (int32 ItemID : CachedGroundItemIDs)
		{
			const FVector* ItemLocation = Locations.Find(ItemID);
			if (!ItemLocation)
			{
				continue;
			}

			float CameraDot = -1.0f;
			float PlayerDot = -1.0f;
			float Distance = 0.0f;
			float AimOffset = 0.0f;
			float Score = -BIG_NUMBER;
			if (!CalculateScore(
				*ItemLocation,
				CameraDot,
				PlayerDot,
				Distance,
				AimOffset,
				Score))
			{
				continue;
			}

			FGroundItemInteractionCandidate& Candidate = OutGroundItemCandidates.AddDefaulted_GetRef();
			Candidate.ItemID = ItemID;
			Candidate.Score = Score;
			Candidate.Distance = Distance;
			Candidate.AimOffset = AimOffset;
			Candidate.WorldLocation = *ItemLocation;
			Candidate.CameraForwardDot = CameraDot;
			Candidate.PlayerForwardDot = PlayerDot;
			Candidate.FocusBonus = ItemID == CurrentItemID ? CurrentFocusScoreBonus : 0.0f;
		}

		// Ordered by base score, so a stack of items sitting on top of each other
		// keeps one stable order to cycle through: identical scores fall through
		// to the ItemID tie-break rather than swapping around at random.
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

	// Picked after truncation so the automatic winner is always a candidate the
	// InteractionManager can still find by index.
	int32 BestItemID = INDEX_NONE;
	float BestItemScore = -BIG_NUMBER;
	for (const FGroundItemInteractionCandidate& Candidate : OutGroundItemCandidates)
	{
		const float EffectiveScore = Candidate.Score + Candidate.FocusBonus;
		if (EffectiveScore > BestItemScore)
		{
			BestItemScore = EffectiveScore;
			BestItemID = Candidate.ItemID;
		}
	}

	LastTraceResult = FHitResult();
	if (BestItemID != INDEX_NONE && BestItemScore > BestActorScore)
	{
		OutGroundItemID = BestItemID;
	}
	else if (BestActor)
	{
		OutInteractable = MakeInteractableInterface(BestActor);
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

#include "Character/Components/PHCharacterMovementComponent.h"

#include "Character/PHBaseCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float WallSurfaceNormalInterpSpeed = 24.0f;
	constexpr float WallToGroundReattachDelay = 0.75f;
	constexpr float GroundExitFlatNormalZ = 0.98f;
}

UPHCharacterMovementComponent::UPHCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UPHCharacterMovementComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPHCharacterMovementComponent, WallNormal);
	DOREPLIFETIME(UPHCharacterMovementComponent, WallUpDirection);
	DOREPLIFETIME(UPHCharacterMovementComponent, WallTransitionData);
}

bool UPHCharacterMovementComponent::TryStartWallTraversal(
	const EALSMovementState RequestedState,
	const FHitResult* PreferredWallHit)
{
	if (!CharacterOwner || IsWallTraversing())
	{
		return false;
	}

	if (RequestedState != EALSMovementState::WallRunning &&
		RequestedState != EALSMovementState::WallClimbing)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (World && World->GetTimeSeconds() - LastWallDetachTime < WallReattachCooldown)
	{
		return false;
	}

	FHitResult WallHit;
	if (PreferredWallHit && IsWallSurface(*PreferredWallHit, true))
	{
		WallHit = *PreferredWallHit;
	}
	else if (!FindAttachableWall(WallHit, true))
	{
		return false;
	}

	// Only grab the wall when the character is actually heading toward or running
	// along it. Sprinting past, away from, or off a wall must not snap back on.
	if (!IsApproachingWall(WallHit))
	{
		return false;
	}

	UpdateWallSurface(WallHit, true);
	WallTransitionData = FALSWallTransitionData();

	const EPHCustomMovementMode NewMode =
		RequestedState == EALSMovementState::WallRunning
			? EPHCustomMovementMode::WallRunning
			: EPHCustomMovementMode::WallClimbing;

	Velocity = FVector::VectorPlaneProject(Velocity, WallNormal);
	WallLostFrames = 0;
	WallTraversalElapsed = 0.0f;
	SetMovementMode(MOVE_Custom, static_cast<uint8>(NewMode));
	SnapToWall(WallHit, bAlignCapsuleToWall && bSnapRotationOnAttach);
	CharacterOwner->ForceNetUpdate();
	return true;
}

void UPHCharacterMovementComponent::StopWallTraversal()
{
	if (!IsWallTraversing())
	{
		return;
	}

	RestoreWorldUpRotation();
	WallTransitionData = FALSWallTransitionData();
	WallSurfaceComponent.Reset();
	RecordWallDetachTime();
	SetMovementMode(MOVE_Falling);
	CharacterOwner->ForceNetUpdate();
}

void UPHCharacterMovementComponent::JumpOffWall()
{
	if (!IsWallTraversing())
	{
		return;
	}

	const FVector JumpNormal = WallNormal;
	RestoreWorldUpRotation();
	WallTransitionData = FALSWallTransitionData();
	WallSurfaceComponent.Reset();
	RecordWallDetachTime();
	SetMovementMode(MOVE_Falling);
	Velocity = JumpNormal * WallJumpAwaySpeed + FVector::UpVector * WallJumpUpSpeed;
	CharacterOwner->ForceNetUpdate();
}

bool UPHCharacterMovementComponent::IsWallTraversing() const
{
	if (MovementMode != MOVE_Custom)
	{
		return false;
	}

	const EPHCustomMovementMode Mode = static_cast<EPHCustomMovementMode>(CustomMovementMode);
	return Mode == EPHCustomMovementMode::WallRunning ||
		Mode == EPHCustomMovementMode::WallClimbing ||
		Mode == EPHCustomMovementMode::WallToGround;
}

bool UPHCharacterMovementComponent::IsWallRunning() const
{
	return MovementMode == MOVE_Custom &&
		CustomMovementMode == static_cast<uint8>(EPHCustomMovementMode::WallRunning);
}

bool UPHCharacterMovementComponent::IsWallClimbing() const
{
	return MovementMode == MOVE_Custom &&
		CustomMovementMode == static_cast<uint8>(EPHCustomMovementMode::WallClimbing);
}

bool UPHCharacterMovementComponent::IsTransitioningFromWallToGround() const
{
	return MovementMode == MOVE_Custom &&
		CustomMovementMode == static_cast<uint8>(EPHCustomMovementMode::WallToGround);
}

FVector UPHCharacterMovementComponent::GetWallUp() const
{
	const FVector TransportedUp =
		FVector::VectorPlaneProject(WallUpDirection, WallNormal).GetSafeNormal();
	if (!TransportedUp.IsNearlyZero())
	{
		return TransportedUp;
	}

	const FVector WorldUpOnSurface =
		FVector::VectorPlaneProject(FVector::UpVector, WallNormal).GetSafeNormal();
	if (!WorldUpOnSurface.IsNearlyZero())
	{
		return WorldUpOnSurface;
	}

	const FVector VelocityOnSurface =
		FVector::VectorPlaneProject(Velocity, WallNormal).GetSafeNormal();
	return VelocityOnSurface.IsNearlyZero()
		? FVector::ForwardVector
		: VelocityOnSurface;
}

FVector UPHCharacterMovementComponent::GetWallRight() const
{
	FVector Result = FVector::CrossProduct(WallNormal, GetWallUp()).GetSafeNormal();
	if (Result.IsNearlyZero() && UpdatedComponent)
	{
		Result = FVector::VectorPlaneProject(
			UpdatedComponent->GetRightVector(),
			WallNormal).GetSafeNormal();
	}
	return Result;
}

void UPHCharacterMovementComponent::RestoreWorldUpRotation()
{
	if (!UpdatedComponent)
	{
		return;
	}

	FHitResult RotationHit;
	SafeMoveUpdatedComponent(
		FVector::ZeroVector,
		GetWorldUpRotation(),
		true,
		RotationHit);
}

FVector UPHCharacterMovementComponent::ConvertWorldDirectionToWallDirection(
	const FVector& WorldDirection) const
{
	if (!IsWallTraversing() || WorldDirection.IsNearlyZero() || WallNormal.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	// ALS ground movement uses camera yaw, so ignore world Z here as well.
	const FVector HorizontalDirection = WorldDirection.GetSafeNormal2D();
	if (HorizontalDirection.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	const FVector WallUp = GetWallUp();
	const FVector WallRight = GetWallRight();

	// On a vertical wall, pressing toward the surface means travel along the
	// transported wall-up tangent. On the top or underside of an arc, camera
	// direction already lies in the surface plane and behaves like floor input.
	const float LateralInput = FVector::DotProduct(HorizontalDirection, WallRight);
	const float SurfaceForwardInput = FVector::DotProduct(HorizontalDirection, WallUp);
	const float IntoSurfaceInput = FVector::DotProduct(HorizontalDirection, -WallNormal);
	// After crossing the top of an arc, transported wall-up points downward.
	// Camera-forward then points away from the far side of the arc, which used
	// to negate the tangent and send the character back uphill. Preserve the
	// forward tangent on that descending side; the input value still controls
	// forward versus backward movement.
	const float SurfaceFacingInput = WallUp.Z < -KINDA_SMALL_NUMBER
		? FMath::Abs(IntoSurfaceInput)
		: IntoSurfaceInput;
	const float VerticalInput = SurfaceForwardInput + SurfaceFacingInput;

	return (WallRight * LateralInput + WallUp * VerticalInput).GetSafeNormal();
}

float UPHCharacterMovementComponent::GetMaxSpeed() const
{
	return IsWallTraversing() ? GetWallTraversalSpeed() : Super::GetMaxSpeed();
}

float UPHCharacterMovementComponent::GetMaxAcceleration() const
{
	return IsWallTraversing() ? WallAcceleration : Super::GetMaxAcceleration();
}

float UPHCharacterMovementComponent::GetMaxBrakingDeceleration() const
{
	return IsWallTraversing() ? WallBrakingDeceleration : Super::GetMaxBrakingDeceleration();
}

void UPHCharacterMovementComponent::HandleImpact(
	const FHitResult& Hit,
	const float TimeSlice,
	const FVector& MoveDelta)
{
	Super::HandleImpact(Hit, TimeSlice, MoveDelta);

	const bool bCanAttachFromCurrentMode =
		MovementMode == MOVE_Falling ||
		MovementMode == MOVE_Walking ||
		MovementMode == MOVE_NavWalking;
	if (!bCanAttachFromCurrentMode || IsWallTraversing() || !CharacterOwner ||
		CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)
	{
		return;
	}

	APHBaseCharacter* PHCharacter = Cast<APHBaseCharacter>(CharacterOwner);
	if (!PHCharacter || !PHCharacter->WantsWallTraversal())
	{
		return;
	}

	FHitResult WallHit = Hit;
	if (!IsWallSurface(WallHit, true))
	{
		const FVector ImpactDirection =
			(WallHit.ImpactPoint - UpdatedComponent->GetComponentLocation()).GetSafeNormal();
		if (!TraceWall(ImpactDirection, WallHit, true))
		{
			return;
		}
	}

	const EALSMovementState RequestedState =
		PHCharacter->SelectWallTraversalState(PHCharacter->GetWallTraversalWeight());
	if (RequestedState == EALSMovementState::WallRunning ||
		RequestedState == EALSMovementState::WallClimbing)
	{
		TryStartWallTraversal(RequestedState, &WallHit);
	}
}

void UPHCharacterMovementComponent::PhysCustom(const float DeltaTime, const int32 Iterations)
{
	const EPHCustomMovementMode Mode = static_cast<EPHCustomMovementMode>(CustomMovementMode);
	if (Mode == EPHCustomMovementMode::WallRunning ||
		Mode == EPHCustomMovementMode::WallClimbing)
	{
		PhysWallTraversal(DeltaTime, Iterations);
		return;
	}
	if (Mode == EPHCustomMovementMode::WallToGround)
	{
		PhysWallToGroundTransition(DeltaTime, Iterations);
		return;
	}

	Super::PhysCustom(DeltaTime, Iterations);
}

void UPHCharacterMovementComponent::PhysWallTraversal(const float DeltaTime, const int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME || !CharacterOwner || !UpdatedComponent)
	{
		return;
	}

	FHitResult WallHit;
	TArray<FHitResult> WallHits;
	bool bHasWall =
		TraceWallSurfaces(-WallNormal, WallHits, false) &&
		BuildAveragedWallHit(WallHits, WallHit);
	if (!bHasWall)
	{
		// The wall normal may have rotated across a corner or curve, so the
		// straight-back trace can miss a wall that is still right beside us.
		// Widen the search before giving up.
		bHasWall = FindAttachableWall(WallHit, false);
	}

	if (bHasWall)
	{
		WallLostFrames = 0;
		UpdateWallSurface(WallHit, false);
	}
	else
	{
		// Tolerate brief losses (seams, corners, sphere-sweep normal noise)
		// before detaching. Coast along the last-known wall during the grace
		// window instead of dropping on the first missed frame.
		if (++WallLostFrames > MaxConsecutiveWallLostFrames)
		{
			HandleWallLost();
			return;
		}
	}

	RestorePreAdditiveRootMotionVelocity();

	Acceleration = FVector::VectorPlaneProject(Acceleration, WallNormal);
	Acceleration = Acceleration.GetClampedToMaxSize(GetMaxAcceleration());
	CalcVelocity(DeltaTime, WallMovementFriction, false, GetMaxBrakingDeceleration());
	Velocity = FVector::VectorPlaneProject(Velocity, WallNormal);
	Velocity = Velocity.GetClampedToMaxSize(GetMaxSpeed());

	WallTraversalElapsed += DeltaTime;

	const bool bMovingTowardGround =
		FVector::DotProduct(Velocity, FVector::UpVector) < -KINDA_SMALL_NUMBER ||
		FVector::DotProduct(Acceleration, FVector::UpVector) < -KINDA_SMALL_NUMBER;

	// Drop to walking when a walkable floor is directly underfoot. During a vertical
	// climb the surface below is the wall (not walkable), so this stays inactive
	// while climbing and fires when the character crests onto a walkable top surface
	// (the top-of-wall case) or is grazing the ground. The post-attach grace lets a
	// ground launch clear the floor first so it doesn't immediately pull them back.
	if (WallTraversalElapsed >= WallGroundExitGraceTime)
	{
		FHitResult StandingGroundHit;
		if (IsStandingOnWalkableFloor(StandingGroundHit))
		{
			BeginWallToGroundTransition(StandingGroundHit);
			return;
		}
	}

	// Begin before collision when descending toward a floor that is still a little
	// below. This gives animation time to transfer one foot instead of snapping
	// both at impact.
	FHitResult NearbyGroundHit;
	if (bMovingTowardGround && FindTransitionGround(NearbyGroundHit))
	{
		BeginWallToGroundTransition(NearbyGroundHit);
		return;
	}

	// Movement remains tangent to the surface. Surface attachment is handled by
	// the post-move probe and snap below; also pushing into the surface here
	// makes collision sliding and snapping fight each other and causes jitter.
	const FVector MoveDelta = Velocity * DeltaTime;

	FQuat NewRotation = UpdatedComponent->GetComponentQuat();
	if (bAlignCapsuleToWall)
	{
		const FQuat TargetRotation = GetWallTraversalRotation();
		const float RotationAlpha =
			1.0f - FMath::Exp(-WallRotationInterpSpeed * DeltaTime);
		NewRotation = FQuat::Slerp(NewRotation, TargetRotation, RotationAlpha).GetNormalized();
	}

	FHitResult MoveHit;
	SafeMoveUpdatedComponent(MoveDelta, NewRotation, true, MoveHit);
	if (MoveHit.IsValidBlockingHit())
	{
		// A nearby floor may overlap the capsule's movement while traveling up
		// or sideways along a wall. Only downward travel may switch to ground.
		if (bMovingTowardGround && ShouldTransitionToGround(MoveHit))
		{
			BeginWallToGroundTransition(MoveHit);
			return;
		}

		SlideAlongSurface(MoveDelta, 1.0f - MoveHit.Time, MoveHit.Normal, MoveHit, true);
	}

	FHitResult PostMoveWallHit;
	TArray<FHitResult> PostMoveWallHits;
	if (TraceWallSurfaces(-WallNormal, PostMoveWallHits, false) &&
		BuildAveragedWallHit(PostMoveWallHits, PostMoveWallHit))
	{
		UpdateWallSurface(PostMoveWallHit, false);
		SnapToWall(PostMoveWallHit);
	}
	else if (bHasWall)
	{
		// Keep the last valid contact during the short seam/corner grace window.
		SnapToWall(WallHit);
	}
	Velocity = FVector::VectorPlaneProject(Velocity, WallNormal);
}

void UPHCharacterMovementComponent::PhysWallToGroundTransition(
	const float DeltaTime,
	const int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME || !CharacterOwner || !UpdatedComponent ||
		!WallTransitionData.bActive)
	{
		CompleteWallToGroundTransition();
		return;
	}

	WallToGroundElapsed += DeltaTime;
	const float RawAlpha = FMath::Clamp(
		WallToGroundElapsed / FMath::Max(WallTransitionData.Duration, MIN_TICK_TIME),
		0.0f,
		1.0f);
	const float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, RawAlpha);

	WallToGroundTargetLocation +=
		WallToGroundPlanarVelocity * WallToGroundMovementScale * DeltaTime;
	const FVector DesiredLocation = FMath::Lerp(
		WallToGroundStartLocation,
		WallToGroundTargetLocation,
		SmoothAlpha);
	const FQuat DesiredRotation = FQuat::Slerp(
		WallToGroundStartRotation,
		WallToGroundTargetRotation,
		SmoothAlpha).GetNormalized();

	const FVector MoveDelta = DesiredLocation - UpdatedComponent->GetComponentLocation();
	FHitResult MoveHit;
	SafeMoveUpdatedComponent(MoveDelta, DesiredRotation, true, MoveHit);
	if (MoveHit.IsValidBlockingHit() && !IsWalkable(MoveHit))
	{
		SlideAlongSurface(
			MoveDelta,
			1.0f - MoveHit.Time,
			MoveHit.Normal,
			MoveHit,
			true);
	}

	Velocity = FMath::Lerp(
		FVector::VectorPlaneProject(Velocity, WallNormal),
		WallToGroundPlanarVelocity,
		SmoothAlpha);

	if (RawAlpha >= 1.0f)
	{
		CompleteWallToGroundTransition();
	}
}

bool UPHCharacterMovementComponent::FindAttachableWall(
	FHitResult& OutHit,
	const bool bRequireInitialWall) const
{
	if (!CharacterOwner)
	{
		return false;
	}

	TArray<FVector, TInlineAllocator<20>> CandidateDirections;
	const auto AddDirection = [&CandidateDirections](const FVector& Direction)
	{
		const FVector NormalizedDirection = Direction.GetSafeNormal();
		if (NormalizedDirection.IsNearlyZero())
		{
			return;
		}

		for (const FVector& ExistingDirection : CandidateDirections)
		{
			if (FVector::DotProduct(ExistingDirection, NormalizedDirection) > 0.98f)
			{
				return;
			}
		}

		CandidateDirections.Add(NormalizedDirection);
	};

	const FVector InputDirection = bRequireInitialWall
		? CharacterOwner->GetLastMovementInputVector().GetSafeNormal2D()
		: CharacterOwner->GetLastMovementInputVector().GetSafeNormal();
	AddDirection(InputDirection);

	const FVector AccelerationDirection = bRequireInitialWall
		? Acceleration.GetSafeNormal2D()
		: Acceleration.GetSafeNormal();
	AddDirection(AccelerationDirection);

	const FVector VelocityDirection = Velocity.GetSafeNormal();
	AddDirection(bRequireInitialWall
		? VelocityDirection.GetSafeNormal2D()
		: VelocityDirection);

	if (!bRequireInitialWall && !WallNormal.IsNearlyZero())
	{
		const FVector WallProbe = -FVector(WallNormal);
		const FVector WallUp = GetWallUp();
		const FVector WallRight = GetWallRight();
		AddDirection(WallProbe);
		AddDirection(WallUp);
		AddDirection(-WallUp);
		AddDirection(WallRight);
		AddDirection(-WallRight);
		AddDirection(WallProbe + WallUp);
		AddDirection(WallProbe - WallUp);
		AddDirection(WallProbe + WallRight);
		AddDirection(WallProbe - WallRight);
	}

	// The 8-direction actor-relative fan is only needed to acquire a wall from
	// scratch. During traversal we already have the wall-basis probes above, so
	// skipping this fan roughly halves the sweep count on the per-frame recovery
	// path without losing coverage.
	if (bRequireInitialWall)
	{
		const FVector ActorForward = CharacterOwner->GetActorForwardVector().GetSafeNormal2D();
		const FVector ActorRight = CharacterOwner->GetActorRightVector().GetSafeNormal2D();
		AddDirection(ActorForward);
		AddDirection(-ActorForward);
		AddDirection(ActorRight);
		AddDirection(-ActorRight);
		AddDirection(ActorForward + ActorRight);
		AddDirection(ActorForward - ActorRight);
		AddDirection(-ActorForward + ActorRight);
		AddDirection(-ActorForward - ActorRight);
	}

	bool bFoundWall = false;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();

	for (const FVector& Direction : CandidateDirections)
	{
		TArray<FHitResult> CandidateHits;
		FHitResult CandidateHit;
		if (!TraceWallSurfaces(Direction, CandidateHits, bRequireInitialWall) ||
			!BuildAveragedWallHit(CandidateHits, CandidateHit))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(
			UpdatedComponent->GetComponentLocation(),
			CandidateHit.ImpactPoint);
		if (!bFoundWall || DistanceSquared < ClosestDistanceSquared)
		{
			bFoundWall = true;
			ClosestDistanceSquared = DistanceSquared;
			OutHit = CandidateHit;
		}
	}

	return bFoundWall;
}

bool UPHCharacterMovementComponent::FindTransitionGround(FHitResult& OutHit) const
{
	if (!GetWorld() || !CharacterOwner || !UpdatedComponent)
	{
		return false;
	}

	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	if (!Capsule)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(PHWallToGroundTransition),
		false,
		CharacterOwner);
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const float CurrentGroundSupport =
		GetCapsuleSupportDistance(FVector::UpVector, UpdatedComponent->GetUpVector());
	const FVector End =
		Start - FVector::UpVector * (CurrentGroundSupport + GroundTransitionDetectionDistance);
	const float ProbeRadius = FMath::Max(Capsule->GetScaledCapsuleRadius() * 0.5f, 5.0f);

	const bool bHit = GetWorld()->SweepSingleByChannel(
		OutHit,
		Start,
		End,
		FQuat::Identity,
		WallDetectionChannel,
		FCollisionShape::MakeSphere(ProbeRadius),
		QueryParams);

	return bHit && ShouldTransitionToGround(OutHit);
}

bool UPHCharacterMovementComponent::IsStandingOnWalkableFloor(FHitResult& OutGroundHit) const
{
	if (!GetWorld() || !CharacterOwner || !UpdatedComponent)
	{
		return false;
	}

	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	if (!Capsule)
	{
		return false;
	}

	// Tight downward probe: only a floor within the capsule's own support distance
	// (plus a small margin) counts as actually standing on the ground. This is
	// independent of travel direction, so a floor beneath the player ends/blocks a
	// wall run instead of letting it persist at ground level.
	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(PHWallGroundedCheck),
		false,
		CharacterOwner);
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const float StandingSupport =
		GetCapsuleSupportDistance(FVector::UpVector, UpdatedComponent->GetUpVector());
	const FVector End =
		Start - FVector::UpVector * (StandingSupport + GroundedContactMargin);
	const float ProbeRadius = FMath::Max(Capsule->GetScaledCapsuleRadius() * 0.5f, 5.0f);

	const bool bHit = GetWorld()->SweepSingleByChannel(
		OutGroundHit,
		Start,
		End,
		FQuat::Identity,
		WallDetectionChannel,
		FCollisionShape::MakeSphere(ProbeRadius),
		QueryParams);

	return bHit && ShouldTransitionToGround(OutGroundHit);
}

bool UPHCharacterMovementComponent::TraceWall(
	const FVector& Direction,
	FHitResult& OutHit,
	const bool bRequireInitialWall) const
{
	TArray<FHitResult> WallHits;
	return TraceWallSurfaces(Direction, WallHits, bRequireInitialWall) &&
		BuildAveragedWallHit(WallHits, OutHit);
}

bool UPHCharacterMovementComponent::TraceWallSurfaces(
	const FVector& Direction,
	TArray<FHitResult>& OutHits,
	const bool bRequireInitialWall) const
{
	if (!GetWorld() || !CharacterOwner || !UpdatedComponent || Direction.IsNearlyZero())
	{
		return false;
	}

	OutHits.Reset();

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PHWallTraversal), false, CharacterOwner);
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector TraceDirection = Direction.GetSafeNormal();
	const FVector End = Start + TraceDirection * (GetDesiredWallDistance() + WallDetectionReach);

	TArray<FHitResult> Hits;
	const bool bHit = GetWorld()->SweepMultiByChannel(
		Hits,
		Start,
		End,
		FQuat::Identity,
		WallDetectionChannel,
		FCollisionShape::MakeSphere(WallTraceRadius),
		QueryParams);

	if (!bHit)
	{
		return false;
	}

	TArray<FHitResult> UsableHits;
	float ClosestHitTime = TNumericLimits<float>::Max();
	const FVector CurrentWallNormal = FVector(WallNormal).GetSafeNormal();

	// Iterate by reference: refining initial-attach normals mutates the element
	// in place and avoids copying a full FHitResult every loop iteration.
	for (FHitResult& CandidateHit : Hits)
	{
		if (!CandidateHit.IsValidBlockingHit())
		{
			continue;
		}

		// Sphere sweeps return rounded/averaged normals on edges and corners,
		// which can spike the normal's Z and make a valid wall fail
		// IsWallSurface. Refine initial attachment only. During traversal, the
		// sweep normal is the stable contact normal required to follow a curved
		// surface; replacing it with triangle face normals produces visible jitter
		// on an arc.
		if (bRequireInitialWall)
		{
			FHitResult FaceHit;
			const FVector FaceEnd =
				CandidateHit.ImpactPoint + TraceDirection * (WallTraceRadius + 2.0f);
			if (GetWorld()->LineTraceSingleByChannel(
					FaceHit, Start, FaceEnd, WallDetectionChannel, QueryParams) &&
				FaceHit.IsValidBlockingHit())
			{
				CandidateHit.ImpactNormal = FaceHit.ImpactNormal;
				CandidateHit.Normal = FaceHit.Normal;
			}
		}

		if (!IsWallSurface(CandidateHit, bRequireInitialWall))
		{
			continue;
		}

		FVector CandidateNormal = CandidateHit.Normal.GetSafeNormal();
		if (CandidateNormal.IsNearlyZero())
		{
			CandidateNormal = CandidateHit.ImpactNormal.GetSafeNormal();
		}

		const FVector TowardCharacter =
			(Start - CandidateHit.ImpactPoint).GetSafeNormal();
		if (!TowardCharacter.IsNearlyZero() &&
			FVector::DotProduct(CandidateNormal, TowardCharacter) < 0.0f)
		{
			CandidateNormal *= -1.0f;
		}

		const bool bSameTraversalComponent =
			!bRequireInitialWall &&
			WallSurfaceComponent.IsValid() &&
			CandidateHit.GetComponent() == WallSurfaceComponent.Get();

		if (!bRequireInitialWall && WallSurfaceComponent.IsValid() &&
			!CurrentWallNormal.IsNearlyZero() && !CandidateNormal.IsNearlyZero())
		{
			const float NormalDot =
				FVector::DotProduct(CandidateNormal, CurrentWallNormal);
			if (!bSameTraversalComponent && NormalDot < MinContinuedWallNormalDot)
			{
				continue;
			}
			if (bSameTraversalComponent && NormalDot < -0.1f)
			{
				continue;
			}
		}

		if (!CandidateNormal.IsNearlyZero())
		{
			CandidateHit.Normal = CandidateNormal;
			CandidateHit.ImpactNormal = CandidateNormal;
		}

		ClosestHitTime = FMath::Min(ClosestHitTime, CandidateHit.Time);
		UsableHits.Add(CandidateHit);
	}

	if (UsableHits.IsEmpty())
	{
		return false;
	}

	// This mirrors the simple climbing implementation's "process all surface
	// hits" idea, but keeps the averaged set local to the nearest contact band.
	// That prevents a far wall behind a close corner from pulling the run normal.
	constexpr float SurfaceHitTimeBand = 0.2f;
	for (const FHitResult& UsableHit : UsableHits)
	{
		if (UsableHit.Time <= ClosestHitTime + SurfaceHitTimeBand)
		{
			OutHits.Add(UsableHit);
		}
	}

	if (OutHits.IsEmpty())
	{
		OutHits.Add(UsableHits[0]);
	}

	return true;
}

bool UPHCharacterMovementComponent::BuildAveragedWallHit(
	const TArray<FHitResult>& WallHits,
	FHitResult& OutHit) const
{
	if (WallHits.IsEmpty())
	{
		return false;
	}

	FVector AveragePoint = FVector::ZeroVector;
	FVector AverageLocation = FVector::ZeroVector;
	FVector AverageNormal = FVector::ZeroVector;
	float BestTime = TNumericLimits<float>::Max();
	int32 BestIndex = 0;

	const FVector ComponentLocation = UpdatedComponent
		? UpdatedComponent->GetComponentLocation()
		: FVector::ZeroVector;

	for (int32 Index = 0; Index < WallHits.Num(); ++Index)
	{
		const FHitResult& WallHit = WallHits[Index];
		if (WallHit.Time < BestTime)
		{
			BestTime = WallHit.Time;
			BestIndex = Index;
		}

		AveragePoint += WallHit.ImpactPoint;
		AverageLocation += WallHit.Location;

		FVector SurfaceNormal = WallHit.Normal.GetSafeNormal();
		if (SurfaceNormal.IsNearlyZero())
		{
			SurfaceNormal = WallHit.ImpactNormal.GetSafeNormal();
		}

		const FVector TowardCharacter =
			(ComponentLocation - WallHit.ImpactPoint).GetSafeNormal();
		if (!TowardCharacter.IsNearlyZero() &&
			FVector::DotProduct(SurfaceNormal, TowardCharacter) < 0.0f)
		{
			SurfaceNormal *= -1.0f;
		}

		AverageNormal += SurfaceNormal;
	}

	const float HitCount = static_cast<float>(WallHits.Num());
	OutHit = WallHits[BestIndex];
	OutHit.ImpactPoint = AveragePoint / HitCount;
	OutHit.Location = AverageLocation / HitCount;

	const FVector AveragedNormal = AverageNormal.GetSafeNormal();
	if (!AveragedNormal.IsNearlyZero())
	{
		OutHit.Normal = AveragedNormal;
		OutHit.ImpactNormal = AveragedNormal;
	}

	return true;
}

bool UPHCharacterMovementComponent::IsWallSurface(
	const FHitResult& Hit,
	const bool bRequireInitialWall) const
{
	if (!Hit.IsValidBlockingHit())
	{
		return false;
	}

	if (!bRequireInitialWall)
	{
		return true;
	}

	// Normal ground cannot start traversal. Downward-facing surfaces remain
	// valid so jumping into the underside of an arch can attach to its ceiling.
	return !IsWalkable(Hit) && Hit.ImpactNormal.Z <= MaxWallNormalZ;
}

bool UPHCharacterMovementComponent::IsApproachingWall(const FHitResult& WallHit) const
{
	FVector SurfaceNormal = WallHit.ImpactNormal.GetSafeNormal();
	if (SurfaceNormal.IsNearlyZero())
	{
		SurfaceNormal = WallHit.Normal.GetSafeNormal();
	}
	if (SurfaceNormal.IsNearlyZero())
	{
		// No usable normal to test against; don't block attachment on this alone.
		return true;
	}

	// Use horizontal intent so running alongside a wall still qualifies. Prefer
	// current velocity, then steering acceleration, then raw input when blocked
	// against the surface with little velocity.
	FVector MoveDir = Velocity.GetSafeNormal2D();
	if (MoveDir.IsNearlyZero())
	{
		MoveDir = Acceleration.GetSafeNormal2D();
	}
	if (MoveDir.IsNearlyZero() && CharacterOwner)
	{
		MoveDir = CharacterOwner->GetLastMovementInputVector().GetSafeNormal2D();
	}
	if (MoveDir.IsNearlyZero())
	{
		// Standing still is not "running into" a wall.
		return false;
	}

	// Positive when heading into the wall, ~0 when parallel, negative when moving
	// away. Reject only clear outward motion.
	const float ApproachDot = FVector::DotProduct(MoveDir, -SurfaceNormal);
	return ApproachDot >= MinWallApproachDot;
}

bool UPHCharacterMovementComponent::IsCurrentTraversalSurface(
	const FHitResult& Hit) const
{
	if (!Hit.IsValidBlockingHit() || !WallSurfaceComponent.IsValid())
	{
		return false;
	}

	// A curved mesh naturally becomes walkable at the top. It is still the
	// active traversal surface, not a separate floor transition.
	return Hit.GetComponent() == WallSurfaceComponent.Get();
}

bool UPHCharacterMovementComponent::ShouldTransitionToGround(
	const FHitResult& Hit) const
{
	if (!IsWalkable(Hit))
	{
		return false;
	}

	// A (near) flat top means the wall has been crested: stand up even when the
	// top face belongs to the same mesh that was being climbed. Without this, a
	// solid wall whose top is one continuous collision component reads as "still
	// the climbing surface" indefinitely, so the character keeps reporting wall
	// traversal at the top instead of returning to walking. Genuinely curved
	// walkable patches (an arch rolled over) stay below this threshold and remain
	// traversal until they actually flatten out.
	if (Hit.ImpactNormal.Z >= GroundExitFlatNormalZ)
	{
		return true;
	}

	return !IsCurrentTraversalSurface(Hit);
}

void UPHCharacterMovementComponent::UpdateWallSurface(
	const FHitResult& WallHit,
	const bool bInitialAttach)
{
	FVector NewNormal = bInitialAttach
		? WallHit.ImpactNormal.GetSafeNormal()
		: WallHit.Normal.GetSafeNormal();
	if (NewNormal.IsNearlyZero())
	{
		NewNormal = WallHit.ImpactNormal.GetSafeNormal();
	}
	if (NewNormal.IsNearlyZero())
	{
		return;
	}

	// Always orient the contact normal from the surface toward the character.
	// This keeps the capsule's feet against both the outside and underside of
	// an arch even if collision-face winding changes.
	if (UpdatedComponent)
	{
		const FVector TowardCharacter =
			(UpdatedComponent->GetComponentLocation() - WallHit.ImpactPoint).GetSafeNormal();
		if (!TowardCharacter.IsNearlyZero() &&
			FVector::DotProduct(NewNormal, TowardCharacter) < 0.0f)
		{
			NewNormal *= -1.0f;
		}
	}

	if (!bInitialAttach && !WallNormal.IsNearlyZero() && GetWorld())
	{
		const float NormalAlpha =
			1.0f - FMath::Exp(
				-WallSurfaceNormalInterpSpeed * GetWorld()->GetDeltaSeconds());
		const FQuat NormalDelta = FQuat::FindBetweenNormals(
			FVector(WallNormal).GetSafeNormal(),
			NewNormal);
		NewNormal = FQuat::Slerp(FQuat::Identity, NormalDelta, NormalAlpha)
			.RotateVector(FVector(WallNormal).GetSafeNormal())
			.GetSafeNormal();
	}

	const FVector PreviousWallUp = FVector(WallUpDirection).GetSafeNormal();
	FVector NewWallUp;

	if (!bInitialAttach && !PreviousWallUp.IsNearlyZero())
	{
		// Project the previous tangent onto the new tangent plane. Repeating
		// this as the normal changes parallel-transports traversal direction
		// over an arch without degenerating at its horizontal top or underside.
		NewWallUp =
			FVector::VectorPlaneProject(PreviousWallUp, NewNormal).GetSafeNormal();
		if (!NewWallUp.IsNearlyZero() &&
			FVector::DotProduct(NewWallUp, PreviousWallUp) < 0.0f)
		{
			NewWallUp *= -1.0f;
		}
	}

	if (NewWallUp.IsNearlyZero())
	{
		NewWallUp =
			FVector::VectorPlaneProject(FVector::UpVector, NewNormal).GetSafeNormal();
	}
	if (NewWallUp.IsNearlyZero())
	{
		NewWallUp = FVector::VectorPlaneProject(Velocity, NewNormal).GetSafeNormal();
	}
	if (NewWallUp.IsNearlyZero() && UpdatedComponent)
	{
		NewWallUp = FVector::VectorPlaneProject(
			UpdatedComponent->GetForwardVector(),
			NewNormal).GetSafeNormal();
	}
	if (NewWallUp.IsNearlyZero())
	{
		NewWallUp = FVector::CrossProduct(NewNormal, FVector::RightVector).GetSafeNormal();
	}
	if (NewWallUp.IsNearlyZero())
	{
		NewWallUp = FVector::CrossProduct(NewNormal, FVector::ForwardVector).GetSafeNormal();
	}

	WallNormal = NewNormal;
	WallUpDirection = NewWallUp;
	WallImpactPoint = WallHit.ImpactPoint;
	WallSurfaceComponent = WallHit.GetComponent();
}

void UPHCharacterMovementComponent::SnapToWall(
	const FHitResult& WallHit,
	const bool bSnapRotation)
{
	if (!UpdatedComponent || !WallHit.IsValidBlockingHit())
	{
		return;
	}

	FQuat TargetRotation = UpdatedComponent->GetComponentQuat();
	FVector TargetCapsuleUp = UpdatedComponent->GetUpVector();
	if (bSnapRotation)
	{
		TargetRotation = GetWallTraversalRotation();
		TargetCapsuleUp = TargetRotation.GetAxisZ();
	}

	const float CurrentDistance = FVector::DotProduct(
		UpdatedComponent->GetComponentLocation() - WallHit.ImpactPoint,
		WallHit.ImpactNormal);
	const float DistanceError =
		CurrentDistance - GetDesiredWallDistance(TargetCapsuleUp);

	// A one-centimeter dead zone prevents tiny collision-normal changes from
	// moving the capsule back and forth every frame.
	if (FMath::Abs(DistanceError) <= 1.0f &&
		TargetRotation.Equals(UpdatedComponent->GetComponentQuat()))
	{
		return;
	}

	const FVector Correction = -WallHit.ImpactNormal * DistanceError;
	FHitResult CorrectionHit;
	SafeMoveUpdatedComponent(
		Correction,
		TargetRotation,
		true,
		CorrectionHit);
}

void UPHCharacterMovementComponent::BeginWallToGroundTransition(const FHitResult& GroundHit)
{
	if (!UpdatedComponent || !CharacterOwner || !IsWalkable(GroundHit) ||
		IsTransitioningFromWallToGround())
	{
		return;
	}

	const FVector GroundNormal = GroundHit.ImpactNormal.GetSafeNormal();
	WallTransitionData.bActive = true;
	WallTransitionData.bGroundFootIsLeft = ChooseGroundTransitionFoot(GroundHit);
	WallTransitionData.bStartedFromWallClimbing = IsWallClimbing();
	WallTransitionData.GroundSurfacePoint = GroundHit.ImpactPoint;
	WallTransitionData.GroundSurfaceNormal = GroundNormal;
	WallTransitionData.WallSurfacePoint = WallImpactPoint;
	WallTransitionData.WallSurfaceNormal = FVector(WallNormal).GetSafeNormal();
	WallTransitionData.Duration = WallToGroundTransitionDuration;

	WallToGroundElapsed = 0.0f;
	WallToGroundStartLocation = UpdatedComponent->GetComponentLocation();
	WallToGroundStartRotation = UpdatedComponent->GetComponentQuat();
	WallToGroundTargetRotation = GetWorldUpRotation();
	WallToGroundPlanarVelocity = FVector::VectorPlaneProject(Velocity, GroundNormal);

	// Solve the final upright capsule location against both surfaces. At the
	// corner this places the capsule half-height above the floor and one radius
	// away from the wall.
	WallToGroundTargetLocation = WallToGroundStartLocation;
	const float GroundDistance =
		GetCapsuleSupportDistance(GroundNormal, FVector::UpVector) + WallSurfaceGap;
	const float WallDistance =
		GetCapsuleSupportDistance(WallNormal, FVector::UpVector) + WallSurfaceGap;
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		WallToGroundTargetLocation += GroundNormal * (
			GroundDistance - FVector::DotProduct(
				WallToGroundTargetLocation - GroundHit.ImpactPoint,
				GroundNormal));
		WallToGroundTargetLocation += FVector(WallNormal) * (
			WallDistance - FVector::DotProduct(
				WallToGroundTargetLocation - WallImpactPoint,
				FVector(WallNormal)));
	}

	WallLostFrames = 0;
	SetMovementMode(
		MOVE_Custom,
		static_cast<uint8>(EPHCustomMovementMode::WallToGround));
	CharacterOwner->ForceNetUpdate();
}

bool UPHCharacterMovementComponent::ChooseGroundTransitionFoot(
	const FHitResult& GroundHit) const
{
	const USkeletalMeshComponent* Mesh = CharacterOwner ? CharacterOwner->GetMesh() : nullptr;
	if (Mesh &&
		Mesh->DoesSocketExist(TransitionLeftFootBone) &&
		Mesh->DoesSocketExist(TransitionRightFootBone))
	{
		const FVector GroundNormal = GroundHit.ImpactNormal.GetSafeNormal();
		const float LeftDistance = FMath::Abs(FVector::DotProduct(
			Mesh->GetSocketLocation(TransitionLeftFootBone) - GroundHit.ImpactPoint,
			GroundNormal));
		const float RightDistance = FMath::Abs(FVector::DotProduct(
			Mesh->GetSocketLocation(TransitionRightFootBone) - GroundHit.ImpactPoint,
			GroundNormal));
		return LeftDistance <= RightDistance;
	}

	// Fallback: use travel direction so lateral movement leads with that side.
	return FVector::DotProduct(Velocity, GetWallRight()) <= 0.0f;
}

void UPHCharacterMovementComponent::CompleteWallToGroundTransition()
{
	if (!IsTransitioningFromWallToGround() || !UpdatedComponent || !CharacterOwner)
	{
		return;
	}

	const FVector GroundNormal =
		WallTransitionData.GroundSurfaceNormal.GetSafeNormal();
	FHitResult CompletionHit;
	SafeMoveUpdatedComponent(
		WallToGroundTargetLocation - UpdatedComponent->GetComponentLocation(),
		WallToGroundTargetRotation,
		true,
		CompletionHit);

	Velocity = FVector::VectorPlaneProject(
		WallToGroundPlanarVelocity,
		GroundNormal.IsNearlyZero() ? FVector::UpVector : GroundNormal);
	WallTransitionData = FALSWallTransitionData();
	WallToGroundElapsed = 0.0f;
	WallLostFrames = 0;
	WallSurfaceComponent.Reset();
	RecordWallDetachTime();
	if (const UWorld* World = GetWorld())
	{
		// Ground and wall collision can both remain in reach at a tight corner.
		// Extend only this detach timestamp so held traversal input cannot
		// immediately reattach and alternate between the two movement modes.
		LastWallDetachTime = World->GetTimeSeconds() +
			FMath::Max(WallToGroundReattachDelay - WallReattachCooldown, 0.0f);
	}
	SetMovementMode(MOVE_Walking);
	CharacterOwner->ForceNetUpdate();
}

void UPHCharacterMovementComponent::HandleWallLost()
{
	const bool bLeavingUpwardEdge =
		FVector::DotProduct(Velocity, FVector::UpVector) > WallMantleMinimumUpSpeed;

	// Note: losing the wall does NOT arm the reattach cooldown. The cooldown
	// exists to stop instantly re-grabbing a wall after an intentional detach
	// (StopWallTraversal / JumpOffWall). Arming it here would block legitimate
	// reattaches for WallReattachCooldown seconds and read as "won't connect".
	WallLostFrames = 0;
	WallTransitionData = FALSWallTransitionData();
	WallSurfaceComponent.Reset();
	RestoreWorldUpRotation();
	SetMovementMode(MOVE_Falling);

	if (bLeavingUpwardEdge)
	{
		if (APHBaseCharacter* PHCharacter = Cast<APHBaseCharacter>(CharacterOwner))
		{
			PHCharacter->TryWallTopMantle();
		}
	}
}

FQuat UPHCharacterMovementComponent::GetWallTraversalRotation() const
{
	const FVector SurfaceUp = WallNormal.GetSafeNormal();
	if (SurfaceUp.IsNearlyZero())
	{
		return UpdatedComponent ? UpdatedComponent->GetComponentQuat() : FQuat::Identity;
	}

	FVector SurfaceForward = FVector::VectorPlaneProject(Velocity, SurfaceUp).GetSafeNormal();
	if (SurfaceForward.IsNearlyZero())
	{
		SurfaceForward = FVector::VectorPlaneProject(Acceleration, SurfaceUp).GetSafeNormal();
	}
	if (SurfaceForward.IsNearlyZero() && UpdatedComponent)
	{
		SurfaceForward =
			FVector::VectorPlaneProject(UpdatedComponent->GetForwardVector(), SurfaceUp).GetSafeNormal();
	}
	if (SurfaceForward.IsNearlyZero())
	{
		SurfaceForward = GetWallUp();
	}

	return FRotationMatrix::MakeFromZX(SurfaceUp, SurfaceForward).ToQuat();
}

FQuat UPHCharacterMovementComponent::GetWorldUpRotation() const
{
	FVector UprightForward =
		FVector::VectorPlaneProject(-WallNormal, FVector::UpVector).GetSafeNormal();
	if (UprightForward.IsNearlyZero() && UpdatedComponent)
	{
		UprightForward =
			FVector::VectorPlaneProject(UpdatedComponent->GetForwardVector(), FVector::UpVector).GetSafeNormal();
	}
	if (UprightForward.IsNearlyZero())
	{
		UprightForward = FVector::ForwardVector;
	}

	return FRotationMatrix::MakeFromZX(FVector::UpVector, UprightForward).ToQuat();
}

float UPHCharacterMovementComponent::GetDesiredWallDistance() const
{
	const UCapsuleComponent* Capsule = CharacterOwner ? CharacterOwner->GetCapsuleComponent() : nullptr;
	if (!Capsule)
	{
		return WallSurfaceGap;
	}

	return GetDesiredWallDistance(Capsule->GetUpVector());
}

float UPHCharacterMovementComponent::GetDesiredWallDistance(const FVector& CapsuleUp) const
{
	const UCapsuleComponent* Capsule = CharacterOwner ? CharacterOwner->GetCapsuleComponent() : nullptr;
	if (!Capsule)
	{
		return WallSurfaceGap;
	}

	const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	if (!bAlignCapsuleToWall || WallNormal.IsNearlyZero())
	{
		return CapsuleRadius + WallSurfaceGap;
	}

	return GetCapsuleSupportDistance(WallNormal, CapsuleUp) + WallSurfaceGap;
}

float UPHCharacterMovementComponent::GetCapsuleSupportDistance(
	const FVector& SurfaceNormal,
	const FVector& CapsuleUp) const
{
	const UCapsuleComponent* Capsule = CharacterOwner ? CharacterOwner->GetCapsuleComponent() : nullptr;
	if (!Capsule)
	{
		return 0.0f;
	}

	// Support distance of an arbitrarily rotated capsule along a surface normal.
	const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float CylinderHalfLength = FMath::Max(CapsuleHalfHeight - CapsuleRadius, 0.0f);
	const float AxisAlignment =
		FMath::Abs(FVector::DotProduct(
			CapsuleUp.GetSafeNormal(),
			SurfaceNormal.GetSafeNormal()));
	return CapsuleRadius + CylinderHalfLength * AxisAlignment;
}

float UPHCharacterMovementComponent::GetWallTraversalSpeed() const
{
	if (IsWallRunning())
	{
		const float ALSSpeed = CurrentMovementSettings.SprintSpeed;
		return ALSSpeed > 0.0f
			? ALSSpeed * WallRunningSpeedMultiplier
			: WallRunningFallbackSpeed;
	}

	const float ALSSpeed = CurrentMovementSettings.RunSpeed;
	return ALSSpeed > 0.0f
		? ALSSpeed * WallClimbingSpeedMultiplier
		: WallClimbingFallbackSpeed;
}

void UPHCharacterMovementComponent::RecordWallDetachTime()
{
	if (const UWorld* World = GetWorld())
	{
		LastWallDetachTime = World->GetTimeSeconds();
	}
}

void UPHCharacterMovementComponent::OnRep_WallNormal()
{
	WallNormal = FVector(WallNormal).GetSafeNormal();
	WallUpDirection = FVector(WallUpDirection).GetSafeNormal();
}

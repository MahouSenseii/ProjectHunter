#include "Character/Components/PHCharacterMovementComponent.h"

#include "Character/PHBaseCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"

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
	if (PreferredWallHit && IsWallSurface(*PreferredWallHit))
	{
		WallHit = *PreferredWallHit;
	}
	else if (!FindAttachableWall(WallHit))
	{
		return false;
	}

	WallNormal = WallHit.ImpactNormal.GetSafeNormal();
	WallImpactPoint = WallHit.ImpactPoint;

	const EPHCustomMovementMode NewMode =
		RequestedState == EALSMovementState::WallRunning
			? EPHCustomMovementMode::WallRunning
			: EPHCustomMovementMode::WallClimbing;

	Velocity = FVector::VectorPlaneProject(Velocity, WallNormal);
	WallLostFrames = 0;
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
	RecordWallDetachTime();
	SetMovementMode(MOVE_Falling);
}

void UPHCharacterMovementComponent::JumpOffWall()
{
	if (!IsWallTraversing())
	{
		return;
	}

	const FVector JumpNormal = WallNormal;
	RestoreWorldUpRotation();
	RecordWallDetachTime();
	SetMovementMode(MOVE_Falling);
	Velocity = JumpNormal * WallJumpAwaySpeed + FVector::UpVector * WallJumpUpSpeed;
}

bool UPHCharacterMovementComponent::IsWallTraversing() const
{
	if (MovementMode != MOVE_Custom)
	{
		return false;
	}

	const EPHCustomMovementMode Mode = static_cast<EPHCustomMovementMode>(CustomMovementMode);
	return Mode == EPHCustomMovementMode::WallRunning ||
		Mode == EPHCustomMovementMode::WallClimbing;
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

FVector UPHCharacterMovementComponent::GetWallUp() const
{
	const FVector Result = FVector::VectorPlaneProject(FVector::UpVector, WallNormal).GetSafeNormal();
	return Result.IsNearlyZero() ? FVector::UpVector : Result;
}

FVector UPHCharacterMovementComponent::GetWallRight() const
{
	return FVector::CrossProduct(WallNormal, GetWallUp()).GetSafeNormal();
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

	// Looking/pressing toward the wall means upward travel. Looking/pressing
	// away means downward travel. Tangential camera direction remains lateral.
	const float LateralInput = FVector::DotProduct(HorizontalDirection, WallRight);
	const float VerticalInput = FVector::DotProduct(HorizontalDirection, -WallNormal);

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
	if (!IsWallSurface(WallHit))
	{
		const FVector ImpactDirection =
			(WallHit.ImpactPoint - UpdatedComponent->GetComponentLocation()).GetSafeNormal();
		if (!TraceWall(ImpactDirection, WallHit))
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

	Super::PhysCustom(DeltaTime, Iterations);
}

void UPHCharacterMovementComponent::PhysWallTraversal(const float DeltaTime, const int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME || !CharacterOwner || !UpdatedComponent)
	{
		return;
	}

	FHitResult WallHit;
	bool bHasWall = TraceWall(-WallNormal, WallHit);
	if (!bHasWall)
	{
		// The wall normal may have rotated across a corner or curve, so the
		// straight-back trace can miss a wall that is still right beside us.
		// Widen the search before giving up.
		bHasWall = FindAttachableWall(WallHit);
	}

	if (bHasWall)
	{
		WallLostFrames = 0;
		WallNormal = WallHit.ImpactNormal.GetSafeNormal();
		WallImpactPoint = WallHit.ImpactPoint;
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

	const FVector MoveVelocity = Velocity - WallNormal * WallAdhesionSpeed;
	const FVector MoveDelta = MoveVelocity * DeltaTime;

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
		if (IsWalkable(MoveHit))
		{
			TransitionFromWallToGround(MoveHit);
			return;
		}

		SlideAlongSurface(MoveDelta, 1.0f - MoveHit.Time, MoveHit.Normal, MoveHit, true);
	}

	if (bHasWall)
	{
		SnapToWall(WallHit);
	}
	Velocity = FVector::VectorPlaneProject(Velocity, WallNormal);
}

bool UPHCharacterMovementComponent::FindAttachableWall(FHitResult& OutHit) const
{
	if (!CharacterOwner)
	{
		return false;
	}

	TArray<FVector, TInlineAllocator<5>> CandidateDirections;

	const FVector InputDirection = CharacterOwner->GetLastMovementInputVector().GetSafeNormal2D();
	if (!InputDirection.IsNearlyZero())
	{
		CandidateDirections.Add(InputDirection);
	}

	const FVector VelocityDirection = Velocity.GetSafeNormal2D();
	if (!VelocityDirection.IsNearlyZero())
	{
		CandidateDirections.Add(VelocityDirection);
	}

	CandidateDirections.Add(CharacterOwner->GetActorForwardVector());
	CandidateDirections.Add(CharacterOwner->GetActorRightVector());
	CandidateDirections.Add(-CharacterOwner->GetActorRightVector());

	bool bFoundWall = false;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();

	for (const FVector& Direction : CandidateDirections)
	{
		FHitResult CandidateHit;
		if (!TraceWall(Direction, CandidateHit))
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

bool UPHCharacterMovementComponent::TraceWall(const FVector& Direction, FHitResult& OutHit) const
{
	if (!GetWorld() || !CharacterOwner || !UpdatedComponent || Direction.IsNearlyZero())
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PHWallTraversal), false, CharacterOwner);
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector End = Start + Direction.GetSafeNormal() * (GetDesiredWallDistance() + WallDetectionReach);

	const bool bHit = GetWorld()->SweepSingleByChannel(
		OutHit,
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

	// Sphere sweeps return rounded/averaged normals on edges and corners, which
	// can spike the normal's Z and make a valid wall fail IsWallSurface. Refine
	// with a line trace to the impact point to recover the true face normal.
	FHitResult FaceHit;
	const FVector FaceEnd = OutHit.ImpactPoint + Direction.GetSafeNormal() * (WallTraceRadius + 2.0f);
	if (GetWorld()->LineTraceSingleByChannel(
			FaceHit, Start, FaceEnd, WallDetectionChannel, QueryParams) &&
		FaceHit.IsValidBlockingHit())
	{
		OutHit.ImpactNormal = FaceHit.ImpactNormal;
		OutHit.Normal = FaceHit.Normal;
	}

	return IsWallSurface(OutHit);
}

bool UPHCharacterMovementComponent::IsWallSurface(const FHitResult& Hit) const
{
	return Hit.IsValidBlockingHit() &&
		!IsWalkable(Hit) &&
		FMath::Abs(Hit.ImpactNormal.Z) <= MaxWallNormalZ;
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

	if (FMath::Abs(DistanceError) <= KINDA_SMALL_NUMBER &&
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

void UPHCharacterMovementComponent::TransitionFromWallToGround(const FHitResult& GroundHit)
{
	if (!UpdatedComponent || !IsWalkable(GroundHit))
	{
		return;
	}

	const FVector GroundNormal = GroundHit.ImpactNormal.GetSafeNormal();
	const FQuat UprightRotation = GetWorldUpRotation();
	const float CurrentGroundDistance = FVector::DotProduct(
		UpdatedComponent->GetComponentLocation() - GroundHit.ImpactPoint,
		GroundNormal);
	const float DesiredGroundDistance =
		GetCapsuleSupportDistance(GroundNormal, FVector::UpVector) + WallSurfaceGap;
	const FVector GroundCorrection =
		GroundNormal * (DesiredGroundDistance - CurrentGroundDistance);

	FHitResult TransitionHit;
	SafeMoveUpdatedComponent(
		GroundCorrection,
		UprightRotation,
		true,
		TransitionHit);

	Velocity = FVector::VectorPlaneProject(Velocity, GroundNormal);
	WallLostFrames = 0;
	RecordWallDetachTime();
	SetMovementMode(MOVE_Walking);
}

void UPHCharacterMovementComponent::HandleWallLost()
{
	// Note: losing the wall does NOT arm the reattach cooldown. The cooldown
	// exists to stop instantly re-grabbing a wall after an intentional detach
	// (StopWallTraversal / JumpOffWall). Arming it here would block legitimate
	// reattaches for WallReattachCooldown seconds and read as "won't connect".
	WallLostFrames = 0;
	RestoreWorldUpRotation();
	SetMovementMode(MOVE_Falling);

	if (APHBaseCharacter* PHCharacter = Cast<APHBaseCharacter>(CharacterOwner))
	{
		PHCharacter->TryWallTopMantle();
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
}

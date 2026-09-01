#include "Character/Components/PHCharacterMovementComponent.h"

#include "Animation/AnimInstance.h"
#include "Character/PHBaseCharacter.h"
#include "Character/Library/FunctionLibraries/PHWallTraversalFunctionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogPHWallTraversal);

namespace
{
	constexpr float WallSurfaceNormalInterpSpeed = 15.0f;
	constexpr float WallToGroundReattachDelay = 0.75f;
	constexpr float GroundExitFlatNormalZ = 0.98f;

	// Two attach-probe directions closer than this (dot) are treated as duplicates.
	constexpr float DirectionDuplicateDot = 0.98f;

	// Surface hits within this fraction of the closest hit's sweep time are
	// averaged together as one contact band.
	constexpr float SurfaceHitTimeBand = 0.2f;

	// Extra length added past the sphere radius when refining a face normal.
	constexpr float FaceRefineExtension = 2.0f;

	// Minimum normal agreement for a hit on the SAME traversal component before it
	// is rejected as a back-facing/corner artifact.
	constexpr float SameComponentNormalFlipDot = -0.1f;

	// Fixed-point passes used to solve the upright wall->ground corner location.
	constexpr int32 WallToGroundSolverPasses = 2;
}

UPHCharacterMovementComponent::UPHCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	bWantsWallTraversalInput = 0;
}

void UPHCharacterMovementComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// The owning client computes these locally for prediction; only simulated
	// proxies need them replicated to drive remote animation and wall IK.
	DOREPLIFETIME_CONDITION(UPHCharacterMovementComponent, WallNormal, COND_SimulatedOnly);
	DOREPLIFETIME_CONDITION(UPHCharacterMovementComponent, WallUpDirection, COND_SimulatedOnly);
	DOREPLIFETIME_CONDITION(UPHCharacterMovementComponent, WallTransitionData, COND_SimulatedOnly);
}

void UPHCharacterMovementComponent::UpdateFromCompressedFlags(const uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	// FLAG_Custom_0 is owned by ALS (movement settings change); wall intent uses
	// the next free predicted flag bit so the server replays the same decision.
	bWantsWallTraversalInput =
		(Flags & FSavedMove_Character::FLAG_Custom_1) != 0 ? 1 : 0;
}

FNetworkPredictionData_Client* UPHCharacterMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr);

	if (!ClientPredictionData)
	{
		UPHCharacterMovementComponent* MutableThis =
			const_cast<UPHCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_PH(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.0f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.0f;
	}

	return ClientPredictionData;
}

void UPHCharacterMovementComponent::SetWantsWallTraversalInput(const bool bWants)
{
	bWantsWallTraversalInput = bWants ? 1 : 0;
}

void UPHCharacterMovementComponent::FSavedMove_PH::Clear()
{
	Super::Clear();
	bSavedWantsWallTraversal = 0;
	SavedWallAttachRetryAccumulator = 0.0f;
}

uint8 UPHCharacterMovementComponent::FSavedMove_PH::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if (bSavedWantsWallTraversal)
	{
		Result |= FLAG_Custom_1;
	}
	return Result;
}

bool UPHCharacterMovementComponent::FSavedMove_PH::CanCombineWith(
	const FSavedMovePtr& NewMove,
	ACharacter* InCharacter,
	const float MaxDelta) const
{
	// Do not merge moves across a change in wall intent, or the server would
	// replay the attach/detach frame with the wrong flag.
	if (const FSavedMove_PH* NewPHMove = static_cast<const FSavedMove_PH*>(NewMove.Get()))
	{
		if (bSavedWantsWallTraversal != NewPHMove->bSavedWantsWallTraversal)
		{
			return false;
		}
	}
	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void UPHCharacterMovementComponent::FSavedMove_PH::SetMoveFor(
	ACharacter* Character,
	const float InDeltaTime,
	FVector const& NewAccel,
	FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	if (const UPHCharacterMovementComponent* PHMovement =
		Cast<UPHCharacterMovementComponent>(Character->GetCharacterMovement()))
	{
		bSavedWantsWallTraversal = PHMovement->bWantsWallTraversalInput;
		SavedWallAttachRetryAccumulator = PHMovement->WallAttachRetryAccumulator;
	}
}

void UPHCharacterMovementComponent::FSavedMove_PH::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	// Replaying a corrected move must restart the attach probe from the same
	// point in its cycle, or the replay can probe on a frame the original did not.
	if (UPHCharacterMovementComponent* PHMovement =
		Cast<UPHCharacterMovementComponent>(Character->GetCharacterMovement()))
	{
		PHMovement->WallAttachRetryAccumulator = SavedWallAttachRetryAccumulator;
	}
}

UPHCharacterMovementComponent::FNetworkPredictionData_Client_PH::FNetworkPredictionData_Client_PH(
	const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr UPHCharacterMovementComponent::FNetworkPredictionData_Client_PH::AllocateNewMove()
{
	return MakeShared<FSavedMove_PH>();
}

bool UPHCharacterMovementComponent::TryStartWallTraversal(
	const EALSMovementState RequestedState,
	const FHitResult* PreferredWallHit)
{
	if (!CharacterOwner || IsWallTraversing())
	{
		return false;
	}

	const bool bCanAttachFromCurrentMode =
		MovementMode == MOVE_Falling ||
		MovementMode == MOVE_Walking ||
		MovementMode == MOVE_NavWalking;
	if (!bCanAttachFromCurrentMode)
	{
		if (bDebugWallTraversal)
		{
			PH_LOG(LogPHWallTraversal, Verbose,
				"Attach rejected: movement mode cannot attach (Mode=%d Custom=%d).",
				static_cast<int32>(MovementMode),
				static_cast<int32>(CustomMovementMode));
		}
		return false;
	}

	if (const APHBaseCharacter* PHCharacter = Cast<APHBaseCharacter>(CharacterOwner);
		PHCharacter && !PHCharacter->CanUseStaminaMovement())
	{
		if (bDebugWallTraversal)
		{
			PH_LOG(LogPHWallTraversal, Verbose, "Attach rejected: stamina movement is blocked.");
		}
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
		if (bDebugWallTraversal)
		{
			PH_LOG(LogPHWallTraversal, Verbose,
				"Attach rejected: reattach cooldown active (%.2fs remaining).",
				WallReattachCooldown - (World->GetTimeSeconds() - LastWallDetachTime));
		}
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
		if (bDebugWallTraversal)
		{
			PH_LOG(LogPHWallTraversal, Verbose,
				"Attach rejected: not approaching wall (Normal=%s).",
				*WallHit.ImpactNormal.ToString());
		}
		return false;
	}

	UpdateWallSurface(WallHit, true, 0.0f);
	WallTransitionData = FALSWallTransitionData();

	const EPHCustomMovementMode NewMode =
		RequestedState == EALSMovementState::WallRunning
			? EPHCustomMovementMode::WallRunning
			: EPHCustomMovementMode::WallClimbing;

	Velocity = FVector::VectorPlaneProject(Velocity, WallNormal);
	WallLostFrames = 0;
	WallTraversalElapsed = 0.0f;
	SetMovementMode(MOVE_Custom, static_cast<uint8>(NewMode));
	ApplyWallTraversalRootMotionMode();
	SnapToWall(WallHit, bAlignCapsuleToWall && bSnapRotationOnAttach);
	CharacterOwner->ForceNetUpdate();

	if (bDebugWallTraversal)
	{
		PH_LOG(LogPHWallTraversal, Log,
			"Attached: Mode=%s Normal=%s Surface=%s.",
			NewMode == EPHCustomMovementMode::WallRunning ? TEXT("Run") : TEXT("Climb"),
			*FVector(WallNormal).ToString(),
			*GetNameSafe(WallSurfaceComponent.Get()));
	}
	return true;
}

void UPHCharacterMovementComponent::StopWallTraversal()
{
	if (!IsWallTraversing() || !CharacterOwner)
	{
		return;
	}

	RestoreWorldUpRotation();
	ClearWallTraversalState();
	RecordWallDetachTime();
	SetMovementMode(MOVE_Falling);
	CharacterOwner->ForceNetUpdate();
}

void UPHCharacterMovementComponent::JumpOffWall()
{
	if (!IsWallTraversing() || !CharacterOwner)
	{
		return;
	}

	const FVector JumpNormal = FVector(WallNormal).GetSafeNormal();
	RestoreWorldUpRotation();
	ClearWallTraversalState();
	RecordWallDetachTime();
	SetMovementMode(MOVE_Falling);
	Velocity = JumpNormal * WallJumpAwaySpeed + FVector::UpVector * WallJumpUpSpeed;
	CharacterOwner->ForceNetUpdate();
}

void UPHCharacterMovementComponent::AddWallTraversalPlaneImpulse(
	FVector WorldDirection,
	const float Speed,
	const bool bReplaceCurrentWallVelocity)
{
	if ((!IsWallRunning() && !IsWallClimbing()) || IsWallTraversalCombatMovementLocked() ||
		Speed <= 0.0f || WallNormal.IsNearlyZero())
	{
		return;
	}

	FVector DirectionOnWall =
		FVector::VectorPlaneProject(WorldDirection, WallNormal).GetSafeNormal();
	if (DirectionOnWall.IsNearlyZero())
	{
		DirectionOnWall = GetWallUp();
	}
	if (DirectionOnWall.IsNearlyZero())
	{
		return;
	}

	const FVector CurrentWallVelocity =
		FVector::VectorPlaneProject(Velocity, WallNormal);
	Velocity = bReplaceCurrentWallVelocity
		? DirectionOnWall * Speed
		: CurrentWallVelocity + DirectionOnWall * Speed;
	Velocity = FVector::VectorPlaneProject(Velocity, WallNormal);
}

void UPHCharacterMovementComponent::SetWallTraversalCombatMovementScale(
	const float NewMovementScale,
	const float Duration)
{
	WallTraversalCombatMovementScale = FMath::Clamp(NewMovementScale, 0.0f, 1.0f);

	if (IsWallTraversalCombatMovementLocked() && (IsWallRunning() || IsWallClimbing()))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
	}

	const UWorld* World = GetWorld();
	WallTraversalCombatMovementScaleEndTime =
		World && Duration > 0.0f ? World->GetTimeSeconds() + Duration : -1.0f;
}

void UPHCharacterMovementComponent::ClearWallTraversalCombatMovementScale()
{
	WallTraversalCombatMovementScale = 1.0f;
	WallTraversalCombatMovementScaleEndTime = -1.0f;
}

float UPHCharacterMovementComponent::GetWallTraversalCombatMovementScale() const
{
	return WallTraversalCombatMovementScale;
}

void UPHCharacterMovementComponent::SetWallTraversalCombatMovementLocked(
	const bool bLocked,
	const float Duration)
{
	if (bLocked)
	{
		SetWallTraversalCombatMovementScale(0.0f, Duration);
		return;
	}

	ClearWallTraversalCombatMovementScale();
}

bool UPHCharacterMovementComponent::IsWallTraversalCombatMovementLocked() const
{
	return WallTraversalCombatMovementScale <= KINDA_SMALL_NUMBER;
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

FRotator UPHCharacterMovementComponent::GetWallTraversalRotation() const
{
	return ComputeWallTraversalRotation().Rotator();
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
	return ConvertCameraDirectionToWallDirection(WorldDirection, true);
}

FVector UPHCharacterMovementComponent::ConvertCameraDirectionToWallDirection(
	const FVector& WorldDirection,
	const bool bTreatIntoSurfaceAsUp) const
{
	if (!IsWallTraversing() || WorldDirection.IsNearlyZero() || WallNormal.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	const FVector CameraDirection = WorldDirection.GetSafeNormal();
	if (CameraDirection.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	const FVector WallUp = GetWallUp();
	const FVector WallRight = GetWallRight();

	// Use the current camera vector every input frame. Keep the result camera-led
	// by projecting onto the wall plane instead of turning any into-wall look into
	// extra vertical input.
	const FVector SurfaceDirection =
		FVector::VectorPlaneProject(CameraDirection, WallNormal).GetSafeNormal();
	if (!SurfaceDirection.IsNearlyZero())
	{
		return SurfaceDirection;
	}

	// If the player looks straight into the wall, projection has no tangent
	// component. For the forward axis, keep the expected wall-climb behavior:
	// forward runs up the wall, backward runs down. Strafe input does not use this
	// fallback because "right into the wall" should not become vertical movement.
	if (bTreatIntoSurfaceAsUp)
	{
		const float IntoSurfaceInput = FVector::DotProduct(CameraDirection, -WallNormal);
		if (FMath::Abs(IntoSurfaceInput) > KINDA_SMALL_NUMBER)
		{
			return IntoSurfaceInput >= 0.0f ? WallUp : -WallUp;
		}
	}

	// Last-resort strafe fallback. This prevents camera-right from producing no
	// movement when it is nearly parallel to the wall normal.
	return bTreatIntoSurfaceAsUp ? FVector::ZeroVector : WallRight;
}

void UPHCharacterMovementComponent::OnMovementUpdated(
	const float DeltaTime,
	const FVector& OldLocation,
	const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaTime, OldLocation, OldVelocity);
	UpdateWallAttachRetry(DeltaTime);
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
		!UpdatedComponent ||
		CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)
	{
		return;
	}

	// Use the predicted wall intent (the server restores it from the move's
	// compressed flags) so client and server evaluate attachment on the same frame.
	if (!bWantsWallTraversalInput)
	{
		return;
	}

	APHBaseCharacter* PHCharacter = Cast<APHBaseCharacter>(CharacterOwner);
	if (!PHCharacter)
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
			if (bDebugWallTraversal)
			{
				PH_LOG(LogPHWallTraversal, Verbose,
					"Impact ignored: no wall surface near %s.",
					*Hit.ImpactPoint.ToString());
			}
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

void UPHCharacterMovementComponent::OnMovementModeChanged(
	const EMovementMode PreviousMovementMode,
	const uint8 PreviousCustomMode)
{
	const bool bWasWallTraversing = PreviousMovementMode == MOVE_Custom &&
		(PreviousCustomMode == static_cast<uint8>(EPHCustomMovementMode::WallRunning) ||
			PreviousCustomMode == static_cast<uint8>(EPHCustomMovementMode::WallClimbing) ||
			PreviousCustomMode == static_cast<uint8>(EPHCustomMovementMode::WallToGround));
	if (bWasWallTraversing && !IsWallTraversing())
	{
		// Ragdoll, mantle and external mode changes can bypass StopWallTraversal.
		// Restore the capsule before the new mode probes its floor or notifies listeners.
		RestoreWorldUpRotation();
		ClearWallTraversalState();
	}

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
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

void UPHCharacterMovementComponent::PhysWallTraversal(const float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME || !CharacterOwner || !UpdatedComponent)
	{
		return;
	}

	// Sub-step long frames so one large tick cannot tunnel the capsule through a
	// wall or overshoot the wall->ground corner. GetSimulationTimeStep returns the
	// whole DeltaTime at normal frame rates, so this loops once in the common case.
	float RemainingTime = DeltaTime;
	while (RemainingTime >= MIN_TICK_TIME &&
		Iterations < MaxSimulationIterations &&
		CharacterOwner && UpdatedComponent &&
		(IsWallRunning() || IsWallClimbing()))
	{
		++Iterations;
		const float StepTime = GetSimulationTimeStep(RemainingTime, Iterations);
		RemainingTime -= StepTime;

		if (!WallTraversalStep(StepTime))
		{
			// The sub-step changed movement mode (ground transition or detach).
			// Hand the rest of the frame to the new mode instead of dropping it,
			// or dropping off a wall costs a frame of gravity. StartNewPhysics
			// no-ops on a spent remainder or exhausted iterations.
			StartNewPhysics(RemainingTime, Iterations);
			return;
		}
	}
}

bool UPHCharacterMovementComponent::WallTraversalStep(const float DeltaTime)
{
	// Attachment checks stamina once; without this the character keeps traversing
	// for free after running dry, since nothing else re-evaluates it while attached.
	if (const APHBaseCharacter* PHCharacter = Cast<APHBaseCharacter>(CharacterOwner);
		PHCharacter && !PHCharacter->CanUseStaminaMovement())
	{
		if (bDebugWallTraversal)
		{
			PH_LOG(LogPHWallTraversal, Log, "Exiting traversal: stamina movement is blocked.");
		}
		StopWallTraversal();
		return false;
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
		UpdateWallSurface(WallHit, false, DeltaTime);
	}
	else
	{
		// Tolerate brief losses (seams, corners, sphere-sweep normal noise)
		// before detaching. Coast along the last-known wall during the grace
		// window instead of dropping on the first missed frame.
		if (++WallLostFrames > MaxConsecutiveWallLostFrames)
		{
			HandleWallLost();
			return false;
		}
	}

	ApplyWallTraversalRootMotionMode();
	UpdateWallTraversalCombatMovementScale();

	const float CombatMovementScale = GetWallTraversalCombatMovementScale();
	const float MaxWallSpeed = GetWallTraversalSpeed() * CombatMovementScale;

	RestorePreAdditiveRootMotionVelocity();
	const bool bUsingRootMotion =
		HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity();

	// Each substep starts from the same input; scaling the stored acceleration
	// would compound a combat slowdown whenever a long frame is subdivided.
	TGuardValue<FVector> RestoreAcceleration(Acceleration,
		FVector::VectorPlaneProject(Acceleration, WallNormal)
			.GetClampedToMaxSize(GetMaxAcceleration()) * CombatMovementScale);
	if (!bUsingRootMotion)
	{
		CalcVelocity(DeltaTime, WallMovementFriction, false, GetMaxBrakingDeceleration());
	}
	ApplyRootMotionToVelocity(DeltaTime);

	// Attack montages/root motion are allowed to carry the character along the
	// wall, but never away from it. Removing the wall-normal component keeps an
	// attack from peeling traversal off the surface.
	Velocity = FVector::VectorPlaneProject(Velocity, WallNormal);
	Velocity = Velocity.GetClampedToMaxSize(MaxWallSpeed);

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
			if (bDebugWallTraversal)
			{
				PH_LOG(LogPHWallTraversal, Log, "Exiting traversal: standing on walkable floor.");
			}
			BeginWallToGroundTransition(StandingGroundHit);
			return false;
		}
	}

	// Begin before collision when descending toward a floor that is still a little
	// below. This gives animation time to transfer one foot instead of snapping
	// both at impact.
	FHitResult NearbyGroundHit;
	if (!bUsingRootMotion && bMovingTowardGround && FindTransitionGround(NearbyGroundHit))
	{
		if (bDebugWallTraversal)
		{
			PH_LOG(LogPHWallTraversal, Log, "Exiting traversal: descending onto nearby floor.");
		}
		BeginWallToGroundTransition(NearbyGroundHit);
		return false;
	}

	// Movement remains tangent to the surface. Surface attachment is handled by
	// the post-move probe and snap below; also pushing into the surface here
	// makes collision sliding and snapping fight each other and causes jitter.
	const FVector MoveDelta = Velocity * DeltaTime;
	const FVector OldLocation = UpdatedComponent->GetComponentLocation();

	FQuat NewRotation = UpdatedComponent->GetComponentQuat();
	if (bAlignCapsuleToWall)
	{
		const FQuat TargetRotation = ComputeWallTraversalRotation();
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
		if (!bUsingRootMotion && bMovingTowardGround &&
			IsUsableGroundTransitionHit(MoveHit))
		{
			if (bDebugWallTraversal)
			{
				PH_LOG(LogPHWallTraversal, Log, "Exiting traversal: blocked by floor while descending.");
			}
			BeginWallToGroundTransition(MoveHit);
			return false;
		}

		SlideAlongSurface(MoveDelta, 1.0f - MoveHit.Time, MoveHit.Normal, MoveHit, true);
	}

	FHitResult PostMoveWallHit;
	TArray<FHitResult> PostMoveWallHits;
	if (TraceWallSurfaces(-WallNormal, PostMoveWallHits, false) &&
		BuildAveragedWallHit(PostMoveWallHits, PostMoveWallHit))
	{
		UpdateWallSurface(PostMoveWallHit, false, DeltaTime);
		SnapToWall(PostMoveWallHit);
	}
	else if (bHasWall)
	{
		// Keep the last valid contact during the short seam/corner grace window.
		SnapToWall(WallHit);
	}
	else
	{
		// During montage/root-motion frames the probe can briefly miss because
		// the animation pushes against the wall. Stay glued to the last stable
		// wall plane while the wall-lost grace is active.
		SnapToCurrentWall();
	}
	if (!bUsingRootMotion)
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / DeltaTime;
	}
	Velocity = FVector::VectorPlaneProject(Velocity, WallNormal);
	Velocity = Velocity.GetClampedToMaxSize(MaxWallSpeed);

	if (bDebugWallTraversal && GetWorld() && UpdatedComponent)
	{
		const FVector Origin = UpdatedComponent->GetComponentLocation();
		DrawDebugDirectionalArrow(GetWorld(), Origin, Origin + FVector(WallNormal) * 50.0f,
			12.0f, FColor::Cyan, false, -1.0f, 0, 1.5f);
		DrawDebugDirectionalArrow(GetWorld(), Origin, Origin + GetWallUp() * 50.0f,
			12.0f, FColor::Green, false, -1.0f, 0, 1.5f);
	}

	return true;
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
			if (FVector::DotProduct(ExistingDirection, NormalizedDirection) > DirectionDuplicateDot)
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
		if (bRequireInitialWall && !IsApproachingWall(CandidateHit))
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
	return FindGroundBelow(OutHit, GroundTransitionDetectionDistance);
}

bool UPHCharacterMovementComponent::IsStandingOnWalkableFloor(FHitResult& OutGroundHit) const
{
	return FindGroundBelow(OutGroundHit, GroundedContactMargin);
}

bool UPHCharacterMovementComponent::FindGroundBelow(
	FHitResult& OutGroundHit,
	const float ExtraDistance) const
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
		SCENE_QUERY_STAT(PHWallGroundBelow),
		false,
		CharacterOwner);
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const float GroundSupport =
		GetCapsuleSupportDistance(FVector::UpVector, UpdatedComponent->GetUpVector());
	const float ProbeRadius = FMath::Max(Capsule->GetScaledCapsuleRadius() * 0.5f, 5.0f);
	// The sweep's leading edge, rather than its center, must stop at the contact margin.
	const FVector End = Start - FVector::UpVector *
		FMath::Max(GroundSupport + ExtraDistance - ProbeRadius, 0.0f);

	const bool bHit = GetWorld()->SweepSingleByChannel(
		OutGroundHit,
		Start,
		End,
		FQuat::Identity,
		WallDetectionChannel,
		FCollisionShape::MakeSphere(ProbeRadius),
		QueryParams);

	return bHit && IsUsableGroundTransitionHit(OutGroundHit);
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
			// The refine ray runs center -> impact point, which is not parallel to
			// the sweep, so it can strike unrelated geometry (a rail, a pillar, a
			// door frame) on the way. Adopting that normal would leave the hit
			// describing one surface by normal and another by component and impact
			// point, and IsWalkable/IsWallSurface would then judge two different
			// surfaces. Only take the refinement when it lands on the same surface.
			FHitResult FaceHit;
			const FVector FaceEnd =
				CandidateHit.ImpactPoint + TraceDirection * (WallTraceRadius + FaceRefineExtension);
			if (GetWorld()->LineTraceSingleByChannel(
					FaceHit, Start, FaceEnd, WallDetectionChannel, QueryParams) &&
				FaceHit.IsValidBlockingHit() &&
				FaceHit.GetComponent() == CandidateHit.GetComponent())
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
			if (bSameTraversalComponent && NormalDot < SameComponentNormalFlipDot)
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

	if (bDebugWallTraversal && GetWorld())
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Silver, false, -1.0f, 0, 0.5f);
		for (const FHitResult& DrawHit : OutHits)
		{
			DrawDebugPoint(GetWorld(), DrawHit.ImpactPoint, 8.0f, FColor::Yellow, false, -1.0f);
		}
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
	if (!Hit.IsValidBlockingHit() ||
		!UPHWallTraversalFunctionLibrary::IsValidWallTraversalSurface(Hit.GetComponent()))
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

bool UPHCharacterMovementComponent::IsUsableGroundTransitionHit(
	const FHitResult& Hit) const
{
	if (!ShouldTransitionToGround(Hit) || !UpdatedComponent)
	{
		return false;
	}

	// Walkable faces beside a wall edge can overlap the capsule sweep even when
	// they are not actually under the character. Require the candidate ground to
	// be below the capsule center before starting the wall-to-ground transfer.
	const float BelowCenter = FVector::DotProduct(
		UpdatedComponent->GetComponentLocation() - Hit.ImpactPoint,
		FVector::UpVector);
	return BelowCenter >= MinGroundTransitionBelowCenter;
}

void UPHCharacterMovementComponent::UpdateWallSurface(
	const FHitResult& WallHit,
	const bool bInitialAttach,
	const float DeltaTime)
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

	if (!bInitialAttach && !WallNormal.IsNearlyZero())
	{
		const float NormalAlpha =
			1.0f - FMath::Exp(-WallSurfaceNormalInterpSpeed * DeltaTime);
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
	if (!WallHit.IsValidBlockingHit())
	{
		return;
	}

	FVector SnapNormal = FVector(WallNormal).GetSafeNormal();
	if (SnapNormal.IsNearlyZero())
	{
		SnapNormal = WallHit.ImpactNormal.GetSafeNormal();
	}
	if (SnapNormal.IsNearlyZero())
	{
		return;
	}

	SnapToWallPlane(WallHit.ImpactPoint, SnapNormal, bSnapRotation);
}

void UPHCharacterMovementComponent::SnapToCurrentWall(const bool bSnapRotation)
{
	SnapToWallPlane(
		WallImpactPoint,
		FVector(WallNormal).GetSafeNormal(),
		bSnapRotation);
}

void UPHCharacterMovementComponent::SnapToWallPlane(
	const FVector& SurfacePoint,
	const FVector& SurfaceNormal,
	const bool bSnapRotation)
{
	if (!UpdatedComponent || SurfaceNormal.IsNearlyZero())
	{
		return;
	}

	FQuat TargetRotation = UpdatedComponent->GetComponentQuat();
	FVector TargetCapsuleUp = UpdatedComponent->GetUpVector();
	if (bSnapRotation)
	{
		TargetRotation = ComputeWallTraversalRotation();
		TargetCapsuleUp = TargetRotation.GetAxisZ();
	}

	const float CurrentDistance = FVector::DotProduct(
		UpdatedComponent->GetComponentLocation() - SurfacePoint,
		SurfaceNormal);
	const float DistanceError =
		CurrentDistance - GetDesiredWallDistance(TargetCapsuleUp);

	if (FMath::Abs(DistanceError) <= 1.0f &&
		TargetRotation.Equals(UpdatedComponent->GetComponentQuat()))
	{
		return;
	}

	FHitResult CorrectionHit;
	SafeMoveUpdatedComponent(
		-SurfaceNormal * DistanceError,
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
	for (int32 Pass = 0; Pass < WallToGroundSolverPasses; ++Pass)
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
	ClearWallTraversalState();
	WallToGroundElapsed = 0.0f;
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

	// Ordinary wall loss does not arm the reattach cooldown. The top-mantle path
	// below is the exception: it needs one short frame window where mantle wins
	// over the held traversal input's immediate wall-reattach retry.
	if (bDebugWallTraversal)
	{
		PH_LOG(LogPHWallTraversal, Log,
			"Wall lost: entering falling%s.",
			bLeavingUpwardEdge ? TEXT(" (attempting top mantle)") : TEXT(""));
	}

	// Order matters: RestoreWorldUpRotation derives the upright facing from the
	// wall normal, which ClearWallTraversalState now resets.
	RestoreWorldUpRotation();
	ClearWallTraversalState();
	if (bLeavingUpwardEdge)
	{
		// Set the normal short cooldown before entering falling so the movement
		// mode callback cannot immediately reattach while top mantle is probing.
		RecordWallDetachTime();
	}
	SetMovementMode(MOVE_Falling);

	if (bLeavingUpwardEdge)
	{
		if (APHBaseCharacter* PHCharacter = Cast<APHBaseCharacter>(CharacterOwner))
		{
			PHCharacter->TryWallTopMantle();
		}
	}
}

FQuat UPHCharacterMovementComponent::ComputeWallTraversalRotation() const
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

void UPHCharacterMovementComponent::UpdateWallTraversalCombatMovementScale()
{
	if (WallTraversalCombatMovementScaleEndTime < 0.0f)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World || World->GetTimeSeconds() < WallTraversalCombatMovementScaleEndTime)
	{
		return;
	}

	ClearWallTraversalCombatMovementScale();
}

void UPHCharacterMovementComponent::UpdateWallAttachRetry(const float DeltaTime)
{
	// Not every wall is met head-on: drifting alongside one never produces the
	// impact that HandleImpact attaches from, so falling is probed periodically.
	// This runs from the movement update, which the server replays move for move,
	// rather than from Tick, whose wall-clock timer free-runs per machine and
	// would let the two sides attach on different frames.
	if (!CharacterOwner ||
		CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy ||
		!bWantsWallTraversalInput ||
		MovementMode != MOVE_Falling ||
		IsWallTraversing())
	{
		WallAttachRetryAccumulator = 0.0f;
		return;
	}

	WallAttachRetryAccumulator += DeltaTime;
	if (WallAttachRetryAccumulator < WallAttachRetryInterval)
	{
		return;
	}

	WallAttachRetryAccumulator = 0.0f;

	const APHBaseCharacter* PHCharacter = Cast<APHBaseCharacter>(CharacterOwner);
	if (!PHCharacter)
	{
		return;
	}

	const EALSMovementState RequestedState =
		PHCharacter->SelectWallTraversalState(PHCharacter->GetWallTraversalWeight());
	if (RequestedState == EALSMovementState::WallRunning ||
		RequestedState == EALSMovementState::WallClimbing)
	{
		TryStartWallTraversal(RequestedState);
	}
}

void UPHCharacterMovementComponent::ApplyWallTraversalRootMotionMode()
{
	if (!bIgnoreRootMotionWhileWallTraversing || !CharacterOwner)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = CharacterOwner->GetMesh();
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	if (!bHasSavedWallTraversalRootMotionMode)
	{
		SavedWallTraversalRootMotionMode = AnimInstance->RootMotionMode;
		bHasSavedWallTraversalRootMotionMode = true;
	}

	if (AnimInstance->RootMotionMode != ERootMotionMode::IgnoreRootMotion)
	{
		AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}
}

void UPHCharacterMovementComponent::RestoreWallTraversalRootMotionMode()
{
	if (!bHasSavedWallTraversalRootMotionMode)
	{
		return;
	}

	const TEnumAsByte<ERootMotionMode::Type> ModeToRestore =
		SavedWallTraversalRootMotionMode;
	bHasSavedWallTraversalRootMotionMode = false;

	USkeletalMeshComponent* Mesh = CharacterOwner ? CharacterOwner->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (AnimInstance && AnimInstance->RootMotionMode != ModeToRestore)
	{
		AnimInstance->SetRootMotionMode(ModeToRestore);
	}
}

void UPHCharacterMovementComponent::ClearWallTraversalState()
{
	RestoreWallTraversalRootMotionMode();
	WallTransitionData = FALSWallTransitionData();
	WallSurfaceComponent.Reset();
	WallLostFrames = 0;
	WallTraversalElapsed = 0.0f;
	WallAttachRetryAccumulator = 0.0f;
	WallToGroundElapsed = 0.0f;
	ClearWallTraversalCombatMovementScale();

	// The wall basis is read by GetWallNormal/GetWallUp/GetWallTraversalRotation,
	// which the anim layer polls; leaving it set makes a detached character keep
	// reporting the wall it just left. Callers that need the outgoing wall must
	// read it (RestoreWorldUpRotation, JumpOffWall) before calling this.
	WallNormal = FVector::ZeroVector;
	WallUpDirection = FVector::UpVector;
	WallImpactPoint = FVector::ZeroVector;
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

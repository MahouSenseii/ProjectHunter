#pragma once

#include "CoreMinimal.h"
#include "Character/ALSCharacterMovementComponent.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "Library/ALSCharacterStructLibrary.h"
#include "PHCharacterMovementComponent.generated.h"

class UPrimitiveComponent;

UENUM(BlueprintType)
enum class EPHCustomMovementMode : uint8
{
	None = 0 UMETA(Hidden),
	WallRunning = 1 UMETA(DisplayName = "Wall Running"),
	WallClimbing = 2 UMETA(DisplayName = "Wall Climbing"),
	Gliding = 3 UMETA(DisplayName = "Gliding"),
	WallToGround = 4 UMETA(DisplayName = "Wall To Ground")
};

/**
 * ProjectHunter movement physics.
 *
 * Wall running and wall climbing use the same surface-locomotion physics.
 * Their custom-mode value selects speed, animation state, and future stamina
 * rules without duplicating the movement implementation.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHCharacterMovementComponent : public UALSCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UPHCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Attempts to attach using a known impact when available, otherwise searches
	 * nearby surfaces. Passing the falling impact prevents sprinting characters
	 * from bouncing off a wall before the periodic attach trace sees it.
	 */
	bool TryStartWallTraversal(
		EALSMovementState RequestedState,
		const FHitResult* PreferredWallHit = nullptr);

	/** Detaches from the current wall and enters falling movement. */
	UFUNCTION(BlueprintCallable, Category = "Movement|Wall Traversal")
	void StopWallTraversal();

	/** Pushes the character away from the current wall and enters falling movement. */
	UFUNCTION(BlueprintCallable, Category = "Movement|Wall Traversal")
	void JumpOffWall();

	UFUNCTION(BlueprintPure, Category = "Movement|Wall Traversal")
	bool IsWallTraversing() const;

	UFUNCTION(BlueprintPure, Category = "Movement|Wall Traversal")
	bool IsWallRunning() const;

	UFUNCTION(BlueprintPure, Category = "Movement|Wall Traversal")
	bool IsWallClimbing() const;

	UFUNCTION(BlueprintPure, Category = "Movement|Wall Traversal")
	bool IsTransitioningFromWallToGround() const;

	UFUNCTION(BlueprintPure, Category = "Movement|Wall Traversal")
	FALSWallTransitionData GetWallTransitionData() const { return WallTransitionData; }

	/** Completes the transition early, normally from an animation notify. */
	UFUNCTION(BlueprintCallable, Category = "Movement|Wall Traversal")
	void CompleteWallToGroundTransition();

	UFUNCTION(BlueprintPure, Category = "Movement|Wall Traversal")
	FVector GetWallNormal() const { return WallNormal; }

	/** Up axis of the current wall surface. */
	UFUNCTION(BlueprintPure, Category = "Movement|Wall Traversal")
	FVector GetWallUp() const;

	/** Right axis of the current wall surface. */
	UFUNCTION(BlueprintPure, Category = "Movement|Wall Traversal")
	FVector GetWallRight() const;

	/** Restores the capsule to world-up before falling, mantling, or ragdolling. */
	void RestoreWorldUpRotation();

	/**
	 * Converts an ALS camera-relative world direction into wall movement:
	 * along the wall stays lateral, toward the wall becomes up, and away
	 * from the wall becomes down.
	 */
	UFUNCTION(BlueprintPure, Category = "Movement|Wall Traversal")
	FVector ConvertWorldDirectionToWallDirection(const FVector& WorldDirection) const;

	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
	virtual float GetMaxBrakingDeceleration() const override;
	virtual void HandleImpact(
		const FHitResult& Hit,
		float TimeSlice = 0.0f,
		const FVector& MoveDelta = FVector::ZeroVector) override;

protected:
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

	void PhysWallTraversal(float DeltaTime, int32 Iterations);
	void PhysWallToGroundTransition(float DeltaTime, int32 Iterations);
	bool FindAttachableWall(FHitResult& OutHit, bool bRequireInitialWall = true) const;
	bool FindTransitionGround(FHitResult& OutHit) const;
	bool TraceWall(
		const FVector& Direction,
		FHitResult& OutHit,
		bool bRequireInitialWall = true) const;
	bool IsWallSurface(const FHitResult& Hit, bool bRequireInitialWall) const;
	bool IsCurrentTraversalSurface(const FHitResult& Hit) const;
	bool ShouldTransitionToGround(const FHitResult& Hit) const;
	void UpdateWallSurface(const FHitResult& WallHit, bool bInitialAttach);
	void SnapToWall(const FHitResult& WallHit, bool bSnapRotation = false);
	void BeginWallToGroundTransition(const FHitResult& GroundHit);
	bool ChooseGroundTransitionFoot(const FHitResult& GroundHit) const;
	void HandleWallLost();
	FQuat GetWallTraversalRotation() const;
	FQuat GetWorldUpRotation() const;
	float GetDesiredWallDistance() const;
	float GetDesiredWallDistance(const FVector& CapsuleUp) const;
	float GetCapsuleSupportDistance(const FVector& SurfaceNormal, const FVector& CapsuleUp) const;
	float GetWallTraversalSpeed() const;
	void RecordWallDetachTime();

	UFUNCTION()
	void OnRep_WallNormal();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Detection",
		meta = (ClampMin = "0.0"))
	float WallDetectionReach = 55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Detection",
		meta = (ClampMin = "0.0"))
	float WallTraceRadius = 14.0f;

	/**
	 * Maximum upward normal Z for initial attachment. Continued traversal may
	 * follow floors, curved tops, and ceilings; normal ground is still rejected
	 * as an initial wall attachment.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Detection",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxWallNormalZ = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Detection")
	TEnumAsByte<ECollisionChannel> WallDetectionChannel = ECC_Visibility;

	/** Gap between the capsule and wall after accounting for capsule radius. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Physics",
		meta = (ClampMin = "0.0"))
	float WallSurfaceGap = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Physics",
		meta = (ClampMin = "0.0"))
	float WallAdhesionSpeed = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Physics",
		meta = (ClampMin = "0.0"))
	float WallMovementFriction = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Physics",
		meta = (ClampMin = "0.0"))
	float WallAcceleration = 2048.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Physics",
		meta = (ClampMin = "0.0"))
	float WallBrakingDeceleration = 2048.0f;

	/** Rotates the capsule so its feet/end cap are planted against the wall. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Rotation")
	bool bAlignCapsuleToWall = true;

	/**
	 * Makes the wall become the character's floor on the attachment frame.
	 * Rotation interpolation still handles later changes around curves/corners.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Rotation")
	bool bSnapRotationOnAttach = true;

	/** How quickly the capsule aligns with changing wall surfaces and travel direction. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Rotation",
		meta = (ClampMin = "0.0"))
	float WallRotationInterpSpeed = 12.0f;

	/** Multiplies the current ALS sprint speed while wall running. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Speed",
		meta = (ClampMin = "0.0"))
	float WallRunningSpeedMultiplier = 1.0f;

	/** Multiplies the current ALS run speed while wall climbing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Speed",
		meta = (ClampMin = "0.0"))
	float WallClimbingSpeedMultiplier = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Speed",
		meta = (ClampMin = "0.0"))
	float WallRunningFallbackSpeed = 650.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Speed",
		meta = (ClampMin = "0.0"))
	float WallClimbingFallbackSpeed = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Jump",
		meta = (ClampMin = "0.0"))
	float WallJumpAwaySpeed = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Jump",
		meta = (ClampMin = "0.0"))
	float WallJumpUpSpeed = 550.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Jump",
		meta = (ClampMin = "0.0"))
	float WallReattachCooldown = 0.25f;

	/**
	 * Number of consecutive frames the wall trace may miss before detaching.
	 * Bridges brief losses over corners, seams, and noisy sweep normals so a
	 * single missed frame does not drop the character off the wall.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Detection",
		meta = (ClampMin = "0"))
	int32 MaxConsecutiveWallLostFrames = 3;

	/** How far ahead/down the wall to start the one-foot-at-a-time floor transfer. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Transition",
		meta = (ClampMin = "0.0"))
	float GroundTransitionDetectionDistance = 75.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Transition",
		meta = (ClampMin = "0.05"))
	float WallToGroundTransitionDuration = 0.4f;

	/** Retains some run momentum while the capsule rotates toward world-up. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Transition",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WallToGroundMovementScale = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Transition")
	FName TransitionLeftFootBone = TEXT("ik_foot_l");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Transition")
	FName TransitionRightFootBone = TEXT("ik_foot_r");

	/**
	 * Surface up-axis for wall traversal. Custom movement mode/transform are
	 * already replicated by CharacterMovement; this supplies remote animation
	 * and wall IK with the matching surface orientation.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_WallNormal)
	FVector_NetQuantizeNormal WallNormal = FVector::ZeroVector;

	/** Tangent transported across changing normals so arcs never lose "forward/up". */
	UPROPERTY(Replicated)
	FVector_NetQuantizeNormal WallUpDirection = FVector::UpVector;

	UPROPERTY(Replicated)
	FALSWallTransitionData WallTransitionData;

	FVector WallImpactPoint = FVector::ZeroVector;
	TWeakObjectPtr<UPrimitiveComponent> WallSurfaceComponent;
	float LastWallDetachTime = -BIG_NUMBER;
	int32 WallLostFrames = 0;
	float WallToGroundElapsed = 0.0f;
	FVector WallToGroundStartLocation = FVector::ZeroVector;
	FVector WallToGroundTargetLocation = FVector::ZeroVector;
	FVector WallToGroundPlanarVelocity = FVector::ZeroVector;
	FQuat WallToGroundStartRotation = FQuat::Identity;
	FQuat WallToGroundTargetRotation = FQuat::Identity;
};

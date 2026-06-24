#pragma once

#include "CoreMinimal.h"
#include "Character/ALSCharacterMovementComponent.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "PHCharacterMovementComponent.generated.h"

UENUM(BlueprintType)
enum class EPHCustomMovementMode : uint8
{
	None = 0 UMETA(Hidden),
	WallRunning = 1 UMETA(DisplayName = "Wall Running"),
	WallClimbing = 2 UMETA(DisplayName = "Wall Climbing"),
	Gliding = 3 UMETA(DisplayName = "Gliding")
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

	/** Attempts to attach to a nearby wall using the requested ALS movement state. */
	bool TryStartWallTraversal(EALSMovementState RequestedState);

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

protected:
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

	void PhysWallTraversal(float DeltaTime, int32 Iterations);
	bool FindAttachableWall(FHitResult& OutHit) const;
	bool TraceWall(const FVector& Direction, FHitResult& OutHit) const;
	bool IsWallSurface(const FHitResult& Hit) const;
	void SnapToWall(const FHitResult& WallHit);
	void HandleWallLost();
	FQuat GetWallTraversalRotation() const;
	float GetDesiredWallDistance() const;
	float GetWallTraversalSpeed() const;
	void RecordWallDetachTime();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Detection",
		meta = (ClampMin = "0.0"))
	float WallDetectionReach = 55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Wall Traversal|Detection",
		meta = (ClampMin = "0.0"))
	float WallTraceRadius = 14.0f;

	/** Maximum absolute wall-normal Z. Lower values require a more vertical wall. */
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

	FVector WallNormal = FVector::ZeroVector;
	FVector WallImpactPoint = FVector::ZeroVector;
	float LastWallDetachTime = -BIG_NUMBER;
};

// Fill out your copyright notice in the Description page of Project Settings.

// World/Actor/ISMContainerActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ISMContainerActor.generated.h"

class UInstancedStaticMeshComponent;

// ═══════════════════════════════════════════════════════════════════════
// ANIMATION STATE
// ═══════════════════════════════════════════════════════════════════════

/**
 * Per-item animation bookkeeping. Stored in AISMContainerActor so
 * Tick can drive spin + bob without touching the subsystem every frame.
 */
USTRUCT()
struct FGroundItemAnimState
{
	GENERATED_BODY()

	UPROPERTY()
	UInstancedStaticMeshComponent* ISMComponent = nullptr;

	/** ISM instance index — kept in sync by UpdateItemAnimationIndex after swap-pop */
	int32 InstanceIndex = INDEX_NONE;

	/** World-space base location (no bob applied) */
	FVector BaseLocation = FVector::ZeroVector;

	/** Preserved pitch from flip + offset.  Only Yaw is animated. */
	float BasePitch = 0.0f;

	/** Preserved roll from offset. Only Yaw is animated. */
	float BaseRoll = 0.0f;

	/** Unique per-item phase in radians so items don't all sync up */
	float PhaseOffset = 0.0f;

	bool IsValid() const { return ISMComponent != nullptr && InstanceIndex != INDEX_NONE; }
};

/**
 * AISMContainerActor
 *
 * Simple container actor for ISM components managed by UGroundItemSubsystem.
 * Also owns the per-frame spin + bob animation for all ground items.
 */
UCLASS(NotBlueprintable, NotPlaceable)
class ALS_PROJECTHUNTER_API AISMContainerActor : public AActor
{
	GENERATED_BODY()

public:
	AISMContainerActor();

	virtual void Tick(float DeltaTime) override;

	// ═══════════════════════════════════════════════
	// ANIMATION CONFIG
	// ═══════════════════════════════════════════════

	/** Degrees per second the item spins on its Yaw axis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Items|Animation")
	float SpinDegreesPerSecond = 90.0f;

	/** Vertical bob amplitude in world units (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Items|Animation")
	float BobAmplitudeCm = 8.0f;

	/** Bob cycles per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Items|Animation")
	float BobFrequencyHz = 1.0f;

	// ═══════════════════════════════════════════════
	// ANIMATION REGISTRATION (called by GroundItemSubsystem)
	// ═══════════════════════════════════════════════

	/**
	 * Register a new ground item for spin+bob animation.
	 * Call immediately after AddInstance in GroundItemSubsystem.
	 */
	void RegisterItemForAnimation(
		int32 ItemID,
		UInstancedStaticMeshComponent* ISM,
		int32 InstanceIndex,
		FVector BaseLocation,
		FRotator BaseRotation);

	/**
	 * Unregister a ground item so it no longer animates.
	 * Call before RemoveInstance in GroundItemSubsystem.
	 */
	void UnregisterItemFromAnimation(int32 ItemID);

	/**
	 * Update the stored ISM instance index after a swap-pop removal.
	 * GroundItemSubsystem calls this inside UpdateIndexAfterSwap whenever
	 * an item's ISM index changes.
	 */
	void UpdateItemAnimationIndex(int32 ItemID, int32 NewInstanceIndex);

	/** Clear all animation state (called by GroundItemSubsystem::ClearAllItems) */
	void ClearAllAnimationState();

protected:
	/** Root component (required for actor to exist) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;

private:
	/**
	 * Per-item animation bookkeeping, keyed by the subsystem's ItemID.
	 * Marked UPROPERTY so the GC tracks the embedded ISMComponent pointer.
	 */
	UPROPERTY()
	TMap<int32, FGroundItemAnimState> AnimationStates;

	/** Accumulated world time used for deterministic animation */
	float AnimationTime = 0.0f;
};

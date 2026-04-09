// Character/Component/Interaction/InteractionTraceManager.h
#pragma once

#include "CoreMinimal.h"
#include "InteractionTraceManager.generated.h"

// Forward declarations
class IInteractable;
class UItemInstance;
class UGroundItemSubsystem;
class APlayerController;
class AALSPlayerCameraManager;
struct FInteractionDebugManager;

DECLARE_LOG_CATEGORY_EXTERN(LogInteractionTraceManager, Log, All);

/**
 * Interaction Trace Manager
 * 
 * SINGLE RESPONSIBILITY: Perform traces to find interactables
 * - Actor interactable detection (line trace)
 * - Ground item detection (radius query)
 * - Camera view point calculation (ALS-aware)
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FInteractionTraceManager
{
	GENERATED_BODY()

public:
	FInteractionTraceManager();

	// ═══════════════════════════════════════════════
	// INITIALIZATION
	// ═══════════════════════════════════════════════

	void Initialize(AActor* Owner, UWorld* World);
	void SetDebugManager(FInteractionDebugManager* InDebugManager);

	// ═══════════════════════════════════════════════
	// CONFIGURATION
	// ═══════════════════════════════════════════════

	/** Maximum interaction distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	float InteractionDistance;

	/** How often to check for interactables (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	float CheckFrequency;

	/** Collision channel for interaction traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	TEnumAsByte<ECollisionChannel> InteractionTraceChannel;

	/** Use ALS camera origin calculation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|ALS")
	bool bUseALSCameraOrigin;

	/** Forward offset from pivot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|ALS", meta = (EditCondition = "bUseALSCameraOrigin"))
	float OffsetForward;

	/** Right offset from pivot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|ALS", meta = (EditCondition = "bUseALSCameraOrigin"))
	float OffsetRight;

	/** Up offset from pivot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|ALS", meta = (EditCondition = "bUseALSCameraOrigin"))
	float OffsetUp;

	// ═══════════════════════════════════════════════
	// PRIMARY FUNCTIONS
	// ═══════════════════════════════════════════════

	/**
	 * Trace for actor-based interactables
	 * @return Interface to found interactable, or null
	 */
	TScriptInterface<IInteractable> TraceForActorInteractable();

	/**
	 * Find nearest ground item within interaction distance
	 * @param OutItemID - Output item ID
	 * @return Item instance if found
	 */
	UItemInstance* FindNearestGroundItem(int32& OutItemID);

	/**
	 * Line-trace detection for ground items — mirrors how actor interactables
	 * (loot chests, portals) are detected. Performs a visibility trace and checks
	 * whether the hit component is an InstancedStaticMeshComponent; if so, uses
	 * FHitResult::Item (the ISM instance index) to reverse-look up the ItemID.
	 *
	 * This gives ground items the same "look-at" UX as actor interactables, as
	 * opposed to the proximity fallback (FindNearestGroundItem) which is
	 * direction-agnostic.
	 *
	 * @return ItemID of the ground item the player is looking at, or INDEX_NONE
	 */
	int32 FindGroundItemByTrace();

	/**
	 * Get camera view point with ALS-style offsets
	 * @param OutLocation - Camera location
	 * @param OutRotation - Camera rotation
	 * @return True if successful
	 */
	bool GetCameraViewPoint(FVector& OutLocation, FRotator& OutRotation);


	// B-1 FIX: Non-const because GetCameraViewPoint refreshes the cached controller.
	void GetTraceOrigin(FVector& OutCameraLocation, FVector& OutCameraDirection);
	
	/**
	 * Check if owner is locally controlled
	 */
	bool IsLocallyControlled() const;

	/** Get last trace hit result */
	const FHitResult& GetLastTraceResult() const { return LastTraceResult; }

private:
	// ═══════════════════════════════════════════════
	// INTERNAL HELPERS
	// ═══════════════════════════════════════════════

	void CacheComponents();
	FVector GetTraceStartLocation(const FVector& CameraLocation, const FRotator& CameraRotation) const;
	FVector GetTraceEndLocation(const FVector& CameraLocation, const FRotator& CameraRotation) const;
	bool PerformLineTrace(const FVector& Start, const FVector& End, FHitResult& OutHit);
	bool IsActorInteractable(AActor* Actor) const;

	// ═══════════════════════════════════════════════
	// CACHED REFERENCES
	// ═══════════════════════════════════════════════

	AActor* OwnerActor;
	UWorld* WorldContext;
	APlayerController* CachedPlayerController;
	AALSPlayerCameraManager* CachedALSCameraManager;
	UGroundItemSubsystem* CachedGroundItemSubsystem;
	FInteractionDebugManager* DebugManager;

	// ═══════════════════════════════════════════════
	// STATE
	// ═══════════════════════════════════════════════

	FHitResult LastTraceResult;
};

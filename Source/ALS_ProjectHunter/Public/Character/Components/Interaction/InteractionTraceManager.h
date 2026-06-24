// Character/Components/Interaction/InteractionTraceManager.h
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

	/**
	 * Collision channel for interaction traces.
	 *
	 * Defaults to ECC_Visibility, which works because interactables (chests,
	 * portals) happen to block Visibility — but it also lets the focus trace
	 * hit ANY Visibility-blocking geometry. The project's dedicated channel is
	 * PHInteractionChannels::Interaction (Interactable/Library/InteractionChannels.h);
	 * switch this to it once all interactable Blueprints block that channel,
	 * and LootChest's bBlockInteractable toggle becomes fully meaningful.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	TEnumAsByte<ECollisionChannel> InteractionTraceChannel;

	// ═══════════════════════════════════════════════
	// AIM / CANDIDATE SCORING
	// ═══════════════════════════════════════════════

	/**
	 * Radius of the sphere overlap performed at the line-trace hit point (cm).
	 * Every interactable inside this sphere is scored by dot product and the
	 * most-centred one wins — no minimum-dot rejection gate.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "1.0", ClampMax = "300.0"))
	float OverlapRadius = 75.0f;

	/**
	 * Player-facing gate: target must be in front of the character, not only in
	 * front of the camera. 0 = front half-space (anything not strictly behind).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "-1.0", ClampMax = "0.999"))
	float MinPlayerForwardDot = 0.0f;

	/**
	 * Hysteresis: dot bonus given to the currently focused item so a new
	 * candidate must be meaningfully more centred to steal focus.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "0.0", ClampMax = "0.1"))
	float CurrentFocusDotBonus = 0.02f;

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
	 * Trace for actor-based interactables (delegates to FindBestInteractionTarget).
	 * @return Interface to found interactable, or null
	 */
	TScriptInterface<IInteractable> TraceForActorInteractable();

	/**
	 * Unified interaction search:
	 *   1. Line trace up to InteractionDistance.
	 *   2. Sphere overlap (OverlapRadius) at the hit point.
	 *   3. Dot-product scoring — highest dot wins, no rejection threshold.
	 *
	 * @param CurrentInteractable  Currently focused actor interactable (for hysteresis).
	 * @param CurrentItemID        Currently focused ground item ID (for hysteresis).
	 */
	void FindBestInteractionTarget(
		const TScriptInterface<IInteractable>& CurrentInteractable,
		int32 CurrentItemID,
		TScriptInterface<IInteractable>& OutInteractable,
		int32& OutGroundItemID);

	/**
	 * Find nearest ground item within interaction distance.
	 * @param OutItemID - Output item ID
	 * @return Item instance if found
	 */
	UItemInstance* FindNearestGroundItem(int32& OutItemID);

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
	 * Returns the world-space point the active trace fires from.
	 * When bUseALSCameraOrigin is true and the ALS camera is present this is the
	 * actual camera location; otherwise it falls back to the pivot + offsets
	 * calculation.  Use this for debug visualisations that must match the trace.
	 */
	FVector GetTraceStart();

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
	bool PerformLineTrace(const FVector& Start, const FVector& End, FHitResult& OutHit);
	bool IsActorInteractable(AActor* Actor) const;

	/** Wrap an interactable actor (component preferred, interface fallback). */
	TScriptInterface<IInteractable> MakeInteractableInterface(AActor* Actor) const;

	/** Returns true when TargetLocation is in the character's front half-space. */
	bool PassesPlayerForwardGate(const FVector& TargetLocation, float& OutDot) const;

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

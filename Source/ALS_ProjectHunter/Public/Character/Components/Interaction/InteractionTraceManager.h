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
	// AIM FORGIVENESS / CANDIDATE SCORING
	// All tunable per-character in BP. Defaults give a "feels right" baseline.
	// ═══════════════════════════════════════════════

	/**
	 * Sphere radius for the primary aim trace (cm). The old pixel-perfect line
	 * trace required dead-center aim and slipped off mesh edges constantly.
	 * 0 = legacy line-trace behavior.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float TraceSphereRadius = 20.0f;

	/**
	 * LOOK-AT GATE: every focus target must pass
	 *   dot(CameraForward, normalize(Target - Camera)) >= MinLookAtDot.
	 * The trace HIT finds the target first; the dot product only validates
	 * that the camera is actually looking at that traced interactable.
	 *   0.95 ≈ 18°   0.966 ≈ 15°   0.985 ≈ 10°
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "0.7", ClampMax = "0.999"))
	float MinLookAtDot = 0.95f;

	/** Look-at gate for ground items (tighter — loot piles need precision). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "0.7", ClampMax = "0.999"))
	float MinGroundLookAtDot = 0.965f;

	/**
	 * Player-facing gate: every focus target must also be in front of the
	 * character, not only in front of the camera. 0 = front half-space.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "-1.0", ClampMax = "0.999"))
	float MinPlayerForwardDot = 0.0f;

	/**
	 * Max distance from the camera ray for ground-item aim candidates (cm).
	 * This keeps loot selection precise in piles: items must be near the ray
	 * and inside the dot-product cone before they can compete for focus.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "0.0", ClampMax = "150.0"))
	float GroundItemAimRadius = 45.0f;

	/**
	 * When the camera sweep hits a ground item, only score other ground items
	 * this far behind that hit along the camera ray. This keeps the dot-product
	 * pass focused on overlapping/nearby loot instead of distant items farther
	 * down the same view line.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "0.0", ClampMax = "300.0"))
	float GroundItemTraceDepthTolerance = 80.0f;

	/**
	 * Hysteresis as a dot bonus for the current focus (0.02 ≈ a few degrees):
	 * a new target must be meaningfully more centered to steal focus.
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
	 * Trace for actor-based interactables
	 * @return Interface to found interactable, or null
	 */
	TScriptInterface<IInteractable> TraceForActorInteractable();

	/**
	 * Actor-interactable search: the first camera trace must hit an
	 * interactable, then camera and player-forward dot gates validate it.
	 */
	TScriptInterface<IInteractable> FindBestActorInteractable();

	/**
	 * Find nearest ground item within interaction distance
	 * @param OutItemID - Output item ID
	 * @return Item instance if found
	 */
	UItemInstance* FindNearestGroundItem(int32& OutItemID);

	/**
	 * Camera-trace detection for ground items. The first trace must hit a
	 * registered ground-item ISM instance; only then can nearby items around
	 * that hit depth be scored by dot product.
	 *
	 * This gives ground items the same "look-at" UX as actor interactables
	 * without accepting the first overlapping ISM hit as the winner.
	 *
	 * @param CurrentItemID - currently focused ground item for stickiness.
	 * @return ItemID of the ground item the player is looking at, or INDEX_NONE
	 */
	int32 FindGroundItemByTrace(int32 CurrentItemID);

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

	/** Sphere sweep when TraceSphereRadius > 0, line trace otherwise. */
	bool PerformAimTrace(const FVector& Start, const FVector& End, FHitResult& OutHit);

	bool IsActorInteractable(AActor* Actor) const;

	/** Wrap an interactable actor (component preferred, interface fallback). */
	TScriptInterface<IInteractable> MakeInteractableInterface(AActor* Actor) const;

	/**
	 * The look-at gate: dot(Forward, normalize(Target - ViewOrigin)).
	 * Returns the dot via OutDot; true when it passes MinDot.
	 */
	bool PassesLookAtGate(const FVector& ViewOrigin, const FVector& Forward,
		const FVector& TargetLocation, float MinDot, float& OutDot) const;

	/** Gate against the owning character's forward vector. */
	bool PassesPlayerForwardGate(const FVector& TargetLocation, float& OutDot) const;

	/** Shared gate helper: target must be camera-aligned and in front of player. */
	bool PassesCameraAndPlayerGates(const FVector& ViewOrigin, const FVector& CameraForward,
		const FVector& TargetLocation, float MinCameraDot, float& OutCameraDot, float& OutPlayerDot) const;

	/** Score nearby ground items only after the camera trace hit a ground item. */
	int32 FindBestGroundItemFromTraceHit(int32 CurrentItemID, float HitProjectionDistance);

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

#pragma once

#include "CoreMinimal.h"
#include "InteractionTraceManager.generated.h"
class IInteractable;
class UItemInstance;
class UGroundItemSubsystem;
class APlayerController;
class AALSPlayerCameraManager;
struct FInteractionDebugManager;

DECLARE_LOG_CATEGORY_EXTERN(LogInteractionTraceManager, Log, All);

/** One ranked ground-item result produced by the stateless trace helper. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FGroundItemInteractionCandidate
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Ground Items")
	int32 ItemID = INDEX_NONE;

	/** Aim score, including the current-focus hysteresis bonus. 1.0 = dead on the aim ray. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Ground Items")
	float Score = -1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Ground Items")
	float Distance = 0.0f;

	/** Perpendicular distance from the aim ray in cm - what the score ranks on. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Ground Items")
	float AimOffset = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Ground Items")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Ground Items")
	float CameraForwardDot = -1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Ground Items")
	float PlayerForwardDot = -1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Ground Items")
	float FocusBonus = 0.0f;
};

/**
 * Interaction Trace Manager
 *
 * Query and score interactables around the player
 * - Actor interactable detection (sphere overlap)
 * - Ground item detection (saved-location radius query)
 * - Camera view point calculation (ALS-aware)
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FInteractionTraceManager
{
	GENERATED_BODY()

public:
	FInteractionTraceManager();

	// INITIALIZATION

	void Initialize(AActor* Owner, UWorld* World);
	void SetDebugManager(FInteractionDebugManager* InDebugManager);

	// CONFIGURATION

	/** Maximum interaction distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	float InteractionDistance;

	/** How often to check for interactables (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	float CheckFrequency;

	/** Low-cost discovery rate while the player proximity sphere is empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float IdleCheckFrequency = 0.20f;

	/**
	 * Collision channel for actor-interactable overlap queries.
	 *
	 * Defaults to ECC_Visibility, which works because interactables (chests,
	 * portals) happen to block Visibility - but it also lets the focus trace
	 * hit ANY Visibility-blocking geometry. The project's dedicated channel is
	 * PHInteractionChannels::Interaction (Interactable/Library/InteractionChannels.h);
	 * switch this to it once all interactable Blueprints block that channel,
	 * and LootChest's bBlockInteractable toggle becomes fully meaningful.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	TEnumAsByte<ECollisionChannel> InteractionTraceChannel;

	// AIM / CANDIDATE SCORING

	/** Deprecated: player-centered searches now use InteractionDistance as their radius. */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use InteractionDistance. Searches are now player-centered."))
	float OverlapRadius = 75.0f;

	/** Minimum camera-facing dot required before a candidate can be selected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "-1.0", ClampMax = "0.999"))
	float MinCameraForwardDot = 0.25f;

	/**
	 * Player-facing gate: target must be in front of the character, not only in
	 * front of the camera. 0 = front half-space (anything not strictly behind).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "-1.0", ClampMax = "0.999"))
	float MinPlayerForwardDot = 0.0f;

	/**
	 * Targets within this distance of the player bypass both forward gates.
	 *
	 * Point-blank targets sit at an unstable angle from the player centre - a
	 * small strafe flips the forward dot's sign - and sit almost straight down
	 * from the camera whenever camera collision pulls it in close. Gating them
	 * makes items at the player's feet flicker or vanish; the aim score still
	 * ranks them, so bypassing only makes them reachable, not preferred.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "0.0", ClampMax = "400.0"))
	float NearFieldBypassRadius = 150.0f;

	/**
	 * World-space radius around the aim ray that still counts as "aimed at".
	 *
	 * Ranking uses perpendicular distance from the aim ray in cm rather than the
	 * angle to it. A fixed angular tolerance covers almost no world space close
	 * to the camera and a lot of it far away, which is why targets at the
	 * player's feet used to lose to targets across the room.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "10.0", ClampMax = "1000.0"))
	float AimRadius = 120.0f;

	/** How strongly closer targets are preferred over better-aimed ones. 0 = pure aim. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DistanceWeight = 0.35f;

	/**
	 * Hysteresis score bonus given to the current target. A challenger must
	 * exceed this amount before focus changes. Score is in AimRadius units, so
	 * 0.05 means a challenger must be ~0.05 * AimRadius cm closer to the aim ray.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float CurrentFocusScoreBonus = 0.05f;

	/** Maximum number of ranked ground items returned to UInteractionManager. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "1", ClampMax = "64"))
	int32 MaxGroundItemCandidates = 16;

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

	// PRIMARY FUNCTIONS

	/**
	 * Trace for actor-based interactables (delegates to FindBestInteractionTarget).
	 * @return Interface to found interactable, or null
	 */
	TScriptInterface<IInteractable> TraceForActorInteractable();

	/**
	 * Player-centered interaction search:
	 *   1. Gather actors and saved ground-item locations inside InteractionDistance.
	 *   2. Gate by camera and player forward vectors (near-field targets bypass).
	 *   3. Rank by distance from the aim ray, proximity, and current-focus hysteresis.
	 *
	 * @param CurrentInteractable  Currently focused actor interactable (for hysteresis).
	 * @param CurrentItemID        Currently focused ground item ID (for hysteresis).
	 * @param bRefreshCandidates   Re-run the spatial queries. Pass false to re-score
	 *                             the previously gathered set against the current
	 *                             camera, which is cheap enough to run every frame.
	 */
	void FindBestInteractionTarget(
		const TScriptInterface<IInteractable>& CurrentInteractable,
		int32 CurrentItemID,
		TScriptInterface<IInteractable>& OutInteractable,
		int32& OutGroundItemID,
		TArray<FGroundItemInteractionCandidate>& OutGroundItemCandidates,
		bool& bOutHasProximityCandidates,
		bool bRefreshCandidates = true);

	/** Drop the gathered candidate set so the next scoring pass re-queries. */
	void InvalidateCandidateCache();

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


	// Non-const because GetCameraViewPoint refreshes the cached controller.
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
	// INTERNAL HELPERS

	void CacheComponents();
	FVector GetTraceStartLocation(const FVector& CameraLocation, const FRotator& CameraRotation) const;
	bool PerformLineTrace(const FVector& Start, const FVector& End, FHitResult& OutHit);
	bool IsActorInteractable(AActor* Actor) const;

	/** Wrap an interactable actor (component preferred, interface fallback). */
	TScriptInterface<IInteractable> MakeInteractableInterface(AActor* Actor) const;

	/** Returns true when TargetLocation is in the character's front half-space. */
	bool PassesPlayerForwardGate(const FVector& TargetLocation, float& OutDot) const;

	/** Re-run the spatial queries that populate the cached candidate set. */
	void GatherCandidates(const FVector& PlayerCenter, float SearchRadius);

	/**
	 * Point on an actor used for scoring: the point on its bounds nearest the
	 * aim ray, so large interactables are not penalised for a distant pivot.
	 */
	FVector GetActorScoreLocation(const AActor* Actor, const FVector& RayOrigin, const FVector& RayDirection) const;

	// CACHED REFERENCES

	UPROPERTY()
	AActor* OwnerActor;
	
	UPROPERTY()
	UWorld* WorldContext;
	
	UPROPERTY()
	APlayerController* CachedPlayerController;
	
	UPROPERTY()
	AALSPlayerCameraManager* CachedALSCameraManager;
	
	UPROPERTY()
	UGroundItemSubsystem* CachedGroundItemSubsystem;
	
	FInteractionDebugManager* DebugManager;

	// STATE

	FHitResult LastTraceResult;

	// CACHED CANDIDATE SET
	//
	// The spatial queries run on the InteractionManager's timer, but scoring runs
	// every frame - the camera moves each frame while these results do not.

	TArray<TWeakObjectPtr<AActor>> CachedActorCandidates;
	TArray<int32> CachedGroundItemIDs;
	bool bHasGatheredCandidates = false;
};

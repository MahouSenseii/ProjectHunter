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

	/** Camera-alignment score, including the current-focus hysteresis bonus. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Ground Items")
	float Score = -1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Ground Items")
	float Distance = 0.0f;

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
	 * Hysteresis score bonus given to the current target. A challenger must
	 * exceed this amount before focus changes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace|Aim", meta = (ClampMin = "0.0", ClampMax = "0.25"))
	float CurrentFocusDotBonus = 0.015f;

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
	 *   2. Gate by camera and player forward vectors.
	 *   3. Rank by camera dot, player dot, distance, and current-focus hysteresis.
	 *
	 * @param CurrentInteractable  Currently focused actor interactable (for hysteresis).
	 * @param CurrentItemID        Currently focused ground item ID (for hysteresis).
	 */
	void FindBestInteractionTarget(
		const TScriptInterface<IInteractable>& CurrentInteractable,
		int32 CurrentItemID,
		TScriptInterface<IInteractable>& OutInteractable,
		int32& OutGroundItemID,
		TArray<FGroundItemInteractionCandidate>& OutGroundItemCandidates,
		bool& bOutHasProximityCandidates);

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

	// CACHED REFERENCES

	AActor* OwnerActor;
	UWorld* WorldContext;
	APlayerController* CachedPlayerController;
	AALSPlayerCameraManager* CachedALSCameraManager;
	UGroundItemSubsystem* CachedGroundItemSubsystem;
	FInteractionDebugManager* DebugManager;

	// STATE

	FHitResult LastTraceResult;
};

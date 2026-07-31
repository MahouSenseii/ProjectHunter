#pragma once

#include "CoreMinimal.h"
#include "Character/Library/Enums/InteractionDebugEnumLibrary.h"
#include "InteractionDebugManager.generated.h"
class UInteractableManager;
class AActor;
class UWorld;
class UALSDebugComponent;
struct FGroundItemInteractionCandidate;

DECLARE_LOG_CATEGORY_EXTERN(LogInteractionDebugManager, Log, All);

/**
 * A manager class for handling interaction debugging functionality.
 * Manages the display and control of debug data to aid in development and testing.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FInteractionDebugManager
{
	GENERATED_BODY()

public:
	FInteractionDebugManager();

	// CONFIGURATION (Blueprint-editable)

	/** Debug visualization mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	EInteractionDebugMode DebugMode = EInteractionDebugMode::None;

	/** Draw trace lines */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Visualization")
	bool bDrawTraceLines = true;

	/** Draw hit points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Visualization")
	bool bDrawHitPoints = true;

	/** Draw interaction range sphere */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Visualization")
	bool bDrawInteractionRange = true;

	/** Draw ground items */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Visualization")
	bool bDrawGroundItems = true;

	/** Draw the look-at gate cone (the forward cone a target must be inside to take focus). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Visualization")
	bool bDrawLookAtCone = true;

	/** Draw aim candidates with dot values (green=winner, yellow=in cone but lost, orange=failed gate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Visualization")
	bool bDrawAimCandidates = true;

	/** Draw the exact ground-item sphere search volume at the trace stop point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Visualization")
	bool bDrawGroundItemAimWindow = true;

	/** Draw every ranked ground-item candidate, its order, score, and selection state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Visualization")
	bool bDrawGroundItemCandidateStack = true;

	/** Show on-screen debug text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Visualization")
	bool bShowDebugText = true;

	/** Color for successful trace hits */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor TraceHitColor = FColor::Green;

	/** Color for trace misses */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor TraceMissColor = FColor::Red;

	/** Color for interactable objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor InteractableColor = FColor::Cyan;

	/** Color for ground items */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor GroundItemColor = FColor::Yellow;

	/** Color for the item selected automatically by aim scoring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor AutomaticCandidateColor = FColor::Cyan;

	/** Color for the currently selected item while manual cycling is locked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor ManualCandidateColor = FColor(255, 80, 255);

	/** Color for items rejected by the player-forward gate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors")
	FColor RejectedCandidateColor = FColor::Orange;

	/** How long to display debug shapes (0 = single frame) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Display")
	float DrawDuration = 0.0f;

	/** Thickness of debug lines */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Display")
	float DrawThickness = 2.0f;

	// INITIALIZATION

	void Initialize(AActor* Owner, UWorld* World);

	// DEBUG DRAWING

	void DrawTraceLine(FVector Start, FVector End, bool bHit);
	void DrawTraceResult(FVector Start, FVector End, const FHitResult& HitResult, bool bHit, float TraceRadius);
	void DrawHitPoint(FVector HitLocation, FVector HitNormal, float Radius = 12.0f);
	void DrawInteractionRange(FVector Center, float Radius);
	void DrawGroundItem(FVector ItemLocation, int32 ItemID);
	void DrawInteractableInfo(UInteractableManager* Interactable, float Distance, FVector TraceOrigin);

	/**
	 * Visualize the look-at gate: a cone from the view origin along camera
	 * forward with half-angle acos(MinDot). Anything outside it cannot take focus.
	 */
	void DrawLookAtCone(FVector Origin, FVector Forward, float MinDot, float Length);

	/** Visualize the owning player's forward gate. */
	void DrawPlayerForwardGate(FVector Origin, FVector Forward, float MinDot, float Length);

	/**
	 * Visualize one aim candidate and its dot value.
	 * Green = winner (took focus), yellow = passed the gate but lost,
	 * orange = failed the gate (outside the cone).
	 */
	void DrawAimCandidate(FVector Location, float Dot, bool bPassedGate, bool bWinner);

	/** Visualize the player-centered sphere used to gather all candidates. */
	void DrawGroundItemSearchVolume(FVector Center, float Radius);

	/** Draw only the player-forward and camera-forward directions used by scoring. */
	void DrawSelectionDirections(
		FVector PlayerCenter,
		FVector PlayerForward,
		FVector CameraOrigin,
		FVector CameraForward,
		float Length);

	/** Visualize an item discarded by the player-forward gate. */
	void DrawRejectedGroundItemCandidate(FVector Location, int32 ItemID, float PlayerForwardDot);

	/**
	 * Draw the ranked candidate snapshot owned by UInteractionManager.
	 * Labels are vertically staggered so fully overlapping items stay readable.
	 */
	void DrawGroundItemCandidateStack(
		FVector TraceOrigin,
		const TArray<FGroundItemInteractionCandidate>& Candidates,
		int32 SelectedItemID,
		int32 AutomaticItemID,
		bool bManualSelectionLocked,
		float ManualLockRemaining);

	// DEBUG TEXT

	void DisplayInteractionState(
		UInteractableManager* Interactable,
		float Distance,
		int32 GroundItemID,
		int32 SelectionNumber,
		int32 CandidateCount,
		int32 AutomaticItemID,
		const FGroundItemInteractionCandidate* SelectedCandidate,
		bool bHasProximityCandidates,
		float EvaluationInterval,
		bool bManualSelectionLocked,
		float ManualLockRemaining);
	void DisplayPerformanceMetrics(float TraceTime, float ValidationTime);

	// LOGGING

	void LogInteraction(UInteractableManager* Interactable, bool bSuccess, const FString& Reason = "");
	void LogGroundItemPickup(int32 ItemID, bool bToInventory, bool bSuccess);
	void LogValidationFailure(const FString& ValidationReason, float Distance, float MaxDistance);

	// STATS

	void PrintDebugStats();

	// ALS DEBUG INTEGRATION

	/**
	 * Check if debug traces should be shown
	 * Integrates with ALS Debug component if available
	 */
	bool ShouldShowDebugTraces() const;

private:
	// CACHED REFERENCES (Not Blueprint-exposed)

	AActor* OwnerActor = nullptr;
	UWorld* WorldContext = nullptr;
	UALSDebugComponent* CachedALSDebugComponent = nullptr;

	// Debug statistics (Not Blueprint-exposed)
	int32 TotalInteractions = 0;
	int32 SuccessfulInteractions = 0;
	int32 FailedInteractions = 0;
	int32 TotalGroundItemsPickedUp = 0;
	float AverageTraceTime = 0.0f;
	float AverageValidationTime = 0.0f;
};

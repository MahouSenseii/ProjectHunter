// Character/Components/Interaction/InteractionDebugManager.h
#pragma once

#include "CoreMinimal.h"
#include "Character/Components/Library/InteractionDebugEnumLibrary.h"
#include "InteractionDebugManager.generated.h"

// Forward declarations
class UInteractableManager;
class AActor;
class UWorld;
class UALSDebugComponent;

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

	// ═══════════════════════════════════════════════
	// CONFIGURATION (Blueprint-editable)
	// ═══════════════════════════════════════════════

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

	/** Draw the ground-item camera-ray window: aim radius and trace-depth limit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Visualization")
	bool bDrawGroundItemAimWindow = true;

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

	/** How long to display debug shapes (0 = single frame) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Display")
	float DrawDuration = 0.0f;

	/** Thickness of debug lines */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Display")
	float DrawThickness = 2.0f;

	// ═══════════════════════════════════════════════
	// INITIALIZATION
	// ═══════════════════════════════════════════════

	void Initialize(AActor* Owner, UWorld* World);

	// ═══════════════════════════════════════════════
	// DEBUG DRAWING
	// ═══════════════════════════════════════════════

	void DrawTraceLine(FVector Start, FVector End, bool bHit);
	void DrawTraceResult(FVector Start, FVector End, const FHitResult& HitResult, bool bHit, float TraceRadius);
	void DrawHitPoint(FVector HitLocation, FVector HitNormal, float Radius = 12.0f);
	void DrawInteractionRange(FVector Center, float Radius);
	void DrawGroundItem(FVector ItemLocation, int32 ItemID);
	void DrawInteractableInfo(UInteractableManager* Interactable, float Distance);

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

	/** Visualize the ground-item candidate volume used before dot-product scoring. */
	void DrawGroundItemAimWindow(
		FVector Origin, FVector Forward, float MinDistance, float MaxDistance, float Radius, bool bLimitedByTraceHit);

	// ═══════════════════════════════════════════════
	// DEBUG TEXT
	// ═══════════════════════════════════════════════

	void DisplayInteractionState(UInteractableManager* Interactable, float Distance, int32 GroundItemID);
	void DisplayPerformanceMetrics(float TraceTime, float ValidationTime);

	// ═══════════════════════════════════════════════
	// LOGGING
	// ═══════════════════════════════════════════════

	void LogInteraction(UInteractableManager* Interactable, bool bSuccess, const FString& Reason = "");
	void LogGroundItemPickup(int32 ItemID, bool bToInventory, bool bSuccess);
	void LogValidationFailure(const FString& ValidationReason, float Distance, float MaxDistance);

	// ═══════════════════════════════════════════════
	// STATS
	// ═══════════════════════════════════════════════

	void PrintDebugStats();

	// ═══════════════════════════════════════════════
	// ALS DEBUG INTEGRATION
	// ═══════════════════════════════════════════════

	/**
	 * Check if debug traces should be shown
	 * Integrates with ALS Debug component if available
	 */
	bool ShouldShowDebugTraces() const;

private:
	// ═══════════════════════════════════════════════
	// CACHED REFERENCES (Not Blueprint-exposed)
	// ═══════════════════════════════════════════════

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

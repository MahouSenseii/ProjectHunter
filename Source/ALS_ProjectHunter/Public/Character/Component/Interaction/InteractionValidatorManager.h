// Character/Component/InteractionValidatorManager.h
#pragma once

#include "CoreMinimal.h"
#include "InteractionValidatorManager.generated.h"

// Forward declarations
class IInteractable;
class UInteractableManager;
class UGroundItemSubsystem;
class AActor;
class UWorld;

DECLARE_LOG_CATEGORY_EXTERN(LogInteractionValidatorManager, Log, All);

/**
 * Manages the lifecycle and validation of interaction-related objects.
 * Responsible for ensuring interactions adhere to specific rules
 * and constraints across different contexts.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FInteractionValidatorManager
{
	GENERATED_BODY()
	
public:
	FInteractionValidatorManager();

	// ═══════════════════════════════════════════════
	// CONFIGURATION (Blueprint-editable)
	// ═══════════════════════════════════════════════

	/** Additional distance buffer for network latency/lag compensation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	float LatencyBuffer = 50.0f;

	/** Use dynamic latency buffer based on player ping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	bool bUseDynamicLatencyBuffer = true;

	/** Minimum latency buffer (ms to units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	float MinLatencyBuffer = 50.0f;

	/** Maximum latency buffer (ms to units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	float MaxLatencyBuffer = 200.0f;

	/** Require line of sight for actor interactions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	bool bRequireLineOfSight = false;

	/** Log validation failures for anti-cheat monitoring */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	bool bLogValidationFailures = true;

	// ═══════════════════════════════════════════════
	// INITIALIZATION
	// ═══════════════════════════════════════════════

	/**
	 * Initialize validator manager
	 * @param Owner - Owner actor (character)
	 * @param World - World context
	 */
	void Initialize(AActor* Owner, UWorld* World);

	// ═══════════════════════════════════════════════
	// VALIDATION FUNCTIONS (Server-side)
	// ═══════════════════════════════════════════════

	/**
	 * Validate actor interaction request
	 */
	bool ValidateActorInteraction(AActor* TargetActor, FVector ClientLocation, float MaxDistance);

	/**
	 * Validate ground item pickup request
	 */
	bool ValidateGroundItemPickup(int32 ItemID, FVector ClientLocation, float MaxDistance);

	/**
	 * Check if actor can be interacted with
	 */
	bool IsValidInteractable(AActor* Actor, AActor* Interactor) const;

	/**
	 * Validate distance between two points
	 */
	bool ValidateDistance(FVector LocationA, FVector LocationB, float MaxDistance, bool bUseLatencyBuffer = true);

	/**
	 * Check line of sight between two points
	 * @param Start - Start location (typically player/camera location)
	 * @param End - End location (typically target location)
	 * @param SourceActor - Actor to ignore at start (typically player)
	 * @param TargetActor - Target actor we're checking line of sight TO (hitting this counts as valid LOS)
	 * @return True if clear line of sight exists (or if we hit the target actor itself)
	 */
	bool HasLineOfSight(FVector Start, FVector End, AActor* SourceActor = nullptr, AActor* TargetActor = nullptr);

	/**
	 * Calculate dynamic latency buffer based on player ping
	 */
	float GetDynamicLatencyBuffer() const;

	/**
	 * Get player ping in milliseconds
	 */
	float GetPlayerPing() const;

	/**
	 * Check if this has server authority
	 */
	bool HasAuthority() const;

	/**
	 * Log validation failure for monitoring
	 */
	void LogValidationFailure(const FString& Reason, FVector ClientLocation, FVector TargetLocation);

private:
	// ═══════════════════════════════════════════════
	// CACHED REFERENCES (Not Blueprint-exposed)
	// ═══════════════════════════════════════════════

	AActor* OwnerActor = nullptr;
	UWorld* WorldContext = nullptr;
	UGroundItemSubsystem* CachedGroundItemSubsystem = nullptr;

	// Anti-cheat tracking
	int32 ValidationFailureCount = 0;
	float LastValidationFailureTime = 0.0f;

	// ═══════════════════════════════════════════════
	// INTERNAL HELPERS
	// ═══════════════════════════════════════════════

	void CacheComponents();
};
// Tower/Subsystems/PortalSubsystem.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PortalSubsystem.generated.h"

class APortalActor;

DECLARE_LOG_CATEGORY_EXTERN(LogPortalSubsystem, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class ALS_PROJECTHUNTER_API UPortalSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ── Registration ──────────────────────────────────────────────────────────

	/**
	 * Called by APortalActor::BeginPlay.
	 * Logs a warning if a portal with the same ID is already registered.
	 */
	UFUNCTION(BlueprintCallable, Category = "Portal")
	void RegisterPortal(FName PortalID, APortalActor* Portal);

	/**
	 * Called by APortalActor::EndPlay.
	 * Safe to call even if the ID was not registered.
	 */
	UFUNCTION(BlueprintCallable, Category = "Portal")
	void UnregisterPortal(FName PortalID);

	// ── Query ─────────────────────────────────────────────────────────────────

	/**
	 * Returns the APortalActor registered under PortalID, or nullptr if not found.
	 */
	UFUNCTION(BlueprintPure, Category = "Portal")
	APortalActor* FindPortal(FName PortalID) const;

	/**
	 * Returns all currently registered portals (for map/UI queries).
	 */
	UFUNCTION(BlueprintPure, Category = "Portal")
	TArray<APortalActor*> GetAllPortals() const;

	/** Number of portals currently registered. */
	UFUNCTION(BlueprintPure, Category = "Portal")
	int32 GetPortalCount() const { return PortalRegistry.Num(); }

private:
	/** FName → APortalActor* — rebuilt each level load via APortalActor::BeginPlay */
	TMap<FName, TWeakObjectPtr<APortalActor>> PortalRegistry;
};

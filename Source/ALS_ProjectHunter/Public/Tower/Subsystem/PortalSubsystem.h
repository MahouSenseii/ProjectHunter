// Tower/Subsystem/PortalSubsystem.h
// Lightweight registry that maps PortalIDs to APortalActor pointers.
//
// WHY a separate subsystem?
//   APortalActor instances need to find each other by ID at runtime without
//   iterating all actors every time.  UPortalSubsystem provides an O(1) TMap
//   lookup and acts as the single source of truth for what portals exist in
//   the current level.
//
// Lifecycle:
//   • APortalActor::BeginPlay  → RegisterPortal()
//   • APortalActor::EndPlay    → UnregisterPortal()
//   • Portal finds destination → FindPortal()
//
// UWorldSubsystem is used (not UGameInstanceSubsystem) because portals are
// level-bound.  The registry clears automatically when the world is torn down.
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

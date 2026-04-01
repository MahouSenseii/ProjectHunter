// AI/Mob/MobWanderInterface.h
// Interface implemented by spawnable characters so the AI Controller (set up
// in Blueprint) can query wander data without knowing the concrete actor type.
//
// BLUEPRINT SETUP:
//   1. Open your NPC Blueprint (child of APHBaseCharacter).
//   2. Add "Interfaces → MobWanderable" in Class Settings.
//   3. Implement the three events.  SetHomeLocation stores the vector in a
//      Blueprint variable; GetHomeLocation returns it; GetWanderRadius returns
//      whatever you set on the mob or fallback to the manager default.
//
// AI CONTROLLER USAGE:
//   Cast the controlled pawn to IMobWanderable (or use the interface call node)
//   to get HomeLocation and WanderRadius when picking the next move target.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MobWanderInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UMobWanderable : public UInterface
{
	GENERATED_BODY()
};

class ALS_PROJECTHUNTER_API IMobWanderable
{
	GENERATED_BODY()

public:
	// ── Called by AMobManagerActor on finalize ────────────────────────────────

	/**
	 * Store the spawn point this mob should wander around.
	 * Called once by AMobManagerActor immediately after the mob is made visible.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mob|Wander")
	void SetHomeLocation(const FVector& HomeLocation);

	/**
	 * Store the wander radius passed in from the manager.
	 * Lets per-mob Blueprint overrides differ from the manager default.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mob|Wander")
	void SetWanderRadius(float WanderRadius);

	// ── Queried by AI Controller ──────────────────────────────────────────────

	/** Returns the home (spawn) location this mob wanders around. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mob|Wander")
	FVector GetHomeLocation() const;

	/** Returns the maximum wander distance from home. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mob|Wander")
	float GetWanderRadius() const;

	/**
	 * Optional: notify the mob it was just spawned by a manager.
	 * Useful to kick off initial AI behaviour, play spawn effects, etc.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mob|Wander")
	void OnSpawnedByManager(AActor* ManagerActor);
};

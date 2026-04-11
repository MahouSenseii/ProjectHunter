// Item/ItemPlacementActor.h
//
// Editor-placeable item for "this item always lives at this exact spot."
//
// Drop one of these in a level, point its BaseItemHandle at any row in your
// item DataTable, and on BeginPlay it will create a fresh UItemInstance and
// register it with UGroundItemSubsystem at this actor's transform.  From the
// player's perspective the item is indistinguishable from a dropped loot
// item — same ISM rendering, same spin/bob, same pickup flow through
// FGroundItemPickupManager.
//
// Works for ANY item type — weapons, armor, consumables, materials, quest
// items, currency.  The DataTable row determines the mesh, type, and pickup
// behaviour automatically.
//
// Use this for fixed-location pickups (tutorial weapons, quest rewards,
// shrine offerings, vendor stock, set dressing, etc.) without needing a
// chest, container, or any custom pickup logic.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Item/Library/ItemEnums.h"
#include "ItemPlacementActor.generated.h"

class UItemInstance;
class UBillboardComponent;
class USceneComponent;

/**
 * AItemPlacementActor — places a single item in the world via the
 * existing UGroundItemSubsystem (ISM-backed ground items).
 */
UCLASS(Blueprintable, BlueprintType,
       meta = (DisplayName = "Item Placement (Ground Item)"))
class ALS_PROJECTHUNTER_API AItemPlacementActor : public AActor
{
	GENERATED_BODY()

public:
	AItemPlacementActor();

	// ═══════════════════════════════════════════════
	// CONFIG (set in editor or Blueprint)
	// ═══════════════════════════════════════════════

	/** Row handle pointing at the FItemBase row to spawn at this location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item",
	          meta = (DisplayName = "Base Item"))
	FDataTableRowHandle BaseItemHandle;

	/** Item level (1–100) — affects affix tier rolls. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item",
	          meta = (ClampMin = "1", ClampMax = "100"))
	int32 ItemLevel = 1;

	/** Item grade (F–SS) — determines affix count. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemRarity Rarity = EItemRarity::IR_GradeF;

	/** Roll affixes on spawn (only meaningful for Grade E+ equipment). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bGenerateAffixes = true;

	/** Stack quantity (only meaningful for stackable items: consumables, materials, currency). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item",
	          meta = (ClampMin = "1"))
	int32 Quantity = 1;

	/** Auto-spawn the item into the GroundItemSubsystem on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bSpawnOnBeginPlay = true;

	/**
	 * If true, the item is removed from the GroundItemSubsystem when this
	 * actor is destroyed (e.g. on level unload).  Leave true unless you want
	 * the placed item to outlive its placement actor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bRemoveOnDestroyed = true;

	// ═══════════════════════════════════════════════
	// API
	// ═══════════════════════════════════════════════

	/**
	 * Build the UItemInstance and register it with UGroundItemSubsystem.
	 * Called automatically from BeginPlay if bSpawnOnBeginPlay is true.
	 * Safe to call manually (it no-ops if an item is already registered).
	 *
	 * @return The new ground-item ID, or INDEX_NONE on failure.
	 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	int32 SpawnIntoWorld();

	/**
	 * Remove the placed item from the GroundItemSubsystem (if registered).
	 * @return The freed UItemInstance, or nullptr if nothing was registered.
	 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	UItemInstance* RemoveFromWorld();

	/** Returns the registered ground-item ID (INDEX_NONE if not spawned yet). */
	UFUNCTION(BlueprintPure, Category = "Item")
	int32 GetRegisteredItemID() const { return RegisteredItemID; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Deferred spawn hook.  Bound to UWorld::OnWorldBeginPlay when this actor's
	 * own BeginPlay runs before the world has finished its BeginPlay dispatch
	 * phase.  Fires exactly once, after HasBegunPlay() has flipped, which is
	 * when UGroundItemSubsystem::EnsureISMContainerExists can actually spawn
	 * the ISM container.
	 */
	void HandleWorldBeginPlay();

	/** Editor-only billboard so designers can find the placement in viewport. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBillboardComponent> EditorBillboard;
#endif

private:
	/** Cached ID returned by AddItemToGround.  INDEX_NONE until SpawnIntoWorld runs. */
	UPROPERTY(Transient)
	int32 RegisteredItemID = INDEX_NONE;

	/** Handle for the OnWorldBeginPlay subscription so we can unbind in EndPlay. */
	FDelegateHandle WorldBeginPlayHandle;
};

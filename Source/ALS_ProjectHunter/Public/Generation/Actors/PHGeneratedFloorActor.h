// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Generation/Data/PHBiomeModuleSet.h"
#include "Generation/Generators/PHLayoutGenerator.h"
#include "Generation/Library/Structs/BlockoutPlanStructs.h"
#include "Generation/Library/Structs/LayoutRequestStructs.h"
#include "Generation/Library/Structs/EncounterPlanStructs.h"
#include "Character/PHBaseCharacter.h"
#include "Tower/Library/Structs/RunStructs.h"
#include "PHGeneratedFloorActor.generated.h"

class UInstancedStaticMeshComponent;
class AMobManagerActor;
class APortalActor;
class ALootChest;

/**
 * Editor-facing harness for the generation pipeline: drop one in a level, press Generate, and walk
 * the result. It exists to make generation inspectable by hand and to give the first playable route
 * somewhere to live; it does not replace Dungeon Architect and does not own combat.
 *
 * It builds a floor, puts the player on its entry tile, and puts a portal on its exit tile. Using
 * that portal produces the next floor: through URunSubsystem when a run is active, so the run owner
 * stays the single authority on floor number and seed, and by rolling the next seed in place when
 * no run is running, so the route is walkable from a bare test level.
 *
 * Geometry is built with instanced meshes because a floor is thousands of copies of a few pieces.
 */
UCLASS(Blueprintable)
class ALS_PROJECTHUNTER_API APHGeneratedFloorActor : public AActor
{
	GENERATED_BODY()

public:
	APHGeneratedFloorActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void Destroyed() override;

	/** Plans and validates a replacement before clearing the current floor. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Generation")
	void Generate();

	/** Removes everything a previous Generate built. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Generation")
	void ClearBuilt();

	/** Rolls a new seed and regenerates, for flipping through layouts quickly. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Generation")
	void GenerateNextSeed();

	/** Builds the floor for a given layout seed and moves any player onto its entry tile. */
	UFUNCTION(BlueprintCallable, Category = "Generation")
	void GenerateForSeed(int32 LayoutSeed);

	/**
	 * What the exit portal does. With a run active this asks URunSubsystem to advance, which comes
	 * back as a generation request; the exit is refused while the run owner says the objective is
	 * unfinished, because satisfying that gate from here would make it meaningless. With no run
	 * active it rolls the next seed in place, which is the test-level route.
	 */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Generation")
	void TravelToNextFloor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FPHLayoutRequest Request;

	/** Resolves logical pieces to meshes. Without one, nothing can be built. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TObjectPtr<UPHBiomeModuleSet> ModuleSet;

	/** Defaults to the dungeon strategy when left unset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TSubclassOf<UPHLayoutGenerator> GeneratorClass;

	/**
	 * Caps the floor with Piece.Ceiling. A sealed room is lit only from inside, so a directional
	 * light above the level will no longer reach it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bBuildCeiling = true;

	/**
	 * Inclusive corridor width in tiles, drawn per connection, so routes differ in breadth instead
	 * of every passage reading the same. 1..1 gives uniform single-tile corridors.
	 *
	 * Kept at 2 by default: at the 400 tile a 3-wide corridor is 12 m across, as wide as a room,
	 * which erases the distinction between passage and space and leaves corridors covering most of
	 * the floor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "1"))
	int32 MinCorridorWidth = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "1"))
	int32 MaxCorridorWidth = 2;

	/**
	 * Spawns point lights from the plan. A ceilinged floor is otherwise pitch black, since a
	 * directional light above the level cannot reach a sealed room.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Lighting")
	bool bBuildLighting = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Lighting", meta = (ClampMin = "0.0"))
	float LightIntensity = 5000.0f;

	/**
	 * 0 derives the radius from LightSpacingTiles, which is what keeps reach and coverage from
	 * drifting apart: a light that does not reach as far as the next one leaves a dark band between
	 * them however evenly the two were placed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Lighting", meta = (ClampMin = "0.0"))
	float LightAttenuationRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Lighting")
	FLinearColor LightColor = FLinearColor(1.0f, 0.86f, 0.68f);

	/**
	 * Furthest a floor tile may be from a light, in tiles. Lights are spread by farthest-point
	 * sampling over the whole floor, so rooms and corridors are covered by one rule and a large
	 * room gets as many lights as it needs instead of one in the middle. 0 plans no lights.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Lighting", meta = (ClampMin = "0"))
	int32 LightSpacingTiles = 3;

	/**
	 * Marks the exit with a piece while blocking out. The entry is never marked: the player spawns
	 * on that tile and a pillar there puts them inside geometry. Ignored once ExitPortalClass is
	 * set, because the portal is the better marker.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bMarkEndpoints = true;

	/** Props scattered over the built floor. Empty leaves a bare blockout. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Decoration")
	TArray<FPHPropRule> PropRules;

	/**
	 * Places an APlayerStart on the entry tile. A level's authored PlayerStart cannot follow a
	 * layout that changes with every seed, which is how a player ends up spawning outside the floor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Route")
	bool bPlacePlayerStart = true;

	/** Height above the floor tile the entry pose is lifted to, clearing the pawn's capsule. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Route", meta = (ClampMin = "0.0"))
	double PlayerStartLift = 100.0;

	/**
	 * Portal placed on the exit tile. Leave unset for a floor with no way out. The class needs an
	 * InteractableManager configured for tap interaction, so a Blueprint subclass is the practical
	 * choice.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Route")
	TSubclassOf<APortalActor> ExitPortalClass;

	/**
	 * Forwards each mob death from the encounter owners this actor placed to the run objective.
	 * The run owner counts progress; this actor is the only thing that knows which managers exist,
	 * so it is the listener that reports to it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Route")
	bool bReportKillsToRun = true;

	/**
	 * Extra layout seeds tried when construction refuses the first one.
	 *
	 * Construction legitimately refuses some logical layouts - two rooms can share
	 * less frontage than the authored corridor width, and a required connection
	 * that cannot be built must not be silently dropped. That refusal used to end
	 * the request with no retry and no report, leaving the run in Generating with
	 * nothing to advance it (ISSUE-PH-20260831-06).
	 *
	 * Retries are derived from the same floor seed, so a seed still replays.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Route", meta = (ClampMin = "0"))
	int32 MaxLayoutRetries = 8;

	/**
	 * Reports one enemy death from this floor to the run owner.
	 *
	 * Named separately from the delegate handler because it is the actual
	 * operation: the handler only adapts the mob-died signature to it. Public so
	 * the behaviour can be exercised without a spawned encounter and without
	 * reaching into a private handler by reflection.
	 */
	UFUNCTION(BlueprintCallable, Category = "Generation|Route")
	void ReportMobDeathToRun();

	/**
	 * Leaves the exit portal shut until the run owner says the floor is finished. Without this the
	 * exit is visibly usable from the moment you arrive and simply refuses, which reads as a bug.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Route")
	bool bGateExitOnObjective = true;

	/**
	 * Chest placed on every Anchor.Chest the layout seats. Leave unset for a floor without
	 * containers. How many appear and where is authored as anchor rules on the Request, so chest
	 * frequency is content rather than code.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Route")
	TSubclassOf<ALootChest> ChestClass;

	/**
	 * How far below the ceiling the point light sits.
	 *
	 * The fixture mesh mounts flush on the ceiling plane; the light itself needs
	 * to clear that geometry or it lights the far side of it. Small on purpose -
	 * this is clearance, not a hanging height.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Route", meta = (ClampMin = "0.0"))
	double LightDropBelowCeiling = 24.0;

	/** Rolls the next seed and rebuilds when the exit is used with no run active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Route")
	bool bRegenerateOnExit = true;

	/**
	 * Rebuilds whenever URunSubsystem requests a floor, using that floor's seed. This is what makes
	 * every floor of a run a different map while leaving the run owner in charge of which floor it
	 * is, so there is still one owner of floor state.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Route")
	bool bFollowRunSubsystem = true;

	/**
	 * Checks the level's authored navigation data and bounds after building. This does not create
	 * a brush volume or synchronously rebuild navigation; runtime-changing floors need Dynamic
	 * navigation and a NavMeshBoundsVolume covering every possible layout.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Encounters")
	bool bBuildNavigation = true;

	/**
	 * Encounter owner placed in each region that carries enemy anchors. Leave unset for a floor
	 * with no encounters. Generation never spawns mobs itself; this actor owns that.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Encounters")
	TSubclassOf<AMobManagerActor> EncounterManagerClass;

	/**
	 * Mob classes handed to each placed manager. Leave empty to use whatever the manager's own
	 * Blueprint already defines.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Encounters")
	TArray<TSubclassOf<APHBaseCharacter>> EncounterMobClasses;

	/** Distance each encounter volume is pulled back from its room walls. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Encounters", meta = (ClampMin = "0.0"))
	double EncounterInset = 200.0;

	/** Keeps the entry room free of enemies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Encounters")
	bool bSkipStartRegion = true;

	/**
	 * Derived from the layout seed through its own labelled branch, so changing decoration cannot
	 * disturb the layout the seed produces.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Decoration")
	bool bDecorate = true;

	/** Read-only summary of the last build attempt, including any refusal reason. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation|Result")
	FString LastResult;

	/** World pose of the last build's entry point, on the centre of a tile that was built. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation|Result")
	FTransform LastPlayerStart = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation|Result")
	FTransform LastExit = FTransform::Identity;

	/** Read-only count of encounter owners placed by the last build. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation|Result")
	int32 LastEncounterCount = 0;

	/** Chests placed by the last build. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation|Result")
	int32 LastChestCount = 0;

	/** Enemies the last build asked its encounter owners for; the run objective target. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation|Result")
	int32 LastEnemyBudget = 0;

	/** Regions the last build placed. The request draws this from a range, so it varies by seed. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation|Result")
	int32 LastRegionCount = 0;

	/** Whether usable navigation data and authored bounds covered the last build when checked. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation|Result")
	bool bLastBuildHadNavigation = false;

private:
	/** Instances are grouped per mesh; one component per distinct mesh the plan uses. */
	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> BuiltComponents;

	/** Actors this harness spawned, tracked so ClearBuilt can remove them. */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	/** Held separately from SpawnedActors so the route can be re-bound after a level load. */
	UPROPERTY()
	TObjectPtr<APortalActor> ExitPortal;

	/** One queued transition at a time; clearing a floor invalidates callbacks from its old exit. */
	FTimerHandle ExitTravelTimer;
	FTimerHandle MovePlayersTimer;
	bool bGenerating = false;

	UFUNCTION()
	void HandleExitPortalActivated(APortalActor* Portal, APawn* Traveller);

	UFUNCTION()
	void HandleFloorGenerationRequested(FRunFloorData FloorData);

	UFUNCTION()
	void HandleMobDied(APHBaseCharacter* Mob);

	UFUNCTION()
	void HandleFloorRewardReady(FRunFloorData FloorData);

	/** Opens or shuts the exit portal, if one was placed. */
	void SetExitOpen(bool bOpen);

	bool TryGenerate();
	bool TryGenerateForSeed(int32 LayoutSeed);
	void BuildNavigationBounds(const FBox& LayoutBounds);
	void PlaceLights(const FPHBlockoutPlan& Plan);
	float GetLightRadius(const FPHBlockoutPlan& Plan) const;
	bool PlaceEncounters(const FPHEncounterPlan& Plan, int32 LayoutSeed);
	bool PlaceRouteActors();

	/** Spawns ChestClass on each Anchor.Chest pose the layout seated. */
	bool PlaceChests(const FPHGeneratedLayout& Layout);

	/** Moves every player pawn onto the entry tile. Runtime only; there is no pawn in the editor. */
	void MovePlayersToStart();

	UInstancedStaticMeshComponent* FindOrAddComponentFor(UStaticMesh* Mesh,
		TMap<UStaticMesh*, UInstancedStaticMeshComponent*>& Cache);
};

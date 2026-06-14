# Project Hunter — Run Loop Implementation Plan

Decisions locked: **separate map per floor** (OpenLevel), **separate hub map**,
**death loses everything not stashed**, **every floor gated by a main objective**
(boss, maze, survival waves, clear-all, ...).

The loop:

```
HUB (stash, vendors)
  └─ Start Portal → StartRun() → snapshot loadout → OpenLevel(TowerFloor)
       FLOOR N: FloorManager → DA build(seed) → navmesh → spawners on →
                Objective starts → complete → Exit Portal unlocks →
                AdvanceFloor() → snapshot loadout → OpenLevel(TowerFloor)
       DEATH:   NotifyDeath → EndRun() → carryover CLEARED → OpenLevel(Hub)
                → end-of-run screen (OnRunEnded session data) → stash intact
```

Key consequence of separate-map floors: `OpenLevel` destroys the pawn and every
`UItemInstance` outered to it. Carried gear survives a floor transition ONLY if
serialized across — which is also exactly how the death rule works: death simply
skips the snapshot. One system implements both.

---

## Phase 1 — Travel backbone (the carryover layer)

**New: `URunCarryoverSubsystem : UGameInstanceSubsystem`** (Tower/Subsystems/)
Survives OpenLevel. Owns everything that must cross a map boundary:

- `FPendingFloor` — floor number, dungeon seed, FloorDefinition row, area level.
- `PendingArrivalPortalID` — finishes the cross-map portal arrival stub in
  `APortalActor::ExecuteTravel` (currently a placeholder debug call).
- **Loadout snapshot** — serialized inventory + equipment, reusing the
  `StashSubsystem` pattern (`ArIsSaveGame` memory archive + `PrepareForSave` /
  `PostLoadInit` on ItemInstance, which already exist for this).
  `SnapshotLoadout(Character)` / `RestoreLoadout(Character)`.
- `bReturnToHub` / `ERunTravelReason` (StartRun, NextFloor, Death, Extract).

**Edits:**
- `PortalActor::ExecuteTravel` — write arrival ID + reason into the subsystem
  before `OpenLevel` (replaces the stub).
- Arrival: on floor/hub map load, GameMode (or FloorManager) queries the
  subsystem, places the pawn at the matching portal, restores loadout.
- Death flow: BP death event already calls `NotifyDeath`; add
  `EndRun()` + travel-to-hub with **no snapshot** (rule: lose all unstashed).

**Exit criteria:** Hub → Floor 1 → back to hub with items intact; dying returns
to hub with carried items gone, stash untouched. (No DA yet — empty floor map.)

## Phase 2 — FloorManager + Dungeon Architect contract (BP-first)

**New: `AFloorManagerActor`** (Tower/Actors/) — one per floor map. C++ owns the
sequence; DA stays Blueprint glue (no module dependency):

1. BeginPlay: read `FPendingFloor` from carryover → fire
   `UFUNCTION(BlueprintImplementableEvent) BP_BuildDungeon(Seed, FloorDef)`.
   *Your BP child calls the DA Dungeon actor's Build with that seed.*
2. BP calls back `NotifyDungeonBuilt()` (BlueprintCallable) when DA finishes.
3. C++ then: waits for navmesh (dynamic navmesh required — Project Settings →
   runtime generation = Dynamic), positions player at entry, configures every
   `AMobManagerActor` (`bAutoActivate=false` in floor BPs; manager sets
   `AreaLevel = Floor`, scales `MaxNumOfMobs`, then `StartSpawning()`),
   registers the **exit portal locked** (`SetPortalActive(false)`).
4. Spawns + starts the floor objective (Phase 3). Objective completion →
   `SetPortalActive(true)` on the exit + HUD notify.
5. Exit portal used → `RunSubsystem.AdvanceFloor()` → carryover snapshot →
   build next `FPendingFloor` → OpenLevel.

Spawner/chest/portal placement: simplest robust pattern is post-build scan —
DA theme places marker actors (or you tag DA-emitted actors); FloorManager
collects them after `NotifyDungeonBuilt` and activates/configures. Avoids any
C++ dependency on DA types.

**Exit criteria:** seeded floors build repeatably, mobs spawn scaled to floor,
exit advances the loop indefinitely.

## Phase 3 — Floor Objective framework (the "main quest" system)

**New: `UFloorObjective : UObject`** (Tower/Objectives/) — Blueprintable,
modular by design; each objective type is its own subclass:

- API: `Initialize(FloorManager, Params)` → `StartObjective()` →
  `OnObjectiveCompleted` / `OnObjectiveFailed` delegates →
  `GetObjectiveText()` / `GetProgressText()` for the HUD.
- Ships with (C++, BP-subclassable):
  - **KillTarget** — boss/named mob dies (binds the mob's `OnDeath`).
  - **SurviveWaves** — timer + drives MobManagers (`ForceSpawnBatch`,
    escalating `PendingForcedTier` via special-spawn rules); kill pressure
    until timer ends, then spawners stop.
  - **ClearAll** — every registered MobManager empty + kill quota met.
  - **ReachExit (maze)** — exit unlocked by reaching a trigger; DA maze theme
    does the spatial work.
- **`FFloorDefinition` DataTable row**: floor range, weighted objective class +
  params, DA theme reference (soft), mob set, area-level curve, reward tier,
  corruption settings (your `FLootDropSettings` corruption fields slot straight
  in for corrupted floors later).
- HUD: small objective widget (HunterHUDBaseWidget child) bound to the active
  objective's text/progress — same pattern as your XP widget.

**Exit criteria:** each floor rolls an objective from the table; exit stays
locked until it completes; at least boss/survival/maze/clear-all playable.

## Phase 4 — Death, rewards, stats polish

- Confirm chain: mob `NotifyDeath` → MobManager kill counts → killer's
  `PHPlayerState.RecordKill` (feeds `RunSubsystem.RegisterKill`) → XP via
  `AwardExperienceFromKill` (all hooks exist; wire the BP death event calls).
- End-of-run screen from `OnRunEnded(FRunSessionData)` — floors, kills, time.
- Hub stash interaction = the only banking point (extraction tension).
- Optional "extract" portal on some floors: travel reason Extract = snapshot
  + return to hub (banking run loot at the cost of ending the run).

## Phase 5 — Variety & cadence (post-loop)

More objectives (escort, hunt-the-named, defend), floor modifiers (corrupted
floors via existing corruption pipeline, mob-tier boosts via
`SpecialSpawnRules`), rest floors every N with stash access, seed-of-the-day.

---

## Risks / notes

- **Multiplayer + OpenLevel:** OpenLevel is single-player/local. For co-op
  later, swap to `ServerTravel` with `bUseSeamlessTravel`; the carryover
  subsystem survives either way on the host — clients re-replicate. Design the
  subsystem API now, swap the travel call later.
- **DA build timing:** never `StartSpawning()` before navmesh is green —
  FinalizeSpawn already warns on nav projection failure; FloorManager's
  post-build wait is the guard.
- **Item Outer on restore:** `RestoreLoadout` must re-outer items to the new
  pawn (InventoryAdder already does `Rename` reparenting — reuse it).
- **RunSubsystem timer** already real-time (survives OpenLevel) ✓.
- **PHPlayerState floor mirror** already delegates to RunSubsystem ✓.

## Suggested order of attack

1. Carryover subsystem + portal arrival (unblocks everything).
2. Hub map + tower floor map skeletons, StartRun portal, death return.
3. FloorManager + DA BP contract on the floor map.
4. Objective base + ClearAll (simplest), then KillTarget, SurviveWaves, maze.
5. FloorDefinition table + HUD objective widget.
6. End-of-run screen + extract portal.

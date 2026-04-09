# ProjectHunter Architecture Refactor — Ticket Breakdown

**Source roadmap:** ProjectHunter Architecture Redesign Roadmap (April 8, 2026)
**Format:** Epics → Stories with T-shirt sizes (S/M/L/XL) and dependencies
**Guardrails for every ticket:**
- Keep public facade APIs and Blueprint-facing entry points stable.
- Keep delegate names stable; treat them as the listener boundary.
- No behavioral changes to gameplay, save format, or replication unless the ticket explicitly versions a migration.
- Land each cluster behind a feature flag or as an isolated PR; do **not** mega-cutover.
- Each PR must include before/after `.cpp` line counts for the owners it touches.

---

## Sequencing Overview

```
EPIC-0 (Bootstrap Coordinator)        ──┐
                                        ├──► EPIC-1 (Equipment)
                                        ├──► EPIC-2 (Stats)
                                        ├──► EPIC-3 (Inventory)
                                        ├──► EPIC-4 (Interaction)
                                        └──► EPIC-5 (Combat)
EPIC-6 (Items)        ── parallel with 1–5 once EPIC-0 lands
EPIC-7 (AI / Spawning) ── depends on EPIC-8 (Player Location Cache)
EPIC-9 (Cleanup)       ── parallel, low-risk
```

EPIC-0 is the only true blocker for the character-side epics. Items (6) and AI (7) can run in parallel tracks.

---

## EPIC-0 — Character System Coordinator (Bootstrap Layer)

**Goal:** One place owns cross-system discovery, listener wiring, and one-time bootstrap for all character managers. `APHBaseCharacter` becomes a composition root only.

| ID | Title | Size | Depends on | Acceptance Criteria |
|---|---|---|---|---|
| PH-0.1 | Add `UCharacterSystemCoordinatorComponent` skeleton | S | — | New component compiles, attaches to `APHBaseCharacter`, no behavior yet. Init order documented in header. |
| PH-0.2 | Migrate manager discovery into coordinator | M | PH-0.1 | All `FindComponentByClass<UStatsManager/Equipment/Inventory/Interaction/Combat>` calls in gameplay hot paths are removed. Coordinator caches references once on `BeginPlay`. Grep for `FindComponentByClass` in `Source/ALS_ProjectHunter/` returns zero hits in domain code. |
| PH-0.3 | Migrate cross-manager listener binding into coordinator | M | PH-0.2 | Equipment→Stats, Equipment→Moveset, Equipment→Presentation, Inventory→UI, Combat→Stats bindings all happen in the coordinator. No manager binds to another manager's delegate directly. |
| PH-0.4 | Strip orchestration logic from `APHBaseCharacter` | M | PH-0.3 | `APHBaseCharacter.cpp` has no manager wiring code; only lifecycle, ownership, and replicated character-wide state. Diff shows net line reduction. |
| PH-0.5 | Bootstrap idempotency + replication safety pass | S | PH-0.4 | Coordinator handles client/server init order, late join, and possession changes. Manual smoke test on listen-server + dedicated server. |

**Cluster acceptance:** Character bootstrap test plan from roadmap passes. Hot-path `FindComponentByClass` audit is clean.

---

## EPIC-1 — Equipment Cluster

**Goal:** `UEquipmentManager` becomes pure state owner. Presentation moves to a dedicated component. Reusable rules move to a function library.

| ID | Title | Size | Depends on | Acceptance Criteria |
|---|---|---|---|---|
| PH-1.1 | Add `UEquipmentFunctionLibrary` with slot/two-hand/ring/compatibility helpers | S | — | New static helpers cover slot determination, compatibility checks, ring auto-slot, and two-hand rules. Unit-test-friendly (pure functions). |
| PH-1.2 | Replace inline equipment rule logic in `UEquipmentManager` with function-library calls | M | PH-1.1 | No rule branches remain inline in the manager. Behavior unchanged in equip/unequip/swap. |
| PH-1.3 | Add `UEquipmentPresentationComponent` (mesh attach, socket resolution, runtime actor lifecycle, `OnWeaponUpdated`) | M | PH-0.1 | New component compiles and attaches in composition. No behavior wired yet. |
| PH-1.4 | Move attached-mesh / runtime-actor / socket / cleanup code from `UEquipmentManager` into `UEquipmentPresentationComponent` | L | PH-1.3, PH-0.3 | `UEquipmentManager.cpp` no longer touches `USkeletalMeshComponent`, sockets, or spawn/destroy of weapon actors. Coordinator binds `OnEquipmentChanged → presentation` instead. |
| PH-1.5 | Move stat-effect application off `UEquipmentManager` (bind via coordinator into `UStatsManager`) | M | PH-0.3 | Equipment manager only mutates equipped state and broadcasts. Stats manager owns GE apply/remove on equipment events. |
| PH-1.6 | Equipment cluster regression sweep | M | PH-1.2, PH-1.4, PH-1.5 | Manual + automation: equip, unequip, swap, two-hand, ring auto-slot, replication to client, visual update, stat application, moveset reaction all behaviorally identical. Before/after `.cpp` line counts captured. |

**Cluster acceptance:** `UEquipmentManager.cpp` is materially smaller and contains only state + replication + inventory mutation + delegate broadcast.

---

## EPIC-2 — Stats Cluster

**Goal:** Internals split into private helpers. Public surface unchanged.

| ID | Title | Size | Depends on | Acceptance Criteria |
|---|---|---|---|---|
| PH-2.1 | Extract stat initialization into `FStatsInitializer` (or `Private/StatsInitialization.*`) | M | PH-0.3 | Init code lives outside `UStatsManager.cpp`. Manager calls into helper at `BeginPlay`/possession. |
| PH-2.2 | Extract query resolution helpers (attribute lookup, derived value calc) | M | PH-2.1 | Manager methods become thin pass-throughs. |
| PH-2.3 | Extract equipment-effect application helpers (GE apply/remove for equipped items) | M | PH-2.2, PH-1.5 | Coordinator route Equipment→Stats lands on this helper, not on inline manager code. |
| PH-2.4 | Stats cluster regression sweep | S | PH-2.3 | All current attribute, derived, and equipment-driven stat behavior unchanged. |

**Cluster acceptance:** `UStatsManager.cpp` line count materially reduced. Public ASC/attribute API unchanged.

---

## EPIC-3 — Inventory Cluster

**Goal:** Reusable stack/search/filter logic moves to shared helpers. `UInventoryManager` stays the owner.

| ID | Title | Size | Depends on | Acceptance Criteria |
|---|---|---|---|---|
| PH-3.1 | Extract stack/merge logic into `UInventoryFunctionLibrary` | S | — | Pure-function helpers for stack/merge/can-stack rules. |
| PH-3.2 | Extract search/filter/predicate helpers into the same library | S | PH-3.1 | Equipment, pickup, and UI all call shared helpers instead of duplicating loops. |
| PH-3.3 | Replace duplicated callsites in equipment/pickup/UI with helper calls | M | PH-3.2 | Grep finds no remaining duplicate stack/filter loops outside the library. |
| PH-3.4 | Inventory cluster regression sweep | S | PH-3.3 | Add/remove/stack/split/sort/use behavior unchanged. Save/load unaffected. |

---

## EPIC-4 — Interaction Cluster

**Goal:** `UInteractionManager` stays as facade. Internals move to focused services.

| ID | Title | Size | Depends on | Acceptance Criteria |
|---|---|---|---|---|
| PH-4.1 | Extract focus resolution into `FInteractionFocusResolver` (actor + ground-item focus) | M | PH-0.3 | Focus tracing/scoring/selection lives outside `UInteractionManager.cpp`. |
| PH-4.2 | Extract tap/hold/mash/continuous state machine into `FInteractionInputState` | M | PH-4.1 | Input timing/state transitions removed from manager body; manager calls helper per tick. |
| PH-4.3 | Extract widget/highlight presentation into `UInteractionPresentationComponent` (or service object) | M | PH-4.2 | Manager no longer touches widgets or highlight materials directly. |
| PH-4.4 | Extract pickup routing (to inventory vs to equip) into helper | S | PH-4.3, PH-3.3 | Single call site for pickup-to-inventory and pickup-to-equip decisions. |
| PH-4.5 | Interaction cluster regression sweep | M | PH-4.4 | Actor focus, ground-item focus, tap/hold/continuous/mash, widget, highlight, and pickup flows all behaviorally identical. Existing delegates unchanged. |

**Cluster acceptance:** `UInteractionManager.cpp` drops well below the ~1494 baseline; remaining content is the facade + delegate plumbing.

---

## EPIC-5 — Combat Cluster

**Goal:** Split `UCombatManager` internals into packet building, mitigation, and secondary effects. Centralize damage-type routing.

| ID | Title | Size | Depends on | Acceptance Criteria |
|---|---|---|---|---|
| PH-5.1 | Extract `FDamagePacketBuilder` (raw input → normalized packet) | M | PH-0.3 | Builder is unit-testable and called once per hit. |
| PH-5.2 | Extract `FDamageMitigationService` (block/armor/resist/crit) | M | PH-5.1 | Mitigation is deterministic given a packet + defender snapshot. |
| PH-5.3 | Extract `FCombatSecondaryEffectsService` (ailments, reflect, on-hit recovery hooks) | M | PH-5.2 | Missing GE content remains a no-op inside this service, not inside the resolver. |
| PH-5.4 | Replace switch-heavy damage-type routing with a single binding/registry table | M | PH-5.1 | Adding a new damage type touches one registry, not multiple switches. |
| PH-5.5 | Move ailment + reflect TODO hooks into `FCombatSecondaryEffectsService` | S | PH-5.3 | Resolver no longer carries TODO branches. |
| PH-5.6 | Combat cluster regression sweep | M | PH-5.4, PH-5.5 | Damage, crit, block, on-hit recovery numbers identical to baseline. Ailment/reflect remain no-op as today. |

**Cluster acceptance:** Damage flow is `packet build → mitigate → apply → secondary effects`, with one routing table for damage types.

---

## EPIC-6 — Item Cluster

**Goal:** `UItemInstance` becomes the owner of serialized state only. Generation/naming/consumable/affix/post-load logic moves to services.

| ID | Title | Size | Depends on | Acceptance Criteria |
|---|---|---|---|---|
| PH-6.1 | Extract `FItemGenerationService` (rolls, affixes, rarity) | M | — | Generation is callable without an existing `UItemInstance`. |
| PH-6.2 | Extract `FItemNamingService` (display name, procedural pieces) | S | PH-6.1 | Naming is deterministic given seed + affix set. |
| PH-6.3 | Extract `FConsumableUseService` (cooldowns, effects, charges) | M | PH-6.1 | `UItemInstance::Use` becomes a thin call into the service. |
| PH-6.4 | Extract `FAffixEffectApplicator` (apply/remove affix GEs to a target) | M | PH-1.5 | Equip/unequip routes affix application here instead of inside `UItemInstance`. |
| PH-6.5 | Move `PostLoadInit`/rebuild into a dedicated migration helper with version gates | M | PH-6.1 | Helper respects existing serialization version field; corrupted items follow current recovery path. |
| PH-6.6 | Item cluster regression sweep | M | PH-6.2, PH-6.3, PH-6.4, PH-6.5 | Init, corruption recovery, save/load `PostLoadInit`, consumable cooldown/use, affix apply/remove, and display-name regeneration unchanged. |

**Cluster acceptance:** `UItemInstance.cpp` only owns serialized state + accessors + delegate broadcast.

---

## EPIC-7 — AI / Mob Spawning Cluster

**Goal:** `AMobManagerActor` owns population + timers only. Everything else moves to helpers.

| ID | Title | Size | Depends on | Acceptance Criteria |
|---|---|---|---|---|
| PH-7.1 | Extract `FMobCandidateGenerator` (which archetypes are eligible at this location/time) | M | EPIC-8 | Generator is pure given world snapshot + budget. |
| PH-7.2 | Extract `FMobSpawnValidator` (LoS, distance, navmesh, density caps) | M | PH-7.1 | Validator returns reason codes for debug history. |
| PH-7.3 | Extract `FMobPackSpawnService` (pack composition + placement) | M | PH-7.2 | Pack spawning no longer lives in the manager actor body. |
| PH-7.4 | Move debug history/telemetry into `FMobSpawnDebugLog` | S | PH-7.3 | Debug ring buffer is opt-in and decoupled from spawn logic. |
| PH-7.5 | AI cluster regression sweep | M | PH-7.4 | Spawn budget, pooling, pack spawning, death/recycle, and debug traces unchanged across multiple managers. |

---

## EPIC-8 — Player Location Cache Subsystem

**Goal:** Spawn systems consume one shared player-location snapshot per tick instead of each manager scanning pawns.

| ID | Title | Size | Depends on | Acceptance Criteria |
|---|---|---|---|---|
| PH-8.1 | Add `UPlayerLocationCacheSubsystem` (UWorldSubsystem) | S | — | Subsystem refreshes on a configurable cadence and exposes `GetPlayerSnapshots()`. |
| PH-8.2 | Replace direct pawn iteration in `AMobManagerActor` and any other spawn manager with the subsystem | M | PH-8.1 | Grep for `GetPlayerControllerIterator` / pawn-scan loops in spawn code returns zero hits. |
| PH-8.3 | Cache invalidation + multiplayer correctness pass | S | PH-8.2 | Snapshot handles player join/leave, possession changes, and seamless travel. |

---

## EPIC-9 — Cleanup Track (Parallel, Low-Risk)

| ID | Title | Size | Depends on | Acceptance Criteria |
|---|---|---|---|---|
| PH-9.1 | Repoint `GameDefaultMap` and `EditorStartupMap` in `Config/DefaultEngine.ini` to the real ProjectHunter startup map | S | — | Editor and packaged builds boot into ProjectHunter content, not ALS demo. |
| PH-9.2 | One-time Blueprint hierarchy cleanup so `PHBaseCharacter` does not need runtime null-component recovery | M | PH-0.4 | Recovery code can be removed/asserted instead of silently fixing state. |
| PH-9.3 | Split `FPHGameplayTags` registration by domain (or move to a generated/data-driven registry) | M | — | New tags can be added without touching one monolithic header. |
| PH-9.4 | Archive or annotate stale review docs from March 21, 2026 so solved findings don't drive duplicate work | S | — | Stale findings clearly marked "RESOLVED" with commit refs. |
| PH-9.5 | Grow `CombatFunctionLibrary` beyond hostility/health queries with the helpers identified during EPIC-5 | S | PH-5.6 | Cross-cutting combat queries live in one place. |

---

## Explicitly Deferred (Do NOT Pull Into This Refactor)

- Splitting the Unreal game module. Stabilize boundaries inside the existing module first.
- Splitting `HunterAttributeSet`. Revisit only if post-refactor profiling still shows memory/replication pressure.
- Ailment GE asset authoring, reflect GE hookup, procedural legendary naming content. These are content/gameplay TODOs that become *easier* after their owning services exist (EPIC-5, EPIC-6) but are not part of this refactor's scope.
- Reopening any March 21, 2026 fix already in the repo (`PHGameMode`, `PHPlayerState`, `PHGameState`, actor pooling, item serialization versioning, pickup RPCs).

---

## Definition of Done (Whole Refactor)

1. Every cluster's regression sweep ticket is closed and signed off.
2. Owner `.cpp` line counts are materially reduced from the April 8, 2026 baselines (`InteractionManager` ~1494, `StatsManager` ~1283, `MobManagerActor` ~1245, `CombatManager` ~1100, `ItemInstance` ~1004, `EquipmentManager` ~890). Capture the new numbers in the closeout PR.
3. No domain code calls `FindComponentByClass` on character-managers in hot paths.
4. No manager binds to another manager's delegate directly; all cross-system listeners route through `UCharacterSystemCoordinatorComponent`.
5. Save format, replication format, and Blueprint-facing manager APIs are unchanged unless an explicit migration ticket says otherwise.
6. Cleanup track (EPIC-9) items 9.1, 9.2, 9.4 are landed. 9.3 and 9.5 may slip to a follow-up if needed.

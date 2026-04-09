# ProjectHunter Code Review

**Date:** 2026-04-08
**Reviewer:** Sr Developer pass against `quentin_ue_coding_skill.md`
**Scope:** Full high-level sweep, thorough audit
**Codebase:** `Source/ALS_ProjectHunter`

---

## 1. Executive Summary

ProjectHunter shows clear evidence of an architectural migration in progress toward the Quentin UE Coding Skill's layout, but execution is inconsistent. Core ownership is generally identifiable and the newer `Systems/` folder demonstrates real intent. Three issues dominate the audit:

1. **Dual-folder duplication** — feature code lives in both the old top-level layout (`Public/Item/`, `Public/Combat/`, `Public/Loot/`, `Public/Interactable/`) and the new `Public/Systems/<Feature>/` layout simultaneously. The migration is unfinished and ownership for Item/Combat/Loot is ambiguous.
2. **Logging macro is defined but not used** — `Public/Core/Logging/PHLogging.h` already exposes a `PH_LOG_WARNING` / `PH_LOG_ERROR` wrapper that injects `__FUNCTION__` and `__LINE__`, but roughly 127 raw `UE_LOG(..., Warning/Error, ...)` calls across `Systems/` do not use it. The skill treats function+line context as mandatory.
3. **Several oversized owner classes without responsibility splits** — `StatsManager` (~1530 lines cpp), `InventoryManager` (~615 lines cpp), and `ItemInstance` (~618 lines header) mix multiple responsibilities (add/remove/replication/visuals/effects) in a single class, directly violating Rule E (split before unreadable).

Reusability across projects today sits around 50–60%. The good news: the foundations exist (log categories, Public/Private discipline, `Systems/` layout, logging macro). The work is primarily *completion* and *enforcement*, not redesign.

---

## 2. Folder Layout Assessment

### 2.1 Dual-Layout Problem (CRITICAL)

Two parallel hierarchies exist:

**Old top-level layout:**
`Public/Item/`, `Public/Combat/`, `Public/Loot/`, `Public/Interactable/`, `Public/AI/`, `Public/AbilitySystem/`, `Public/Character/`, `Public/Tower/`.

**New Systems layout:**
`Public/Systems/Equipment/`, `Public/Systems/Inventory/`, `Public/Systems/Stats/`, `Public/Systems/Tags/`, `Public/Systems/Combat/`, `Public/Systems/Item/`, `Public/Systems/Loot/`, `Public/Systems/Character/`, `Public/Systems/Progression/`, `Public/Systems/Moveset/`, `Public/Systems/AI/`.

Observed duplication or split-ownership:

- **Item** — real 618-line `ItemInstance.h` at `Public/Item/ItemInstance.h`; `Public/Systems/Item/Library/` only contains `ItemLog.h` and `ItemStructsLog.h` stubs.
- **Combat** — `Public/Combat/CombatManager.h` (~106 lines) vs `Public/Systems/Combat/Components/DoTManager.h` (~259 lines). Unclear which is authoritative.
- **Loot** — full system at `Public/Loot/Subsystem|Generation|Component|Library/`; `Public/Systems/Loot/Library/` is a near-empty stub.
- **Interactable** — full system at `Public/Interactable/`; **no** `Systems/Interactable/` equivalent.

**Impact:** developers cannot predict which folder owns a bug, duplicated types risk drifting, and the debug-path promise of the skill (Rule D) is broken.

### 2.2 Public/Private Discipline (GOOD)

No `.cpp` files were found inside `Public/`, and no `.h` files inside `Private/`. This part of the skill is respected cleanly and should be preserved during any consolidation.

### 2.3 Library Subfolders (MIXED)

| Feature | Library/ exists? | Split into enums/structs/func-lib/log? |
|---|---|---|
| Equipment | Yes | Only `EquipmentFunctionLibrary.h` (~69 lines); missing `EquipmentEnums.h`, `EquipmentStructs.h`, `EquipmentLog.h` |
| Inventory | Yes | Good — `InventoryEnums.h`, `InventoryFunctionLibrary.h`, `InventoryLog.h` present |
| Stats | No | No Library/ subfolder at all |
| Combat | Yes (new) | `CombatStructs.h` only (~172 lines); old `Public/Combat/Library/` duplicates `CombatEnumLibrary`, `CombatFunctionLibrary` |
| Loot | Stub | Real library at old `Public/Loot/Library/` |
| Item | Fragmented | Old `Public/Item/Library/` is complete; `Systems/Item/Library/` is stubs only |
| Character | Yes | Only `PHCharacterLog.h`; missing enums/structs/func-lib |
| Interactable | Yes (old) | `InteractionEnumLibrary.h` is ~428 lines — too large |

---

## 3. Per-System Audit

### 3.1 Equipment

- **Owner:** `UEquipmentManager` (~239 lines) at `Public/Systems/Equipment/Components/`.
- **Responsibility split:** partial — `UEquipmentPresentationComponent` (~143 lines) isolates visuals; stats application delegated to `UStatsManager`.
- **Missing helpers:** `EquipmentAdder`, `EquipmentRemover`, `EquipmentValidator`.
- **Library:** exists but only contains a ~69-line function library. No split enums/structs/log file.
- **Logging:** `LogEquipmentManager` and `LogEquipmentPresentation` declared. ~8+ raw `UE_LOG` calls lack `__FUNCTION__`/`__LINE__`.
- **Verdict:** best example of proper ownership in the codebase, but still needs add/remove/validator split and full library.

### 3.2 Inventory

- **Owner:** `UInventoryManager` (~387-line header, ~615-line cpp) at `Systems/Inventory/Components/`.
- **Responsibility split:** essentially none — add, remove, swap, weight, replication all in one class.
- **Existing helper:** `InventoryGroundDropHelper` (~12 lines). Name violates Rule B (vague "Helper" suffix).
- **Library:** good — `InventoryEnums.h`, `InventoryFunctionLibrary.h`, `InventoryLog.h` all split.
- **Logging:** `LogInventoryManager` and `LogInventoryGroundDropHelper` declared. Raw `UE_LOG` calls lack function/line context.
- **Verdict:** high god-class risk. Needs `InventoryAdder`, `InventoryRemover`, `InventorySwapper`, `InventoryWeightCalculator`, `InventoryValidator`.

### 3.3 Item

- **Owner:** `UItemInstance` (~618 lines header) at `Public/Item/ItemInstance.h`.
- **Responsibility split:** none — data, visuals, GAS effects, replication, crafting all in one class.
- **Library:** duplicated — complete library at `Public/Item/Library/` (including a very large `ItemStructs.h`), stub library at `Public/Systems/Item/Library/`.
- **Logging:** `LogItemInstance` declared; hard to verify full coverage because the file is so large.
- **Verdict:** critical. Split into `ItemDataComponent`, `ItemVisualHandler`, `ItemEffectHandler`, `ItemReplicationHandler`. Consolidate Library into Systems/Item.

### 3.4 Stats

- **Owner:** `UStatsManager` (~335-line header, **~1530-line cpp**) — the largest manager in the project.
- **Responsibility split:** none — GAS lifecycle, equipment stat application, initialization, data-asset loading, and debugging all mixed.
- **Library:** none (no `Systems/Stats/Library/`).
- **Logging:** `LogStatsManager` and `LogStatsDebugManager` declared. Raw `UE_LOG(Error/Warning)` calls lack `__FUNCTION__`/`__LINE__`.
- **Verdict:** critical. Needs `StatsInitializer`, `EquipmentStatsApplier`, `StatsEffectCleaner`, `StatsDebugger` splits and a proper Library subfolder.

### 3.5 Combat

- **Owner:** unclear — `UDoTManager` (~259 lines) in `Systems/Combat/Components/` vs `ACombatManager` (~106 lines) at old `Public/Combat/`.
- **Library:** dual — `Systems/Combat/Library/CombatStructs.h` and old `Public/Combat/Library/CombatEnumLibrary`/`CombatFunctionLibrary`.
- **Logging:** `LogDoTManager` declared; raw `UE_LOG` calls missing function/line.
- **Verdict:** pick the authoritative owner, migrate the rest, delete duplicates.

### 3.6 Loot

- **Owner:** `ULootSubsystem` (~280 lines) at `Public/Loot/Subsystem/` (old layout).
- **Responsibility split:** okay — `LootGenerator` (~152 lines) and `LootComponent` (~155 lines) are role-based, but no validator/resolver.
- **Library:** real library at `Public/Loot/Library/`; `Systems/Loot/Library/` stub should be deleted or filled.
- **Logging:** **no custom log category found** for any Loot class — direct violation of Logging Rule 1.
- **Verdict:** migrate to `Systems/Loot/`, add `LogLootSubsystem`/`LogLootGenerator`/`LogLootComponent`.

### 3.7 Interactable

- **Owner:** `UInteractableManager` (~299 lines) at `Public/Interactable/Component/` (old layout only).
- **Responsibility split:** none inside the manager. Widget files (`InteractableWidget` ~313 lines, `ItemTooltipWidget` ~114 lines) handle presentation.
- **Library:** `InteractionEnumLibrary.h` is ~428 lines — too large, needs splitting into `InteractionEnums.h` and context-specific files.
- **Logging:** **no custom log category found** for InteractableManager — violates Logging Rule 1.
- **Verdict:** migrate to `Systems/Interactable/`, add `InteractableValidator`, `InteractableEventHandler`, and a log category.

### 3.8 Character

- **Owner:** `UCharacterSystemCoordinatorComponent` (~190-line cpp) at `Systems/Character/Components/`.
- **Responsibility split:** good — acts as a listener/router between Equipment, Stats, and Moveset. Exemplary for the skill.
- **Library:** thin — only `PHCharacterLog.h`. No character-specific enums/structs/func-lib yet.
- **Logging:** `LogCharacterSystemCoordinator` and `LogPHBaseCharacter` declared. A couple of raw `UE_LOG` calls lack function/line.
- **Verdict:** cleanest system in the project. Use as the reference model when splitting Stats/Inventory/Item.

### 3.9 Progression, Tags, Moveset, AI, Tower (light pass)

- **CharacterProgressionManager** (~335-line header): custom `LogCharacterProgressionManager`; raw `UE_LOG` lacks function/line.
- **TagManager** (~547-line cpp): custom `LogTagManager`/`LogTagDebugManager`; raw `UE_LOG` lacks function/line. Likely oversized.
- **MovesetManager**: custom `LogMovesetManager`; same macro gap.
- **MonsterModifierComponent** (~358 lines): custom `LogMonsterModifier`; same macro gap.
- **Tower**: still only at old top-level `Public/Tower/`; minimal Systems/ integration.

---

## 4. Logging Audit

### 4.1 Custom Categories (GOOD)

Sixteen custom `Log*` categories were found across `Systems/` features, covering Equipment, Inventory, Stats, Combat (DoT), Character, Progression, Tags, Moveset, and Monster modifiers. Coverage is strong for the migrated systems and missing for non-migrated ones (Loot, Interactable, Tower).

### 4.2 Macro Usage (CRITICAL FAILURE)

A macro wrapper lives at `Public/Core/Logging/PHLogging.h` (`PH_LOG_WARNING` / `PH_LOG_ERROR`), matching the skill's pattern. It is used by roughly 16% of warning/error calls in `Systems/`. The remaining ~127 warning/error calls are raw `UE_LOG(..., Warning|Error, ...)` and carry neither `__FUNCTION__` nor `__LINE__`.

**Example violations (paraphrased):**

- `UE_LOG(LogEquipmentManager, Error, TEXT("EquipmentManager: No InventoryManager found on '%s'."), *GetNameSafe(Owner));`
- `UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyGameplayEffectToSelf: Invalid effect class"));`

**Required form per skill:**

- `PH_LOG_ERROR(LogEquipmentManager, "No InventoryManager found on '%s'", *GetNameSafe(Owner));`

This is the single highest-impact, lowest-risk cleanup in the codebase — a mechanical sweep converts 127 call sites to compliant form and delivers function+line context everywhere a warning or error fires.

### 4.3 Severity Discipline (OK)

Severity is broadly used correctly. There is no obvious spam of `Warning`/`Error` for normal flow, though a handful of recoverable conditions should probably demote to `Display` or `Log`.

---

## 5. Cross-Cutting Issues

1. **Dual-layout migration unfinished** — Item, Combat, Loot, Interactable, Tower all still live (fully or partly) in the old top-level layout while `Systems/` has either stubs or partial duplicates.
2. **Oversized Library files** — `InteractionEnumLibrary.h` (~428 lines), the old `Item/Library/ItemStructs.h` is very large. Violates the skill's "split enums and structs" rule and the dependency-light rule.
3. **Missing Library content** — Equipment, Character, Stats, and Systems/Item libraries are incomplete (missing enums/structs/log/function-library files the skill requires per feature).
4. **Vague helper naming** — `InventoryGroundDropHelper` is the clearest example of the "Helper" suffix anti-pattern.
5. **No enforced Blueprint exposure convention** — Blueprint exposure is ad-hoc. There is no documented policy for what is `BlueprintReadOnly` vs `BlueprintReadWrite`, which classes are `Blueprintable`, or where extension points (`BlueprintImplementableEvent`/`BlueprintNativeEvent`) are allowed.
6. **Missing log categories for Loot, Interactable, Tower.**

---

## 6. Top 15 Ranked Findings

### CRITICAL

**F1. Dual-folder duplication across Item, Combat, Loot, Interactable.**
Folder layout must predict the debug path (Rule D). Right now it does not. *Fix:* pick `Systems/<Feature>/` as canonical, migrate the last holdouts, delete old top-level folders (or mark them `// deprecated` and lock them).

**F2. `StatsManager.cpp` is ~1530 lines mixing GAS lifecycle, equipment effect application, initialization, and debugging.**
Violates Rule E and Rule B. *Fix:* extract `StatsInitializer`, `EquipmentStatsApplier`, `StatsEffectCleaner`, `StatsDebugger`; add `Systems/Stats/Library/`.

**F3. `ItemInstance.h` is ~618 lines owning data, visuals, GAS, and replication.**
Violates Rule E. *Fix:* split into `ItemDataComponent`, `ItemVisualHandler`, `ItemEffectHandler`, `ItemReplicationHandler`; move to `Systems/Item/`.

### HIGH

**F4. ~127 raw `UE_LOG(..., Warning|Error, ...)` calls lack `__FUNCTION__`/`__LINE__` context.**
Logging Rules 5 and 6. *Fix:* mechanical sweep replacing `UE_LOG(Cat, Warning, ...)` with `PH_LOG_WARNING(Cat, ...)` throughout `Systems/`.

**F5. `InventoryManager` (~615 lines cpp) has no add/remove/swap/weight split.**
Rule E. *Fix:* `InventoryAdder`, `InventoryRemover`, `InventorySwapper`, `InventoryWeightCalculator`, `InventoryValidator`.

**F6. Equipment Library is undersized.**
Missing `EquipmentEnums.h`, `EquipmentStructs.h`, `EquipmentLog.h`. *Fix:* create the four skill-required files.

**F7. `InteractableManager` (~299 lines) has no validator/event-handler split and no log category.**
Rule B + Logging Rule 1. *Fix:* split helpers and add `LogInteractableManager`.

### MEDIUM

**F8. `InteractionEnumLibrary.h` (~428 lines) is too dense.**
Feature Library Rules. *Fix:* split into `InteractionEnums.h` and context-specific files.

**F9. Old `Item/Library/ItemStructs.h` is oversized.**
Dependency-Light Design Rule. *Fix:* split into `ItemBaseStructs.h`, `ItemAffixStructs.h`, `ItemCraftingStructs.h`, `ItemEquipmentStructs.h`. Use forward declarations in headers.

**F10. No log categories for Loot system.**
Logging Rule 1. *Fix:* add `LogLootSubsystem`, `LogLootGenerator`, `LogLootComponent`.

**F11. No log categories for Interactable or Tower systems.**
Logging Rule 1. *Fix:* add matching `Log*` categories and use them.

**F12. `InventoryGroundDropHelper` naming violates Rule B.**
*Fix:* rename to a responsibility-based name such as `UInventoryGroundItemHandler` or `FInventoryGroundDropResolver`.

**F13. `TagManager` (~547 lines) and `CharacterProgressionManager` (~335+ lines) are unsplit.**
Rule E. *Fix:* identify 2–4 responsibility-based helpers per manager (e.g., `TagApplier`, `TagQuerier`, `TagReplicationHandler`).

**F14. `Systems/Character/Library/` is thin (log only).**
Feature Library Rules. *Fix:* add `CharacterEnums.h` / `CharacterStructs.h` only if character-specific types exist; otherwise document the dependency on Equipment/Stats libraries in a short header comment.

**F15. No documented Blueprint exposure policy.**
Blueprint Exposure Rule and Safe Exposure Rule. *Fix:* add a short doc (or a section in the skill) listing which classes are `Blueprintable`, which properties are `BlueprintReadWrite` vs `BlueprintReadOnly`, and where `BlueprintImplementableEvent`/`BlueprintNativeEvent` extension points live.

---

## 7. Recommended Cleanup Order

The findings above are intentionally split into "quick wins" and "structural". A pragmatic sequence:

1. **Logging macro sweep (F4)** — mechanical, low-risk, immediate debuggability win.
2. **Add missing log categories (F10, F11)** — ~15 minutes per system.
3. **Library completeness pass (F6, F9, F14)** — splits enums/structs, adds missing log headers, improves include hygiene across the whole project.
4. **Rename vague helpers (F12)** — trivial refactor, sets the naming baseline.
5. **Folder consolidation (F1)** — pick `Systems/` as canonical and finish the migration of Item, Combat, Loot, Interactable, Tower.
6. **StatsManager split (F2)** — biggest architectural win; treat as its own mini-project.
7. **ItemInstance split (F3)** — coordinate with equipment/visual/GAS code; requires careful save/load and replication review.
8. **InventoryManager split (F5)** — similar playbook to StatsManager.
9. **TagManager / ProgressionManager splits (F13).**
10. **Blueprint exposure policy (F15).**

---

## 8. What the Project Already Does Well

- Custom log categories exist for every major system in `Systems/`.
- `Public/`/`Private/` separation is clean — no mixed folders.
- `PH_LOG_WARNING`/`PH_LOG_ERROR` macro already exists at `Public/Core/Logging/PHLogging.h`; enforcement is the missing piece, not the infrastructure.
- `CharacterSystemCoordinatorComponent` is a textbook listener/router — use it as the reference model for future owner/helper/listener splits.
- `EquipmentManager` + `EquipmentPresentationComponent` shows the intended owner/helper pattern at a small scale.
- `Systems/Inventory/Library/` is the cleanest library split in the project — use it as the template for the other features.

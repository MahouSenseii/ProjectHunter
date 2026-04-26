# Project Hunter Structure

This project now treats Project Hunter code as a feature-oriented Unreal module and leaves ALS-specific code and assets alone.

## Source Layout

Use these folders as the canonical homes for new Project Hunter runtime code:

```text
Source/ALS_ProjectHunter/
  Public/
    Systems/
      Character/
      Combat/
      Equipment/
      Inventory/
      Loot/
      Moveset/
      Progression/
      Stats/
      Tags/
    AI/
    Character/
    Interactable/
    Item/
    Tower/
  Private/
    Systems/
      Character/
      Combat/
      Equipment/
      Inventory/
      Loot/
      Moveset/
      Progression/
      Stats/
      Tags/
    AI/
    Character/
    Interactable/
    Item/
    Tower/
```

### Canonical rules

- Put state-owning gameplay systems under `Systems/<Feature>/Components`, `Systems/<Feature>/Subsystems`, or `Systems/<Feature>/Actors`.
- Put stateless helpers under `Systems/<Feature>/Library` or nearby helper headers/cpps.
- Keep item definitions, data structs, affixes, and reusable item runtime types under `Item/*` unless they clearly belong to a single owner system.
- Keep AI, Interactable, Tower, and HUD code in their feature roots unless they become cross-system runtime owners.
- Leave ALS code and ALS assets in their current homes.

## Architecture Model

Project Hunter should follow an `owner -> helpers -> listeners` model:

- Owners mutate state and broadcast what changed.
- Helpers contain reusable stateless logic.
- Listeners react after the mutation without forcing the owner to know every downstream system.

### Current owner examples

- `UEquipmentManager` owns equipped slot state and broadcasts equipment changes.
- `UInventoryManager` owns bag state, stacking, swaps, and drop requests.
- `UStatsManager` owns stat initialization and GAS-backed stat application.
- `UCombatManager` owns hit resolution and damage application.
- `ULootSubsystem` owns loot source lookup and world-spawn coordination.

### Current listener examples

- `UCharacterSystemCoordinatorComponent` listens to equipment changes and routes reactions to stats, presentation, and moveset systems.
- HUD widgets and actor-specific components should subscribe or query after the owner changes state rather than embedding owner logic themselves.

## Legacy Paths

Legacy headers under:

- `Public/Combat/*`
- `Public/Loot/*`
- `Public/Item/Runtime/EquippedItemRuntimeActor.h`

have been removed. New code should include the canonical `Systems/*` paths directly.

## Content Layout Direction

The `Content/ProjectHunter` root is still the correct root for PH assets, but the next cleanup should happen inside the Unreal Editor, not by moving `.uasset` files from the shell.

### Recommended content targets

- `Content/ProjectHunter/Core`
- `Content/ProjectHunter/Systems/Combat`
- `Content/ProjectHunter/Systems/Items`
- `Content/ProjectHunter/Systems/Loot`
- `Content/ProjectHunter/Systems/Stats`
- `Content/ProjectHunter/UI`
- `Content/ProjectHunter/World`
- `Content/ProjectHunter/Art/Materials`
- `Content/ProjectHunter/Art/Textures`

### Editor-only migration work

These should be fixed in Unreal Editor so redirectors can be created and cleaned up safely:

- Rename `Content/ProjectHunter/Libary` to `Content/ProjectHunter/Core` or `Content/ProjectHunter/BlueprintLibraries`.
- Merge overlapping art folders like `Materials/*` and `Textures/*` into a single intentional art taxonomy.
- Move loot tables, combat blueprints, and item assets under feature folders that mirror the code owners.
- Run `Fix Up Redirectors` after each feature-area migration.

## Practical rule for future refactors

If a new gameplay rule changes owned state, place it in the owner. If the rule is reusable across systems, move it into a helper. If the rule only reacts to a change, bind it as a listener instead of expanding the owner.

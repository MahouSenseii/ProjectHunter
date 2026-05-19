# ProjectHunter Code Review — 2026-04-08

## Systems

| System | Owner class | What it does |
|---|---|---|
| Equipment | `UEquipmentManager` | Owns equipped slot state, replication, and `OnEquipmentChanged` broadcast |
| Equipment Presentation | `UEquipmentPresentationComponent` | Attaches/detaches weapon meshes and runtime actors on equipment changes |
| Inventory | `UInventoryManager` | Stores item instances, handles add/remove/swap/weight/replication |
| Stats | `UStatsManager` | GAS lifecycle, attribute initialization, equipment stat application |
| Combat | `UCombatManager` / `UDoTManager` | Damage resolution and damage-over-time tracking |
| Loot | `ULootSubsystem` + `ULootGenerator` + `ULootComponent` | Generates and distributes loot drops |
| Interaction | `UInteractableManager` | Focus tracing, input state (tap/hold/mash), widget/highlight presentation, pickup routing |
| Character Coordinator | `UCharacterSystemCoordinatorComponent` | Discovers all character managers once on BeginPlay and wires cross-system delegates |
| Item | `UItemInstance` | Serialized item data — base handle, stats, affixes, durability, rarity, quantity |
| Tags | `UTagManager` | Applies and queries Gameplay Tags on the owning character |
| Moveset | `UMovesetManager` | Selects and applies moveset based on equipped weapon |
| Progression | `UCharacterProgressionManager` | Tracks XP, level, and stat growth |
| AI / Spawning | `AMobManagerActor` | Population budget, spawn timing, pack composition, actor pooling |
| Player Location Cache | `UPlayerLocationCacheSubsystem` | Shared per-tick player-location snapshot for spawn systems |

## Known issues

- Item, Combat, Loot, Interactable, and Tower still live in the old top-level layout (`Public/Item/`, etc.) instead of `Public/Systems/`.
- `UStatsManager` (~1530 lines), `UInventoryManager` (~615 lines), and `UItemInstance` (~618-line header) are oversized and mix multiple responsibilities.
- ~127 `UE_LOG(Warning/Error)` calls do not use the `PH_LOG_WARNING`/`PH_LOG_ERROR` macros from `Public/Core/Logging/PHLogging.h`.
- Loot, Interactable, and Tower have no custom log categories.

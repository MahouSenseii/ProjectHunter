# ProjectHunter Refactor Tickets

## Epics

| Epic | Goal |
|---|---|
| EPIC-0 | Add `UCharacterSystemCoordinatorComponent` to own cross-system wiring and manager discovery |
| EPIC-1 | Make `UEquipmentManager` a pure state owner; move presentation to `UEquipmentPresentationComponent` |
| EPIC-2 | Split `UStatsManager` internals into init, query, and effect-application helpers |
| EPIC-3 | Move stack/search/filter logic out of `UInventoryManager` into `UInventoryFunctionLibrary` |
| EPIC-4 | Split `UInteractionManager` into focus resolver, input state machine, and presentation component |
| EPIC-5 | Split `UCombatManager` into packet builder, mitigation service, and secondary effects service |
| EPIC-6 | Split `UItemInstance` so it only owns serialized state; move generation, naming, and consumable logic to services |
| EPIC-7 | Split `AMobManagerActor` into candidate generator, spawn validator, and pack spawn service |
| EPIC-8 | Add `UPlayerLocationCacheSubsystem` so spawn systems share one player-location snapshot per tick |
| EPIC-9 | Cleanup — fix startup map, Blueprint hierarchy, tag registry, and stale docs |

## Dependency order

EPIC-0 must land first. EPIC-1 through EPIC-5 depend on it. EPIC-6 and EPIC-8 run in parallel. EPIC-7 depends on EPIC-8. EPIC-9 runs in parallel throughout.

## Out of scope

Module splits, `HunterAttributeSet` splits, ailment/reflect GE authoring, procedural legendary naming content.

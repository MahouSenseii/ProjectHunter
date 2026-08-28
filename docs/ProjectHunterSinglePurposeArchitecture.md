# ProjectHunter Single-Purpose Architecture Guide

This guide combines the ProjectHunter architecture workflow, the Unreal Engine C++ coding standard, and the current refactor direction for ProjectHunter systems. Use it when checking C++ files for single-purpose ownership, reusable enums and structs, function libraries, Blueprint boundaries, and modular layout.

The goal is simple: when something changes, the correct file should be easy to find, the owner should be obvious, and shared information should be passed through small values instead of full objects whenever possible.

## Source Rules

Use these sources before changing architecture:

- `docs/UnrealEngineCoding.md`
- The ProjectHunter owner/helper/listener architecture model
- The current code under `Source/ALS_ProjectHunter`
- Unreal Engine documentation for specialized engine areas such as Gameplay Ability System, movement, networking, animation, and Blueprint integration

Do not guess ownership from file names alone. Read the current owner, the nearest helper, and the listener or delegate touchpoints before moving code.

## Core Architecture

ProjectHunter uses this model:

```text
Owner -> Helpers -> Listeners
```

The owner stores state and performs the main mutation.

Helpers contain reusable, usually stateless logic.

Listeners react after a state change and should not be required knowledge for the owner.

### Owner

Owners are usually actors, actor components, movement components, subsystems, managers, or game framework classes.

Owners are responsible for:

- State changes
- Validation before mutation
- Authority checks when gameplay state matters
- Replication for owned state
- Broadcasting after state changes
- Keeping external dependencies narrow

Good owner examples:

```text
EquipmentManager owns equipped item state.
InventoryManager owns inventory state.
UPHCharacterMovementComponent owns wall traversal physics.
UHunterAbilitySystemComponent owns ability input, activation routing, and active GameplayEffect routing.
UHunterAttributeSet owns attributes.
```

### Helpers

Helpers are reusable code that should not stay trapped inside one owner.

Helpers include:

- Function libraries
- Runtime structs
- Validation helpers
- Calculation helpers
- Formatting helpers
- Query helpers
- Conversion helpers

Helpers should not own replicated gameplay state. A helper can answer a question or compute a result, but the owner should still decide whether to mutate state.

### Listeners

Listeners react to changes.

Listeners include:

- UI widgets
- Presentation components
- Animation Blueprints
- VFX and audio presentation systems
- Other systems subscribed to delegates

Listeners should read state through narrow APIs or react to broadcasts. They should not become hidden owners.

## Folder Layout

ProjectHunter code should stay inside the game module unless the code is truly plugin-level reusable:

```text
Source/ALS_ProjectHunter/Public
Source/ALS_ProjectHunter/Private
```

Use domain folders for gameplay systems:

```text
AbilitySystem
Character
Combat
Equipment
Inventory
Item
Loot
Menu
Stats
System
Tags
Tower
```

Use this split for shared domain code:

```text
Source/ALS_ProjectHunter/Public/<Domain>/Library/Enums
Source/ALS_ProjectHunter/Public/<Domain>/Library/Structs
Source/ALS_ProjectHunter/Public/<Domain>/Library/FunctionLibraries
Source/ALS_ProjectHunter/Private/<Domain>/Library/Structs
Source/ALS_ProjectHunter/Private/<Domain>/Library/FunctionLibraries
```

Use a global `Library` only when the code is genuinely cross-domain.

Do not create empty enum, struct, or function library files just to satisfy a pattern. Empty files add search noise. Create them when the system has shared information or reusable behavior.

## Enums

Enums belong in `Library/Enums`.

Use enums when callers only need to pass a category, state, mode, policy, result type, or resource type and do not need a full object.

Rules:

- Use `enum class`.
- Use `UENUM(BlueprintType)` only when Blueprint needs the enum.
- Use `uint8` for Blueprint-friendly enums.
- Keep names specific.
- Do not put unrelated values into one enum.
- Prefer passing an enum over passing a full object when the object is only being used to identify a type or state.

Example:

```cpp
UENUM(BlueprintType)
enum class EPHAbilityActivationGroup : uint8
{
	Independent,
	Exclusive_Replaceable,
	Exclusive_Blocking,

	MAX UMETA(Hidden)
};
```

## Structs

Structs belong in `Library/Structs`.

Use structs for:

- Settings
- Inputs
- Results
- Snapshots
- Lightweight runtime state
- Data that can be passed without requiring the full owner object

Rules:

- Use `USTRUCT(BlueprintType)` only when Blueprint needs it.
- Initialize every value.
- Keep structs focused.
- Do not put owner-only authority decisions inside a shared struct.
- Prefer an input/result struct when a function would otherwise take many loosely related parameters.

Good examples:

```text
FPHResourceReservationInput
FPHResourceReservationResult
FPHPrimaryDerivedResourceInput
FPHAbilityActivationGroupRuntimeState
```

## Function Libraries

Function libraries belong in `Library/FunctionLibraries`.

Use `UBlueprintFunctionLibrary` for stateless reusable logic.

Good uses:

- Calculations
- Validation checks
- Data lookups
- Formatting
- Type conversion
- Shared gameplay queries
- Rules that are used by more than one owner or calculation

Bad uses:

- Owning gameplay state
- Hiding server-authoritative mutations
- Acting like a global manager
- Calling UI or animation code directly
- Mutating an actor without clear owner approval

If a function changes game state, its name must make that side effect clear, and the owner should usually call it rather than letting random systems mutate directly.

## Interfaces

Use interfaces when unrelated classes need to expose the same capability without sharing a base class.

Good interface cases:

- Equipment, loot, and interactables all need to expose display data.
- Multiple actor types can receive damage.
- Multiple systems can provide stat modifiers.
- UI needs to read a capability from different object types without knowing the concrete class.

Avoid interfaces when:

- Only one class implements the behavior.
- The caller already has the correct owner type.
- A function library or data struct solves the problem more clearly.
- The interface would hide who owns mutation.

Interface naming:

```text
UPHCombatTargetInterface
IPHCombatTargetInterface
UPHStatSourceInterface
IPHStatSourceInterface
UPHInteractableInterface
IPHInteractableInterface
```

Interfaces should expose narrow capabilities, not entire system internals.

## Managers And Sub-Objects

Managers are components that route and own a system's state transitions. This is a good pattern when it prevents the player from carrying many separate components that each do small pieces of one gameplay domain.

Use a manager when:

- One actor needs a single entry point for a gameplay domain.
- The manager owns state transitions and broadcasts changes.
- Internal helpers can stay as plain structs or UObjects instead of actor components.
- The system benefits from fewer actor components on the player.

Do not make the manager a god class. If the file grows because it contains reusable checks, calculations, formatting, or presentation logic, move those parts into helpers or listeners.

### Better Names Than Worker

`Worker` is vague. Use names that describe the role.

Recommended suffixes:

- `Handler` for a plain, non-inherited C++ class that owns one piece of deterministic logic for a manager to direct (preferred over `Processor`).
- `RuntimeState` for small state containers owned by the manager.
- `Resolver` for choosing or deriving a result.
- `Evaluator` for checks and scoring.
- `Presenter` for UI-facing or visual presentation logic.
- `Router` for dispatching requests to owned systems.
- `Coordinator` for sequencing multiple owned helpers.
- `Service` for subsystem-style support with clear scope.

`Handler` is the default choice for manager-directed logic: a plain C++ class (no `UCLASS`, all-static or simple instance methods), one file per concern, with dependencies pointing toward the manager (Manager -> Handler, never Handler -> Manager). Prefer splitting a growing handler into more single-purpose handlers over letting one grow into a god class.

Example combat layout:

```text
CombatManager                    - ActorComponent, directs the pipeline and applies results through GAS.
UCombatStatusEffectApplier       - owned sub-object, handles status effects (Bleed/Ignite/Chill/etc.).
FCombatOutgoingDamageCalculator  - plain C++, handles outgoing damage math.
FCombatIncomingDamageResolver    - plain C++, handles mitigation, block, routing, and stagger.
HunterDamagePopupPresentationComponent - listener, presents damage popups.
```

`CombatManager` resolves damage on the server and sends the resolved cosmetic
popup payload only to the attacking player's owning client. The presentation
component remains a local listener: it does not calculate damage, replicate
gameplay state, or decide who receives the event.

On-screen stats debug follows the same presentation boundary. Only the owner's
canonical `StatsManager` on a locally controlled pawn writes to the screen;
replicated and server-side pawn copies may still write explicitly enabled debug
output to the log without creating duplicate viewport panels.

Use `Presenter` for visual/UI reactions. Use `Handler`, `Resolver`, or `RuntimeState` for gameplay helpers.

## Performance Impact

The single-purpose split should not make the game meaningfully slower when done correctly.

Low-cost changes:

- Moving enums into enum headers
- Moving structs into struct headers
- Moving pure calculations into function libraries
- Replacing repeated formulas with helper calls
- Passing small structs or enums instead of full objects
- Using plain runtime structs inside an existing component

Potentially costly changes:

- Adding many ticking actor components
- Adding per-frame Blueprint calls from C++
- Adding broad searches such as `GetAllActorsOfClass`
- Adding repeated casts or interface calls in hot loops
- Adding allocations every tick
- Creating hidden global managers that query the world constantly

Interfaces have a small dispatch cost, but that cost is usually not the issue. The real cost is using interfaces in hot loops without caching or using them to hide ownership. Use interfaces for modularity when they reduce coupling, and profile before optimizing away clean architecture.

Function libraries and structs do not add component overhead. They usually improve maintainability without measurable runtime cost.

## Blueprint Boundary

Some ProjectHunter behavior is intentionally handled in Blueprint. Before moving logic into C++, identify what Blueprint currently owns.

Ask before changing:

- Does Blueprint own presentation only?
- Does Blueprint own tuning values only?
- Does Blueprint call a C++ command that validates the action?
- Does Blueprint currently mutate gameplay state directly?
- Does the value need to be edited in assets?
- Does the logic need replication or authority checks later?

Keep in C++:

- Gameplay rules
- Authority checks
- Movement physics
- Combat validation
- Inventory and equipment state
- Attribute math
- Replication-sensitive logic
- Complex reusable queries

Keep in Blueprint when appropriate:

- UI layout
- Animation graph wiring
- Cosmetic timing
- VFX/audio presentation
- Data asset references
- Tuning values exposed by C++

Use `BlueprintReadWrite` only when Blueprint is intentionally allowed to change a value. Prefer `BlueprintReadOnly` for state that C++ owns.

## AbilitySystem First Refactor Direction

AbilitySystem should be refactored one file or one small group at a time. Do not split `UHunterAttributeSet` into multiple AttributeSet classes without checking Blueprint and GameplayEffect references first, because existing GameplayEffects and assets may reference the current attributes directly.

Current AbilitySystem ownership:

```text
UHunterAbilitySystemComponent
	Owns ability input, activation routing, active GameplayEffect routing, passive regen effect handles, exhaustion handling.

UHunterAttributeSet
	Owns attributes, clamping, reserve/effective values, depleted-resource notifications.

UPHGameplayAbility
	Owns per-ability activation policy, activation group, cancel behavior, and spawn activation.

UPHAbilitySet
	Owns granting grouped abilities, effects, and attribute sets from data assets.

MMC classes
	Own captured attributes and resource-specific magnitude calculations.
```

Current AbilitySystem helper layout:

```text
AbilitySystem/Library/Enums
	PHAbilityEnums.h
	HunterResourceEnums.h

AbilitySystem/Library/Structs
	PHAbilitySetStructs.h
	PHAbilityRuntimeStructs.h
	PHResourceStructs.h
	PHSkillStructs.h

AbilitySystem/Library/FunctionLibraries
	PHAbilitySystemFunctionLibrary.h
	PHResourceFunctionLibrary.h
	PHSkillFunctionLibrary.h
```

AbilitySystem rules:

- Keep `UHunterAbilitySystemComponent` as the owner of activation routing.
- Keep `UHunterAttributeSet` as the owner of attributes.
- Keep enum policy checks in `PHAbilitySystemFunctionLibrary`.
- Keep resource reserve/effective math in `PHResourceFunctionLibrary`.
- Keep resource data passing in `PHResourceStructs`.
- Keep ability set grant handles in `PHAbilitySetStructs`.
- Keep authored skill defaults and resolved skill snapshots in `PHSkillStructs`.
- Keep skill-stat folding in `FPHSkillDataResolver`; it is stateless and does not activate abilities, spawn projectiles, or apply damage.
- Use runtime structs for small owned bookkeeping when it makes the owner easier to read.
- Do not move GameplayEffect application into a generic library unless the owner still controls authority and handles.

Base skill data flow:

```text
Content/ProjectHunter/Combat/BP_GameplayAbility (data-only Blueprint)
    -> UPHGameplayAbility::SkillData stores authored identity, timing, costs, range, projectile, aura, and damage input
    -> UGameplayAbility asset tags store Skill.Attack / Skill.Spell / Skill.Projectile / Skill.Aura keywords
    -> FPHSkillDataResolver combines that data with UHunterAttributeSet and an optional FResolvedWeaponStats snapshot
    -> FPHResolvedSkillData is consumed by the ability and any projectile/aura listeners it creates
    -> CombatManager remains the owner of authoritative final hit math
```

Do not create another generic base skill Blueprint. `BP_GameplayAbility` already derives from `UPHGameplayAbility` and is data-only. Child skills author `SkillData` and standard GAS asset tags; behavior belongs only in the child or its single-purpose execution listeners.

### Effective And Reserved Resources

`EffectiveHealth`, `MaxEffectiveHealth`, `ReservedHealth`, and similar values are intentional. They support reserved resources, such as reserving health without treating the full raw max as available.

Do not remove this concept.

Reserve/effective rules:

- Raw max value remains the source maximum.
- Reserved value reduces the effective maximum.
- Current resource should clamp to the effective maximum.
- Explicit reserved values should remain possible when no component-derived reservation overrides them.
- Component-derived reserved values can be calculated from flat and percentage reservation inputs.
- MMC calculations should reuse the same resource math where possible.

This lets gameplay reserve part of a resource while the rest of the system can pass small resource structs and enum types instead of full objects.

## One-File-At-A-Time Workflow

Use this workflow for every refactor pass.

1. Pick one file or one tightly connected pair.
2. Identify the owner.
3. Identify reusable enums, structs, and function-library candidates.
4. Identify Blueprint references and asset risk.
5. Move only the reusable logic.
6. Keep state mutation in the owner.
7. Remove stale includes.
8. Build after C++ changes.
9. Stop and report questions if Blueprint ownership is unclear.

Do not do broad renames and behavior changes in the same pass unless the existing code cannot compile without them.

## File Review Checklist

Use this checklist when checking a C++ file.

### Ownership

- What system does this file belong to?
- What state does it own?
- Is there only one owner for that state?
- Does it mutate state from another system directly?
- Does it broadcast after state changes?
- Does it know too much about UI, animation, audio, or VFX?

### Enums

- Are there local enums that should move to `Library/Enums`?
- Is a full object being passed when an enum would be enough?
- Is the enum Blueprint-exposed only when needed?
- Are enum names specific to the system?

### Structs

- Are grouped parameters repeated across functions?
- Would an input/result struct make the API clearer?
- Is a full object being passed when a small snapshot would be enough?
- Are default values safe?
- Is the struct focused?

### Function Libraries

- Is the file repeating calculations or validation logic?
- Is the logic stateless and reusable?
- Can it move to a function library without moving ownership?
- Would Blueprint need the helper?
- Does the helper avoid hidden mutations?

### Interfaces

- Do unrelated classes expose the same capability?
- Would an interface reduce hard references?
- Is the interface narrow?
- Does it preserve owner authority?
- Is it used outside hot loops or cached when needed?

### Blueprint

- Does Blueprint already own part of this behavior?
- Is Blueprint mutating state that C++ should own?
- Are `UPROPERTY` and `UFUNCTION` permissions intentional?
- Would changing this break existing Blueprint assets?
- Does this need a Core Redirect if renamed?

### Naming

- Does the class use Unreal prefixes correctly?
- Do booleans use `b`?
- Do functions say whether they compute, get, set, try, apply, start, stop, or refresh?
- Are vague names like `Worker`, `Thing`, `Data`, and `Helper` avoidable?
- Does each file name match its main type?

### Comments

- Are comments needed?
- Do comments explain intent, engine constraints, or non-obvious ordering?
- Are stale comments removed?
- Are obvious comments deleted?

### Performance

- Does this add new ticking components?
- Does this add broad per-frame queries?
- Does this move work into Blueprint inside a hot path?
- Can state be event-driven instead of tick-driven?
- Is there a measurable cost, or is the change mostly compile-time organization?

## Naming Conventions

Use Unreal naming conventions:

```text
A  Actor
U  UObject, component, interface UClass side
F  Struct
I  Interface implementation side
E  Enum
b  Boolean variable
T  Template
```

Use clear action names:

```text
CanStartWallTraversal
TryStartWallTraversal
StopWallTraversal
ApplyDamageToTarget
ResolveCombatTarget
CalculateReservedResource
BuildInventorySnapshot
BroadcastEquipmentChanged
```

Avoid vague names:

```text
DoStuff
HandleThing
Worker
DataManager
UpdateAll
Check
```

Use `Check` only when the function truly performs a check and the result or side effect is obvious. Prefer `Can`, `Has`, `Is`, `Resolve`, `Calculate`, `Refresh`, or `Apply`.

## Domain Guidance

### Wall Traversal

Owner:

```text
UPHCharacterMovementComponent
```

Responsibilities:

- Wall surface traces
- Wall normals
- Traversal movement mode
- Wall run/climb physics
- Attach/detach transitions
- Ground and wall exit conditions

Helpers:

```text
WallTraversalFunctionLibrary
WallTraversalStructs
WallTraversalEnums
```

Listeners:

```text
APHBaseCharacter input forwarding
Animation Blueprint presentation
VFX/audio/UI reactions
```

Do not put wall traversal truth in animation or UI.

### Equipment

Owner:

```text
EquipmentManager
```

Responsibilities:

- Equip
- Unequip
- Validate slot compatibility
- Own equipped state
- Broadcast equipment changes

Helpers:

```text
EquipmentFunctionLibrary
EquipmentStructs
EquipmentEnums
```

Listeners:

```text
EquipmentPresentationComponent
Menu widgets
Animation or mesh presentation
```

Equipment should not own inventory storage unless explicitly designed. Inventory can request equip, but equipment owns equipped state.

### Inventory

Owner:

```text
InventoryManager
```

Responsibilities:

- Add item
- Remove item
- Move item
- Validate capacity or rules
- Broadcast inventory changes

Helpers:

```text
InventoryFunctionLibrary
InventoryStructs
InventoryEnums
```

Listeners:

```text
Inventory menu widgets
Loot pickup presentation
Equipment requests
```

Inventory should not directly own equipment presentation.

### Item And Affix Pipeline

Owner:

```text
UItemInstance
```

`UItemInstance` owns one generated item's identity, base-row handle, rolled affixes, per-affix identification state, power/grade, and serialization version. Base DataTable rows are definitions, not runtime owners.

Pipeline:

```text
FItemBase / FAffixData definitions
    -> FAffixGenerator selects legal tiers and rolls values
    -> FPHItemStats stores the rolled runtime modifiers
    -> FItemLocalStatResolver builds immutable weapon/armour-local snapshots
    -> FEquipmentStatsApplier applies only persistent global modifiers through GAS
    -> FContextualStatModifierEvaluator resolves source/skill/target-gated modifiers on demand
```

Rules:

- An unidentified affix remains mechanically active. Identification changes only visibility and naming.
- Each affix owns its own `bIsIdentified`; `bForceAllAffixesIdentified` is an explicit item/base override.
- Equipment can roll affixes. Potions and other consumables use authored Gameplay Effects and do not enter the equipment-affix pipeline.
- A local modifier changes only the base values of its owning item. It must never be flattened into shared character attributes.
- A global scalar modifier contributes to the live character AttributeSet while its source is active. Damage conversion and gain-as-extra are rules, not scalars, and resolve once per hit.
- Conditional and skill modifiers use required/blocked source and target tags plus the legacy condition enum.
- Range modifiers store and roll both endpoints. Conversion modifiers retain explicit source and destination damage types plus whether they convert or gain damage as extra.
- Future mob, tower, or world modifier sources should feed the same contextual modifier evaluator instead of duplicating item-specific combat math.

Helpers:

```text
FAffixGenerator
FItemLocalStatResolver
UItemLocalStatFunctionLibrary
FItemNameBuilder
UItemAffixFunctionLibrary
```

Listeners:

```text
Item tooltip widgets
Inventory/equipment menu cells
Ground-item presentation
```

### Stats

Owners:

```text
UHunterAttributeSet - owns replicated live attribute values.
UStatsManager       - coordinates initialization and active source effects.
```

Helpers:

```text
FEquipmentStatsApplier
FStatsAttributeResolver
FStatsModifierMath
FContextualStatModifierEvaluator
FItemLocalStatResolver
FPrimaryAttributeRules
```

Calculation order:

```text
(base + flat) * (1 + sum(increased and reduced) / 100) * product(more and less)
```

An override, when present, is final for that evaluated attribute. Increased/reduced sources share one additive pool; each more/less source is a separate product factor. The AttributeSet may hold actor or global values, but selected weapon-local damage, critical chance, attack speed, and range stay in the selected weapon snapshot.

`FAnimationDamageInfo::WeaponSource` selects main hand, off hand, two hand, automatic primary, or the character-attribute fallback for actors without item equipment. This keeps dual-wield attacks from summing both weapons and preserves compatibility for mobs that author damage directly on their AttributeSet.

`FPrimaryAttributeRules` is the single source of truth for non-resource primary scaling:

```text
Strength     -> physical increased damage
Intelligence -> elemental increased damage
Dexterity    -> attack/cast speed and critical damage
Endurance    -> all resistance points and less stamina degeneration
Affliction   -> damage over time and ailment duration
Luck         -> ailment application chance
Covenant     -> summon damage and maximum health multipliers
```

Maximum Health, Mana, and Stamina remain GAS-derived values through their resource-specific MMCs. Combat and skill resolvers consume the non-resource bonuses; UI and summon execution can read the same resolved primary snapshot through `UPHPrimaryAttributeFunctionLibrary`.

### Combat

Owner:

```text
CombatManager
```

Recommended internal names:

```text
UCombatStatusEffectApplier      - owned sub-object (not a sibling actor component), handles status effects.
UCombatRecoveryProcessor        - owned state helper, handles timed leech, recoup, and Arcane Shield recharge.
FCombatOutgoingDamageCalculator - plain C++ calculator, outgoing damage math.
FCombatIncomingDamageResolver   - plain C++ resolver, incoming mitigation and hit response.
FCombatAilmentResolver          - plain C++ resolver, threshold/chance/avoidance/duration math.
```

Responsibilities:

- Route combat requests
- Validate combat state
- Direct single-purpose handlers for the actual math (damage, reductions, status) rather than doing it inline
- Apply combat results through authoritative systems
- Broadcast combat events

Calculators and resolvers are plain C++ classes (no `UCLASS`), each in its own file under `Combat/Calculators` or `Combat/Resolvers`, with no dependency back on `CombatManager`. Add another single-purpose helper rather than growing an existing one past one clear purpose.

`UCombatRecoveryProcessor` is intentionally stateful and manager-owned. It starts a server timer only while recovery or recharge is pending, applies replicated resource changes through the owner's ASC, and stops when idle. `CombatManager` calculates hit outcomes and queues work; it does not own recovery ticking.

Outgoing damage stages are explicit:

```text
selected weapon base and local conversion
    -> added/skill base damage
    -> skill conversion and gain-as-extra
    -> character/equipment conversion and gain-as-extra
    -> increased/reduced pool
    -> more/less products
    -> critical strike
```

Damage over time is authored in its final damage type and skips hit conversion and critical strikes. `FCombatHitContext::SkillTags` is merged with legacy Blueprint booleans, so existing attacks remain compatible while new skills and conditional affixes can use gameplay tags.

Current integration boundaries:

- `FPHSkillDataResolver` now folds selected-weapon attack speed/range plus attack speed, cast speed, area, projectile count/speed, chain/fork count, cooldown recovery, cost, and aura attributes into one immutable `FPHResolvedSkillData` snapshot.
- The owning ability must use `UseRate`, `UseIntervalSeconds`, `CooldownSeconds`, and resolved costs. Spawned projectile and aura listeners must receive the same snapshot and use its count/speed/chain/fork/radius/effect values. Resolution does not itself execute those mechanics.
- `Skill.Chain` is capability metadata. Only an actual bounce sets `FAnimationDamageInfo::Tags.bIsChainHit`; otherwise chain-capable skills would receive chain-hit damage on their first target.
- `MT_GrantSkill` requires an authored Gameplay Effect, and `MT_SetRank` still needs a typed ability-rank owner. Neither special type is allowed to fall through as a generic numeric attribute modifier.
- The default `DT_Prefixes`, `DT_Suffixes`, and `DT_Enchants` assets are not present under their configured paths. Prefix/suffix generation uses the native starter fallback unless an item base points to an authored affix table; enchant generation requires authored data.
- Reinforcement/quality has a multiplier and tooltip but is not yet folded into resolved weapon or armour bases because its exact scaling rule is not defined here.
- One `FAffixData` tier produces one runtime stat line. Atomic hybrid affixes need an explicit grouped multi-line definition rather than several independently rolled affixes.
- Automatic weapon selection is deterministic (two-hand, main hand, off hand); alternating dual-wield attacks must select the hand in skill data or gain an explicit alternation policy.
- The contextual evaluator accepts modifiers from any source, but mob, tower, and world/global source registration is future wiring.
- `EDamageType::DT_True` is not a supported combat packet type and is rejected for conversion instead of being silently treated as physical.
- Elemental and corruption resistances default to a 75% maximum with a 90% hard ceiling. Penetration applies after the resistance cap and cannot turn positive resistance negative; already-negative resistance still increases damage.
- Block and invincibility remain active-state decisions, not random avoidance rolls. Parry and invincibility clear damage, stagger, ailments, leech, recoup, and recharge notifications for the hit.
- Elemental ailments add damage-versus-threshold chance to authored chance. Bleed and poison require their explicit chance attributes. Luck adds application chance, Affliction adds duration, and defender Ailment Avoidance applies last. A zero Ailment Threshold falls back to effective maximum Health.
- Poison uses physical plus corruption hit damage. Bleed, poison, ignite, and corruption magnitudes use the pre-mitigation hit snapshot, then apply `DamageOverTime` and Affliction scaling, but still require matching post-block damage to land.
- `StunEffectClass` and `PurifyEffectClass`, like the existing status classes, are effect references configured on the owning character Blueprint. Their chance and duration paths are wired in C++; the GameplayEffects own tags, movement restrictions, visuals, and periodic Health damage. Damaging ailment GameplayEffects must modify Health directly when they are intended to bypass Arcane Shield.
- Leech is recovered over time and is capped per second by the resource's effective maximum and `Max*LeechRatePercent`. Defender Leech Resistance reduces the amount created. Life, Mana, and Stamina recoup are separate timed recovery instances on the defender and do not use the leech cap. On-hit recovery remains instant through `RecoveryApplicationGE`.
- Arcane Shield recharge starts after `ArcaneShieldRechargeDelay`, restores `ArcaneShieldRechargeRate` percent of effective maximum per second, and restarts its delay on each damaging hit. Corruption consumes shield at `CorruptionShieldDamageMultiplier`; setting that multiplier to zero is the explicit shield-bypass mode.

Helpers:

```text
CombatFunctionLibrary
CombatStructs
CombatEnums
CombatInterfaces
```

Listeners:

```text
Damage popup presenter
Hit reactions
Audio/VFX
UI status widgets
```

Do not put damage popup logic in the combat state owner. The owner broadcasts what happened; the presenter decides how to show it.

### AbilitySystem

Owner:

```text
UHunterAbilitySystemComponent
UHunterAttributeSet
UPHGameplayAbility
UPHAbilitySet
```

Helpers:

```text
PHAbilitySystemFunctionLibrary
PHResourceFunctionLibrary
FPHSkillDataResolver / UPHSkillFunctionLibrary
PHAbilitySetStructs
PHAbilityRuntimeStructs
PHResourceStructs
PHSkillStructs
PHAbilityEnums
HunterResourceEnums
```

Listeners:

```text
HUD resource widgets
Menus
Gameplay cues
Animation or presentation systems
```

Keep AbilitySystem refactors conservative because GameplayEffects, GameplayAbilities, AttributeSets, and Blueprints can hold serialized references.

## Suggested Refactor Order

Start with `AbilitySystem`, then move through other domains in small passes.

Recommended order:

1. AbilitySystem libraries and MMC cleanup
2. AbilitySystem component state routing
3. AttributeSet helper extraction without asset-breaking splits
4. Equipment manager ownership and helper extraction
5. Inventory manager ownership and helper extraction
6. Item data and reusable item checks
7. Combat manager and combat helper naming
8. Wall traversal movement/component boundaries
9. Presentation listeners and UI-facing presenters

Each pass should build before moving to the next domain.

## Questions To Ask When Blueprint Ownership Is Unclear

Ask before changing code when any answer cannot be found from the repo:

- Is this behavior currently implemented in Blueprint?
- Is Blueprint supposed to own this gameplay decision, or only presentation?
- Are any GameplayEffects, GameplayAbilities, widgets, or animation Blueprints referencing this class or property by name?
- Can this property be renamed safely, or does it need a Core Redirect?
- Is this value meant to be designer-tuned in assets?
- Should this be replicated later?

If Blueprint owns gameplay logic that should move to C++, document the migration plan before changing assets.

## Definition Of Done

A refactor pass is done when:

- The owner is still clear.
- Shared enums are in `Library/Enums`.
- Shared structs are in `Library/Structs`.
- Shared stateless functions are in `Library/FunctionLibraries`.
- Interfaces are used only where they reduce coupling.
- Blueprint exposure is intentional.
- Comments are useful and not noisy.
- Naming follows Unreal conventions.
- C++ builds after the change.
- Any skipped editor or Blueprint validation is stated clearly.
- Unrelated dirty files are left alone.

## Rule Of Thumb

If it changes state, find the owner.

If it is reused, move it to a helper.

If it only reacts, make it a listener.

If Blueprint handles it, confirm whether Blueprint owns gameplay or presentation.

If the change touches GameplayEffects, AttributeSets, movement, networking, or serialized assets, refactor conservatively and validate with a build.

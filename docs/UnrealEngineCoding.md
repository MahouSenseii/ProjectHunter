# ProjectHunter Unreal Engine C++ Coding Standard

This document defines how C++ should be planned, written, reviewed, tested, and maintained in ProjectHunter.

It is based on Epic's official Unreal Engine documentation and the current ProjectHunter architecture direction. The project may begin as single-player, but C++ should be written with future online play in mind unless a feature is explicitly marked as local-only.

## Official Documentation Baseline

Before changing C++ architecture or adding a new system, review the relevant Epic documentation first. Unreal is heavily documented, and the engine already has established patterns for object lifetime, reflection, gameplay framework ownership, networking, animation, and movement.

Required baseline references:

- [Epic C++ Coding Standard](https://dev.epicgames.com/documentation/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)
- [Unreal Engine Documentation](https://dev.epicgames.com/documentation/unreal-engine)
- [Programming with C++](https://dev.epicgames.com/documentation/unreal-engine/programming-with-cplusplus-in-unreal-engine)
- [Gameplay Framework](https://dev.epicgames.com/documentation/unreal-engine/gameplay-framework-in-unreal-engine)
- [Networking and Multiplayer](https://dev.epicgames.com/documentation/unreal-engine/networking-and-multiplayer-in-unreal-engine)
- [Actor Replication](https://dev.epicgames.com/documentation/unreal-engine/replicating-actors-in-unreal-engine)
- [RPCs](https://dev.epicgames.com/documentation/unreal-engine/remote-procedure-calls-in-unreal-engine)
- [Character Movement Component](https://dev.epicgames.com/documentation/unreal-engine/character-movement-component-in-unreal-engine)

When a feature touches a specialized engine area, also review that area before coding:

- Animation Blueprint and montages for animation behavior.
- Enhanced Input for input mapping.
- Gameplay Ability System for abilities, effects, attributes, and prediction.
- AI documentation for behavior trees, perception, EQS, and controllers.
- SaveGame documentation for persistent local data.
- Subsystems documentation for global or scoped services.
- Data Assets and Data Tables documentation for data-driven systems.

## Development Modes

Every non-trivial change should move through four modes:

1. Planning
2. Architecture
3. Developer
4. QA

If the change fails QA, go back to the correct earlier mode. Do not blindly patch symptoms. Repeat the loop until the change reaches a passing grade.

```text
Planning -> Architecture -> Developer -> QA
     ^           ^             ^       |
     |           |             |       |
     +-----------+-------------+-------+
             repeat until passing
```

## Mode 1: Planning

Planning defines the real problem before code is touched.

### Required Planning Checks

- Read the relevant Epic documentation first.
- Identify the exact feature, bug, or system being changed.
- Identify the current owner class or component.
- Identify the current behavior and the desired behavior.
- Identify whether the change affects single-player only, future multiplayer, or both.
- Identify whether the change affects C++, Blueprint, Animation Blueprint, assets, input, UI, or data.
- Identify what must be tested to call the change complete.
- Identify what must not be touched.

### Planning Output

A good plan should answer:

- What owns the state?
- What receives input?
- What changes gameplay state?
- What only reacts visually?
- What must replicate later?
- What is the smallest safe change?
- What test proves the change works?

### Planning Fail Conditions

Planning fails if:

- The owner is unclear.
- The code path is guessed instead of inspected.
- Epic documentation was skipped for a new engine feature.
- Multiplayer impact is ignored.
- The acceptance test is vague.
- The plan requires touching unrelated systems without a reason.

## Mode 2: Architecture

Architecture decides where the code belongs.

ProjectHunter should use a simple OOP model:

```text
Owner -> Helpers -> Listeners
```

## ProjectHunter Architecture Skill Rules

Use the ProjectHunter architecture workflow whenever a task asks where code belongs, how to refactor a gameplay system, or how to keep systems from depending on each other too deeply.

The goal is to understand the smallest useful slice of the repo, not to load the entire project into your head.

### Verify The Workspace

Before applying ProjectHunter-specific architecture rules, confirm the workspace is actually ProjectHunter:

```text
ALS_ProjectHunter.uproject
Source/ALS_ProjectHunter
```

If those anchors are missing, do not assume ProjectHunter-specific paths or ownership rules.

### Read Minimally First

Start with the smallest set of files that can establish ownership and boundaries.

Read in this order:

1. The owner.
2. The nearest reusable helper or function library.
3. The delegate, broadcast, or listener touchpoints.
4. More files only if ownership is still unclear or the bug crosses system boundaries.

Do not trace a full feature area by default. A large read often hides the real ownership question.

### Skill Workflow

When deciding where code should live:

1. Identify the owner first.
2. State the owner plainly.
3. Identify the nearest shared helper location.
4. Identify listeners or delegate consumers.
5. Recommend the smallest change that preserves those boundaries.

Use this wording during review:

- "The owner here is..."
- "This check is reusable, so it belongs in..."
- "This broadcast should only announce the change; listeners can react separately."
- "You do not need the whole system loaded to place this logic."

### Current ProjectHunter Anchor Examples

These are current repo anchors for the owner/helper/listener model. Treat the repo as the source of truth if files move later.

Owner examples:

```text
Source/ALS_ProjectHunter/Public/Equipment/Components/EquipmentManager.h
Source/ALS_ProjectHunter/Private/Equipment/Components/EquipmentManager.cpp
Source/ALS_ProjectHunter/Public/Inventory/Components/InventoryManager.h
Source/ALS_ProjectHunter/Private/Inventory/Components/InventoryManager.cpp
Source/ALS_ProjectHunter/Public/Stats/Components/StatsManager.h
Source/ALS_ProjectHunter/Private/Stats/Components/StatsManager.cpp
```

Helper examples:

```text
Source/ALS_ProjectHunter/Public/Item/Library/ItemFunctionLibrary.h
Source/ALS_ProjectHunter/Private/Item/Library/ItemFunctionLibrary.cpp
Source/ALS_ProjectHunter/Public/Equipment/Library/EquipmentFunctionLibrary.h
Source/ALS_ProjectHunter/Public/Inventory/Library/InventoryFunctionLibrary.h
```

Listener examples:

```text
Source/ALS_ProjectHunter/Public/Equipment/Components/EquipmentPresentationComponent.h
Source/ALS_ProjectHunter/Public/Menu
Animation Blueprints
UI Widgets
Audio/VFX presentation systems
```

Useful boundary searches:

```text
DECLARE_DYNAMIC_MULTICAST_DELEGATE
Broadcast(
UBlueprintFunctionLibrary
ReplicatedUsing
Equip
Unequip
Add
Remove
Apply
Refresh
```

### Quick Placement Heuristic

- If logic changes owned state, it usually belongs in the owner.
- If logic is reusable across owners, it usually belongs in a helper or function library.
- If logic only reacts after a state change, it usually belongs in a listener.

### ProjectHunter Boundary Rules

- Keep managers, components, and subsystems focused on state transitions.
- Put reusable checks, calculations, formatting, conversions, and lookup helpers into Library function libraries.
- Prefer broadcast-state-only delegates. The owner announces what changed; listeners own their response.
- Do not make the owner aware of every UI, animation, audio, or VFX consumer.
- Do not move state ownership out of the owner just to clean up a file.
- Do not turn function libraries into hidden global managers.
- Expand the investigation only when the first pass cannot answer who owns the state or where the logic belongs.

### Owner

The owner stores state and performs the main mutations.

Owners are usually:

- Actors.
- Actor components.
- Movement components.
- Subsystems.
- Managers.
- Game framework classes.

Owners usually contain verbs such as:

- `Equip`
- `Unequip`
- `Add`
- `Remove`
- `Apply`
- `Set`
- `Start`
- `Stop`
- `Refresh`
- `Spawn`
- `Destroy`

Owner rules:

- One system should have one clear owner for each piece of state.
- The owner should validate requests before changing state.
- The owner should own replication for its state when applicable.
- The owner should broadcast state changes after mutation.
- The owner should not know every listener's internal behavior.
- The owner should not become a god class.

### Helpers

Helpers contain reusable logic that does not own gameplay state.

Helpers are usually:

- Function libraries.
- Stateless utility structs.
- Validation helpers.
- Conversion helpers.
- Formatting helpers.
- Query helpers.

Helper rules:

- Keep helpers stateless unless there is a clear reason.
- Do not move state ownership into a helper just to make the owner file smaller.
- Do not hide world-changing behavior in a function that looks like a pure helper.
- If a helper needs a world context, make that dependency explicit.
- If a helper is used by more than one system, it belongs in a Library area.

### Listeners

Listeners react after state changes.

Listeners are usually:

- UI widgets.
- Animation Blueprints.
- VFX systems.
- Audio systems.
- Presentation components.
- Other systems subscribed to delegates.

Listener rules:

- Listeners should not be required knowledge for the owner.
- Listeners should react to events or read state through narrow APIs.
- Listeners should not mutate owner state unless the owner exposes a clear command.
- Animation should present movement state; it should not be the authority for movement state.

### OOP Rules

Use object-oriented programming intentionally.

#### Encapsulation

Keep data private or protected unless external code truly needs access.

Prefer:

```cpp
bool IsWallTraversalActive() const;
void StopWallTraversal();
```

Avoid exposing raw writable state:

```cpp
UPROPERTY(BlueprintReadWrite)
bool bIsWallTraversalActive;
```

Blueprint should not be able to freely corrupt core gameplay state.

#### Single Responsibility

Each class should have one main reason to change.

Good:

- Movement component owns movement physics.
- Equipment manager owns equipped items.
- Inventory manager owns inventory state.
- Widget presenter prepares UI-facing data.
- Function library performs reusable stateless checks.

Bad:

- Character owns movement physics, inventory rules, UI formatting, combat validation, and item generation.
- Animation Blueprint decides whether a movement mode is valid.
- Function library changes replicated gameplay state without going through the owner.

#### Composition Over Inheritance

Prefer components and helpers over deep inheritance when behavior can be attached or reused.

Use inheritance when the class truly "is a" specialized version of the parent.

Use components when the class "has a" behavior.

Use function libraries when the logic is stateless and reusable.

Use interfaces when different classes expose the same capability without sharing a base class.

#### Dependency Direction

Dependencies should point toward stable ownership.

Good:

```text
Character -> Movement Component
UI -> Manager read API
Animation -> Character/Movement read API
Manager -> Function Library
Owner -> Broadcast delegate
Listener -> React to delegate
```

Bad:

```text
Movement Component -> Widget
Function Library -> Specific UI Widget
Animation Notify -> Directly rewrites unrelated manager state
Equipment Manager -> Hard-coded knowledge of every UI screen
```

## Mode 3: Developer

Developer mode is where code is changed.

### Required Developer Checks

- Keep the patch small and scoped.
- Follow Epic's C++ coding standard.
- Use Unreal reflection macros where needed.
- Use Unreal object lifetime rules.
- Use clear ownership and helper boundaries.
- Add comments only where they improve understanding.
- Build after C++ changes.
- Do not touch unrelated dirty files.

### Header Rules

Headers should be clean and stable.

Use headers for:

- Public API.
- Reflected properties.
- Reflected functions.
- Delegates.
- Lightweight inline accessors.
- Forward declarations.

Avoid in headers:

- Heavy includes that can be forward declared.
- Large inline logic.
- Private implementation details.
- Debug-only helper code.
- Unnecessary comments.

Header checklist:

- Does the header include only what it needs?
- Can a type be forward declared?
- Are reflected properties intentional?
- Are Blueprint permissions correct?
- Is writable state protected?
- Are functions named clearly?
- Are comments still accurate?

### Source File Rules

Source files should contain implementation details.

Use `.cpp` files for:

- Full includes.
- Private helper functions.
- Complex logic.
- Debug drawing.
- Runtime validation.
- Implementation comments.

Source checklist:

- Is the function doing one clear job?
- Are early exits readable?
- Are null checks present where needed?
- Are authority checks present where needed?
- Are side effects obvious?
- Are logs useful but not noisy?
- Are debug visuals tied to real gameplay decisions?

## Mode 4: QA

QA proves the change works.

### Required QA Checks

- Build the project after C++ changes.
- Run the feature in-editor when behavior changed.
- Check logs for errors and warnings.
- Test success cases.
- Test failure cases.
- Test transitions.
- Test edge cases.
- Confirm no unrelated files were changed.
- Confirm Blueprint and asset behavior still matches the C++ rules.

### QA For Future Online Support

Even if the current feature is tested in single-player, ask:

- Who owns authority?
- Does the server need to validate this action?
- Does this state need replication?
- Does the owning client need prediction?
- Can a simulated proxy display this correctly?
- Does the code assume local-only control?
- Does the code depend on a widget, camera, or local controller on the server?
- Would this break on a dedicated server?

### Passing Grade

A change passes only when all required gates are satisfied or explicitly waived with a reason.

Minimum passing grade:

- Compiles.
- Does not introduce obvious warnings.
- Matches Epic coding standard.
- Has one clear owner.
- Keeps reusable code in the correct Library/helper area.
- Does not put gameplay authority in Animation Blueprint or UI.
- Is written with future networking in mind.
- Has meaningful validation.
- Leaves unrelated files untouched.

If any required gate fails, return to Planning, Architecture, or Developer mode and repeat.

## Epic C++ Coding Standard Rules

Follow Epic's C++ coding standard unless there is a project-specific reason not to.

### Naming

Use Unreal naming conventions.

- `A` prefix for actors: `APHBaseCharacter`.
- `U` prefix for UObject types and components: `UPHCharacterMovementComponent`.
- `F` prefix for structs: `FWallTraversalSettings`.
- `I` prefix for interfaces.
- `E` prefix for enums: `EALSMovementState`.
- `T` prefix for templates.
- `b` prefix for booleans: `bIsWallRunning`.

Use PascalCase for types, functions, and variables.

Good:

```cpp
void TryStartWallRun();
bool CanAttachToWall() const;
FVector CurrentWallNormal;
```

Bad:

```cpp
void try_start_wall_run();
bool wallrunning;
FVector current_wall_normal;
```

Boolean functions should read like questions:

```cpp
bool IsWallTraversalActive() const;
bool CanStartWallRun() const;
bool HasValidWallSurface() const;
```

Function names should make side effects clear.

Good:

```cpp
FVector ComputeWallMovementDirection() const;
bool TryStartWallRun();
void StopWallTraversal();
void ApplyDamageToTarget();
```

Bad:

```cpp
FVector WallThing();
void HandleStuff();
void Update();
void Check();
```

If a function mutates state, its name should say so.

### Types

Use Unreal types when working in Unreal systems.

Prefer:

- `FString`
- `FName`
- `FText`
- `TArray`
- `TMap`
- `TSet`
- `TObjectPtr`
- `TWeakObjectPtr`
- `TSubclassOf`
- `TSoftObjectPtr`
- `TSoftClassPtr`

Use standard C++ types only when they are clearly appropriate and do not fight Unreal reflection, memory, serialization, or platform rules.

### Const Correctness

Use `const` when a function does not mutate state.

```cpp
bool CanStartWallRun() const;
FVector GetCurrentWallNormal() const;
```

Use `const` references for large input values:

```cpp
void SetWallTraversalSettings(const FWallTraversalSettings& NewSettings);
```

Do not mark a function `const` if it mutates hidden state in a way that affects gameplay behavior.

### Null Checks

Check pointers before use unless the pointer is guaranteed valid by construction and documented by ownership.

Good:

```cpp
if (!MovementComponent)
{
    return false;
}
```

Avoid long chains of unchecked access:

```cpp
Character->GetController()->GetPawn()->GetRootComponent();
```

### Early Returns

Use early returns to keep logic flat and readable.

Good:

```cpp
if (!CanStartWallRun())
{
    return false;
}

if (!FindWallSurface(OutHit))
{
    return false;
}

StartWallRunFromHit(OutHit);
return true;
```

Avoid deeply nested logic when each failure can exit clearly.

## Unreal Reflection Rules

Use reflection when Unreal needs to see the type, property, or function.

Use:

- `UCLASS` for UObject classes.
- `USTRUCT` for reflected structs.
- `UENUM` for reflected enums.
- `UPROPERTY` for UObject references, serialization, editor exposure, replication, or Blueprint access.
- `UFUNCTION` for Blueprint calls, RPCs, delegates, or reflection-driven calls.

Do not expose something to reflection just because it is convenient.

### UPROPERTY

Use `UPROPERTY` for UObject references that must survive garbage collection or be visible to Unreal systems.

```cpp
UPROPERTY()
TObjectPtr<UStaticMeshComponent> CachedWallComponent;
```

Use clear access.

Good:

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wall Traversal", meta = (AllowPrivateAccess = "true"))
float WallRunSpeed = 650.0f;
```

Avoid exposing core state as freely writable:

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite)
bool bIsWallRunning;
```

### UObject References

Use `TObjectPtr` for UObject member references.

Use `TWeakObjectPtr` for observed objects that may disappear.

Use `TSoftObjectPtr` or `TSoftClassPtr` for assets or classes that should be loaded on demand.

Do not store long-lived raw UObject pointers unless lifetime is guaranteed and the reason is clear.

### UFUNCTION

Use `BlueprintCallable` for explicit commands Blueprint may call.

Use `BlueprintPure` only when the function has no side effects.

Use `BlueprintAuthorityOnly` where Blueprint should only execute server-authoritative behavior.

Use RPC specifiers only when the function truly crosses the network boundary.

## Comment Rules

Comments should help a future developer understand intent, risk, or non-obvious behavior.

Do not comment obvious code.

Bad:

```cpp
// Set speed to wall run speed
WallRunSpeed = NewWallRunSpeed;
```

Good:

```cpp
// Keep this above max walk speed so client prediction does not blend back into normal grounded movement.
WallRunSpeed = NewWallRunSpeed;
```

### Function Comments

Only add a function comment when the function is complex, public-facing, or has important constraints.

A function comment should explain:

- What the function guarantees.
- What the function refuses to do.
- What assumptions it depends on.
- Any important side effects.
- Any networking or authority requirement.

Do not write a comment that only repeats the function name.

Bad:

```cpp
// Starts wall run.
bool TryStartWallRun();
```

Better:

```cpp
// Attempts to attach to a valid static wall surface. Returns false without changing movement mode if no valid surface is found.
bool TryStartWallRun();
```

### Inline Comments

Use inline comments for complex logic only.

Good places for comments:

- Non-obvious math.
- Network prediction assumptions.
- Engine behavior that looks strange but is required.
- Temporary compatibility work.
- Important ordering constraints.

Bad places for comments:

- Repeating variable names.
- Explaining simple `if` statements.
- Leaving old code descriptions after the code changed.
- File path comments that drift out of date.

### TODO Comments

TODOs must be specific.

Good:

```cpp
// TODO(Network): Replace local-only stamina check with server-authoritative stamina validation before enabling multiplayer.
```

Bad:

```cpp
// TODO fix later
```

Every TODO should include a reason or category:

- `TODO(Network)`
- `TODO(Animation)`
- `TODO(Performance)`
- `TODO(Editor)`
- `TODO(Design)`

Delete stale TODOs and stale comments when touching nearby code.

## Project Source Layout

ProjectHunter code lives under:

```text
Source/ALS_ProjectHunter/Public
Source/ALS_ProjectHunter/Private
```

Use domain folders for gameplay systems:

```text
Character
Combat
Data
Equipment
GameModes
Interactable
Inventory
Item
Loot
Menu
Player
Progression
Stats
System
Tags
Tower
```

New code should stay inside the game module unless it is truly plugin-level reusable code.

Do not move ProjectHunter-specific systems into the ALS plugin just because the ALS plugin has a clean folder shape. Use ALS as a style reference, not as the destination for game-specific code.

## Library Folder Rule

If code will be used by more than one place, it should not stay trapped inside one actor or component.

Create or use a `Library` folder for shared code.

Preferred new layout:

```text
Source/ALS_ProjectHunter/Public/<Domain>/Library/Enums
Source/ALS_ProjectHunter/Public/<Domain>/Library/Structs
Source/ALS_ProjectHunter/Public/<Domain>/Library/FunctionLibraries
Source/ALS_ProjectHunter/Private/<Domain>/Library/FunctionLibraries
```

For truly global shared code:

```text
Source/ALS_ProjectHunter/Public/Library/Enums
Source/ALS_ProjectHunter/Public/Library/Structs
Source/ALS_ProjectHunter/Public/Library/FunctionLibraries
Source/ALS_ProjectHunter/Private/Library/FunctionLibraries
```

Use the domain Library first when the code belongs to one gameplay area. Use the global Library only when the code is genuinely cross-domain.

Existing flat Library files are allowed. Do not churn working files just to rename folders. New shared code should use the split layout when practical.

### Enums

Enums belong in:

```text
Library/Enums
```

Use `enum class`.

Use `UENUM(BlueprintType)` only when Blueprint needs the enum.

Use `uint8` when the enum must be Blueprint-friendly.

Example:

```cpp
UENUM(BlueprintType)
enum class EPHWallTraversalMode : uint8
{
    None,
    WallRunning,
    WallClimbing
};
```

Enum rules:

- Keep names specific.
- Do not use generic names like `EState` or `EMode`.
- Do not put unrelated enum values in the same enum.
- Do not expose to Blueprint unless needed.

### Structs

Structs belong in:

```text
Library/Structs
```

Use structs for grouped data, settings, results, and lightweight value types.

Use `USTRUCT(BlueprintType)` only when Blueprint needs it.

Initialize all values.

Example:

```cpp
USTRUCT(BlueprintType)
struct FPHWallTraversalSettings
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wall Traversal")
    float WallRunSpeed = 650.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wall Traversal")
    float WallAttachDistance = 75.0f;
};
```

Struct rules:

- Keep structs focused.
- Give every field a safe default.
- Put validation helpers near the owner or in a function library.
- Avoid storing owner-only mutable gameplay state in a shared struct unless the owner controls it.

### Function Libraries

Function libraries belong in:

```text
Library/FunctionLibraries
```

Use `UBlueprintFunctionLibrary` for stateless reusable logic.

Good function library uses:

- Calculations.
- Validation checks.
- Data lookup helpers.
- Formatting.
- Type conversion.
- Shared gameplay queries.

Bad function library uses:

- Owning replicated state.
- Mutating an actor without clear ownership.
- Acting like a hidden global manager.
- Calling UI or animation code directly.
- Hiding server authority decisions.

Example:

```cpp
UCLASS()
class ALS_PROJECTHUNTER_API UPHWallTraversalFunctionLibrary final : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Wall Traversal")
    static bool IsValidWallTraversalSurface(const UPrimitiveComponent* Component);
};
```

Function library checklist:

- Is the function stateless?
- Can more than one system use it?
- Are side effects absent or clearly named?
- Does it avoid owning state?
- Does it avoid depending on a specific actor unless needed?
- Is it safe for server and client use?

## Blueprint Rules

Blueprint should be used where it is strong:

- Presentation.
- Tuning exposed values.
- Animation graph wiring.
- UI layout.
- Asset references.
- High-level design iteration.

C++ should own:

- Gameplay rules.
- Authority checks.
- Movement physics.
- Core combat validation.
- Inventory and equipment state.
- Replication-sensitive logic.
- Complex queries.
- Save/load rules.
- Performance-sensitive loops.

Expose C++ to Blueprint carefully.

Prefer:

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
float AttackRange = 150.0f;
```

Use `BlueprintReadWrite` only when Blueprint is intentionally allowed to change the value.

Use `BlueprintCallable` for commands.

Use `BlueprintPure` only for side-effect-free reads.

## Animation Rules

Animation should present gameplay state. It should not own gameplay truth.

Animation may handle:

- Pose blending.
- State machine transitions.
- IK.
- Aim offsets.
- Additives.
- Montage slots.
- Blend alphas.
- Cosmetic timing.

Animation should not decide:

- Whether the character can wall run.
- Whether the character is attached to a wall.
- Whether a wall surface is valid.
- Whether an attack is allowed by gameplay rules.
- Whether replicated movement state is true.

Montage notifies may request gameplay actions, but the owner must validate them.

If an attack montage plays during special locomotion, it must either:

- Preserve the current movement attachment and rotation, or
- Intentionally request an exit from that locomotion state.

Do not let a montage accidentally reset movement state or character rotation.

## Movement Rules

Movement code must be owned by the movement component or the appropriate movement owner.

For ProjectHunter traversal:

- `UPHCharacterMovementComponent` owns traversal physics, wall surface state, snapping, normals, movement mode, and transitions.
- `APHBaseCharacter` should forward input and high-level requests.
- ALS state and Animation Blueprint should present the result.
- Mantle logic should remain owned by the mantle system unless a clear handoff is required.

Movement code should:

- Use traces or sweeps that match the real gameplay decision.
- Reject invalid surfaces.
- Keep input conversion separate from surface detection.
- Store and update surface normals as needed.
- Handle curved surfaces.
- Handle nearby floor and wall transitions.
- Guard against rapid state flipping.
- Preserve network-relevant state.

Movement code should not:

- Depend on widgets.
- Depend on animation graph internals.
- Accept pawns or characters as wall traversal surfaces unless explicitly designed.
- Use only attach-time camera direction when live input direction is required.
- Hide movement mode changes in unrelated animation notifies.

## Networking and Online-Ready Rules

ProjectHunter may start single-player, but C++ should avoid local-only assumptions.

### Authority

Gameplay authority should be clear.

Server-authoritative examples:

- Damage.
- Inventory changes.
- Equipment changes.
- Item pickup.
- Loot generation.
- Stamina spending.
- Ability activation validation.
- Quest or progression rewards.
- Final movement state correction.

Client-side examples:

- Camera.
- Local input.
- UI.
- Prediction.
- Cosmetic effects.
- Local animation presentation.

Never trust a client-only result for important gameplay state.

### Game Framework Ownership

Use Unreal's framework classes correctly.

Typical ownership:

- `GameMode`: server-only match rules.
- `GameState`: replicated match state.
- `PlayerController`: player input, client ownership, UI bridge.
- `PlayerState`: replicated player data.
- `Pawn` or `Character`: controllable body.
- `ActorComponent`: reusable actor-attached behavior.
- `Subsystem`: scoped service.

Do not put replicated player data only in `PlayerController` if other clients need to see it. Use `PlayerState` or another replicated actor/component when appropriate.

Do not put server-only rules in UI.

Do not rely on local player controller access on a dedicated server.

### Replication

Replicate state, not implementation noise.

Good replication candidates:

- Current gameplay state.
- Current equipment.
- Current health/stamina values.
- Current replicated movement-relevant state.
- Important ability state.
- Interactable state visible to other players.

Bad replication candidates:

- Debug flags.
- Purely local widget state.
- Cosmetic-only blend values.
- Temporary local camera values.
- Data that can be derived cheaply from already replicated state.

Use `ReplicatedUsing` when clients need to react to a state change.

Use delegates or local handlers from `OnRep` functions to update presentation.

### RPCs

Use RPCs for events crossing the network boundary.

Rules:

- Client RPCs should be presentation or client-specific responses.
- Server RPCs should validate requests before mutating authoritative state.
- Multicast RPCs should be used carefully for events all relevant clients need.
- Do not spam reliable RPCs for high-frequency updates.
- Do not use RPCs when replicated state is the correct model.

Server RPC checklist:

- Is the caller allowed to request this?
- Is the target valid?
- Is the distance/range valid?
- Is the cooldown valid?
- Is the resource cost valid?
- Is the current state valid?
- What happens if the client lies?

### Prediction

Movement and combat feel may need prediction later.

When writing local movement or action code, keep the logic separable:

- Input request.
- Local prediction.
- Server validation.
- Replicated correction.
- Visual response.

Do not bake all behavior into local-only Blueprint graphs if the feature may become networked.

## Data Rules

Use data-driven design where it improves iteration.

Good data-driven candidates:

- Item definitions.
- Equipment stats.
- Loot tables.
- Enemy archetypes.
- Ability tuning.
- Combat values.
- Traversal tuning.
- UI text.

Use C++ for:

- Validation.
- Ownership.
- Runtime state.
- Replication.
- Algorithms.
- Safety checks.

Use Data Assets or Data Tables for authored content, not for hiding gameplay authority.

## Logging and Debug Rules

Debug output should prove the real code path.

Use logs and debug draw to show:

- What input was received.
- What trace was run.
- What surface or actor was hit.
- Why a result was accepted.
- Why a result was rejected.
- What state changed.
- What authority path was used.
- Why a transition exited.

Debug visuals must match the actual gameplay query. If debug uses a different trace than the real decision, it is not useful.

Logging rules:

- Use clear categories.
- Keep logs actionable.
- Avoid noisy per-frame logs unless guarded by a debug flag.
- Do not leave temporary spam in committed code.
- Log rejection reasons for complex validation.

## Error Handling and Validation

Validation should happen at system boundaries.

Validate:

- Pointers.
- Components.
- Actor ownership.
- Authority.
- Input ranges.
- Array indexes.
- Data asset references.
- Gameplay tags.
- Movement modes.
- Collision channels.
- Surface types.
- Replication assumptions.

Use `ensure` for unexpected non-fatal issues that should be visible in development.

Use early returns for expected invalid cases.

Do not crash the game for a normal failed gameplay check.

## Performance Rules

Performance-sensitive code should be intentional.

Avoid:

- Expensive per-frame searches.
- Repeated `GetAllActorsOfClass`.
- Allocating large arrays every tick without reason.
- Blueprint calls inside hot C++ loops.
- Unbounded traces every frame.
- Recomputing data that can be cached safely.

Prefer:

- Cached references with valid lifetime handling.
- Event-driven updates.
- Timers instead of tick when possible.
- Narrow traces.
- Explicit debug toggles.
- Profiling before large optimization passes.

Tick rules:

- Do not add tick by default.
- If tick is needed, document why.
- Disable tick when inactive.
- Keep tick work small.
- Avoid network-heavy work in tick.

## Include and Dependency Rules

Keep dependencies narrow.

Header rules:

- Forward declare when possible.
- Include only what the header needs.
- Do not include large system headers casually.
- Do not use `using namespace` in headers.

Source rules:

- Include the matching header first.
- Include required engine/project headers in the `.cpp`.
- Remove unused includes when touching a file.

Circular dependencies usually mean ownership is unclear. Fix the ownership rather than forcing includes.

## API Design Rules

Public API should be small and intentional.

Good public API:

```cpp
bool CanStartWallRun() const;
bool TryStartWallRun();
void StopWallTraversal();
FVector GetCurrentWallNormal() const;
```

Bad public API:

```cpp
void SetEverything();
void DoMovementStuff();
UPROPERTY(BlueprintReadWrite)
FVector WallNormal;
```

API checklist:

- Does the function name describe the action?
- Is it clear whether the function mutates state?
- Is the return value useful?
- Is authority required?
- Is Blueprint access necessary?
- Can this be private?
- Can this be a helper?

## Review Checklist

Use this checklist during review.

### Planning Review

- Relevant Epic docs were reviewed.
- Owner is identified.
- Multiplayer impact is identified.
- Acceptance tests are listed.
- Out-of-scope systems are named.

### Architecture Review

- State has one owner.
- Helpers are stateless.
- Listeners react instead of owning.
- Library code is used for shared logic.
- Blueprint exposure is minimal.
- Animation does not own gameplay truth.
- Networking path is clear.

### Code Review

- Follows Epic naming.
- Uses Unreal types correctly.
- Uses reflection intentionally.
- Uses `TObjectPtr`, `TWeakObjectPtr`, or soft references appropriately.
- Uses `const` correctly.
- Uses null checks where needed.
- Uses early returns to avoid deep nesting.
- Comments explain intent, not obvious code.
- No stale comments.
- No unrelated cleanup.

### QA Review

- C++ build passes.
- PIE behavior is tested.
- Logs are clean enough.
- Edge cases are tested.
- Invalid input is tested.
- Replication assumptions are documented or tested.
- No unrelated files changed.

## Definition of Done

A change is done when:

- The feature or fix behaves as requested.
- The editor target builds after C++ changes.
- The code follows Epic's C++ coding standard.
- The code follows ProjectHunter owner/helper/listener architecture.
- Shared logic is placed in a Library folder when reused.
- Comments are accurate and useful.
- Future multiplayer assumptions are not broken.
- The result has been tested enough for the risk level.
- Any skipped validation is clearly stated.

## Project Rule of Thumb

If the code changes state, find the owner.

If the code is reused, move it to a Library helper.

If the code only reacts, make it a listener.

If the code affects gameplay authority, design it as online-ready from the start.

If the code depends on an Unreal system you are not sure about, read the Epic documentation before proceeding.

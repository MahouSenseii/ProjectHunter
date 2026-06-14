# Project Hunter — Climbing as an ALS Movement State (v2)

**Design corrections from v1 (this version):**
1. Climb is NOT a side component with its own mini state machine — it is a
   **movement state exactly like Grounded/InAir**, flowing through ALS's
   MovementState + Gait model.
2. There is no "WallRunLeft/Right". **Wall run = the Sprint gait while
   Climbing** — fast movement in ANY direction on the wall. Slow climb =
   Walk gait, normal climb = Run gait, wall run = Sprint gait. One state,
   three gaits, the same sprint input as on the ground.

```
EALSMovementState:  Grounded | InAir | Mantling | Ragdoll | + Climbing  ← new
EALSGait (existing): Walking → slow climb
                     Running → climb
                     Sprinting → WALL RUN (all directions)
Speeds resolve through the same machinery as ground movement:
PHBaseCharacter::GetTargetMovementSettings() returns ClimbingMovementSettings
while Climbing → UALSCharacterMovementComponent::GetMaxSpeed() does the rest.
```

The physics lives where ALS physics lives: a CharacterMovementComponent
subclass with a custom movement mode (`PhysClimb`), not external velocity
poking. This also rides ALS's existing saved-move/replication path.

---

## Step 1 — ALS plugin edit (one enum value)

`Plugins/ALSV4_CPP/.../ALSCharacterEnumLibrary.h` — add to `EALSMovementState`:

```cpp
UENUM(BlueprintType)
enum class EALSMovementState : uint8
{
	None,
	Grounded,
	InAir,
	Mantling,
	Ragdoll,
	Climbing    UMETA(DisplayName = "Climbing")   // ← add
};
```

Your ALS AnimBP's MovementState switches get a new `Climbing` case (Step 5).
Existing switch nodes treat the unknown value as default until then — safe.

## Step 2 — New: `UPHCharacterMovementComponent` (the physics)

### `Public/Character/Components/PHCharacterMovementComponent.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Character/ALSCharacterMovementComponent.h"
#include "PHCharacterMovementComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPHMovement, Log, All);

/** Custom movement modes for MOVE_Custom. */
UENUM(BlueprintType)
enum class EPHCustomMovement : uint8
{
	CMOVE_None  = 0 UMETA(Hidden),
	CMOVE_Climb = 1 UMETA(DisplayName = "Climb")
};

/**
 * Project movement component: ALS movement + wall climbing as a first-class
 * movement mode. Wall run is NOT a separate mode — it is the Sprint gait
 * while climbing (speeds resolve through CurrentMovementSettings exactly like
 * ground gaits; the character feeds ClimbingMovementSettings while Climbing).
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHCharacterMovementComponent : public UALSCharacterMovementComponent
{
	GENERATED_BODY()

public:
	// ── Climb tuning (speeds come from the ALS movement settings, NOT here) ──

	/** Desired capsule distance from the wall surface. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Climb")
	float WallOffset = 45.f;

	/** Wall detection/tracking distance from capsule center. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Climb")
	float WallDetectDistance = 70.f;

	/** Sphere radius for wall traces (forgiveness on uneven surfaces). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Climb")
	float WallTraceRadius = 20.f;

	/** |dot(Normal, Up)| must be <= this for a surface to be climbable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Climb", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxWallUpDot = 0.45f;

	/** Spring strength holding the capsule at WallOffset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Climb")
	float WallSpring = 4.f;

	/** Yaw interp speed to face the wall. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Climb")
	float FaceWallInterpSpeed = 10.f;

	/** Holding input AWAY from the wall this long detaches (Genshin back-off). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Climb")
	float PullAwayDetachTime = 0.25f;

	/** Trace channel for wall geometry. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Climb")
	TEnumAsByte<ECollisionChannel> WallChannel = ECC_Visibility;

	// ── API ──────────────────────────────────────────────────────────────

	/** Attach to the wall ahead if climbable. */
	bool TryStartClimb();

	/** Leave the wall (falling). */
	void StopClimb();

	/** Climb-jump impulse along the wall plane in the current input direction. */
	void DoClimbJump(float Impulse);

	UFUNCTION(BlueprintPure, Category = "Climb")
	bool IsClimbing() const
	{
		return MovementMode == MOVE_Custom
			&& CustomMovementMode == static_cast<uint8>(EPHCustomMovement::CMOVE_Climb);
	}

	/** Wall-plane input for the AnimBP blendspace: X = right, Y = up. */
	UFUNCTION(BlueprintPure, Category = "Climb")
	FVector2D GetClimbInput() const { return ClimbPlaneInput; }

	UFUNCTION(BlueprintPure, Category = "Climb")
	FVector GetWallNormal() const { return WallNormal; }

protected:
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	void PhysClimb(float deltaTime, int32 Iterations);

	bool TraceWall(const FVector& Direction, float Distance, FHitResult& OutHit) const;
	bool IsClimbableSurface(const FVector& Normal) const;

	FVector   WallNormal = FVector::ZeroVector;
	FVector   WallImpact = FVector::ZeroVector;
	FVector2D ClimbPlaneInput = FVector2D::ZeroVector;
	float     PullAwayTime = 0.f;
};
```

### `Private/Character/Components/PHCharacterMovementComponent.cpp`

```cpp
#include "Character/Components/PHCharacterMovementComponent.h"

#include "Character/PHBaseCharacter.h"
#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY(LogPHMovement);

bool UPHCharacterMovementComponent::TryStartClimb()
{
	if (IsClimbing() || !CharacterOwner)
	{
		return false;
	}

	FHitResult Hit;
	if (!TraceWall(CharacterOwner->GetActorForwardVector(), WallDetectDistance, Hit)
		|| !IsClimbableSurface(Hit.ImpactNormal))
	{
		return false;
	}

	WallNormal = Hit.ImpactNormal;
	WallImpact = Hit.ImpactPoint;
	PullAwayTime = 0.f;

	SetMovementMode(MOVE_Custom, static_cast<uint8>(EPHCustomMovement::CMOVE_Climb));
	StopMovementImmediately();
	return true;
}

void UPHCharacterMovementComponent::StopClimb()
{
	if (IsClimbing())
	{
		SetMovementMode(MOVE_Falling);
	}
}

void UPHCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	if (CustomMovementMode == static_cast<uint8>(EPHCustomMovement::CMOVE_Climb))
	{
		PhysClimb(deltaTime, Iterations);
		return;
	}

	Super::PhysCustom(deltaTime, Iterations);
}

void UPHCharacterMovementComponent::PhysClimb(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME || !CharacterOwner)
	{
		return;
	}

	// ── 1) Track the wall (curved/uneven surfaces re-resolve every tick) ──
	FHitResult Hit;
	if (!TraceWall(-WallNormal, WallDetectDistance + WallOffset, Hit)
		|| !IsClimbableSurface(Hit.ImpactNormal))
	{
		// Wall ended above us → hand the ledge to the ALS mantle.
		if (APHBaseCharacter* PHChar = Cast<APHBaseCharacter>(CharacterOwner))
		{
			if (PHChar->TryWallTopMantle())
			{
				SetMovementMode(MOVE_Falling); // mantle takes over from here
				return;
			}
		}
		SetMovementMode(MOVE_Falling);
		return;
	}
	WallNormal = Hit.ImpactNormal;
	WallImpact = Hit.ImpactPoint;

	// ── 2) Wall-plane axes ──
	const FVector WallUp    = FVector::VectorPlaneProject(FVector::UpVector, WallNormal).GetSafeNormal();
	const FVector WallRight = FVector::CrossProduct(WallUp, WallNormal).GetSafeNormal();

	// ── 3) Map player intent (Acceleration is the camera-relative input ALS
	//       already feeds) onto the wall plane: toward-wall = up, away = down/
	//       detach, lateral stays lateral. Genshin mapping. ──
	const FVector InputDir = Acceleration.GetSafeNormal();
	float InputRight = 0.f;
	float InputUp    = 0.f;
	float InputAway  = 0.f;

	if (!InputDir.IsNearlyZero())
	{
		InputRight = FVector::DotProduct(InputDir, WallRight);
		InputAway  = FVector::DotProduct(InputDir, WallNormal);   // + = pulling away
		InputUp    = -FVector::DotProduct(InputDir, WallNormal) > 0.f
			? FMath::Max(0.f, -InputAway)                          // pushing in = climb up
			: 0.f;

		// Explicit down: input opposing WallUp (e.g. stick down with camera level).
		const float DownIntent = FVector::DotProduct(InputDir, -WallUp);
		if (DownIntent > 0.25f)
		{
			InputUp = -DownIntent;
		}
	}

	// Pull-away detach (hold away from wall briefly → drop).
	PullAwayTime = (InputAway > 0.5f) ? PullAwayTime + deltaTime : 0.f;
	if (PullAwayTime >= PullAwayDetachTime)
	{
		SetMovementMode(MOVE_Falling);
		return;
	}

	ClimbPlaneInput = FVector2D(InputRight, InputUp).GetClampedToMaxSize(1.f);

	// ── 4) Velocity: speed comes from the SAME gait pipeline as the ground.
	//       Walk gait = slow climb, Sprint gait = WALL RUN (any direction).
	//       GetMaxSpeed() reads CurrentMovementSettings, which the character
	//       swaps to ClimbingMovementSettings while in the Climbing state. ──
	const float MaxClimbSpeed = GetMaxSpeed();

	FVector DesiredVelocity =
		(WallRight * ClimbPlaneInput.X + WallUp * ClimbPlaneInput.Y) * MaxClimbSpeed;

	// Spring to hold WallOffset from the surface.
	const float OffsetError = FVector::Dist(UpdatedComponent->GetComponentLocation(), WallImpact) - WallOffset;
	DesiredVelocity += -WallNormal * OffsetError * WallSpring * 100.f * deltaTime
		* (1.f / FMath::Max(deltaTime, KINDA_SMALL_NUMBER)); // frame-rate stable push
	DesiredVelocity += -WallNormal * 30.f;                   // constant gentle press

	Velocity = FMath::VInterpTo(Velocity, DesiredVelocity, deltaTime, 10.f);

	// ── 5) Move + face the wall ──
	const FVector Delta = Velocity * deltaTime;
	FHitResult MoveHit;
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, MoveHit);
	if (MoveHit.IsValidBlockingHit())
	{
		SlideAlongSurface(Delta, 1.f - MoveHit.Time, MoveHit.Normal, MoveHit, true);
	}

	const FRotator TargetRot(0.f, (-WallNormal).Rotation().Yaw, 0.f);
	const FRotator NewRot = FMath::RInterpTo(
		UpdatedComponent->GetComponentRotation(), TargetRot, deltaTime, FaceWallInterpSpeed);
	UpdatedComponent->SetWorldRotation(NewRot);
}

void UPHCharacterMovementComponent::DoClimbJump(float Impulse)
{
	if (!IsClimbing())
	{
		return;
	}

	const FVector WallUp    = FVector::VectorPlaneProject(FVector::UpVector, WallNormal).GetSafeNormal();
	const FVector WallRight = FVector::CrossProduct(WallUp, WallNormal).GetSafeNormal();

	FVector Dir = WallRight * ClimbPlaneInput.X + WallUp * ClimbPlaneInput.Y;
	if (Dir.IsNearlyZero())
	{
		Dir = WallUp;
	}

	Velocity = Dir.GetSafeNormal() * Impulse + WallNormal * 60.f;
	// Stay in climb mode: the wall spring recaptures after the hop.
}

bool UPHCharacterMovementComponent::TraceWall(const FVector& Direction, float Distance, FHitResult& OutHit) const
{
	if (!GetWorld() || !CharacterOwner)
	{
		return false;
	}

	const FVector Start = UpdatedComponent->GetComponentLocation();
	return GetWorld()->SweepSingleByChannel(
		OutHit,
		Start,
		Start + Direction.GetSafeNormal() * Distance,
		FQuat::Identity,
		WallChannel,
		FCollisionShape::MakeSphere(WallTraceRadius),
		FCollisionQueryParams(NAME_None, false, CharacterOwner));
}

bool UPHCharacterMovementComponent::IsClimbableSurface(const FVector& Normal) const
{
	return FMath::Abs(FVector::DotProduct(Normal, FVector::UpVector)) <= MaxWallUpDot;
}
```

## Step 3 — `APHBaseCharacter` additions (state + gait + stamina glue)

`PHBaseCharacter.h` — add:

```cpp
	// ═══════════════════════════════════════════════
	// CLIMBING (ALS movement state — wall run = Sprint gait while climbing)
	// ═══════════════════════════════════════════════

	/** Attach to the wall ahead (bind to your climb input). */
	UFUNCTION(BlueprintCallable, Category = "Movement|Climb")
	bool TryStartClimb();

	/** Drop off the wall. */
	UFUNCTION(BlueprintCallable, Category = "Movement|Climb")
	void StopClimb();

	/** Climb-jump (gated by stamina). Bind to jump input while climbing. */
	UFUNCTION(BlueprintCallable, Category = "Movement|Climb")
	void ClimbJump();

	UFUNCTION(BlueprintPure, Category = "Movement|Climb")
	bool IsClimbing() const;

	/** ALS mantle bridge for climb top-outs (MantleCheck is protected on ALS). */
	UFUNCTION(BlueprintCallable, Category = "Movement|Climb")
	bool TryWallTopMantle();

	/**
	 * Speeds for the Climbing state, resolved by gait through the SAME
	 * pipeline as ground movement:
	 *   WalkSpeed  = slow climb     (e.g. 90)
	 *   RunSpeed   = climb          (e.g. 150)
	 *   SprintSpeed= WALL RUN       (e.g. 450+, any direction)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Climb")
	FALSMovementSettings ClimbingMovementSettings;

	/** Stamina drain per second per gait while climbing (Walk/Run/Sprint). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Climb")
	FVector ClimbStaminaPerSecByGait = FVector(2.f, 5.f, 12.f);

	/** Flat climb-jump stamina cost. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Climb")
	float ClimbJumpStaminaCost = 12.f;

	/** Climb-jump impulse. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Climb")
	float ClimbJumpImpulse = 420.f;

	/** Instant GE with SetByCaller key Data.Damage.Stamina (DamageApplicationGE pattern). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Climb")
	TSubclassOf<UGameplayEffect> ClimbStaminaCostGE;

protected:
	/** Maps CMOVE_Climb ↔ EALSMovementState::Climbing. */
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;

	/** While Climbing, feed ClimbingMovementSettings into the gait pipeline. */
	virtual FALSMovementSettings GetTargetMovementSettings() const override;

	/** Sprint is omnidirectional on the wall (ALS's forward-only rule is ground logic). */
	virtual bool CanSprint() const override;

	/** Batched climb stamina drain; detaches at depletion. Called from Tick. */
	void TickClimbStamina(float DeltaTime);

	float ClimbStaminaAccumulator = 0.f;
	float ClimbStaminaTimer = 0.f;
```

`PHBaseCharacter.cpp` — add (and call `TickClimbStamina(DeltaSeconds)` from `Tick`):

```cpp
// Constructor: use the project CMC (chain through the ObjectInitializer —
// AALSBaseCharacter already swaps the CMC class the same way):
APHBaseCharacter::APHBaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPHCharacterMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{ /* existing body unchanged */ }

UPHCharacterMovementComponent* APHBaseCharacter::GetPHMovement() const
{
	return Cast<UPHCharacterMovementComponent>(GetCharacterMovement());
}

bool APHBaseCharacter::IsClimbing() const
{
	const UPHCharacterMovementComponent* Move = GetPHMovement();
	return Move && Move->IsClimbing();
}

bool APHBaseCharacter::TryStartClimb()
{
	if (UPHCharacterMovementComponent* Move = GetPHMovement())
	{
		if (GetHealth() > 0.f && Move->TryStartClimb())
		{
			return true;
		}
	}
	return false;
}

void APHBaseCharacter::StopClimb()
{
	if (UPHCharacterMovementComponent* Move = GetPHMovement())
	{
		Move->StopClimb();
	}
}

void APHBaseCharacter::ClimbJump()
{
	UPHCharacterMovementComponent* Move = GetPHMovement();
	if (!Move || !Move->IsClimbing())
	{
		return;
	}

	// Stamina gate via the existing SetByCaller pattern.
	if (AttributeSet && AttributeSet->GetStamina() <= ClimbJumpStaminaCost)
	{
		return;
	}
	if (ClimbStaminaCostGE && AbilitySystemComponent)
	{
		FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(
			ClimbStaminaCostGE, 1.f, AbilitySystemComponent->MakeEffectContext());
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(
				FPHGameplayTags::Get().Data_Damage_Stamina, -ClimbJumpStaminaCost);
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	Move->DoClimbJump(ClimbJumpImpulse);
}

void APHBaseCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	// Map our custom climb mode onto the ALS movement state BEFORE Super so
	// ALS's own handling never misreads MOVE_Custom.
	if (GetCharacterMovement()->MovementMode == MOVE_Custom
		&& GetCharacterMovement()->CustomMovementMode == static_cast<uint8>(EPHCustomMovement::CMOVE_Climb))
	{
		Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
		SetMovementState(EALSMovementState::Climbing);
		return;
	}

	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
}

FALSMovementSettings APHBaseCharacter::GetTargetMovementSettings() const
{
	if (IsClimbing())
	{
		return ClimbingMovementSettings;
	}
	return Super::GetTargetMovementSettings();
}

bool APHBaseCharacter::CanSprint() const
{
	if (IsClimbing())
	{
		// Wall run: sprint in ANY direction on the wall — ALS's "forward
		// input only" sprint rule is ground logic.
		return AttributeSet && AttributeSet->GetStamina() > 0.f;
	}
	return Super::CanSprint();
}

bool APHBaseCharacter::TryWallTopMantle()
{
	// FallingTraceSettings reach suits grabbing a ledge from a climb.
	// (Fork-name check: some ALSV4_CPP versions expose MantleCheckFalling().)
	return MantleCheck(FallingTraceSettings, EDrawDebugTrace::Type::None);
}

void APHBaseCharacter::TickClimbStamina(float DeltaTime)
{
	if (!IsClimbing() || !AttributeSet)
	{
		return;
	}

	// Per-gait drain: Walk=X (slow climb), Run=Y, Sprint=Z (wall run).
	float DrainPerSec = ClimbStaminaPerSecByGait.X;
	switch (Gait)                       // AALSBaseCharacter's current gait
	{
	case EALSGait::Running:   DrainPerSec = ClimbStaminaPerSecByGait.Y; break;
	case EALSGait::Sprinting: DrainPerSec = ClimbStaminaPerSecByGait.Z; break;
	default: break;
	}

	ClimbStaminaAccumulator += DrainPerSec * DeltaTime;
	ClimbStaminaTimer += DeltaTime;
	if (ClimbStaminaTimer < 0.25f)
	{
		return;
	}
	ClimbStaminaTimer = 0.f;

	if (AttributeSet->GetStamina() - ClimbStaminaAccumulator <= 1.f)
	{
		ClimbStaminaAccumulator = 0.f;
		StopClimb();                    // out of stamina → drop (Genshin rule)
		return;
	}

	if (ClimbStaminaCostGE && AbilitySystemComponent)
	{
		FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(
			ClimbStaminaCostGE, 1.f, AbilitySystemComponent->MakeEffectContext());
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(
				FPHGameplayTags::Get().Data_Damage_Stamina, -ClimbStaminaAccumulator);
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
	ClimbStaminaAccumulator = 0.f;
}
```

**Verify against your ALS copy (fork naming drifts here):**
`GetTargetMovementSettings()` virtual+const, `CanSprint()` virtual,
`MantleCheck(FALSMantleTraceSettings, EDrawDebugTrace::Type)` + `FallingTraceSettings`,
`SetMovementState(EALSMovementState)`, and the `Gait` member. All exist in
mainline ALSV4_CPP; adjust the five call sites if your fork renamed them.

## Step 4 — Input (BP, same routing as everything else)

- **Climb input** (or contextual: jump toward wall) → `TryStartClimb` /
  `StopClimb` toggle.
- **Move input**: unchanged — ALS's `AddMovementInput` already feeds
  `Acceleration`, which `PhysClimb` remaps onto the wall plane. No branch needed.
- **Sprint input**: unchanged — ALS gait handling IS the slow/fast/wall-run
  switch now. Holding sprint on a wall = wall running.
- **Jump while climbing** → `ClimbJump` (branch your jump handler on `IsClimbing`).

## Step 5 — AnimBP

- Add the `Climbing` case to the MovementState switch.
- Inside: one climb blendspace, axes from `GetPHMovement()->GetClimbInput()`
  (X = lateral, Y = vertical). Gait drives play-rate/pose-set selection —
  Sprint gait gets the wall-run lean/intensity. Idle hang at zero input.
- Exit transitions: Climbing → Mantling (ALS mantle fires its own state) and
  Climbing → InAir (detach/stamina-out) already flow from MovementState.

## Phases

1. Enum + CMC + character overrides compile; climb attach/move/detach works
   (no stamina, no anims — capsule test).
2. Gait pipeline check: speeds change with sprint input on the wall; tune
   `ClimbingMovementSettings` (90 / 150 / 450 starting points).
3. Stamina drain + jump costs + mantle top-out.
4. AnimBP state + blendspaces, camera polish (BP).
5. Later: corner wrapping, NoClimb surface tag in `IsClimbableSurface`,
   dedicated-server validation pass (PhysClimb already runs inside the CMC
   pipeline, so saved moves replay it — verify wall re-trace determinism).

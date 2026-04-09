// AI/Mob/MobManagerActor.cpp
#include "AI/Mob/MobManagerActor.h"
#include "AI/Mob/MobPoolSubsystem.h"
#include "AI/Mob/MobWanderInterface.h"
#include "Systems/AI/Components/MonsterModifierComponent.h"
#include "Systems/Stats/Components/StatsManager.h"
#include "Components/BoxComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BrainComponent.h"
#include "AIController.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY(LogMobManager);

AMobManagerActor::AMobManagerActor()
{
	PrimaryActorTick.bCanEverTick = false; // Timer-driven, not tick-driven

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
	SpawnArea->SetupAttachment(SceneRoot);
	SpawnArea->SetBoxExtent(FVector(500.0f, 500.0f, 200.0f));
	SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnArea->SetHiddenInGame(true);
}

void AMobManagerActor::BeginPlay()
{
	Super::BeginPlay();

	// OPT-POOL: Cache the pool subsystem once at startup
	// CQ-2 FIX: Guard GetWorld() against null (editor-only / teardown edge cases).
	if (bUseActorPooling && GetWorld())
	{
		CachedPoolSubsystem = GetWorld()->GetSubsystem<UMobPoolSubsystem>();
	}

	if (bAutoActivate && HasAuthority())
	{
		StartSpawning();
	}
}

void AMobManagerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);

	// FIX #1: Cancel the initial burst timer so the WeakSelf lambda can't
	// fire after this actor has been destroyed (belt-and-suspenders alongside
	// the TWeakObjectPtr guard inside the lambda).
	GetWorldTimerManager().ClearTimer(InitialBurstTimerHandle);

	// Cancel any pending pool-recycle timers so Release() doesn't fire
	// on actors that StopAndClear() may have already destroyed.
	for (FTimerHandle& Handle : PendingRecycleTimers)
	{
		GetWorldTimerManager().ClearTimer(Handle);
	}
	PendingRecycleTimers.Empty();

	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void AMobManagerActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Live-update the SpawnArea box colour so the extent is obvious in editor
	if (SpawnArea)
	{
		SpawnArea->ShapeColor = FColor(0, 200, 255);
	}
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void AMobManagerActor::StartSpawning()
{
	// FIX #5: Guard authority here so a Blueprint call from a client can't
	// start the timer on the wrong machine.  SpawnTick also checks, but this
	// prevents the timer from running at all on clients (saves ticks + noise).
	if (!HasAuthority())
	{
		UE_LOG(LogMobManager, Warning,
			TEXT("[%s] StartSpawning: called without authority — ignoring"), *GetName());
		return;
	}

	if (MobTypes.IsEmpty())
	{
		UE_LOG(LogMobManager, Warning,
			TEXT("[%s] StartSpawning: MobTypes array is empty — nothing to spawn"),
			*GetName());
		return;
	}

	// BUG FIX: Guard against double-calls while the timer is already running.
	// SetTimer() on an active handle restarts it from scratch with the 0.5s
	// initial delay — disrupting the spawn cadence if StartSpawning is called
	// twice (e.g. bAutoActivate fires AND a Blueprint calls it at BeginPlay).
	// Allow the call through if the manager was Paused so it re-arms correctly.
	if (ManagerState == EMobManagerState::Active
		&& GetWorldTimerManager().IsTimerActive(SpawnTimerHandle)
		&& !GetWorldTimerManager().IsTimerPaused(SpawnTimerHandle))
	{
		UE_LOG(LogMobManager, Verbose,
			TEXT("[%s] StartSpawning: already active and running — ignoring duplicate call"),
			*GetName());
		return;
	}

	ManagerState = EMobManagerState::Active;
	bFullEventFired = false;

	GetWorldTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&AMobManagerActor::SpawnTick,
		FMath::Max(0.1f, SpawnInterval),
		true,           // looping
		0.5f);          // first tick 0.5s after start to let the level settle

	UE_LOG(LogMobManager, Log,
		TEXT("[%s] StartSpawning: timer started (interval=%.1fs, max=%d, packs=%s, burst=%s)"),
		*GetName(), SpawnInterval, MaxNumOfMobs,
		bSpawnInPacks ? TEXT("ON") : TEXT("OFF"),
		bInitialBurst ? TEXT("ON") : TEXT("OFF"));

	// Initial burst: immediately fill up to capacity on first activation.
	// Uses a deferred call so the level has one frame to finish loading.
	//
	// FIX #1: Use InitialBurstTimerHandle (member) instead of a local so
	// EndPlay() can cancel it, and capture a TWeakObjectPtr instead of raw
	// 'this' to guard against a UAF if the manager is destroyed in the
	// 0.1s window before the timer fires.
	if (bInitialBurst)
	{
		TWeakObjectPtr<AMobManagerActor> WeakSelf = this;
		GetWorldTimerManager().SetTimer(InitialBurstTimerHandle,
			[WeakSelf]()
			{
				AMobManagerActor* Self = WeakSelf.Get();
				if (!Self || Self->ManagerState != EMobManagerState::Active) { return; }

				Self->CacheTickValues();
				const int32 SlotsToFill = Self->MaxNumOfMobs - Self->GetActiveCount();
				int32 Filled = 0;

				if (Self->bSpawnInPacks)
				{
					while (Filled < SlotsToFill && Self->GetActiveCount() < Self->MaxNumOfMobs)
					{
						const int32 Count = Self->SpawnBatch();
						if (Count == 0) { break; }
						Filled += Count;
					}
				}
				else
				{
					for (int32 i = 0; i < SlotsToFill; ++i)
					{
						if (Self->GetActiveCount() >= Self->MaxNumOfMobs) { break; }
						if (!Self->TrySpawnMob()) { break; }
						++Filled;
					}
				}

				UE_LOG(LogMobManager, Log,
					TEXT("[%s] InitialBurst: spawned %d/%d mobs"),
					*Self->GetName(), Filled, SlotsToFill);
			},
			0.1f, // Small delay to let the level settle
			false);
	}
}

void AMobManagerActor::PauseSpawning()
{
	ManagerState = EMobManagerState::Paused;
	GetWorldTimerManager().PauseTimer(SpawnTimerHandle);
	UE_LOG(LogMobManager, Log, TEXT("[%s] PauseSpawning"), *GetName());
}

void AMobManagerActor::StopAndClear()
{
	ManagerState = EMobManagerState::Disabled;
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);

	// Cancel pending recycle timers BEFORE destroying mobs — otherwise
	// the timer lambda could fire Release() on an already-destroyed actor.
	for (FTimerHandle& Handle : PendingRecycleTimers)
	{
		GetWorldTimerManager().ClearTimer(Handle);
	}
	PendingRecycleTimers.Empty();

	// Return tracked mobs to pool (or destroy if no pool)
	for (const TWeakObjectPtr<APHBaseCharacter>& Weak : ActiveMobs)
	{
		if (Weak.IsValid())
		{
			if (CachedPoolSubsystem)
			{
				CachedPoolSubsystem->Release(Weak.Get());
			}
			else
			{
				Weak->Destroy();
			}
		}
	}
	ActiveMobs.Empty();

	UE_LOG(LogMobManager, Log, TEXT("[%s] StopAndClear: all mobs destroyed"), *GetName());
}

void AMobManagerActor::ForceSpawnOne()
{
	if (!HasAuthority()) { return; }

	// ForceSpawnOne bypasses SpawnTick, so we must cache tick values here too
	CacheTickValues();
	TrySpawnMob();
}

void AMobManagerActor::ForceSpawnBatch()
{
	if (!HasAuthority()) { return; }

	CacheTickValues();

	if (bSpawnInPacks)
	{
		SpawnBatch();
	}
	else
	{
		TrySpawnMob();
	}
}

int32 AMobManagerActor::GetActiveCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<APHBaseCharacter>& Weak : ActiveMobs)
	{
		// Check both pointer validity AND alive state.
		// A mob that died (bIsDead=true) but hasn't been GC'd yet still has
		// a valid weak pointer — without the bIsDead check, the counter
		// stays inflated until CleanActiveMobs happens to run.
		if (Weak.IsValid() && !Weak->bIsDead) { ++Count; }
	}
	return Count;
}

TArray<APHBaseCharacter*> AMobManagerActor::GetActiveMobsArray() const
{
	TArray<APHBaseCharacter*> Result;
	Result.Reserve(ActiveMobs.Num());
	for (const TWeakObjectPtr<APHBaseCharacter>& Weak : ActiveMobs)
	{
		if (Weak.IsValid() && !Weak->bIsDead)
		{
			Result.Add(Weak.Get());
		}
	}
	return Result;
}

void AMobManagerActor::CleanActiveMobs()
{
	// BUG FIX: Also remove dead-but-not-yet-GC'd mobs.  The old code only
	// checked IsValid(), so dead mobs (bIsDead=true) that hadn't been garbage
	// collected yet stayed in the array, making every ActiveMobs iteration
	// (GetActiveCount, GetActiveMobsArray, DrawDebugVisuals, OnMobDeathEvent)
	// slower over time.
	ActiveMobs.RemoveAll([](const TWeakObjectPtr<APHBaseCharacter>& Weak)
	{
		return !Weak.IsValid() || Weak->bIsDead;
	});

	// Also compact stale recycle timer handles (already-fired timers
	// whose handles are no longer valid).
	PendingRecycleTimers.RemoveAll([this](const FTimerHandle& Handle)
	{
		return !GetWorldTimerManager().IsTimerActive(Handle);
	});
}

// ─────────────────────────────────────────────────────────────────────────────
// Spawn pipeline
// ─────────────────────────────────────────────────────────────────────────────

void AMobManagerActor::CacheTickValues()
{
	// OPT: Cache player locations, nav system, and box transform ONCE per tick
	// instead of recomputing them on every spawn attempt.
	CachePlayerLocations();

	MinDistSq = FMath::Square(MinDistanceFromPlayer);
	MaxDistSq = (MaxDistanceFromPlayer > 0.0f) ? FMath::Square(MaxDistanceFromPlayer) : 0.0f;

	// OPT: Cache the nav system pointer — CheckNavMesh was calling
	// FNavigationSystem::GetCurrent on every single attempt.
	CachedNavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	// OPT: Cache box transform + extents — GetRandomSpawnLocation and
	// GetSmartSpawnLocation both called GetScaledBoxExtent/GetComponentTransform
	// per attempt, which involves virtual calls and matrix decomposition.
	if (SpawnArea)
	{
		CachedBoxXform = SpawnArea->GetComponentTransform();
		CachedBoxExtent = SpawnArea->GetScaledBoxExtent();
	}
}

void AMobManagerActor::SpawnTick()
{
	if (ManagerState != EMobManagerState::Active || !HasAuthority())
	{
		return;
	}

	// ── 1. Remove stale pointers ───────────────────────────────────────────
	CleanActiveMobs();

	// ── 2. Population check ────────────────────────────────────────────────
	const int32 Current = GetActiveCount();

	if (Current >= MaxNumOfMobs)
	{
		if (!bFullEventFired)
		{
			bFullEventFired = true;
			OnManagerFull.Broadcast();
			UE_LOG(LogMobManager, Verbose,
				TEXT("[%s] SpawnTick: at capacity (%d/%d)"),
				*GetName(), Current, MaxNumOfMobs);
		}
		return;
	}

	bFullEventFired = false;

	// ── 3. Cache per-tick values ONCE ───────────────────────────────────────
	CacheTickValues();

	// ── 4. Spawn mobs ──────────────────────────────────────────────────────
	if (bSpawnInPacks)
	{
		// Pack mode: spawn a full cluster of mobs around a leader location.
		// MaxSpawnsPerTick caps the number of PACKS this tick, not individual
		// mobs (see header doc).  Use a pack counter — NOT mob count — so that
		// setting MaxSpawnsPerTick = 2 actually yields two separate packs.
		// The old code accumulated BatchCount (mobs) into SpawnedThisTick,
		// which meant any pack >= 2 mobs would instantly exhaust a cap of 1,
		// making MaxSpawnsPerTick > 1 silently ineffective.
		int32 PacksThisTick = 0;
		while (PacksThisTick < MaxSpawnsPerTick && GetActiveCount() < MaxNumOfMobs)
		{
			const int32 BatchCount = SpawnBatch();
			if (BatchCount == 0) { break; }
			++PacksThisTick;
		}
	}
	else
	{
		// Classic mode: one mob per iteration, up to MaxSpawnsPerTick.
		for (int32 i = 0; i < MaxSpawnsPerTick; ++i)
		{
			if (GetActiveCount() >= MaxNumOfMobs) { break; }
			if (!TrySpawnMob()) { break; }
		}
	}

	// ── 5. Optional debug draw ──────────────────────────────────────────────
	if (bShowDebug)
	{
		DrawDebugVisuals();
	}
}

bool AMobManagerActor::TrySpawnMob()
{
	// ── Select a weighted random mob type ──────────────────────────────────
	const int32 TypeIndex = GetWeightedRandomMobTypeIndex();
	if (TypeIndex == INDEX_NONE)
	{
		UE_LOG(LogMobManager, Warning,
			TEXT("[%s] TrySpawnMob: no eligible mob type (all on cooldown or empty)"),
			*GetName());
		OnSpawnFailed.Broadcast(EMobSpawnFailReason::AllOnCooldown);
		BP_OnSpawnFailed(EMobSpawnFailReason::AllOnCooldown);
		return false;
	}

	FMobTypeEntry& ChosenType = MobTypes[TypeIndex];
	const TSubclassOf<APHBaseCharacter> MobClass = ChosenType.MobClass;

	// ── Retry loop ──────────────────────────────────────────────────────────
	int32 DistanceFailCount = 0;

	// OPT-BUDGET: Capture start time so we can bail if the budget is exceeded.
	const double BudgetStartSeconds = FPlatformTime::Seconds();
	const double BudgetLimitSeconds = SpawnBudgetMs > 0.0f
		? (static_cast<double>(SpawnBudgetMs) / 1000.0)
		: 0.0; // 0 = unlimited

	for (int32 Attempt = 0; Attempt < MaxSpawnAttempts; ++Attempt)
	{
		// OPT-BUDGET: Check time budget every few attempts to avoid constant
		// FPlatformTime calls.  Check every 4th attempt (bitmask for speed).
		if (BudgetLimitSeconds > 0.0 && (Attempt & 3) == 3)
		{
			if ((FPlatformTime::Seconds() - BudgetStartSeconds) >= BudgetLimitSeconds)
			{
				UE_LOG(LogMobManager, Verbose,
					TEXT("[%s] TrySpawnMob: budget exhausted (%.1fms) after %d/%d attempts"),
					*GetName(), SpawnBudgetMs, Attempt, MaxSpawnAttempts);
				break;
			}
		}
		// ── 3a. Get a candidate location ────────────────────────────────────
		// Use smart placement first (biases toward the valid ring),
		// fall back to pure random if smart is disabled or fails.
		FVector SpawnLoc = FVector::ZeroVector;
		bool bGotLocation = false;

		if (bUseSmartSpawnPlacement && CachedPlayerLocations.Num() > 0 && MinDistanceFromPlayer > 0.0f)
		{
			bGotLocation = GetSmartSpawnLocation(SpawnLoc);
		}

		if (!bGotLocation)
		{
			if (!GetRandomSpawnLocation(SpawnLoc))
			{
				RecordDebugAttempt(SpawnLoc, false, EMobSpawnFailReason::NavMeshFailed);
				continue;
			}
		}

		// ── 3b. Player distance check ───────────────────────────────────────
		const bool bNeedDistCheck = (MinDistanceFromPlayer > 0.0f || MaxDistanceFromPlayer > 0.0f);
		if (bNeedDistCheck && !CheckDistanceFromPlayers(SpawnLoc))
		{
			++DistanceFailCount;
			RecordDebugAttempt(SpawnLoc, false, EMobSpawnFailReason::DistanceFailed);
			continue;
		}

		// ── 3c. Collision check (without spawning yet) ──────────────────────
		if (bUseCollisionCheck && !CheckCollision(SpawnLoc))
		{
			RecordDebugAttempt(SpawnLoc, false, EMobSpawnFailReason::CollisionFailed);
			continue;
		}

		// ── 3d. Hidden spawn (validation instance) ──────────────────────────
		// BUG FIX: SpawnLoc.Z is the floor / nav-mesh SURFACE (what CheckGround
		// and ProjectPointToNavigation return).  UE's ACharacter places its root
		// component (UCapsuleComponent) CENTER at the actor location — NOT the
		// bottom.  Spawning at the surface therefore puts the capsule center AT
		// floor level, burying the entire lower half of the mesh underground.
		//
		// Fix: lift the Z by CapsuleHalfHeight so the capsule BOTTOM sits on the
		// floor surface.  +2 cm margin prevents micro-penetration on uneven geo.
		//
		// Note: CheckCollision (step 3c) already adds CollisionCapsuleHalfHeight
		// internally, so its validation was always at the correct height; only
		// the final SpawnActor call was using the raw (too-low) floor Z.
		SpawnLoc.Z += CollisionCapsuleHalfHeight + 2.0f;

		const FRotator SpawnRot(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);
		APHBaseCharacter* HiddenMob = HiddenSpawn(MobClass, SpawnLoc, SpawnRot);
		if (!HiddenMob)
		{
			RecordDebugAttempt(SpawnLoc, false, EMobSpawnFailReason::SpawnFailed);
			continue;
		}

		// NOTE: The old code had a redundant second collision check here
		// (step 3e) using CheckCollision(SpawnLoc, HiddenMob).  Since the
		// hidden mob is spawned with collision DISABLED and the capsule
		// dimensions are the same as step 3c, it always produced the same
		// result.  The only scenario it would differ is if the mob's actual
		// collision volume were dramatically different from the configured
		// capsule — but the mob has no collision active at this point, so
		// the IgnoreActor parameter does nothing.  Removing it avoids an
		// expensive OverlapMulti per successful spawn.

		// ── 3e. All checks passed — finalize ───────────────────────────────
		FinalizeSpawn(HiddenMob, SpawnLoc);

		// Update per-type cooldown
		ChosenType.LastSpawnTime = GetWorld()->GetTimeSeconds();

		RecordDebugAttempt(SpawnLoc, true, EMobSpawnFailReason::None);

		UE_LOG(LogMobManager, Log,
			TEXT("[%s] TrySpawnMob: spawned '%s' at %s (attempt %d/%d, active=%d, distFails=%d)"),
			*GetName(),
			*MobClass->GetName(),
			*SpawnLoc.ToString(),
			Attempt + 1,
			MaxSpawnAttempts,
			GetActiveCount(),
			DistanceFailCount);

		return true;
	}

	// All retries exhausted — log the dominant failure reason
	if (DistanceFailCount > MaxSpawnAttempts / 2)
	{
		UE_LOG(LogMobManager, Warning,
			TEXT("[%s] TrySpawnMob: exhausted %d attempts for '%s' — %d/%d were DistanceFailed. "
			     "Consider enlarging SpawnArea, reducing MinDistanceFromPlayer (%.0f), or enabling bUseSmartSpawnPlacement."),
			*GetName(), MaxSpawnAttempts, *MobClass->GetName(),
			DistanceFailCount, MaxSpawnAttempts, MinDistanceFromPlayer);
	}
	else
	{
		UE_LOG(LogMobManager, Warning,
			TEXT("[%s] TrySpawnMob: exhausted %d attempts for class '%s'"),
			*GetName(), MaxSpawnAttempts, *MobClass->GetName());
	}

	OnSpawnFailed.Broadcast(EMobSpawnFailReason::NoValidLocation);
	BP_OnSpawnFailed(EMobSpawnFailReason::NoValidLocation);
	return false;
}

int32 AMobManagerActor::SpawnBatch()
{
	// ── 1. Find a valid leader location using the standard pipeline ─────
	const int32 TypeIndex = GetWeightedRandomMobTypeIndex();
	if (TypeIndex == INDEX_NONE)
	{
		OnSpawnFailed.Broadcast(EMobSpawnFailReason::AllOnCooldown);
		BP_OnSpawnFailed(EMobSpawnFailReason::AllOnCooldown);
		return 0;
	}

	FMobTypeEntry& ChosenType = MobTypes[TypeIndex];
	const TSubclassOf<APHBaseCharacter> MobClass = ChosenType.MobClass;

	// BUG FIX: Apply the same SpawnBudgetMs cap that TrySpawnMob uses.
	// SpawnBatch had no time guard at all — the leader retry loop
	// (MaxSpawnAttempts) plus follower loops (PackSizeMax * PackFollowerAttempts)
	// could run up to 68+ nav/trace queries per call on large sparse areas.
	const double BudgetStartSeconds = FPlatformTime::Seconds();
	const double BudgetLimitSeconds = SpawnBudgetMs > 0.0f
		? (static_cast<double>(SpawnBudgetMs) / 1000.0)
		: 0.0;

	auto IsBudgetExhausted = [&]() -> bool
	{
		return BudgetLimitSeconds > 0.0
			&& (FPlatformTime::Seconds() - BudgetStartSeconds) >= BudgetLimitSeconds;
	};

	// Find the leader spawn location using the standard retry loop
	FVector LeaderLoc = FVector::ZeroVector;
	bool bFoundLeader = false;

	for (int32 Attempt = 0; Attempt < MaxSpawnAttempts; ++Attempt)
	{
		// Check budget every 4th attempt (same cadence as TrySpawnMob)
		if ((Attempt & 3) == 3 && IsBudgetExhausted())
		{
			UE_LOG(LogMobManager, Verbose,
				TEXT("[%s] SpawnBatch: budget exhausted during leader search (attempt %d/%d)"),
				*GetName(), Attempt, MaxSpawnAttempts);
			break;
		}

		FVector SpawnLoc = FVector::ZeroVector;
		bool bGotLocation = false;

		if (bUseSmartSpawnPlacement && CachedPlayerLocations.Num() > 0 && MinDistanceFromPlayer > 0.0f)
		{
			bGotLocation = GetSmartSpawnLocation(SpawnLoc);
		}
		if (!bGotLocation && !GetRandomSpawnLocation(SpawnLoc))
		{
			continue;
		}

		const bool bNeedDistCheck = (MinDistanceFromPlayer > 0.0f || MaxDistanceFromPlayer > 0.0f);
		if (bNeedDistCheck && !CheckDistanceFromPlayers(SpawnLoc))
		{
			continue;
		}

		if (bUseCollisionCheck && !CheckCollision(SpawnLoc))
		{
			continue;
		}

		LeaderLoc = SpawnLoc;
		bFoundLeader = true;
		break;
	}

	if (!bFoundLeader)
	{
		OnSpawnFailed.Broadcast(EMobSpawnFailReason::NoValidLocation);
		BP_OnSpawnFailed(EMobSpawnFailReason::NoValidLocation);
		return 0;
	}

	// ── 2. Determine pack size (clamped to available slots) ─────────────
	// BUG FIX: FMath::RandRange(Min, Max) with Min > Max produces undefined
	// results (negative float math).  Nothing prevents a designer from setting
	// PackSizeMin > PackSizeMax since both are independently clamped to [1,20].
	// Clamp Max to be at least Min before rolling.
	const int32 SlotsAvailable = MaxNumOfMobs - GetActiveCount();
	const int32 ClampedMin = FMath::Max(1, PackSizeMin);
	const int32 ClampedMax = FMath::Max(ClampedMin, PackSizeMax);
	const int32 RolledPackSize = FMath::RandRange(ClampedMin, ClampedMax);
	const int32 PackSize = FMath::Min(RolledPackSize, SlotsAvailable);

	// ── 3. Spawn the leader ─────────────────────────────────────────────
	int32 SpawnCount = 0;

	APHBaseCharacter* Leader = TrySpawnAtLocation(MobClass, LeaderLoc, 0.0f, 1);
	if (Leader)
	{
		++SpawnCount;
		ChosenType.LastSpawnTime = GetWorld()->GetTimeSeconds();
		RecordDebugAttempt(LeaderLoc, true, EMobSpawnFailReason::None);
	}
	else
	{
		RecordDebugAttempt(LeaderLoc, false, EMobSpawnFailReason::SpawnFailed);
		return 0;
	}

	// ── 4. Spawn followers in a cluster around the leader ───────────────
	// BUG FIX: Do NOT re-roll GetWeightedRandomMobTypeIndex() per follower.
	// Re-rolling runs IsEligible() / cooldown checks — which fail immediately
	// when only one mob type is configured (the type is already on cooldown
	// after the leader set LastSpawnTime) or when all types have SpawnCooldown
	// > 0.  Result: every follower returned INDEX_NONE and broke out of the
	// loop, leaving a "pack" of exactly one mob.
	//
	// Fix: followers share the leader's class.  Packs are intentionally
	// homogeneous (a wolf pack, a goblin squad, etc.).  The leader already
	// recorded the cooldown timestamp; no per-follower update is needed.
	for (int32 i = 1; i < PackSize; ++i)
	{
		if (GetActiveCount() >= MaxNumOfMobs) { break; }

		// Bail out of follower placement if we've already burned the budget
		if (IsBudgetExhausted())
		{
			UE_LOG(LogMobManager, Verbose,
				TEXT("[%s] SpawnBatch: budget exhausted during follower placement (%d/%d placed)"),
				*GetName(), SpawnCount, PackSize);
			break;
		}

		APHBaseCharacter* Follower = TrySpawnAtLocation(
			MobClass, LeaderLoc, PackSpreadRadius, PackFollowerAttempts);

		if (Follower)
		{
			++SpawnCount;
			// LastSpawnTime was already set when the leader spawned above;
			// updating it here would extend the cooldown by sub-milliseconds
			// with no practical benefit.
		}
	}

	UE_LOG(LogMobManager, Log,
		TEXT("[%s] SpawnBatch: spawned pack of %d/%d '%s' near %s (active=%d)"),
		*GetName(), SpawnCount, PackSize, *MobClass->GetName(),
		*LeaderLoc.ToString(), GetActiveCount());

	return SpawnCount;
}

APHBaseCharacter* AMobManagerActor::TrySpawnAtLocation(
	TSubclassOf<APHBaseCharacter> MobClass,
	const FVector& Center,
	float JitterRadius,
	int32 Attempts)
{
	for (int32 i = 0; i < Attempts; ++i)
	{
		FVector SpawnLoc = Center;

		// Apply random jitter for followers (JitterRadius > 0)
		if (JitterRadius > 0.0f)
		{
			const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
			const float Dist  = FMath::FRandRange(0.0f, JitterRadius);
			SpawnLoc.X += Dist * FMath::Cos(Angle);
			SpawnLoc.Y += Dist * FMath::Sin(Angle);

			// BUG FIX: Jitter can push followers outside the spawn box.
			// A leader near the box edge + PackSpreadRadius (default 400 u)
			// routinely lands followers completely outside the boundary.
			// Outside the box there is usually no NavMesh and no valid ground,
			// so every follower attempt fails → the "batch" is just the leader.
			// Reject jittered positions in local space before running the
			// expensive nav/collision checks.
			{
				const FVector LocalJittered =
					CachedBoxXform.InverseTransformPosition(SpawnLoc);
				if (FMath::Abs(LocalJittered.X) > CachedBoxExtent.X ||
				    FMath::Abs(LocalJittered.Y) > CachedBoxExtent.Y)
				{
					continue;
				}
			}

			// Re-validate the jittered position — same suite of checks as the
			// leader search, including player distance.  The old code skipped
			// CheckDistanceFromPlayers here, meaning followers could land right
			// next to the player if the jitter happened to push them inside
			// MinDistanceFromPlayer.
			if (bUseNavCheck)
			{
				FVector Projected;
				if (!CheckNavMesh(SpawnLoc, Projected)) { continue; }
				SpawnLoc = Projected;
			}

			if (bUseGroundCheck)
			{
				FVector GroundLoc;
				if (!CheckGround(SpawnLoc, GroundLoc)) { continue; }
				SpawnLoc = GroundLoc;
			}

			if (bUseCollisionCheck && !CheckCollision(SpawnLoc))
			{
				continue;
			}

			// BUG FIX: followers must also respect player min/max distance.
			const bool bNeedDistCheck =
				(MinDistanceFromPlayer > 0.0f || MaxDistanceFromPlayer > 0.0f);
			if (bNeedDistCheck && !CheckDistanceFromPlayers(SpawnLoc))
			{
				continue;
			}
		}

		// BUG FIX: same capsule-center lift as the SpawnTick path.
		// SpawnLoc.Z is the floor / nav-mesh surface after CheckGround /
		// CheckNavMesh.  ACharacter roots at capsule CENTER, so we must lift
		// by HalfHeight to avoid burying the lower half of the mesh.
		SpawnLoc.Z += CollisionCapsuleHalfHeight + 2.0f;

		const FRotator SpawnRot(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);
		APHBaseCharacter* Mob = HiddenSpawn(MobClass, SpawnLoc, SpawnRot);
		if (!Mob) { continue; }

		FinalizeSpawn(Mob, SpawnLoc);
		RecordDebugAttempt(SpawnLoc, true, EMobSpawnFailReason::None);
		return Mob;
	}

	return nullptr;
}

bool AMobManagerActor::GetRandomSpawnLocation(FVector& OutLocation)
{
	// ── Random point in box extents (using per-tick cached values) ─────────
	const FVector& BoxExtent  = CachedBoxExtent;
	const FTransform& BoxXform = CachedBoxXform;

	// Generate in LOCAL space so the point respects box rotation,
	// then transform to world.  The old code used world-aligned offsets
	// which ignored rotation entirely.
	const FVector LocalOffset(
		FMath::RandRange(-BoxExtent.X, BoxExtent.X),
		FMath::RandRange(-BoxExtent.Y, BoxExtent.Y),
		FMath::RandRange(-BoxExtent.Z, BoxExtent.Z));

	FVector Candidate = BoxXform.TransformPosition(LocalOffset);

	// ── NavMesh projection ─────────────────────────────────────────────────
	if (bUseNavCheck)
	{
		FVector Projected;
		if (!CheckNavMesh(Candidate, Projected))
		{
			return false;
		}

		// BUG FIX: ProjectPointToNavigation snaps to the NEAREST nav point,
		// which can be outside the spawn box when the original candidate was
		// near the edge and the closest nav surface is beyond the boundary.
		// Without this check, every spawn attempt after a bad projection starts
		// from a world position outside the box — exactly the "spawning outside
		// the spawn area" symptom the user reported.
		// Validate in local box space; reject if XY left the box after projection.
		const FVector LocalProjected = BoxXform.InverseTransformPosition(Projected);
		if (FMath::Abs(LocalProjected.X) > BoxExtent.X ||
		    FMath::Abs(LocalProjected.Y) > BoxExtent.Y)
		{
			return false;
		}

		Candidate = Projected;
	}

	// ── Ground trace ───────────────────────────────────────────────────────
	if (bUseGroundCheck)
	{
		FVector GroundLoc;
		if (!CheckGround(Candidate, GroundLoc))
		{
			return false;
		}
		Candidate = GroundLoc;
	}

	OutLocation = Candidate;
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Validation helpers
// ─────────────────────────────────────────────────────────────────────────────

bool AMobManagerActor::CheckNavMesh(FVector Candidate, FVector& OutProjected) const
{
	// OPT: Use per-tick cached nav system pointer instead of calling
	// FNavigationSystem::GetCurrent on every single spawn attempt.
	UNavigationSystemV1* NavSys = CachedNavSystem.Get();
	if (!NavSys)
	{
		// No nav system — treat as success so maps without nav still work
		OutProjected = Candidate;
		return true;
	}

	FNavLocation NavLoc;

	// BUG FIX: The old code used NavProjectionExtent for all three axes (XYZ).
	// The NavMesh sits on the ground surface, so when the SpawnArea box extends
	// 200+ units above the floor, the candidate point's Z can easily exceed
	// the 200-unit vertical search range and the projection silently fails,
	// killing every spawn attempt with NavMeshFailed.
	//
	// Use the (typically much larger) NavProjectionVerticalExtent for the Z
	// axis so the search can reach down to the actual nav surface.
	const FVector Extent(NavProjectionExtent, NavProjectionExtent, NavProjectionVerticalExtent);

	if (NavSys->ProjectPointToNavigation(Candidate, NavLoc, Extent))
	{
		OutProjected = NavLoc.Location;
		return true;
	}

	return false;
}

bool AMobManagerActor::CheckCollision(FVector Location, AActor* IgnoreActor) const
{
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (IgnoreActor) { Params.AddIgnoredActor(IgnoreActor); }

	// Sweep shape matching a character capsule
	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(
		CollisionCapsuleRadius, CollisionCapsuleHalfHeight);

	// Object types to collide with: world + pawns (other mobs)
	FCollisionObjectQueryParams ObjQuery;
	ObjQuery.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjQuery.AddObjectTypesToQuery(ECC_Pawn);

	// CheckCollision always receives the raw floor-surface Z (the output of
	// CheckGround / NavMesh projection, which is floor + ~1 cm).  The capsule
	// center for the character will be at Location + CapsuleHalfHeight, so we
	// offset the test center accordingly.  A small GroundClearance (3 cm)
	// lifts the test capsule just off the floor mesh so WorldStatic overlaps
	// from the floor surface itself don't produce false positives.
	// (Previously 5 cm; reduced to 3 cm so the test volume matches the actual
	// spawn volume — see the caller's "+HalfHeight+2" lift — more closely.)
	static constexpr float GroundClearance = 3.0f;
	const FVector TestCenter = Location +
		FVector(0.0f, 0.0f, CollisionCapsuleHalfHeight + GroundClearance);

	// OPT: Use OverlapAnyTestByObjectType instead of OverlapMultiByObjectType.
	// We only care whether ANY overlap exists, not the full list of overlapping
	// actors.  OverlapAny early-outs on the first hit and skips the TArray
	// allocation, which adds up across MaxSpawnAttempts per tick.
	return !GetWorld()->OverlapAnyTestByObjectType(
		TestCenter, FQuat::Identity, ObjQuery, Capsule, Params);
}

bool AMobManagerActor::CheckDistanceFromPlayers(FVector Location) const
{
	// Use pre-cached player locations (gathered once per SpawnTick)
	// instead of the expensive GetAllActorsOfClass(APawn) every attempt.

	if (CachedPlayerLocations.IsEmpty())
	{
		// No players found — allow spawn (server with no players connected yet, etc.)
		return true;
	}

	// BUG FIX: The old code returned false if ANY player was beyond MaxDistance.
	// With 2+ players on opposite sides of the map, nearly every spawn point
	// exceeded MaxDistance from at least one player, silently killing all spawns.
	//
	// Correct logic:
	//   - Fail if too CLOSE to ANY player (min distance is absolute safety).
	//   - Fail if too FAR from ALL players (at least one player must be in range).
	bool bAnyPlayerInMaxRange = (MaxDistSq <= 0.0f); // disabled = always in range

	for (const FVector& PlayerLoc : CachedPlayerLocations)
	{
		const float DistSq = FVector::DistSquared(Location, PlayerLoc);

		// Too close to ANY player? Reject immediately.
		if (MinDistSq > 0.0f && DistSq < MinDistSq)
		{
			return false;
		}

		// Track whether at least one player is within max range.
		if (MaxDistSq > 0.0f && DistSq <= MaxDistSq)
		{
			bAnyPlayerInMaxRange = true;
		}
	}

	return bAnyPlayerInMaxRange;
}

void AMobManagerActor::CachePlayerLocations()
{
	CachedPlayerLocations.Reset();

	// Use the player controller iterator — much cheaper than
	// GetAllActorsOfClass(APawn) which returns every mob/NPC too.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) { continue; }

		APawn* Pawn = PC->GetPawn();
		if (!Pawn) { continue; }

		CachedPlayerLocations.Add(Pawn->GetActorLocation());
	}
}

bool AMobManagerActor::GetSmartSpawnLocation(FVector& OutLocation)
{
	// ── Strategy: pick a random player, then generate a candidate point ──
	// in the valid ring (MinDist..MaxDist) around them, clamped to the
	// spawn box. This dramatically reduces distance-check failures.

	if (CachedPlayerLocations.IsEmpty()) { return false; }

	// OPT: Use per-tick cached box values
	const FVector& BoxExtent = CachedBoxExtent;
	const FTransform& BoxXform = CachedBoxXform;
	const FVector BoxOrigin = BoxXform.GetLocation();

	// Pick a random player to bias toward
	const FVector& PlayerLoc =
		CachedPlayerLocations[FMath::RandRange(0, CachedPlayerLocations.Num() - 1)];

	// Determine the ring radii
	const float InnerR = FMath::Max(MinDistanceFromPlayer, 1.0f);
	const float OuterR = (MaxDistanceFromPlayer > InnerR) ? MaxDistanceFromPlayer
	                     : InnerR + BoxExtent.Size2D(); // If no max set, use box diagonal

	// Generate a random point in the annulus (ring) on XY plane
	const float Angle   = FMath::FRandRange(0.0f, 2.0f * PI);
	// Uniform distribution in an annulus: r = sqrt(lerp(r1^2, r2^2, rand))
	const float RandT   = FMath::FRand();
	const float Radius  = FMath::Sqrt(FMath::Lerp(InnerR * InnerR, OuterR * OuterR, RandT));

	FVector Candidate;
	Candidate.X = PlayerLoc.X + Radius * FMath::Cos(Angle);
	Candidate.Y = PlayerLoc.Y + Radius * FMath::Sin(Angle);
	Candidate.Z = BoxOrigin.Z; // Use box center Z, ground trace will fix it

	// ── Clamp to the spawn box (rotation-aware) ─────────────────────────
	// Transform the world-space candidate into the box's LOCAL space,
	// clamp against the local extents, then transform back.  The old code
	// used world-axis-aligned min/max which broke on any rotated SpawnArea.
	FVector LocalCandidate = BoxXform.InverseTransformPosition(Candidate);
	LocalCandidate.X = FMath::Clamp(LocalCandidate.X, -BoxExtent.X, BoxExtent.X);
	LocalCandidate.Y = FMath::Clamp(LocalCandidate.Y, -BoxExtent.Y, BoxExtent.Y);
	LocalCandidate.Z = FMath::Clamp(LocalCandidate.Z, -BoxExtent.Z, BoxExtent.Z);
	Candidate = BoxXform.TransformPosition(LocalCandidate);

	// ── NavMesh projection ─────────────────────────────────────────────────
	if (bUseNavCheck)
	{
		FVector Projected;
		if (!CheckNavMesh(Candidate, Projected))
		{
			return false;
		}

		// BUG FIX: The clamp above keeps the candidate inside the box before
		// projection, but ProjectPointToNavigation can still snap to a nav
		// surface that sits just outside the box boundary.  Re-validate in
		// local space after projection so we never return an out-of-bounds point.
		const FVector LocalProjected = BoxXform.InverseTransformPosition(Projected);
		if (FMath::Abs(LocalProjected.X) > BoxExtent.X ||
		    FMath::Abs(LocalProjected.Y) > BoxExtent.Y)
		{
			return false;
		}

		Candidate = Projected;
	}

	// ── Ground trace ───────────────────────────────────────────────────────
	if (bUseGroundCheck)
	{
		FVector GroundLoc;
		if (!CheckGround(Candidate, GroundLoc))
		{
			return false;
		}
		Candidate = GroundLoc;
	}

	OutLocation = Candidate;
	return true;
}

bool AMobManagerActor::CheckGround(FVector Candidate, FVector& OutGroundLocation) const
{
	const FVector TraceStart = Candidate + FVector(0.0f, 0.0f, 100.0f);
	const FVector TraceEnd   = Candidate - FVector(0.0f, 0.0f, GroundTraceDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// CQ-2 FIX: Null-guard GetWorld() to prevent crash during teardown.
	UWorld* World = GetWorld();
	if (World && World->LineTraceSingleByChannel(
			Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		// AI-3 FIX: Nudge +1 cm above the hit surface so the spawned capsule
		// doesn't start embedded in the ground and trigger a physics depenetration
		// that can launch the mob into the air on the first frame.
		OutGroundLocation = Hit.ImpactPoint + FVector(0.0f, 0.0f, 1.0f);
		return true;
	}

	return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Spawn lifecycle
// ─────────────────────────────────────────────────────────────────────────────

APHBaseCharacter* AMobManagerActor::HiddenSpawn(
	TSubclassOf<APHBaseCharacter> Class,
	const FVector& Location,
	const FRotator& Rotation) const
{
	if (!Class) { return nullptr; }

	// OPT-POOL: Try acquiring from the pool first — avoids SpawnActor overhead.
	// The pool returns the actor hidden with collision/AI disabled, same as
	// the manual spawn path below.
	if (CachedPoolSubsystem)
	{
		APHBaseCharacter* Mob = CachedPoolSubsystem->Acquire(Class, Location, Rotation);
		if (Mob)
		{
			Mob->SetOwner(const_cast<AMobManagerActor*>(this));
			return Mob;
		}
		// Pool returned nullptr (bad class or world shutting down) — fall through
	}

	// ── Fallback: raw SpawnActor (pool disabled or failed) ───────────────
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner  = const_cast<AMobManagerActor*>(this);
	Params.Instigator = nullptr;

	// CQ-2 FIX: Null-guard GetWorld() before spawning.
	UWorld* World = GetWorld();
	if (!World) { return nullptr; }

	APHBaseCharacter* Mob =
		World->SpawnActor<APHBaseCharacter>(Class, Location, Rotation, Params);

	if (!Mob) { return nullptr; }

	// ── Make invisible and inert for the validation phase ─────────────────
	Mob->SetActorHiddenInGame(true);
	Mob->SetActorEnableCollision(false);

	// Pause any AI controller that may have auto-started
	if (AAIController* AIC = Cast<AAIController>(Mob->GetController()))
	{
		if (UBrainComponent* Brain = AIC->GetBrainComponent())
		{
			Brain->PauseLogic(TEXT("MobManagerValidation"));
		}
	}

	// Stop movement so the hidden actor doesn't drift
	if (UCharacterMovementComponent* Move = Mob->GetCharacterMovement())
	{
		Move->DisableMovement();
	}

	return Mob;
}

void AMobManagerActor::FinalizeSpawn(APHBaseCharacter* Mob, const FVector& SpawnLocation)
{
	if (!Mob) { return; }

	// ────────────────────────────────────────────────────────────────────────
	// CRITICAL ORDER: Track the mob and bind the death delegate BEFORE making
	// it visible/collidable.  Once collision is re-enabled, the mob can
	// immediately take damage and die (e.g. overlapping a damage volume).
	// If the delegate or tracking is set up AFTER collision, the death fires
	// before we're listening → the dead mob stays in ActiveMobs forever,
	// inflating GetActiveCount() and eventually capping the spawner.
	// ────────────────────────────────────────────────────────────────────────

	// ── 1. Track first — so OnMobDeathEvent can find and remove it ────────
	ActiveMobs.Add(TWeakObjectPtr<APHBaseCharacter>(Mob));

	// ── 2. Bind death delegate — so we hear about it immediately ──────────
	Mob->OnDeathEvent.AddDynamic(this, &AMobManagerActor::OnMobDeathEvent);

	// ── 3. Configure data while the mob is still inert ────────────────────
	if (Mob->Implements<UMobWanderable>())
	{
		IMobWanderable::Execute_SetHomeLocation(Mob, SpawnLocation);
		IMobWanderable::Execute_SetWanderRadius(Mob, WanderRadius);
		IMobWanderable::Execute_OnSpawnedByManager(Mob, this);
	}

	ApplyModifierComponent(Mob);

	// ── 3b. Dev-build sanity: warn if stats were never initialized ────────
	// Fresh spawns initialize stats during BeginPlay (PHBaseCharacter::
	// InitializeAbilitySystem → StatsManager::NotifyAbilitySystemReady).
	// Pool-recycled mobs re-initialize inside PrepareMobForReuse.
	// If neither ran, the mob will have MaxHealth=0 and fight at 0 HP.
	// Root cause is almost always a missing StatsData asset on the Blueprint's
	// StatsManager component — configure it in the actor's Details panel.
#if !UE_BUILD_SHIPPING
	if (UStatsManager* Stats = Mob->FindComponentByClass<UStatsManager>())
	{
		if (!Stats->HasInitializedStats())
		{
			UE_LOG(LogMobManager, Warning,
				TEXT("FinalizeSpawn: '%s' (class=%s) has a StatsManager but stats "
				     "were NOT initialized.  Assign a StatsData asset to the "
				     "StatsManager component in the mob's Blueprint, or the mob "
				     "will spawn with MaxHealth=0."),
				*Mob->GetName(), *GetNameSafe(Mob->GetClass()));
		}
	}
#endif

	// ── 4. NOW make the mob live — visible, collidable, moving, thinking ──
	Mob->SetActorHiddenInGame(false);
	Mob->SetActorEnableCollision(true);

	if (UCharacterMovementComponent* Move = Mob->GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Walking);
	}

	if (AAIController* AIC = Cast<AAIController>(Mob->GetController()))
	{
		if (UBrainComponent* Brain = AIC->GetBrainComponent())
		{
			Brain->ResumeLogic(TEXT("MobManagerFinalized"));
		}
	}

	// ── 5. Notify ─────────────────────────────────────────────────────────
	OnMobSpawned.Broadcast(Mob);
	BP_OnMobSpawned(Mob);
}

void AMobManagerActor::ApplyModifierComponent(APHBaseCharacter* Mob) const
{
	if (!Mob) { return; }

	UMonsterModifierComponent* ModComp =
		Mob->FindComponentByClass<UMonsterModifierComponent>();
	if (!ModComp) { return; }

	// BUG FIX: RollAndApplyMods is idempotent — it has a bModsApplied guard
	// that returns early after the first call.  Since HiddenSpawn triggers
	// BeginPlay, which calls RollAndApplyMods automatically, by the time we
	// get here the mods are already locked with the WRONG AreaLevel (whatever
	// default the component had).  Our AreaLevel/MagicFind assignments were
	// dead code — RollAndApplyMods would just return immediately.
	//
	// Fix: Set the correct values, then call RerollMods which properly cleans
	// up the old GEs, resets the idempotency guard, and re-rolls with our
	// manager's AreaLevel.
	ModComp->AreaLevel = AreaLevel;
	ModComp->NearbyPlayerMagicFind = NearbyMagicFind;
	ModComp->RerollMods();
}

// ─────────────────────────────────────────────────────────────────────────────
// Death tracking
// ─────────────────────────────────────────────────────────────────────────────

void AMobManagerActor::OnMobDeathEvent(APHBaseCharacter* DeadMob, AActor* Killer)
{
	if (!DeadMob) { return; }

	// Remove from active list (by finding the matching weak pointer).
	// BUG FIX: Old predicate was (IsValid() && Get() == DeadMob).
	// If some other system calls Destroy() on the mob in the same frame as
	// the death delegate fires (e.g. a GE that destroys the actor during
	// EndAbility), GC can invalidate the weak pointer before this RemoveAll
	// runs.  IsValid() then returns false, short-circuiting the match, and
	// the dead entry is never removed — inflating ActiveMobs permanently.
	// Fix: treat an invalid pointer OR a pointer that matches DeadMob as a
	// hit.  This also opportunistically cleans up any other stale entries in
	// the same pass (same as CleanActiveMobs does anyway).
	ActiveMobs.RemoveAll([DeadMob](const TWeakObjectPtr<APHBaseCharacter>& Weak)
	{
		return !Weak.IsValid() || Weak.Get() == DeadMob;
	});

	OnMobDied.Broadcast(DeadMob);
	BP_OnMobDied(DeadMob);

	bFullEventFired = false; // A slot opened — allow full-event to fire again next cycle

	UE_LOG(LogMobManager, Verbose,
		TEXT("[%s] OnMobDeathEvent: '%s' died (active=%d)"),
		*GetName(), *DeadMob->GetName(), GetActiveCount());

	// OPT-POOL: Return the dead mob to the pool for recycling.
	// Use a short timer so death animations / loot drops / effects can finish
	// before the actor is hidden and deactivated.
	if (CachedPoolSubsystem)
	{
		FTimerHandle RecycleTimer;
		TWeakObjectPtr<APHBaseCharacter> WeakMob = DeadMob;
		TWeakObjectPtr<UMobPoolSubsystem> WeakPool = CachedPoolSubsystem;

		// BUG FIX: Store the handle so StopAndClear()/EndPlay() can cancel it.
		// The old code used a local FTimerHandle that went out of scope
		// immediately, making the timer uncancellable.
		GetWorldTimerManager().SetTimer(RecycleTimer,
			[WeakMob, WeakPool]()
			{
				if (WeakMob.IsValid() && WeakPool.IsValid())
				{
					WeakPool->Release(WeakMob.Get());
				}
			},
			FMath::Max(0.1f, PoolRecycleDelay),
			false);

		PendingRecycleTimers.Add(RecycleTimer);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Weighted random selection
// ─────────────────────────────────────────────────────────────────────────────

int32 AMobManagerActor::GetWeightedRandomMobTypeIndex() const
{
	// FIX #7: GetWorld() can be null during teardown or in certain editor
	// contexts.  Every other GetWorld() call in this file is guarded — this
	// one was missed.  Return INDEX_NONE (no eligible type) on null world.
	UWorld* World = GetWorld();
	if (!World) { return INDEX_NONE; }
	const float Now = World->GetTimeSeconds();

	// ── Build eligible list with cumulative weights ────────────────────────
	int32 TotalWeight = 0;
	// OPT: Use TInlineAllocator to keep the array on the stack for typical
	// mob type counts (<=16).  Avoids a heap allocation every spawn attempt.
	TArray<int32, TInlineAllocator<16>> EligibleIndices;

	for (int32 i = 0; i < MobTypes.Num(); ++i)
	{
		if (MobTypes[i].IsEligible(Now))
		{
			EligibleIndices.Add(i);
			TotalWeight += MobTypes[i].SpawnWeight;
		}
	}

	if (EligibleIndices.IsEmpty() || TotalWeight <= 0)
	{
		return INDEX_NONE;
	}

	// ── Roll in [0, TotalWeight) and walk through ──────────────────────────
	int32 Roll = FMath::RandRange(0, TotalWeight - 1);

	for (int32 Idx : EligibleIndices)
	{
		Roll -= MobTypes[Idx].SpawnWeight;
		if (Roll < 0)
		{
			return Idx;
		}
	}

	// NOTE: This line is unreachable with integer arithmetic — the loop above
	// always finds an entry because subtracting all weights from a Roll in
	// [0, TotalWeight-1] is guaranteed to go negative before the loop ends.
	// The old comment ("Floating-point safety fallback") was a copy from an
	// earlier float-based implementation and no longer applies.
	// Kept as a defensive fallback only; it should never fire in practice.
	return EligibleIndices.Last();
}

// ─────────────────────────────────────────────────────────────────────────────
// Debug
// ─────────────────────────────────────────────────────────────────────────────

void AMobManagerActor::RecordDebugAttempt(
	const FVector& Location, bool bSuccess, EMobSpawnFailReason Reason)
{
	if (!bShowDebug) { return; }

	FSpawnAttemptDebug Entry;
	Entry.Location  = Location;
	Entry.bSuccess  = bSuccess;
	Entry.FailReason = Reason;
	// BUG FIX: Every other GetWorld() call in this file is null-guarded.
	// This one was missed — GetWorld() can be null during teardown (e.g. PIE
	// stop with bShowDebug=true), which would crash here.
	UWorld* World = GetWorld();
	Entry.Timestamp = World ? World->GetTimeSeconds() : 0.0f;

	// Overwrite the oldest entry once history is full (O(1) vs the old
	// RemoveAt(0) which was O(n) per insertion — compounding to O(n²)
	// over time as the buffer stayed full).
	if (DebugHistory.Num() >= MaxDebugHistory)
	{
		DebugHistory[DebugHistoryIndex] = Entry;
		DebugHistoryIndex = (DebugHistoryIndex + 1) % MaxDebugHistory;
	}
	else
	{
		DebugHistory.Add(Entry);
	}

#if ENABLE_DRAW_DEBUG
	const FColor SphereColor = bSuccess ? FColor::Green : FColor::Red;
	DrawDebugSphere(GetWorld(), Location, 40.0f, 8, SphereColor,
		false, DebugDrawDuration, 0, 2.0f);

	if (!bSuccess)
	{
		// Label the failure reason
		DrawDebugString(GetWorld(), Location + FVector(0, 0, 60),
			UEnum::GetValueAsString(Reason), nullptr,
			SphereColor, DebugDrawDuration, false, 1.0f);
	}
#endif
}

void AMobManagerActor::DrawDebugVisuals() const
{
#if ENABLE_DRAW_DEBUG
	// All debug shapes and text use SpawnInterval as their lifetime so they:
	//   • Remain visible for the full duration between spawn ticks (not just
	//     one frame, which is what LifeTime=-1.0f gives for shapes).
	//   • Auto-expire just before the next tick fires, preventing stale
	//     DrawDebugString labels from accumulating at the same world position
	//     (DrawDebugString with LifeTime=-1.0f persists indefinitely in UE,
	//     causing one new overlapping label to be added every SpawnTick).
	//
	// Clamp to a sane upper bound so the shapes disappear promptly if the
	// manager is paused or destroyed while debug is active.
	const float DebugLifetime = FMath::Clamp(SpawnInterval, 0.1f, 30.0f);

	// ── Box outline ───────────────────────────────────────────────────────
	DrawDebugBox(
		GetWorld(),
		SpawnArea->GetComponentLocation(),
		SpawnArea->GetScaledBoxExtent(),
		SpawnArea->GetComponentQuat(),
		FColor(0, 180, 255),
		false, DebugLifetime, 0, 3.0f);

	// ── Active mob home locations (green pillar lines) ────────────────────
	for (const TWeakObjectPtr<APHBaseCharacter>& Weak : ActiveMobs)
	{
		// Also skip dead-but-not-yet-GC'd mobs, consistent with
		// GetActiveCount() and GetActiveMobsArray().  The old code only
		// checked IsValid(), so dead mobs showed debug spheres as if alive,
		// creating a mismatch between the visual count and the HUD counter.
		if (!Weak.IsValid() || Weak->bIsDead) { continue; }

		const FVector MobLoc = Weak->GetActorLocation();
		DrawDebugSphere(GetWorld(), MobLoc, 30.0f, 6,
			FColor::Cyan, false, DebugLifetime, 0, 1.5f);

		// Wander radius ring
		DrawDebugCircle(GetWorld(), MobLoc, WanderRadius,
			32, FColor(0, 80, 255),
			false, DebugLifetime, 0, 1.5f,
			FVector(1, 0, 0), FVector(0, 1, 0), false);
	}

	// ── HUD text ──────────────────────────────────────────────────────────
	const FVector LabelPos =
		SpawnArea->GetComponentLocation() +
		FVector(0, 0, SpawnArea->GetScaledBoxExtent().Z + 30.0f);

	DrawDebugString(GetWorld(), LabelPos,
		FString::Printf(TEXT("%s\n%d / %d mobs"),
			*GetName(), GetActiveCount(), MaxNumOfMobs),
		nullptr, FColor::White, DebugLifetime, false, 1.2f);
#endif
}

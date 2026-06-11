#include "AI/Mob/MobManagerActor.h"
#include "AI/Mob/MobPoolSubsystem.h"
#include "AI/Mob/MobWanderInterface.h"
#include "AI/Mob/MobSpawnConditionEvaluator.h"
#include "AI/ALSAIController.h"
#include "AI/Components/MonsterModifierComponent.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Stats/Components/StatsManager.h"
#include "Components/BoxComponent.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "BrainComponent.h"
#include "AIController.h"
#include "Character/PHBaseCharacter.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY(LogMobManager);

AMobManagerActor::AMobManagerActor()
{
	PrimaryActorTick.bCanEverTick = false;

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


	GetWorldTimerManager().ClearTimer(InitialBurstTimerHandle);


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

	if (SpawnArea)
	{
		SpawnArea->ShapeColor = FColor(0, 200, 255);
	}
}
#endif

void AMobManagerActor::StartSpawning()
{

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
		true,
		0.5f);

	UE_LOG(LogMobManager, Log,
		TEXT("[%s] StartSpawning: timer started (interval=%.1fs, max=%d, packs=%s, burst=%s)"),
		*GetName(), SpawnInterval, MaxNumOfMobs,
		bSpawnInPacks ? TEXT("ON") : TEXT("OFF"),
		bInitialBurst ? TEXT("ON") : TEXT("OFF"));


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
			0.1f,
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

	for (FTimerHandle& Handle : PendingRecycleTimers)
	{
		GetWorldTimerManager().ClearTimer(Handle);
	}
	PendingRecycleTimers.Empty();

	for (const TWeakObjectPtr<APHBaseCharacter>& Weak : ActiveMobs)
	{
		if (Weak.IsValid())
		{
			Weak->OnDeath.RemoveDynamic(this, &AMobManagerActor::OnMobDeathEvent);

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

		if (Weak.IsValid()) { ++Count; }
	}
	return Count;
}

TArray<APHBaseCharacter*> AMobManagerActor::GetActiveMobsArray() const
{
	TArray<APHBaseCharacter*> Result;
	Result.Reserve(ActiveMobs.Num());
	for (const TWeakObjectPtr<APHBaseCharacter>& Weak : ActiveMobs)
	{
		if (Weak.IsValid())
		{
			Result.Add(Weak.Get());
		}
	}
	return Result;
}

void AMobManagerActor::CleanActiveMobs()
{

	ActiveMobs.RemoveAll([](const TWeakObjectPtr<APHBaseCharacter>& Weak)
	{
		return !Weak.IsValid();
	});

	PendingRecycleTimers.RemoveAll([this](const FTimerHandle& Handle)
	{
		return !GetWorldTimerManager().IsTimerActive(Handle);
	});
}

void AMobManagerActor::CacheTickValues()
{

	CachePlayerLocations();

	MinDistSq = FMath::Square(MinDistanceFromPlayer);
	MaxDistSq = (MaxDistanceFromPlayer > 0.0f) ? FMath::Square(MaxDistanceFromPlayer) : 0.0f;


	CachedNavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

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

	
	CleanActiveMobs();


	CacheTickValues();


	EvaluateSpecialSpawnRules();


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


	if (bSpawnInPacks)
	{

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
		for (int32 i = 0; i < MaxSpawnsPerTick; ++i)
		{
			if (GetActiveCount() >= MaxNumOfMobs) { break; }
			if (!TrySpawnMob()) { break; }
		}
	}

	if (bShowDebug)
	{
		DrawDebugVisuals();
	}
}

bool AMobManagerActor::TrySpawnMob()
{
	
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


	int32 DistanceFailCount = 0;

	
	const double BudgetStartSeconds = FPlatformTime::Seconds();
	const double BudgetLimitSeconds = SpawnBudgetMs > 0.0f
		? (static_cast<double>(SpawnBudgetMs) / 1000.0)
		: 0.0;

	for (int32 Attempt = 0; Attempt < MaxSpawnAttempts; ++Attempt)
	{

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

		const bool bNeedDistCheck = (MinDistanceFromPlayer > 0.0f || MaxDistanceFromPlayer > 0.0f);
		if (bNeedDistCheck && !CheckDistanceFromPlayers(SpawnLoc))
		{
			++DistanceFailCount;
			RecordDebugAttempt(SpawnLoc, false, EMobSpawnFailReason::DistanceFailed);
			continue;
		}

		if (bUseCollisionCheck && !CheckCollision(SpawnLoc))
		{
			RecordDebugAttempt(SpawnLoc, false, EMobSpawnFailReason::CollisionFailed);
			continue;
		}


		SpawnLoc.Z += CollisionCapsuleHalfHeight + 2.0f;

		const FRotator SpawnRot(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);
		APHBaseCharacter* HiddenMob = HiddenSpawn(MobClass, SpawnLoc, SpawnRot);
		if (!HiddenMob)
		{
			RecordDebugAttempt(SpawnLoc, false, EMobSpawnFailReason::SpawnFailed);
			continue;
		}




		FinalizeSpawn(HiddenMob, SpawnLoc);
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
	const int32 TypeIndex = GetWeightedRandomMobTypeIndex();
	if (TypeIndex == INDEX_NONE)
	{
		OnSpawnFailed.Broadcast(EMobSpawnFailReason::AllOnCooldown);
		BP_OnSpawnFailed(EMobSpawnFailReason::AllOnCooldown);
		return 0;
	}

	FMobTypeEntry& ChosenType = MobTypes[TypeIndex];
	const TSubclassOf<APHBaseCharacter> MobClass = ChosenType.MobClass;


	const double BudgetStartSeconds = FPlatformTime::Seconds();
	const double BudgetLimitSeconds = SpawnBudgetMs > 0.0f
		? (static_cast<double>(SpawnBudgetMs) / 1000.0)
		: 0.0;

	auto IsBudgetExhausted = [&]() -> bool
	{
		return BudgetLimitSeconds > 0.0
			&& (FPlatformTime::Seconds() - BudgetStartSeconds) >= BudgetLimitSeconds;
	};

	FVector LeaderLoc = FVector::ZeroVector;
	bool bFoundLeader = false;

	for (int32 Attempt = 0; Attempt < MaxSpawnAttempts; ++Attempt)
	{
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


	const int32 SlotsAvailable = MaxNumOfMobs - GetActiveCount();
	const int32 ClampedMin = FMath::Max(1, PackSizeMin);
	const int32 ClampedMax = FMath::Max(ClampedMin, PackSizeMax);
	const int32 RolledPackSize = FMath::RandRange(ClampedMin, ClampedMax);
	const int32 PackSize = FMath::Min(RolledPackSize, SlotsAvailable);

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


	for (int32 i = 1; i < PackSize; ++i)
	{
		if (GetActiveCount() >= MaxNumOfMobs) { break; }

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

		if (JitterRadius > 0.0f)
		{
			const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
			const float Dist  = FMath::FRandRange(0.0f, JitterRadius);
			SpawnLoc.X += Dist * FMath::Cos(Angle);
			SpawnLoc.Y += Dist * FMath::Sin(Angle);

			{
				const FVector LocalJittered =
					CachedBoxXform.InverseTransformPosition(SpawnLoc);
				if (FMath::Abs(LocalJittered.X) > CachedBoxExtent.X ||
				    FMath::Abs(LocalJittered.Y) > CachedBoxExtent.Y)
				{
					continue;
				}
			}

	
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

			const bool bNeedDistCheck =
				(MinDistanceFromPlayer > 0.0f || MaxDistanceFromPlayer > 0.0f);
			if (bNeedDistCheck && !CheckDistanceFromPlayers(SpawnLoc))
			{
				continue;
			}
		}

	
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
	const FVector& BoxExtent  = CachedBoxExtent;
	const FTransform& BoxXform = CachedBoxXform;


	const FVector LocalOffset(
		FMath::RandRange(-BoxExtent.X, BoxExtent.X),
		FMath::RandRange(-BoxExtent.Y, BoxExtent.Y),
		FMath::RandRange(-BoxExtent.Z, BoxExtent.Z));

	FVector Candidate = BoxXform.TransformPosition(LocalOffset);

	if (bUseNavCheck)
	{
		FVector Projected;
		if (!CheckNavMesh(Candidate, Projected))
		{
			return false;
		}

		const FVector LocalProjected = BoxXform.InverseTransformPosition(Projected);
		if (FMath::Abs(LocalProjected.X) > BoxExtent.X ||
		    FMath::Abs(LocalProjected.Y) > BoxExtent.Y)
		{
			return false;
		}

		Candidate = Projected;
	}

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

bool AMobManagerActor::CheckNavMesh(FVector Candidate, FVector& OutProjected) const
{
	UNavigationSystemV1* NavSys = CachedNavSystem.Get();
	if (!NavSys)
	{
		OutProjected = Candidate;
		return true;
	}

	FNavLocation NavLoc;
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

	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(
		CollisionCapsuleRadius, CollisionCapsuleHalfHeight);

	FCollisionObjectQueryParams ObjQuery;
	ObjQuery.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjQuery.AddObjectTypesToQuery(ECC_Pawn);


	static constexpr float GroundClearance = 3.0f;
	const FVector TestCenter = Location +
		FVector(0.0f, 0.0f, CollisionCapsuleHalfHeight + GroundClearance);


	return !GetWorld()->OverlapAnyTestByObjectType(
		TestCenter, FQuat::Identity, ObjQuery, Capsule, Params);
}

bool AMobManagerActor::CheckDistanceFromPlayers(FVector Location) const
{
	if (CachedPlayerLocations.IsEmpty())
	{
		return true;
	}

	bool bAnyPlayerInMaxRange = (MaxDistSq <= 0.0f);

	for (const FVector& PlayerLoc : CachedPlayerLocations)
	{
		const float DistSq = FVector::DistSquared(Location, PlayerLoc);

		if (MinDistSq > 0.0f && DistSq < MinDistSq)
		{
			return false;
		}

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
	if (CachedPlayerLocations.IsEmpty()) { return false; }

	const FVector& BoxExtent = CachedBoxExtent;
	const FTransform& BoxXform = CachedBoxXform;
	const FVector BoxOrigin = BoxXform.GetLocation();

	const FVector& PlayerLoc =
		CachedPlayerLocations[FMath::RandRange(0, CachedPlayerLocations.Num() - 1)];

	const float InnerR = FMath::Max(MinDistanceFromPlayer, 1.0f);
	const float OuterR = (MaxDistanceFromPlayer > InnerR) ? MaxDistanceFromPlayer
	                     : InnerR + BoxExtent.Size2D();

	const float Angle  = FMath::FRandRange(0.0f, 2.0f * PI);
	const float RandT  = FMath::FRand();
	const float Radius = FMath::Sqrt(FMath::Lerp(InnerR * InnerR, OuterR * OuterR, RandT));

	FVector Candidate;
	Candidate.X = PlayerLoc.X + Radius * FMath::Cos(Angle);
	Candidate.Y = PlayerLoc.Y + Radius * FMath::Sin(Angle);
	Candidate.Z = BoxOrigin.Z;

	FVector LocalCandidate = BoxXform.InverseTransformPosition(Candidate);
	LocalCandidate.X = FMath::Clamp(LocalCandidate.X, -BoxExtent.X, BoxExtent.X);
	LocalCandidate.Y = FMath::Clamp(LocalCandidate.Y, -BoxExtent.Y, BoxExtent.Y);
	LocalCandidate.Z = FMath::Clamp(LocalCandidate.Z, -BoxExtent.Z, BoxExtent.Z);
	Candidate = BoxXform.TransformPosition(LocalCandidate);

	if (bUseNavCheck)
	{
		FVector Projected;
		if (!CheckNavMesh(Candidate, Projected))
		{
			return false;
		}

		const FVector LocalProjected = BoxXform.InverseTransformPosition(Projected);
		if (FMath::Abs(LocalProjected.X) > BoxExtent.X ||
		    FMath::Abs(LocalProjected.Y) > BoxExtent.Y)
		{
			return false;
		}

		Candidate = Projected;
	}

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

	UWorld* World = GetWorld();
	if (World && World->LineTraceSingleByChannel(
			Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		OutGroundLocation = Hit.ImpactPoint + FVector(0.0f, 0.0f, 1.0f);
		return true;
	}

	return false;
}

APHBaseCharacter* AMobManagerActor::HiddenSpawn(
	TSubclassOf<APHBaseCharacter> Class,
	const FVector& Location,
	const FRotator& Rotation) const
{
	if (!Class) { return nullptr; }

	if (CachedPoolSubsystem)
	{
		APHBaseCharacter* Mob = CachedPoolSubsystem->Acquire(Class, Location, Rotation);
		if (Mob)
		{
			Mob->SetOwner(const_cast<AMobManagerActor*>(this));
			return Mob;
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner  = const_cast<AMobManagerActor*>(this);
	Params.Instigator = nullptr;

	UWorld* World = GetWorld();
	if (!World) { return nullptr; }

	APHBaseCharacter* Mob =
		World->SpawnActor<APHBaseCharacter>(Class, Location, Rotation, Params);

	if (!Mob) { return nullptr; }

	Mob->SetActorHiddenInGame(true);
	Mob->SetActorEnableCollision(false);
	Mob->AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	if (AAIController* AIC = Cast<AAIController>(Mob->GetController()))
	{
		if (UBrainComponent* Brain = AIC->GetBrainComponent())
		{
			Brain->PauseLogic(TEXT("MobManagerValidation"));
		}
	}

	if (UCharacterMovementComponent* Move = Mob->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->SetComponentTickEnabled(false);
	}

	return Mob;
}

void AMobManagerActor::FinalizeSpawn(APHBaseCharacter* Mob, const FVector& SpawnLocation)
{
	if (!Mob) { return; }

	ActiveMobs.Add(TWeakObjectPtr<APHBaseCharacter>(Mob));

	// Death hookup: Blueprint death logic calls APHBaseCharacter::NotifyDeath(Killer),
	// which broadcasts OnDeath exactly once (server-side). Binding here is what
	// drives kill counting, OnMobDied, and the pool-recycle timer. AddUniqueDynamic
	// keeps pooled re-spawns from double-binding to the same manager.
	Mob->ResetDeathState();
	Mob->OnDeath.AddUniqueDynamic(this, &AMobManagerActor::OnMobDeathEvent);

	if (Mob->Implements<UMobWanderable>())
	{
		IMobWanderable::Execute_SetHomeLocation(Mob, SpawnLocation);
		IMobWanderable::Execute_SetWanderRadius(Mob, WanderRadius);
		IMobWanderable::Execute_OnSpawnedByManager(Mob, this);
	}

	ApplyModifierComponent(Mob);

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

	Mob->SetActorHiddenInGame(false);
	Mob->SetActorEnableCollision(true);

	if (UCharacterMovementComponent* Move = Mob->GetCharacterMovement())
	{
		FFindFloorResult Floor;
		Move->FindFloor(Mob->GetActorLocation(), Floor, false);

		if (Floor.IsWalkableFloor() && Mob->GetCapsuleComponent())
		{
			const float HalfHeight = Mob->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			FVector Adjusted = Mob->GetActorLocation();
			Adjusted.Z = Floor.HitResult.ImpactPoint.Z + HalfHeight;
			Mob->SetActorLocation(Adjusted, false, nullptr, ETeleportType::TeleportPhysics);
		}
		if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()))
		{
			FNavLocation Projected;
			if (NavSys->ProjectPointToNavigation(
					Mob->GetActorLocation(),
					Projected,
					FVector(
						FMath::Max(200.0f, NavProjectionExtent),
						FMath::Max(200.0f, NavProjectionExtent),
						FMath::Max(400.0f, NavProjectionVerticalExtent))))
			{
				if (Mob->GetCapsuleComponent())
				{
					const float HalfHeight = Mob->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
					FVector Adjusted = Projected.Location;
					Adjusted.Z += HalfHeight;
					Mob->SetActorLocation(Adjusted, false, nullptr, ETeleportType::TeleportPhysics);
				}
			}
			else
			{
				UE_LOG(LogMobManager, Warning,
					TEXT("FinalizeSpawn: '%s' could not be projected to navmesh "
					     "from %s — pathfinding will likely fail."),
					*Mob->GetName(), *Mob->GetActorLocation().ToString());
			}
		}

		Move->SetComponentTickEnabled(true);
		if (Move->MovementMode == MOVE_None)
		{
			Move->SetMovementMode(MOVE_Walking);
		}
		Move->StopMovementImmediately();
	}

	if (HasAuthority() && !Mob->GetController())
	{
		Mob->SpawnDefaultController();
	}

	if (AALSAIController* ALSAIC = Cast<AALSAIController>(Mob->GetController()))
	{
		UBrainComponent* Brain = ALSAIC->GetBrainComponent();
		if ((!Brain || !Brain->IsRunning()) && ALSAIC->Behaviour)
		{
			ALSAIC->RunBehaviorTree(ALSAIC->Behaviour);
			Brain = ALSAIC->GetBrainComponent();
		}

		if (Brain)
		{
			Brain->ResumeLogic(TEXT("MobManagerFinalized"));
		}
	}
	else if (AAIController* AIC = Cast<AAIController>(Mob->GetController()))
	{
		if (UBrainComponent* Brain = AIC->GetBrainComponent())
		{
			Brain->ResumeLogic(TEXT("MobManagerFinalized"));
		}
	}
	else
	{
		UE_LOG(LogMobManager, Warning,
			TEXT("FinalizeSpawn: '%s' has no AIController after spawn finalization. "
			     "Assign an AIControllerClass on the mob Blueprint so possession can start."),
			*Mob->GetName());
	}

	OnMobSpawned.Broadcast(Mob);
	BP_OnMobSpawned(Mob);
}

void AMobManagerActor::ApplyModifierComponent(APHBaseCharacter* Mob)
{
	if (!Mob) { return; }

	UMonsterModifierComponent* ModComp =
		Mob->FindComponentByClass<UMonsterModifierComponent>();
	if (!ModComp) { return; }

	ModComp->AreaLevel = AreaLevel;
	ModComp->NearbyPlayerMagicFind = NearbyMagicFind;

	if (PendingForcedTier != EMonsterTier::MT_Normal)
	{
		ModComp->ForcedTier = PendingForcedTier;
		UE_LOG(LogMobManager, Verbose,
			TEXT("[%s] ApplyModifierComponent: consumed PendingForcedTier=%d for '%s'"),
			*GetName(), static_cast<int32>(PendingForcedTier), *Mob->GetName());
		PendingForcedTier = EMonsterTier::MT_Normal;
	}

	ModComp->RerollMods();
}

void AMobManagerActor::OnMobDeathEvent(APHBaseCharacter* DeadMob, AActor* Killer)
{
	if (!DeadMob) { return; }

	// Unbind so a pooled actor re-acquired by a DIFFERENT manager can't notify
	// this one on its next death.
	DeadMob->OnDeath.RemoveDynamic(this, &AMobManagerActor::OnMobDeathEvent);

	ActiveMobs.RemoveAll([DeadMob](const TWeakObjectPtr<APHBaseCharacter>& Weak)
	{
		return !Weak.IsValid() || Weak.Get() == DeadMob;
	});

	if (UClass* DeadClass = DeadMob->GetClass())
	{
		const TSubclassOf<APHBaseCharacter> DeadAsSub(DeadClass);
		KillsByClass.FindOrAdd(DeadAsSub)++;
		++TotalKills;
	}

	OnMobDied.Broadcast(DeadMob);
	BP_OnMobDied(DeadMob);

	bFullEventFired = false;

	UE_LOG(LogMobManager, Verbose,
		TEXT("[%s] OnMobDeathEvent: '%s' died (active=%d)"),
		*GetName(), *DeadMob->GetName(), GetActiveCount());

	if (CachedPoolSubsystem)
	{
		FTimerHandle RecycleTimer;
		TWeakObjectPtr<APHBaseCharacter> WeakMob = DeadMob;
		TWeakObjectPtr<UMobPoolSubsystem> WeakPool = CachedPoolSubsystem;

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

int32 AMobManagerActor::GetWeightedRandomMobTypeIndex() const
{
	UWorld* World = GetWorld();
	if (!World) { return INDEX_NONE; }
	const float Now = World->GetTimeSeconds();

	int32 TotalWeight = 0;
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

	int32 Roll = FMath::RandRange(0, TotalWeight - 1);

	for (int32 Idx : EligibleIndices)
	{
		Roll -= MobTypes[Idx].SpawnWeight;
		if (Roll < 0)
		{
			return Idx;
		}
	}

	return EligibleIndices.Last();
}

void AMobManagerActor::RecordDebugAttempt(
	const FVector& Location, bool bSuccess, EMobSpawnFailReason Reason)
{
	if (!bShowDebug) { return; }

	FSpawnAttemptDebug Entry;
	Entry.Location   = Location;
	Entry.bSuccess   = bSuccess;
	Entry.FailReason = Reason;
	UWorld* World = GetWorld();
	Entry.Timestamp  = World ? World->GetTimeSeconds() : 0.0f;
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
		DrawDebugString(GetWorld(), Location + FVector(0, 0, 60),
			UEnum::GetValueAsString(Reason), nullptr,
			SphereColor, DebugDrawDuration, false, 1.0f);
	}
#endif
}

void AMobManagerActor::DrawDebugVisuals() const
{
#if ENABLE_DRAW_DEBUG
	const float DebugLifetime = FMath::Clamp(SpawnInterval, 0.1f, 30.0f);

	DrawDebugBox(
		GetWorld(),
		SpawnArea->GetComponentLocation(),
		SpawnArea->GetScaledBoxExtent(),
		SpawnArea->GetComponentQuat(),
		FColor(0, 180, 255),
		false, DebugLifetime, 0, 3.0f);

	for (const TWeakObjectPtr<APHBaseCharacter>& Weak : ActiveMobs)
	{
		if (!Weak.IsValid()) { continue; }

		const FVector MobLoc = Weak->GetActorLocation();
		DrawDebugSphere(GetWorld(), MobLoc, 30.0f, 6,
			FColor::Cyan, false, DebugLifetime, 0, 1.5f);

		DrawDebugCircle(GetWorld(), MobLoc, WanderRadius,
			32, FColor(0, 80, 255),
			false, DebugLifetime, 0, 1.5f,
			FVector(1, 0, 0), FVector(0, 1, 0), false);
	}

	const FVector LabelPos =
		SpawnArea->GetComponentLocation() +
		FVector(0, 0, SpawnArea->GetScaledBoxExtent().Z + 30.0f);

	DrawDebugString(GetWorld(), LabelPos,
		FString::Printf(TEXT("%s\n%d / %d mobs"),
			*GetName(), GetActiveCount(), MaxNumOfMobs),
		nullptr, FColor::White, DebugLifetime, false, 1.2f);
#endif
}

bool AMobManagerActor::DoesAnyPlayerHaveKeyItem_Implementation(FName KeyItemId)
{
	return false;
}

int32 AMobManagerActor::GetKillCountForClass(TSubclassOf<APHBaseCharacter> MobClass) const
{
	if (!MobClass)
	{
		return TotalKills;
	}

	int32 Count = 0;
	for (const TPair<TSubclassOf<APHBaseCharacter>, int32>& Pair : KillsByClass)
	{
		if (Pair.Key && Pair.Key->IsChildOf(MobClass))
		{
			Count += Pair.Value;
		}
	}
	return Count;
}

int32 AMobManagerActor::GetTotalKillCount() const
{
	return TotalKills;
}

void AMobManagerActor::ResetKillCounts()
{
	KillsByClass.Empty();
	TotalKills = 0;
}

void AMobManagerActor::EvaluateSpecialSpawnRules()
{
	if (!HasAuthority() || SpecialSpawnRules.Num() == 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World) { return; }

	const float Now = World->GetTimeSeconds();

	for (FMobSpecialSpawnRule& Rule : SpecialSpawnRules)
	{
		if (!UMobSpawnConditionEvaluator::IsRuleReady(Rule, this, Now))
		{
			continue;
		}

		APHBaseCharacter* SpawnedMob = nullptr;

		switch (Rule.Action)
		{
		case EMobSpawnRuleAction::ForceSpawnSpecialMob:
		{
			if (!Rule.SpecialMobClass)
			{
				PH_LOG_WARNING(LogMobManager,
					TEXT("Rule '%s' ForceSpawnSpecialMob has no SpecialMobClass set"),
					*Rule.RuleId.ToString());
				continue;
			}

			PendingForcedTier = Rule.ForcedTier;

			FVector Location;
			if (!GetRandomSpawnLocation(Location))
			{
				PH_LOG_WARNING(LogMobManager,
					TEXT("Rule '%s' could not find a valid spawn location"),
					*Rule.RuleId.ToString());
				PendingForcedTier = EMonsterTier::MT_Normal;
				continue;
			}

			const FRotator Rotation(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);
			SpawnedMob = HiddenSpawn(Rule.SpecialMobClass, Location, Rotation);
			if (!SpawnedMob)
			{
				PH_LOG_WARNING(LogMobManager,
					TEXT("Rule '%s' HiddenSpawn failed"), *Rule.RuleId.ToString());
				PendingForcedTier = EMonsterTier::MT_Normal;
				continue;
			}

			FinalizeSpawn(SpawnedMob, Location);
			break;
		}

		case EMobSpawnRuleAction::BoostNextSpawnTier:
		{
			PendingForcedTier = Rule.ForcedTier;
			break;
		}

		default:
			PH_LOG_WARNING(LogMobManager,
				TEXT("Rule '%s' has unknown Action"), *Rule.RuleId.ToString());
			continue;
		}

		UMobSpawnConditionEvaluator::MarkRuleFired(Rule, Now);
		OnSpecialSpawnExecuted.Broadcast(Rule.RuleId, SpawnedMob);

		UE_LOG(LogMobManager, Log,
			TEXT("[%s] Special spawn rule '%s' fired (Action=%d, Mob=%s)"),
			*GetName(), *Rule.RuleId.ToString(),
			static_cast<int32>(Rule.Action),
			SpawnedMob ? *SpawnedMob->GetName() : TEXT("<none>"));
	}
}

bool AMobManagerActor::ForceSpawnSpecial(FName RuleId)
{
	if (!HasAuthority()) { return false; }

	for (FMobSpecialSpawnRule& Rule : SpecialSpawnRules)
	{
		if (Rule.RuleId != RuleId) { continue; }
		if (!Rule.bEnabled)        { return false; }

		UWorld* World = GetWorld();
		if (!World) { return false; }
		const float Now = World->GetTimeSeconds();

		switch (Rule.Lifetime)
		{
		case EMobSpawnRuleLifetime::OneShot:
			if (Rule.bHasFired) { return false; }
			break;
		case EMobSpawnRuleLifetime::RepeatableWithCooldown:
			if (Rule.LastFireTime >= 0.0f &&
			    (Now - Rule.LastFireTime) < Rule.CooldownSeconds)
			{
				return false;
			}
			break;
		}

		APHBaseCharacter* SpawnedMob = nullptr;

		if (Rule.Action == EMobSpawnRuleAction::ForceSpawnSpecialMob)
		{
			if (!Rule.SpecialMobClass) { return false; }
			PendingForcedTier = Rule.ForcedTier;
			CacheTickValues();

			FVector Location;
			if (!GetRandomSpawnLocation(Location))
			{
				PendingForcedTier = EMonsterTier::MT_Normal;
				return false;
			}

			const FRotator Rotation(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);
			SpawnedMob = HiddenSpawn(Rule.SpecialMobClass, Location, Rotation);
			if (!SpawnedMob)
			{
				PendingForcedTier = EMonsterTier::MT_Normal;
				return false;
			}
			FinalizeSpawn(SpawnedMob, Location);
		}
		else // BoostNextSpawnTier
		{
			PendingForcedTier = Rule.ForcedTier;
		}

		UMobSpawnConditionEvaluator::MarkRuleFired(Rule, Now);
		OnSpecialSpawnExecuted.Broadcast(Rule.RuleId, SpawnedMob);
		return true;
	}

	PH_LOG_WARNING(LogMobManager,
		TEXT("ForceSpawnSpecial: no rule found with ID '%s'"), *RuleId.ToString());
	return false;
}

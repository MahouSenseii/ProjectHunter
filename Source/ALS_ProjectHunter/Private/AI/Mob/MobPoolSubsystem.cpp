// AI/Mob/MobPoolSubsystem.cpp
#include "AI/Mob/MobPoolSubsystem.h"
#include "Character/PHBaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Character/Component/MonsterModifierComponent.h"
#include "Character/Component/StatsManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "BrainComponent.h"

DEFINE_LOG_CATEGORY(LogMobPool);

// ─────────────────────────────────────────────────────────────────────────────
// UWorldSubsystem interface
// ─────────────────────────────────────────────────────────────────────────────

void UMobPoolSubsystem::Deinitialize()
{
	DrainAllPools();
	Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

APHBaseCharacter* UMobPoolSubsystem::Acquire(
	TSubclassOf<APHBaseCharacter> MobClass,
	const FVector& Location,
	const FRotator& Rotation)
{
	if (!MobClass)
	{
		UE_LOG(LogMobPool, Warning, TEXT("Acquire: null MobClass"));
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// ── Try to recycle from pool ─────────────────────────────────────────
	if (TArray<TWeakObjectPtr<APHBaseCharacter>>* ClassPool = Pool.Find(MobClass.Get()))
	{
		while (ClassPool->Num() > 0)
		{
			TWeakObjectPtr<APHBaseCharacter> Weak = ClassPool->Pop(EAllowShrinking::No);
			if (!Weak.IsValid())
			{
				continue; // GC'd since we pooled it — skip stale entry
			}

			APHBaseCharacter* Mob = Weak.Get();
			PrepareMobForReuse(Mob, Location, Rotation);

			// OPT: Compact stale entries while we're here.  After popping
			// through invalid entries above, the array may still contain
			// stale weak pointers at the front.  RemoveAll is O(n) but only
			// runs when we actually found a valid mob, which is the hot path.
			ClassPool->RemoveAll([](const TWeakObjectPtr<APHBaseCharacter>& W)
			{
				return !W.IsValid();
			});

			UE_LOG(LogMobPool, Verbose,
				TEXT("Acquire: recycled '%s' from pool (class=%s, remaining=%d)"),
				*Mob->GetName(), *MobClass->GetName(), ClassPool->Num());

			return Mob;
		}
	}

	// ── Pool empty — spawn fresh (hidden, just like MobManager::HiddenSpawn) ─
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APHBaseCharacter* Mob =
		World->SpawnActor<APHBaseCharacter>(MobClass, Location, Rotation, Params);

	if (!Mob)
	{
		UE_LOG(LogMobPool, Warning,
			TEXT("Acquire: SpawnActor failed for class '%s'"),
			*MobClass->GetName());
		return nullptr;
	}

	// Make invisible and inert — the caller (MobManager) will finalize
	Mob->SetActorHiddenInGame(true);
	Mob->SetActorEnableCollision(false);

	if (AAIController* AIC = Cast<AAIController>(Mob->GetController()))
	{
		if (UBrainComponent* Brain = AIC->GetBrainComponent())
		{
			Brain->PauseLogic(TEXT("MobPoolAcquire"));
		}
	}
	if (UCharacterMovementComponent* Move = Mob->GetCharacterMovement())
	{
		Move->DisableMovement();
	}

	UE_LOG(LogMobPool, Verbose,
		TEXT("Acquire: spawned fresh '%s' (class=%s)"),
		*Mob->GetName(), *MobClass->GetName());

	return Mob;
}

void UMobPoolSubsystem::Release(APHBaseCharacter* Mob)
{
	if (!IsValid(Mob))
	{
		return;
	}

	UClass* MobClass = Mob->GetClass();
	TArray<TWeakObjectPtr<APHBaseCharacter>>& ClassPool = Pool.FindOrAdd(MobClass);

	// FIX #2: Compact stale (GC'd) weak pointers BEFORE checking the pool
	// size.  Without this, Num() counts dead entries so the pool can appear
	// full (e.g. 20/20) while holding zero valid actors, causing every Release
	// to destroy the mob rather than pool it — silently defeating the entire
	// optimization after any mass-death event.
	ClassPool.RemoveAll([](const TWeakObjectPtr<APHBaseCharacter>& W)
	{
		return !W.IsValid();
	});

	// If pool is full after compaction, destroy the incoming mob
	if (ClassPool.Num() >= MaxPoolSizePerClass)
	{
		UE_LOG(LogMobPool, Verbose,
			TEXT("Release: pool full for '%s' (%d/%d) — destroying '%s'"),
			*MobClass->GetName(), ClassPool.Num(), MaxPoolSizePerClass, *Mob->GetName());
		Mob->Destroy();
		return;
	}

	// Deactivate and reset
	DeactivateMob(Mob);
	ResetMobState(Mob);

	ClassPool.Add(TWeakObjectPtr<APHBaseCharacter>(Mob));

	UE_LOG(LogMobPool, Verbose,
		TEXT("Release: returned '%s' to pool (class=%s, pooled=%d)"),
		*Mob->GetName(), *MobClass->GetName(), ClassPool.Num());
}

void UMobPoolSubsystem::DrainAllPools()
{
	int32 TotalDestroyed = 0;

	for (auto& Pair : Pool)
	{
		for (const TWeakObjectPtr<APHBaseCharacter>& Weak : Pair.Value)
		{
			if (Weak.IsValid())
			{
				Weak->Destroy();
				++TotalDestroyed;
			}
		}
		Pair.Value.Empty();
	}
	Pool.Empty();

	UE_LOG(LogMobPool, Log,
		TEXT("DrainAllPools: destroyed %d pooled actors"), TotalDestroyed);
}

int32 UMobPoolSubsystem::GetPooledCount(TSubclassOf<APHBaseCharacter> MobClass) const
{
	if (!MobClass) { return 0; }

	if (const TArray<TWeakObjectPtr<APHBaseCharacter>>* ClassPool = Pool.Find(MobClass.Get()))
	{
		int32 ValidCount = 0;
		for (const auto& Weak : *ClassPool)
		{
			if (Weak.IsValid()) { ++ValidCount; }
		}
		return ValidCount;
	}
	return 0;
}

int32 UMobPoolSubsystem::GetTotalPooledCount() const
{
	int32 Total = 0;
	for (const auto& Pair : Pool)
	{
		for (const auto& Weak : Pair.Value)
		{
			if (Weak.IsValid()) { ++Total; }
		}
	}
	return Total;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

void UMobPoolSubsystem::ResetMobState(APHBaseCharacter* Mob) const
{
	if (!Mob) { return; }

	// ── 1. Reset death state ─────────────────────────────────────────────
	Mob->bIsDead = false;

	// ── 2. Clear GAS state (effects, tags, abilities) ────────────────────
	if (UAbilitySystemComponent* ASC = Mob->GetAbilitySystemComponent())
	{
		// Remove all active gameplay effects (buffs, debuffs, DoTs, modifiers).
		// UAbilitySystemComponent has no RemoveAllActiveEffects(); iterate handles instead.
		TArray<FActiveGameplayEffectHandle> ActiveHandles = ASC->GetActiveEffects(FGameplayEffectQuery());
		for (const FActiveGameplayEffectHandle& Handle : ActiveHandles)
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}

		// Remove all loose gameplay tags
		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);
		for (const FGameplayTag& Tag : OwnedTags)
		{
			// RemoveLooseGameplayTag with count -1 removes all stacks
			const int32 Count = ASC->GetTagCount(Tag);
			for (int32 i = 0; i < Count; ++i)
			{
				ASC->RemoveLooseGameplayTag(Tag);
			}
		}
	}

	// ── 3. Remove abilities so they can be re-granted fresh ──────────────
	Mob->RemoveAllAbilities();

	// ── 4. Reset MonsterModifierComponent if present ─────────────────────
	if (UMonsterModifierComponent* ModComp = Mob->FindComponentByClass<UMonsterModifierComponent>())
	{
		// FIX #3: For MT_Unique monsters, AppliedMods is populated externally
		// (scripted encounter data) BEFORE RollAndApplyMods is called, because
		// RollAndApplyMods rolls 0 mods for Unique and iterates whatever is
		// already in AppliedMods.  Clearing AppliedMods here (the old code)
		// meant a recycled Unique mob re-spawned with no modifiers or name —
		// it kept its Unique tier color but had no actual mod effects.
		//
		// Preserve AppliedMods for Unique-tier mobs so RerollMods can
		// re-apply the same fixed mods on the next life.
		// For all other tiers, clear it — RerollMods will re-roll fresh mods.
		if (ModComp->ForcedTier != EMonsterTier::MT_Unique)
		{
			ModComp->AppliedMods.Empty();
		}

		ModComp->AssignedTier = EMonsterTier::MT_Normal;
		ModComp->FullDisplayName = FText::GetEmpty();
		// The bModsApplied flag is protected — RerollMods resets it internally
	}

	// ── 5. Clear the OnDeathEvent delegate (MobManager will rebind) ──────
	Mob->OnDeathEvent.Clear();
}

void UMobPoolSubsystem::DeactivateMob(APHBaseCharacter* Mob) const
{
	if (!Mob) { return; }

	Mob->SetActorHiddenInGame(true);
	Mob->SetActorEnableCollision(false);

	if (AAIController* AIC = Cast<AAIController>(Mob->GetController()))
	{
		if (UBrainComponent* Brain = AIC->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("MobPoolRelease"));
		}
	}

	if (UCharacterMovementComponent* Move = Mob->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}


	if (UAnimInstance* AnimInst = Mob->GetMesh() ? Mob->GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInst->Montage_StopGroupByName(0.1f, NAME_None);
	}
}

void UMobPoolSubsystem::PrepareMobForReuse(
	APHBaseCharacter* Mob,
	const FVector& Location,
	const FRotator& Rotation) const
{
	if (!Mob) { return; }

	// Reset state from the previous life
	ResetMobState(Mob);

	// Teleport to new location
	Mob->SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::ResetPhysics);

	// Re-initialize attributes to defaults and re-grant abilities
	Mob->InitializeAttributes();
	Mob->GiveDefaultAbilities();
	Mob->ApplyStartupEffects();

	// POOL-FIX: Restore Health / Mana / Stamina to their authored initial values.
	//
	// The problem: ResetMobState() stripped every active GE (including
	// HunterGE_DerivedPrimaryVitals which sets MaxHealth/MaxStamina via MMC) and
	// left current attribute values at their death-state (0).
	// ApplyStartupEffects() above re-applies the GE modifiers so MaxHealth etc.
	// are correct again, but the CURRENT Health is still 0.
	//
	// The fix: call NotifyAbilitySystemReady() which runs InitializeFromDataAsset,
	// which re-sets attribute bases AND re-applies the Instant "set Health=MaxHealth"
	// initialization effects, bringing the mob back to full vitals.
	//
	// Why ResetStatsInitialization() first: bHasInitializedConfiguredStats is
	// already true from the mob's first life.  Without clearing it,
	// TryInitializeConfiguredStats() short-circuits and the call is a no-op.
	if (UStatsManager* Stats = Mob->FindComponentByClass<UStatsManager>())
	{
		Stats->ResetStatsInitialization();
		Stats->NotifyAbilitySystemReady();
	}

	// The mob remains hidden/inert — the caller (MobManager::FinalizeSpawn)
	// will handle making it visible, enabling collision, and starting AI.
	Mob->SetActorHiddenInGame(true);
	Mob->SetActorEnableCollision(false);

	if (UCharacterMovementComponent* Move = Mob->GetCharacterMovement())
	{
		Move->DisableMovement();
	}
}

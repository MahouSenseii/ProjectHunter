#include "AI/Mob/MobPoolSubsystem.h"
#include "Character/PHBaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "AI/Components/MonsterModifierComponent.h"
#include "Stats/Components/StatsManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "BrainComponent.h"

DEFINE_LOG_CATEGORY(LogMobPool);

void UMobPoolSubsystem::Deinitialize()
{
	DrainAllPools();
	Super::Deinitialize();
}

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

	if (TArray<TWeakObjectPtr<APHBaseCharacter>>* ClassPool = Pool.Find(MobClass.Get()))
	{
		while (ClassPool->Num() > 0)
		{
			TWeakObjectPtr<APHBaseCharacter> Weak = ClassPool->Pop(EAllowShrinking::No);
			if (!Weak.IsValid())
			{
				continue;
			}

			APHBaseCharacter* Mob = Weak.Get();
			PrepareMobForReuse(Mob, Location, Rotation);

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

	// Invisible and inert until the caller (MobManager) finalizes placement.
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
		Move->StopMovementImmediately();
		Move->SetComponentTickEnabled(false);
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

	// Compact stale weak pointers before checking pool size: Num() counts dead
	// entries and can make the pool appear full while holding zero valid actors,
	// causing every Release to destroy rather than pool after a mass-death event.
	ClassPool.RemoveAll([](const TWeakObjectPtr<APHBaseCharacter>& W)
	{
		return !W.IsValid();
	});

	if (ClassPool.Num() >= MaxPoolSizePerClass)
	{
		UE_LOG(LogMobPool, Verbose,
			TEXT("Release: pool full for '%s' (%d/%d) — destroying '%s'"),
			*MobClass->GetName(), ClassPool.Num(), MaxPoolSizePerClass, *Mob->GetName());
		Mob->Destroy();
		return;
	}

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

void UMobPoolSubsystem::ResetMobState(APHBaseCharacter* Mob) const
{
	if (!Mob) { return; }

	// Clear the death latch so the recycled actor can broadcast OnDeath again
	// next life. Without this, NotifyDeath() on the reused mob is a no-op and
	// managers never hear about its death.
	Mob->ResetDeathState();

	if (UAbilitySystemComponent* ASC = Mob->GetAbilitySystemComponent())
	{
		// UAbilitySystemComponent has no RemoveAllActiveEffects(); iterate handles instead.
		TArray<FActiveGameplayEffectHandle> ActiveHandles = ASC->GetActiveEffects(FGameplayEffectQuery());
		for (const FActiveGameplayEffectHandle& Handle : ActiveHandles)
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}

		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);
		for (const FGameplayTag& Tag : OwnedTags)
		{
			const int32 Count = ASC->GetTagCount(Tag);
			for (int32 i = 0; i < Count; ++i)
			{
				ASC->RemoveLooseGameplayTag(Tag);
			}
		}
	}

	Mob->RemoveAllAbilities();

	if (UMonsterModifierComponent* ModComp = Mob->FindComponentByClass<UMonsterModifierComponent>())
	{
		// For MT_Unique monsters, AppliedMods is populated externally before RollAndApplyMods
		// is called (RollAndApplyMods rolls 0 mods for Unique and iterates whatever is already
		// in AppliedMods). Clearing it here would cause a recycled Unique to re-spawn with no
		// mod effects. Preserve it so RerollMods can re-apply the same fixed mods next life.
		// For all other tiers, clear it so RerollMods re-rolls fresh mods.
		if (ModComp->ForcedTier != EMonsterTier::MT_Unique)
		{
			ModComp->AppliedMods.Empty();
		}

		ModComp->AssignedTier = EMonsterTier::MT_Normal;
		ModComp->FullDisplayName = FText::GetEmpty();
	}

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
			Brain->PauseLogic(TEXT("MobPoolRelease"));
		}
	}

	if (UCharacterMovementComponent* Move = Mob->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->SetComponentTickEnabled(false);
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

	ResetMobState(Mob);

	Mob->SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::ResetPhysics);

	Mob->InitializeAttributes();
	Mob->GiveDefaultAbilities();
	Mob->ApplyStartupEffects();

	// ResetMobState() stripped every active GE including HunterGE_DerivedPrimaryVitals,
	// leaving current Health/Mana/Stamina at death-state (0). ApplyStartupEffects() restores
	// Max values but not Current values. NotifyAbilitySystemReady() re-runs InitializeFromDataAsset
	// which re-applies the Instant "set Health=MaxHealth" effects.
	// ResetStatsInitialization() must be called first because bHasInitializedConfiguredStats
	// is already true from the mob's first life and would short-circuit the call.
	if (UStatsManager* Stats = Mob->FindComponentByClass<UStatsManager>())
	{
		Stats->ResetStatsInitialization();
		Stats->NotifyAbilitySystemReady();
	}

	Mob->SetActorHiddenInGame(true);
	Mob->SetActorEnableCollision(false);

	if (UCharacterMovementComponent* Move = Mob->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->SetComponentTickEnabled(false);
	}
}

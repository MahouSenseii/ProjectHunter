#include "Item/Runtime/EquippedItemRuntimeActor.h"

#include "Combat/CombatManager.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Item/ItemInstance.h"

DEFINE_LOG_CATEGORY(LogEquippedItemRuntimeActor);

AEquippedItemRuntimeActor::AEquippedItemRuntimeActor()
{
	PrimaryActorTick.bCanEverTick = false;
	// N-19 FIX: bReplicates was false, so clients never saw the runtime actor
	// that the server spawned for equipped items.  Visual representation (mesh,
	// VFX) and hit-detection rely on this actor existing on clients, so it must
	// replicate.  Movement replication stays off — position is driven by
	// attachment to the owner's socket, not by the replication system.
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CombatManager = CreateDefaultSubobject<UCombatManager>(TEXT("CombatManager"));
}

void AEquippedItemRuntimeActor::InitializeFromItem(UItemInstance* Item, APawn* OwnerPawn, EEquipmentSlot Slot)
{
	ItemInstance = Item;
	OwningPawn = OwnerPawn ? OwnerPawn : Cast<APawn>(GetOwner());
	EquippedSlot = Slot;

	// OPT-SAFE: Warn early if we end up with no valid pawn — combat source
	// resolution and Blueprint events will silently get nullptr downstream.
	if (!OwningPawn)
	{
		UE_LOG(LogEquippedItemRuntimeActor, Warning,
			TEXT("InitializeFromItem: OwningPawn is null for '%s'. "
			     "Combat source resolution will fall back to GetOwner()."),
			*GetName());
	}

	ResetHitActorsForCurrentAction();

	HandleItemInitialized();
	K2_OnRuntimeItemInitialized(ItemInstance, OwningPawn, EquippedSlot);
}

void AEquippedItemRuntimeActor::OnEquipped()
{
	bIsEquipped = true;

	HandleEquipped();
	K2_OnEquipped();
}

void AEquippedItemRuntimeActor::OnUnequipped()
{
	if (bPrimaryActionActive)
	{
		EndPrimaryAction();
	}

	bIsEquipped = false;
	ResetHitActorsForCurrentAction();

	HandleUnequipped();
	K2_OnUnequipped();
}

bool AEquippedItemRuntimeActor::BeginPrimaryAction()
{
	if (!CanBeginPrimaryAction())
	{
		UE_LOG(LogEquippedItemRuntimeActor, Verbose, TEXT("BeginPrimaryAction rejected for %s"), *GetName());
		return false;
	}

	bPrimaryActionActive = true;
	ResetHitActorsForCurrentAction();

	HandlePrimaryActionStarted();
	K2_OnPrimaryActionStarted();
	return true;
}

void AEquippedItemRuntimeActor::EndPrimaryAction()
{
	if (!bPrimaryActionActive)
	{
		return;
	}

	bPrimaryActionActive = false;
	if (UWorld* World = GetWorld())
	{
		LastPrimaryActionEndTime = World->GetTimeSeconds();
	}

	HandlePrimaryActionEnded();
	K2_OnPrimaryActionEnded();
}

bool AEquippedItemRuntimeActor::CanBeginPrimaryAction() const
{
	if (!bIsEquipped || !ItemInstance || bPrimaryActionActive)
	{
		return false;
	}

	if (PrimaryActionCooldown <= 0.0f)
	{
		return true;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}

	return LastPrimaryActionEndTime < 0.0f
		|| (World->GetTimeSeconds() - LastPrimaryActionEndTime) >= PrimaryActionCooldown;
}

bool AEquippedItemRuntimeActor::HasAuthorityToApplyDamage() const
{
	if (AActor* SourceActor = GetCombatSourceActor())
	{
		return SourceActor->HasAuthority();
	}

	return HasAuthority();
}

AActor* AEquippedItemRuntimeActor::GetCombatSourceActor() const
{
	if (OwningPawn)
	{
		return OwningPawn;
	}

	return GetOwner();
}

void AEquippedItemRuntimeActor::ResetHitActorsForCurrentAction()
{
	HitActorsThisAction.Reset();
}

bool AEquippedItemRuntimeActor::HasActorBeenHitThisAction(const AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	return HitActorsThisAction.Contains(TWeakObjectPtr<AActor>(const_cast<AActor*>(TargetActor)));
}

bool AEquippedItemRuntimeActor::MarkActorHitThisAction(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	TWeakObjectPtr<AActor> TargetKey(TargetActor);
	if (HitActorsThisAction.Contains(TargetKey))
	{
		return false;
	}

	HitActorsThisAction.Add(TargetKey);
	return true;
}

bool AEquippedItemRuntimeActor::ResolveHitOnActor(AActor* TargetActor, FCombatResolveResult& OutResult)
{
	OutResult = FCombatResolveResult{};

	if (!HasAuthorityToApplyDamage() || !CombatManager || !IsValid(TargetActor))
	{
		return false;
	}

	AActor* SourceActor = GetCombatSourceActor();
	if (!IsValid(SourceActor) || TargetActor == SourceActor || TargetActor == this)
	{
		return false;
	}

	if (!MarkActorHitThisAction(TargetActor))
	{
		return false;
	}

	const bool bResolved = CombatManager->ResolveHit(SourceActor, TargetActor, OutResult);
	if (!bResolved)
	{
		HitActorsThisAction.Remove(TWeakObjectPtr<AActor>(TargetActor));
	}

	return bResolved;
}

int32 AEquippedItemRuntimeActor::ResolveHitResults(const TArray<FHitResult>& HitResults, TArray<FCombatResolveResult>& OutResults)
{
	OutResults.Reset();

	int32 ResolvedHitCount = 0;
	for (const FHitResult& HitResult : HitResults)
	{
		if (AActor* TargetActor = HitResult.GetActor())
		{
			FCombatResolveResult ResolveResult;
			if (ResolveHitOnActor(TargetActor, ResolveResult))
			{
				OutResults.Add(ResolveResult);
				++ResolvedHitCount;
			}
		}
	}

	return ResolvedHitCount;
}

bool AEquippedItemRuntimeActor::PerformSphereTrace(
	FVector Start,
	FVector End,
	float Radius,
	TArray<FHitResult>& OutHits,
	TEnumAsByte<ECollisionChannel> TraceChannel,
	bool bIgnoreOwner) const
{
	OutHits.Reset();

	UWorld* World = GetWorld();
	if (!World || Radius <= 0.0f)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EquippedItemRuntimeSphereTrace), false, this);
	QueryParams.AddIgnoredActor(this);

	if (bIgnoreOwner)
	{
		if (AActor* SourceActor = GetCombatSourceActor())
		{
			QueryParams.AddIgnoredActor(SourceActor);
		}

		if (AActor* OwnerActor = GetOwner())
		{
			QueryParams.AddIgnoredActor(OwnerActor);
		}
	}

	const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(Radius);
	return World->SweepMultiByChannel(
		OutHits,
		Start,
		End,
		FQuat::Identity,
		TraceChannel,
		CollisionShape,
		QueryParams);
}

void AEquippedItemRuntimeActor::HandleItemInitialized()
{
}

void AEquippedItemRuntimeActor::HandleEquipped()
{
}

void AEquippedItemRuntimeActor::HandleUnequipped()
{
}

void AEquippedItemRuntimeActor::HandlePrimaryActionStarted()
{
}

void AEquippedItemRuntimeActor::HandlePrimaryActionEnded()
{
}

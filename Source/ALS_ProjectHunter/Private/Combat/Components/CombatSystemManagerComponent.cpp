#include "Combat/Components/CombatSystemManagerComponent.h"

#include "Combat/Components/CombatManager.h"
#include "Combat/Components/CombatStatusManager.h"

DEFINE_LOG_CATEGORY(LogCombatSystemManager);

UCombatSystemManagerComponent::UCombatSystemManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UCombatSystemManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheCombatManagers();
}

bool UCombatSystemManagerComponent::ApplyHit(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const FCombatHitPacket& HitPacket,
	FCombatResolveResult& OutResult)
{
	if (!CombatManager)
	{
		CacheCombatManagers();
	}

	if (!CombatManager)
	{
		UE_LOG(LogCombatSystemManager, Warning,
			TEXT("ApplyHit failed on %s because no CombatManager was found."),
			*GetNameSafe(GetOwner()));
		OutResult = FCombatResolveResult{};
		return false;
	}

	return CombatManager->ApplyHit(AttackerActor, DefenderActor, HitPacket, OutResult);
}

void UCombatSystemManagerComponent::CleanseAll(AActor* Target)
{
	if (!CombatStatusManager)
	{
		CacheCombatManagers();
	}

	if (!CombatStatusManager)
	{
		UE_LOG(LogCombatSystemManager, Warning,
			TEXT("CleanseAll skipped on %s because no CombatStatusManager was found."),
			*GetNameSafe(GetOwner()));
		return;
	}

	CombatStatusManager->CleanseAll(Target);
}

void UCombatSystemManagerComponent::CacheCombatManagers()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		CombatManager = nullptr;
		CombatStatusManager = nullptr;
		return;
	}

	CombatManager = Owner->FindComponentByClass<UCombatManager>();
	CombatStatusManager = Owner->FindComponentByClass<UCombatStatusManager>();
}

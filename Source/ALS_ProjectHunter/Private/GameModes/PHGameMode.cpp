#include "GameModes/PHGameMode.h"
#include "PHGameplayTags.h"
#include "Player/PHPlayerState.h"
#include "GameModes/PHGameState.h"
#include "Character/PHBaseCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogPHGameMode);

APHGameMode::APHGameMode()
{
	PlayerStateClass = APHPlayerState::StaticClass();
	GameStateClass   = APHGameState::StaticClass();
}

void APHGameMode::BeginPlay()
{
	Super::BeginPlay();
}

AActor* APHGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	return Super::ChoosePlayerStart_Implementation(Player);
}

void APHGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	if (APHBaseCharacter* HunterChar = Cast<APHBaseCharacter>(NewPlayer->GetPawn()))
	{
		UE_LOG(LogPHGameMode, Log,
			TEXT("HandleStartingNewPlayer: confirmed initialized pawn %s for %s"),
			*HunterChar->GetName(), *NewPlayer->GetName());
	}
}

bool APHGameMode::ReadyToStartMatch_Implementation()
{
	return true;
}


void APHGameMode::OnPlayerDied(AController* DeadPlayer, AController* Killer)
{
	if (!DeadPlayer)
	{
		return;
	}

	UE_LOG(LogPHGameMode, Log,
		TEXT("OnPlayerDied: %s killed by %s"),
		*GetNameSafe(DeadPlayer),
		*GetNameSafe(Killer));

	if (!bAutoRespawn)
	{

		return;
	}

	if (RespawnDelay <= 0.f)
	{
		RespawnPlayer(DeadPlayer);
		return;
	}

	FTimerHandle RespawnTimer;
	TWeakObjectPtr<AController> WeakController = DeadPlayer;

	GetWorldTimerManager().SetTimer(RespawnTimer,
		[this, WeakController]()
		{
			if (WeakController.IsValid())
			{
				RespawnPlayer(WeakController.Get());
			}
		},
		RespawnDelay,
		false);
}

void APHGameMode::RespawnPlayer(AController* Controller)
{
	if (!Controller)
	{
		return;
	}

	if (APawn* OldPawn = Controller->GetPawn())
	{
		OldPawn->Destroy();
	}

	RestartPlayer(Controller);

	UE_LOG(LogPHGameMode, Log,
		TEXT("RespawnPlayer: respawned %s"), *Controller->GetName());
}

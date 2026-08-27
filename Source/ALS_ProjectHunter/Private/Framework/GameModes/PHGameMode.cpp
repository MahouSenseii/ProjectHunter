#include "Framework/GameModes/PHGameMode.h"
#include "Tags/PHGameplayTags.h"
#include "Framework/Player/PHPlayerState.h"
#include "Framework/GameModes/PHGameState.h"
#include "Character/PHBaseCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"
#include "Tower/Subsystems/RunSubsystem.h"
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY(LogPHGameMode);

APHGameMode::APHGameMode()
{
	PlayerStateClass = APHPlayerState::StaticClass();
	GameStateClass   = APHGameState::StaticClass();
}

void APHGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (URunSubsystem* RunSubsystem = GameInstance->GetSubsystem<URunSubsystem>())
		{
			RunSubsystem->SyncToGameState();
		}
	}
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

	// Inside a run, death is a run event, not a respawn event. RunSubsystem
	// moves the player to Downed, leaves room for a teammate revive, and ends
	// the run on a party wipe. Solo, that is the first death.
	if (URunSubsystem* RunSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<URunSubsystem>()
		: nullptr)
	{
		if (RunSubsystem->IsRunActive())
		{
			if (APHPlayerState* HunterState = DeadPlayer->GetPlayerState<APHPlayerState>())
			{
				RunSubsystem->NotifyPlayerDowned(HunterState);
			}
			else
			{
				UE_LOG(LogPHGameMode, Warning,
					TEXT("OnPlayerDied: %s has no APHPlayerState; run death handling skipped."),
					*GetNameSafe(DeadPlayer));
			}
			return;
		}
	}

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

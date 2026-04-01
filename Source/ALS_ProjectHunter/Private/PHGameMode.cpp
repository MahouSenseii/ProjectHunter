// PHGameMode.cpp
#include "PHGameMode.h"
#include "PHGameplayTags.h"
#include "PHPlayerState.h"
#include "PHGameState.h"
#include "Character/PHBaseCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"

APHGameMode::APHGameMode()
{
	// N-01/N-02 FIX: Wire custom PlayerState and GameState classes
	PlayerStateClass = APHPlayerState::StaticClass();
	GameStateClass   = APHGameState::StaticClass();
}

void APHGameMode::BeginPlay()
{
	Super::BeginPlay();
}

// ─────────────────────────────────────────────────────────────────────────────
// Player spawn
// ─────────────────────────────────────────────────────────────────────────────

AActor* APHGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Default behaviour — pick a random PlayerStart.
	// Override in a Blueprint subclass or C++ subclass to implement
	// checkpoint / safe-zone respawn logic.
	return Super::ChoosePlayerStart_Implementation(Player);
}

void APHGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	// The pawn should already be possessed by now.
	// Apply startup effects and abilities if the pawn is a PHBaseCharacter.
	if (APHBaseCharacter* HunterChar = Cast<APHBaseCharacter>(NewPlayer->GetPawn()))
	{
		HunterChar->GiveDefaultAbilities();
		HunterChar->ApplyStartupEffects();

		UE_LOG(LogTemp, Log,
			TEXT("HandleStartingNewPlayer: initialized %s for %s"),
			*HunterChar->GetName(), *NewPlayer->GetName());
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Match lifecycle
// ─────────────────────────────────────────────────────────────────────────────

bool APHGameMode::ReadyToStartMatch_Implementation()
{
	// Single-player: always ready.
	// For multiplayer: override to check NumPlayers >= MinPlayers, etc.
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Player death / respawn
// ─────────────────────────────────────────────────────────────────────────────

void APHGameMode::OnPlayerDied(AController* DeadPlayer, AController* Killer)
{
	if (!DeadPlayer)
	{
		return;
	}

	UE_LOG(LogTemp, Log,
		TEXT("OnPlayerDied: %s killed by %s"),
		*GetNameSafe(DeadPlayer),
		*GetNameSafe(Killer));

	if (!bAutoRespawn)
	{
		// Manual respawn — the player must press a button.
		// UI / Blueprint handles showing the respawn prompt.
		return;
	}

	if (RespawnDelay <= 0.f)
	{
		RespawnPlayer(DeadPlayer);
		return;
	}

	// Delayed respawn via timer
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

	// Destroy the old pawn if still alive
	if (APawn* OldPawn = Controller->GetPawn())
	{
		OldPawn->Destroy();
	}

	// Use the engine's restart flow which calls ChoosePlayerStart + SpawnDefaultPawnFor
	RestartPlayer(Controller);

	UE_LOG(LogTemp, Log,
		TEXT("RespawnPlayer: respawned %s"), *Controller->GetName());
}

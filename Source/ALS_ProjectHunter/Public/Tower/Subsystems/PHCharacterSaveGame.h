// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PHCharacterSaveGame.generated.h"

/**
 * Everything a character loses when it dies (GAME_DESIGN §4).
 *
 * This object is deliberately the *only* place character-owned progression is persisted, and it is
 * written to its own save slot. The persistent Chest lives in separate slots owned by
 * UStashSubsystem, so retiring a character can delete this and physically cannot reach secured
 * items - the separation the design asks for is a property of the storage layout, not of the code
 * remembering to be careful.
 *
 * Most of what §4 lists - Constellation relationships and grudges, sponsorships, the Decision
 * Profile, scenario history, NPC relationships, titles, discoveries - has no system yet. Those
 * fields attach here as they are built, and the retirement path already covers them by construction.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHCharacterSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentSaveVersion = 1;

	UPROPERTY()
	int32 SaveVersion = CurrentSaveVersion;

	/** Stable identity. The save slot name is derived from this, never from the display name. */
	UPROPERTY()
	FGuid CharacterID;

	UPROPERTY()
	FString CharacterName;

	// ---- Progression (UCharacterProgressionManager) ----------------------

	UPROPERTY()
	int32 Level = 0;

	UPROPERTY()
	int64 CurrentXP = 0;

	UPROPERTY()
	int32 UnspentStatPoints = 0;

	UPROPERTY()
	int32 TotalStatPoints = 0;

	UPROPERTY()
	int32 UnspentSkillPoints = 0;

	UPROPERTY()
	int32 UnspentPassivePoints = 0;

	UPROPERTY()
	int32 TotalPassivePoints = 0;

	// ---- Bookkeeping -----------------------------------------------------

	/** Set when the character was created, for sorting a character select list. */
	UPROPERTY()
	FDateTime CreatedAtUtc;

	UPROPERTY()
	FDateTime LastPlayedUtc;

	/** Runs this character survived. Reaching a hub or completing a Gate increments it. */
	UPROPERTY()
	int32 RunsCompleted = 0;
};

/**
 * The list of living characters on this profile.
 *
 * Kept beside the character saves rather than inside them so enumerating a character select screen
 * does not have to load every character, and so a retirement is one entry removed plus one slot
 * deleted.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHCharacterIndexSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentSaveVersion = 1;

	UPROPERTY()
	int32 SaveVersion = CurrentSaveVersion;

	/** IDs of characters that are alive. A retired character is removed from this list. */
	UPROPERTY()
	TArray<FGuid> LivingCharacterIDs;
};

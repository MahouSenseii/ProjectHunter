// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tower/Library/Enums/RunEnumLibrary.h"
#include "Tower/Library/Structs/RunStructs.h"
#include "PHCharacterSubsystem.generated.h"

class UPHCharacterSaveGame;
class UCharacterProgressionManager;

DECLARE_LOG_CATEGORY_EXTERN(LogPHCharacter, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterLoaded, const UPHCharacterSaveGame*, Character);
/** The character named here no longer exists. Anything holding character state must drop it. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterRetired, FGuid, CharacterID, ERunEndReason, Reason);

/**
 * The one owner of character identity and persistence, and the half of hardcore death that deletes
 * (GAME_DESIGN §4, AI_RULES §39).
 *
 * Two rules shape everything here:
 *
 * **The Chest is not character data.** Character saves are keyed by character GUID; the persistent
 * Chest is keyed by the profile and outlives every character on it. Retirement deletes character
 * slots by name and has no code path that can reach a stash slot, so the separation holds even if a
 * future caller is careless.
 *
 * **Only a legitimate death retires a character.** Quitting, disconnecting, an invalid run, a crash
 * or a power cut must all leave the character alive - §4 is explicit that a crash is not a death.
 * Retirement is therefore an explicit call made on an explicit run-end reason, never anything that
 * happens on shutdown, and never a timeout.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHCharacterSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---- Identity --------------------------------------------------------

	/** Creates a character, makes it active, and writes it immediately so it survives a crash. */
	UFUNCTION(BlueprintCallable, Category = "Character")
	UPHCharacterSaveGame* CreateCharacter(const FString& CharacterName);

	/** Loads a character by ID and makes it active. Returns null when the slot is missing. */
	UFUNCTION(BlueprintCallable, Category = "Character")
	UPHCharacterSaveGame* LoadCharacter(FGuid CharacterID);

	UFUNCTION(BlueprintPure, Category = "Character")
	UPHCharacterSaveGame* GetActiveCharacter() const { return ActiveCharacter; }

	UFUNCTION(BlueprintPure, Category = "Character")
	bool HasActiveCharacter() const { return ActiveCharacter != nullptr; }

	/** Living characters on this profile, newest first. */
	UFUNCTION(BlueprintPure, Category = "Character")
	TArray<FGuid> GetLivingCharacterIDs() const;

	// ---- Persistence -----------------------------------------------------

	/** Copies progression off the component and writes the active character's slot. */
	UFUNCTION(BlueprintCallable, Category = "Character")
	bool SaveActiveCharacter(const UCharacterProgressionManager* Progression = nullptr);

	/**
	 * Ends a character permanently: removes it from the living index and deletes its slot.
	 *
	 * Refuses any reason that is not a death, because §4 draws that line and this is the only place
	 * that can act on it. Returns false when nothing was retired.
	 */
	UFUNCTION(BlueprintCallable, Category = "Character")
	bool RetireActiveCharacter(ERunEndReason Reason);

	/** True for the reasons that count as a legitimate death. Quitting and disconnecting do not. */
	UFUNCTION(BlueprintPure, Category = "Character")
	static bool IsLegitimateDeath(ERunEndReason Reason);

	// ---- Chest separation ------------------------------------------------

	/**
	 * Slot prefix the persistent Chest is stored under. Profile-wide on purpose: the Chest is what
	 * survives a character, so keying it by character would delete the one thing death must spare.
	 *
	 * Hand this to UStashSubsystem::LoadStashHandles rather than a character name.
	 */
	UFUNCTION(BlueprintPure, Category = "Character|Chest")
	FString GetProfileStashSlotName() const { return ProfileName; }

	// ---- Events ----------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Character|Events")
	FOnCharacterLoaded OnCharacterLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Character|Events")
	FOnCharacterRetired OnCharacterRetired;

	/** Slot name a character's data lives under. Exposed so tests can assert what was deleted. */
	UFUNCTION(BlueprintPure, Category = "Character")
	static FString MakeCharacterSlotName(FGuid CharacterID);

private:
	UPROPERTY()
	TObjectPtr<UPHCharacterSaveGame> ActiveCharacter;

	/** Profile the Chest and the character index belong to. One profile, many characters. */
	FString ProfileName = TEXT("Profile");

	UFUNCTION()
	void HandleRunEnded(FRunSessionData SessionData);

	FString MakeIndexSlotName() const;
	TArray<FGuid> LoadLivingIDs() const;
	void WriteLivingIDs(const TArray<FGuid>& IDs) const;
};

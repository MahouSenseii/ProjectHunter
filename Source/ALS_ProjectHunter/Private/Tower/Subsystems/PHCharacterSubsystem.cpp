// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "Tower/Subsystems/PHCharacterSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Progression/Components/CharacterProgressionManager.h"
#include "Tower/Subsystems/PHCharacterSaveGame.h"
#include "Tower/Subsystems/RunSubsystem.h"

DEFINE_LOG_CATEGORY(LogPHCharacter);

void UPHCharacterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Retirement is driven off the run owner's end reason rather than off a death event on the
	// pawn: the run owner is what distinguishes a death from a quit, and §4 turns on exactly that.
	if (URunSubsystem* Run = GetGameInstance()->GetSubsystem<URunSubsystem>())
	{
		Run->OnRunEnded.AddUniqueDynamic(this, &UPHCharacterSubsystem::HandleRunEnded);
	}
}

void UPHCharacterSubsystem::Deinitialize()
{
	// Deliberately does not save and deliberately does not retire. Shutdown is not a death, and a
	// crash never reaches this function anyway - a character that only persisted here would be lost
	// by the very event §4 says must not cost anything.
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (URunSubsystem* Run = GameInstance->GetSubsystem<URunSubsystem>())
		{
			Run->OnRunEnded.RemoveDynamic(this, &UPHCharacterSubsystem::HandleRunEnded);
		}
	}

	Super::Deinitialize();
}

FString UPHCharacterSubsystem::MakeCharacterSlotName(const FGuid CharacterID)
{
	// Keyed by GUID, never by display name: two characters may share a name, and a rename must not
	// orphan a save or collide with someone else's.
	return TEXT("Character_") + CharacterID.ToString(EGuidFormats::Digits);
}

FString UPHCharacterSubsystem::MakeIndexSlotName() const
{
	return ProfileName + TEXT("_Characters");
}

bool UPHCharacterSubsystem::IsLegitimateDeath(const ERunEndReason Reason)
{
	// Quit, Disconnect and InvalidRun are excluded on purpose. §4: "A crash or power loss must not
	// count as legitimate character death", and a disconnect is indistinguishable from one.
	return Reason == ERunEndReason::PlayerDeath || Reason == ERunEndReason::PartyWipe;
}

TArray<FGuid> UPHCharacterSubsystem::LoadLivingIDs() const
{
	const FString Slot = MakeIndexSlotName();
	if (!UGameplayStatics::DoesSaveGameExist(Slot, 0))
	{
		return {};
	}

	const UPHCharacterIndexSaveGame* Index =
		Cast<UPHCharacterIndexSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	if (!Index)
	{
		UE_LOG(LogPHCharacter, Warning, TEXT("Character index '%s' could not be read."), *Slot);
		return {};
	}

	if (Index->SaveVersion > UPHCharacterIndexSaveGame::CurrentSaveVersion)
	{
		// Refused rather than partially read: a newer index may list characters whose slots this
		// build would mis-parse, and guessing risks deleting one.
		UE_LOG(LogPHCharacter, Warning,
			TEXT("Character index version %d is newer than supported %d; ignoring it."),
			Index->SaveVersion, UPHCharacterIndexSaveGame::CurrentSaveVersion);
		return {};
	}

	return Index->LivingCharacterIDs;
}

void UPHCharacterSubsystem::WriteLivingIDs(const TArray<FGuid>& IDs) const
{
	UPHCharacterIndexSaveGame* Index = Cast<UPHCharacterIndexSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UPHCharacterIndexSaveGame::StaticClass()));
	if (!Index)
	{
		return;
	}

	Index->LivingCharacterIDs = IDs;
	UGameplayStatics::SaveGameToSlot(Index, MakeIndexSlotName(), 0);
}

TArray<FGuid> UPHCharacterSubsystem::GetLivingCharacterIDs() const
{
	return LoadLivingIDs();
}

UPHCharacterSaveGame* UPHCharacterSubsystem::CreateCharacter(const FString& CharacterName)
{
	UPHCharacterSaveGame* Character = Cast<UPHCharacterSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UPHCharacterSaveGame::StaticClass()));
	if (!Character)
	{
		return nullptr;
	}

	Character->CharacterID = FGuid::NewGuid();
	Character->CharacterName = CharacterName.TrimStartAndEnd();
	if (Character->CharacterName.IsEmpty())
	{
		Character->CharacterName = TEXT("Hunter");
	}
	Character->CreatedAtUtc = FDateTime::UtcNow();
	Character->LastPlayedUtc = Character->CreatedAtUtc;

	// Written before it is returned. A character that exists only in memory is one crash away from
	// never having existed, and the player would have lost progress to something that is not death.
	const FString Slot = MakeCharacterSlotName(Character->CharacterID);
	if (!UGameplayStatics::SaveGameToSlot(Character, Slot, 0))
	{
		UE_LOG(LogPHCharacter, Warning, TEXT("Could not write new character to '%s'."), *Slot);
		return nullptr;
	}

	TArray<FGuid> Living = LoadLivingIDs();
	Living.Add(Character->CharacterID);
	WriteLivingIDs(Living);

	ActiveCharacter = Character;
	UE_LOG(LogPHCharacter, Log, TEXT("Created character '%s' (%s)."),
		*Character->CharacterName, *Character->CharacterID.ToString(EGuidFormats::Digits));

	OnCharacterLoaded.Broadcast(ActiveCharacter);
	return ActiveCharacter;
}

UPHCharacterSaveGame* UPHCharacterSubsystem::LoadCharacter(const FGuid CharacterID)
{
	const FString Slot = MakeCharacterSlotName(CharacterID);
	if (!UGameplayStatics::DoesSaveGameExist(Slot, 0))
	{
		UE_LOG(LogPHCharacter, Warning, TEXT("No save for character %s."),
			*CharacterID.ToString(EGuidFormats::Digits));
		return nullptr;
	}

	UPHCharacterSaveGame* Character =
		Cast<UPHCharacterSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	if (!Character || Character->SaveVersion > UPHCharacterSaveGame::CurrentSaveVersion)
	{
		UE_LOG(LogPHCharacter, Warning, TEXT("Character slot '%s' is unreadable or too new."), *Slot);
		return nullptr;
	}

	Character->LastPlayedUtc = FDateTime::UtcNow();
	ActiveCharacter = Character;
	OnCharacterLoaded.Broadcast(ActiveCharacter);
	return ActiveCharacter;
}

bool UPHCharacterSubsystem::SaveActiveCharacter(const UCharacterProgressionManager* Progression)
{
	if (!ActiveCharacter)
	{
		return false;
	}

	// Progression is optional so a caller with nothing live to read - a menu, a test - can still
	// flush bookkeeping without inventing zeroes for a character's level.
	if (Progression)
	{
		ActiveCharacter->Level = Progression->Level;
		ActiveCharacter->CurrentXP = Progression->CurrentXP;
		ActiveCharacter->UnspentStatPoints = Progression->UnspentStatPoints;
		ActiveCharacter->TotalStatPoints = Progression->TotalStatPoints;
		ActiveCharacter->UnspentSkillPoints = Progression->UnspentSkillPoints;
		ActiveCharacter->UnspentPassivePoints = Progression->UnspentPassivePoints;
		ActiveCharacter->TotalPassivePoints = Progression->TotalPassivePoints;
	}

	ActiveCharacter->LastPlayedUtc = FDateTime::UtcNow();
	return UGameplayStatics::SaveGameToSlot(
		ActiveCharacter, MakeCharacterSlotName(ActiveCharacter->CharacterID), 0);
}

bool UPHCharacterSubsystem::RetireActiveCharacter(const ERunEndReason Reason)
{
	if (!ActiveCharacter)
	{
		return false;
	}

	if (!IsLegitimateDeath(Reason))
	{
		UE_LOG(LogPHCharacter, Log,
			TEXT("Not retiring '%s': run ended for a reason that is not a death."),
			*ActiveCharacter->CharacterName);
		return false;
	}

	const FGuid RetiredID = ActiveCharacter->CharacterID;
	const FString RetiredName = ActiveCharacter->CharacterName;

	TArray<FGuid> Living = LoadLivingIDs();
	Living.Remove(RetiredID);
	WriteLivingIDs(Living);

	// Deletes exactly one slot, addressed by the character's own GUID. There is no path from here
	// to a stash slot, which is what makes "the Chest survives death" a property of the layout
	// rather than a promise this function has to keep.
	const FString Slot = MakeCharacterSlotName(RetiredID);
	UGameplayStatics::DeleteGameInSlot(Slot, 0);

	ActiveCharacter = nullptr;

	UE_LOG(LogPHCharacter, Log, TEXT("Retired '%s' (%s). The Chest is untouched."),
		*RetiredName, *RetiredID.ToString(EGuidFormats::Digits));

	OnCharacterRetired.Broadcast(RetiredID, Reason);
	return true;
}

void UPHCharacterSubsystem::HandleRunEnded(FRunSessionData SessionData)
{
	if (IsLegitimateDeath(SessionData.EndReason))
	{
		RetireActiveCharacter(SessionData.EndReason);
		return;
	}

	// Survived the run, so the character keeps what it earned. Saving here rather than on shutdown
	// means a crash costs at most the current run, never the character.
	if (ActiveCharacter)
	{
		++ActiveCharacter->RunsCompleted;
		SaveActiveCharacter();
	}
}

// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "GameFramework/SaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tower/Subsystems/PHCharacterSaveGame.h"
#include "Tower/Subsystems/PHCharacterSubsystem.h"

namespace PHCharacterPersistenceTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	/**
	 * A stand-in for a secured Chest slot.
	 *
	 * The point of the separation test is that retirement deletes character slots and nothing else,
	 * so the Chest is represented by a save in a slot retirement has no reason to touch. Using the
	 * real UStashSubsystem would test the stash instead of the boundary.
	 */
	const FString ChestSlot = TEXT("Profile_Stash_TestTab");

	void WriteChest()
	{
		USaveGame* Chest = UGameplayStatics::CreateSaveGameObject(UPHCharacterIndexSaveGame::StaticClass());
		UGameplayStatics::SaveGameToSlot(Chest, ChestSlot, 0);
	}

	void ClearChest()
	{
		if (UGameplayStatics::DoesSaveGameExist(ChestSlot, 0))
		{
			UGameplayStatics::DeleteGameInSlot(ChestSlot, 0);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHCharacterDeathSparesTheChestTest,
	"ProjectHunter.Character.Persistence.DeathSparesTheChest",
	PHCharacterPersistenceTests::TestFlags)

bool FPHCharacterDeathSparesTheChestTest::RunTest(const FString&)
{
	using namespace PHCharacterPersistenceTests;

	// GAME_DESIGN §4 and AI_RULES §39: dying costs the character everything and costs the Chest
	// nothing. This is the single most important rule in the design, and the one most likely to be
	// broken silently by someone tidying up save slots.
	FTestWorldWrapper TestWorld;
	if (!TestWorld.CreateTestWorld(EWorldType::Game))
	{
		TestWorld.ForwardErrorMessages(this);
		return false;
	}

	UGameInstance* GameInstance = TestWorld.GetTestWorld()->GetGameInstance();
	if (!TestNotNull(TEXT("The test world has a game instance"), GameInstance))
	{
		return false;
	}

	UPHCharacterSubsystem* Characters = GameInstance->GetSubsystem<UPHCharacterSubsystem>();
	if (!TestNotNull(TEXT("The character owner exists"), Characters))
	{
		return false;
	}

	ClearChest();
	WriteChest();

	const UPHCharacterSaveGame* Character = Characters->CreateCharacter(TEXT("Doomed"));
	if (!TestNotNull(TEXT("A character can be created"), Character))
	{
		ClearChest();
		return false;
	}

	const FGuid CharacterID = Character->CharacterID;
	const FString CharacterSlot = UPHCharacterSubsystem::MakeCharacterSlotName(CharacterID);

	TestTrue(TEXT("Creating a character writes it immediately, so a crash cannot lose it"),
		UGameplayStatics::DoesSaveGameExist(CharacterSlot, 0));
	TestTrue(TEXT("The character is listed as living"),
		Characters->GetLivingCharacterIDs().Contains(CharacterID));

	TestTrue(TEXT("A legitimate death retires the character"),
		Characters->RetireActiveCharacter(ERunEndReason::PlayerDeath));

	TestFalse(TEXT("The character's save is gone"),
		UGameplayStatics::DoesSaveGameExist(CharacterSlot, 0));
	TestFalse(TEXT("The character is no longer listed as living"),
		Characters->GetLivingCharacterIDs().Contains(CharacterID));
	TestFalse(TEXT("Nothing is active after a death"), Characters->HasActiveCharacter());

	// The whole point.
	TestTrue(TEXT("The Chest survives the character's death"),
		UGameplayStatics::DoesSaveGameExist(ChestSlot, 0));

	ClearChest();
	TestWorld.ForwardErrorMessages(this);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHCharacterCrashIsNotDeathTest,
	"ProjectHunter.Character.Persistence.OnlyDeathRetires",
	PHCharacterPersistenceTests::TestFlags)

bool FPHCharacterCrashIsNotDeathTest::RunTest(const FString&)
{
	using namespace PHCharacterPersistenceTests;

	// §4: "A crash or power loss must not count as legitimate character death." A quit and a
	// disconnect are indistinguishable from one, so neither may retire anything.
	FTestWorldWrapper TestWorld;
	if (!TestWorld.CreateTestWorld(EWorldType::Game))
	{
		TestWorld.ForwardErrorMessages(this);
		return false;
	}

	UGameInstance* GameInstance = TestWorld.GetTestWorld()->GetGameInstance();
	UPHCharacterSubsystem* Characters =
		GameInstance ? GameInstance->GetSubsystem<UPHCharacterSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("The character owner exists"), Characters))
	{
		return false;
	}

	const ERunEndReason Survivable[] = {
		ERunEndReason::Quit, ERunEndReason::Disconnect, ERunEndReason::InvalidRun,
		ERunEndReason::Completed, ERunEndReason::Extracted, ERunEndReason::None };

	for (const ERunEndReason Reason : Survivable)
	{
		const UPHCharacterSaveGame* Character = Characters->CreateCharacter(TEXT("Survivor"));
		if (!TestNotNull(TEXT("A character can be created"), Character))
		{
			return false;
		}

		const FGuid ID = Character->CharacterID;
		const FString Slot = UPHCharacterSubsystem::MakeCharacterSlotName(ID);

		TestFalse(FString::Printf(TEXT("Reason %d does not retire"), static_cast<int32>(Reason)),
			Characters->RetireActiveCharacter(Reason));
		TestTrue(FString::Printf(TEXT("Reason %d leaves the save in place"), static_cast<int32>(Reason)),
			UGameplayStatics::DoesSaveGameExist(Slot, 0));
		TestTrue(FString::Printf(TEXT("Reason %d leaves the character living"), static_cast<int32>(Reason)),
			Characters->GetLivingCharacterIDs().Contains(ID));

		// Clean up through the death path, which is the only thing allowed to delete.
		Characters->RetireActiveCharacter(ERunEndReason::PlayerDeath);
	}

	TestTrue(TEXT("PlayerDeath counts as death"),
		UPHCharacterSubsystem::IsLegitimateDeath(ERunEndReason::PlayerDeath));
	TestTrue(TEXT("PartyWipe counts as death"),
		UPHCharacterSubsystem::IsLegitimateDeath(ERunEndReason::PartyWipe));
	TestFalse(TEXT("Quitting does not"),
		UPHCharacterSubsystem::IsLegitimateDeath(ERunEndReason::Quit));
	TestFalse(TEXT("Disconnecting does not"),
		UPHCharacterSubsystem::IsLegitimateDeath(ERunEndReason::Disconnect));

	TestWorld.ForwardErrorMessages(this);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHCharacterRoundTripTest,
	"ProjectHunter.Character.Persistence.SurvivesAReload",
	PHCharacterPersistenceTests::TestFlags)

bool FPHCharacterRoundTripTest::RunTest(const FString&)
{
	using namespace PHCharacterPersistenceTests;

	FTestWorldWrapper TestWorld;
	if (!TestWorld.CreateTestWorld(EWorldType::Game))
	{
		TestWorld.ForwardErrorMessages(this);
		return false;
	}

	UGameInstance* GameInstance = TestWorld.GetTestWorld()->GetGameInstance();
	UPHCharacterSubsystem* Characters =
		GameInstance ? GameInstance->GetSubsystem<UPHCharacterSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("The character owner exists"), Characters))
	{
		return false;
	}

	UPHCharacterSaveGame* Created = Characters->CreateCharacter(TEXT("Persistent"));
	if (!TestNotNull(TEXT("A character can be created"), Created))
	{
		return false;
	}

	const FGuid ID = Created->CharacterID;
	Created->Level = 12;
	Created->CurrentXP = 3456;
	Created->UnspentPassivePoints = 7;
	Created->TotalPassivePoints = 12;
	TestTrue(TEXT("The character writes"), Characters->SaveActiveCharacter());

	// Loading by ID has to reconstruct it from disk, not hand back the object still in memory.
	const UPHCharacterSaveGame* Loaded = Characters->LoadCharacter(ID);
	if (!TestNotNull(TEXT("The character loads back"), Loaded))
	{
		Characters->RetireActiveCharacter(ERunEndReason::PlayerDeath);
		return false;
	}

	TestEqual(TEXT("Name survives"), Loaded->CharacterName, FString(TEXT("Persistent")));
	TestEqual(TEXT("Level survives"), Loaded->Level, 12);
	TestEqual(TEXT("XP survives"), Loaded->CurrentXP, static_cast<int64>(3456));
	TestEqual(TEXT("Unspent passive points survive"), Loaded->UnspentPassivePoints, 7);
	TestEqual(TEXT("Total passive points survive"), Loaded->TotalPassivePoints, 12);
	TestEqual(TEXT("Identity survives"), Loaded->CharacterID, ID);

	// A missing character is a null, not a crash and not an empty character that overwrites a slot.
	TestNull(TEXT("An unknown character does not load"),
		Characters->LoadCharacter(FGuid::NewGuid()));

	Characters->LoadCharacter(ID);
	Characters->RetireActiveCharacter(ERunEndReason::PlayerDeath);
	TestWorld.ForwardErrorMessages(this);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "CoreMinimal.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Effects/HunterGE_DerivedPrimaryVitals.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Progression/Components/CharacterProgressionManager.h"
#include "Stats/Components/StatsManager.h"
#include "Stats/Data/BaseStatsData.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace PHBaseStatsLevelTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	constexpr float Tolerance = 0.001f;

	void AddRow(UBaseStatsData& Data, const FName Name, const float Value, const bool bOverride = true)
	{
		FStatInitializationEntry& Row = Data.BaseAttributes.AddDefaulted_GetRef();
		Row.StatName = Name;
		Row.BaseValue = Value;
		Row.bOverrideValue = bOverride;
	}

	UBaseStatsData* MakeData(const float Level, const bool bAuthorLevel = true, const bool bDerivedVitals = false)
	{
		UBaseStatsData* Data = NewObject<UBaseStatsData>();
		Data->SourceAttributeSetClass = UHunterAttributeSet::StaticClass();
		AddRow(*Data, TEXT("PlayerLevel"), Level, bAuthorLevel);
		AddRow(*Data, TEXT("MaxHealth"), 120.0f);
		AddRow(*Data, TEXT("MaxMana"), 80.0f);
		AddRow(*Data, TEXT("MaxStamina"), 90.0f);
		AddRow(*Data, TEXT("Strength"), 0.0f);
		AddRow(*Data, TEXT("Intelligence"), 0.0f);
		AddRow(*Data, TEXT("Endurance"), 0.0f);
		AddRow(*Data, TEXT("XPGainMultiplier"), 1.0f);
		AddRow(*Data, TEXT("XPPenalty"), 1.0f);
		if (bDerivedVitals)
		{
			Data->InitializationEffects.Add(UHunterGE_DerivedPrimaryVitals::StaticClass());
		}
		return Data;
	}

	struct FCharacterFixture
	{
		FTestWorldWrapper TestWorld;
		AActor* Owner = nullptr;
		UAbilitySystemComponent* ASC = nullptr;
		UHunterAttributeSet* Attributes = nullptr;
		UStatsManager* Stats = nullptr;
		UCharacterProgressionManager* Progression = nullptr;

		~FCharacterFixture()
		{
			if (Progression && Progression->HasBegunPlay())
			{
				Progression->EndPlay(EEndPlayReason::Destroyed);
			}
		}

		bool Initialize(FAutomationTestBase& Test, const bool bAddProgression = true)
		{
			// GamePreview avoids game-instance persistence subsystems; the world never begins play.
			if (!TestWorld.CreateTestWorld(EWorldType::GamePreview))
			{
				TestWorld.ForwardErrorMessages(&Test);
				return false;
			}
			Owner = TestWorld.GetTestWorld()->SpawnActor<AActor>();
			if (!Test.TestNotNull(TEXT("A native actor was spawned in the isolated world"), Owner))
			{
				return false;
			}
			ASC = NewObject<UAbilitySystemComponent>(Owner);
			Owner->AddInstanceComponent(ASC);
			ASC->RegisterComponent();
			Attributes = NewObject<UHunterAttributeSet>(Owner);
			ASC->AddAttributeSetSubobject(Attributes);
			ASC->InitAbilityActorInfo(Owner, Owner);

			Stats = NewObject<UStatsManager>(Owner);
			Owner->AddInstanceComponent(Stats);
			Stats->RegisterComponent();
			if (bAddProgression)
			{
				Progression = NewObject<UCharacterProgressionManager>(Owner);
				Owner->AddInstanceComponent(Progression);
				Progression->RegisterComponent();
			}
			return Test.TestTrue(TEXT("The fixture has authority"), Owner->HasAuthority()) &&
				Test.TestTrue(TEXT("The ASC registered the live HunterAttributeSet"),
					ASC->GetSet<UHunterAttributeSet>() == Attributes);
		}

		void BeginProgression()
		{
			// UActorComponent::BeginPlay requires registered component tick functions.
			Progression->RegisterAllComponentTickFunctions(true);
			Progression->BeginPlay();
		}

		void CheckLevel(FAutomationTestBase& Test, const int32 Expected, const FString& Context) const
		{
			Test.TestEqual(Context + TEXT(": progression owns the expected level"), Progression->Level, Expected);
			Test.TestEqual(Context + TEXT(": GAS mirrors the owned level"), Attributes->GetPlayerLevel(),
				static_cast<float>(Expected), Tolerance);
			const int64 ExpectedXP = Expected >= Progression->MaxLevel ? 0 : Progression->GetXPForLevel(Expected + 1);
			Test.TestEqual(Context + TEXT(": next-level XP cost matches the owned level"),
				Progression->XPToNextLevel, ExpectedXP);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBaseStatsAuthoredStartingLevelTest,
	"ProjectHunter.Stats.BaseStatsLevel.AuthoredStartingLevel", PHBaseStatsLevelTests::TestFlags)

bool FPHBaseStatsAuthoredStartingLevelTest::RunTest(const FString&)
{
	using namespace PHBaseStatsLevelTests;
	for (const int32 StartingLevel : {0, 6})
	{
		FCharacterFixture Fixture;
		if (!Fixture.Initialize(*this))
		{
			return false;
		}
		Fixture.Progression->Level = 2;
		UBaseStatsData* Data = MakeData(static_cast<float>(StartingLevel));
		Fixture.Stats->InitializeFromDataAsset(Data);
		Fixture.CheckLevel(*this, StartingLevel, TEXT("Stats initialization before progression startup"));
		Fixture.BeginProgression();
		Fixture.CheckLevel(*this, StartingLevel, TEXT("Progression startup after stats initialization"));
		TestTrue(TEXT("The authored starting level was consumed once"), Fixture.Progression->HasSeededStartingLevel());
		TestEqual(TEXT("Seeding does not award stat points"), Fixture.Progression->UnspentStatPoints, 0);
		TestEqual(TEXT("Seeding does not award skill points"), Fixture.Progression->UnspentSkillPoints, 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBaseStatsOptionalStartingLevelTest,
	"ProjectHunter.Stats.BaseStatsLevel.OptionalSeedingPreservesOwner", PHBaseStatsLevelTests::TestFlags)

bool FPHBaseStatsOptionalStartingLevelTest::RunTest(const FString&)
{
	using namespace PHBaseStatsLevelTests;
	for (const bool bProgressionFirst : {false, true})
	{
		FCharacterFixture Fixture;
		if (!Fixture.Initialize(*this))
		{
			return false;
		}
		Fixture.Progression->Level = 4;
		Fixture.Progression->bSeedStartingLevelFromStatsData = false;
		if (bProgressionFirst)
		{
			Fixture.BeginProgression();
		}
		Fixture.Stats->InitializeFromDataAsset(MakeData(9.0f));
		if (!bProgressionFirst)
		{
			Fixture.BeginProgression();
		}
		Fixture.CheckLevel(*this, 4, TEXT("Disabled data seeding"));
		Fixture.Stats->InitializeFromDataAsset(MakeData(12.0f));
		Fixture.CheckLevel(*this, 4, TEXT("Disabled seeding after a second stats initialization"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBaseStatsUnauthoredStartingLevelTest,
	"ProjectHunter.Stats.BaseStatsLevel.UnauthoredRowPreservesOwner", PHBaseStatsLevelTests::TestFlags)

bool FPHBaseStatsUnauthoredStartingLevelTest::RunTest(const FString&)
{
	using namespace PHBaseStatsLevelTests;
	FCharacterFixture Fixture;
	if (!Fixture.Initialize(*this))
	{
		return false;
	}
	Fixture.Progression->Level = 5;
	Fixture.BeginProgression();
	UBaseStatsData* Data = MakeData(77.0f, false);
	TestFalse(TEXT("An unticked PlayerLevel row is absent from the runtime map"),
		Data->GetAllStatsAsMap().Contains(TEXT("PlayerLevel")));
	Fixture.Stats->InitializeFromDataAsset(Data);
	Fixture.CheckLevel(*this, 5, TEXT("Unauthored PlayerLevel"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBaseStatsStartingLevelBoundsTest,
	"ProjectHunter.Stats.BaseStatsLevel.RoundsAndClampsAtTheOwner", PHBaseStatsLevelTests::TestFlags)

bool FPHBaseStatsStartingLevelBoundsTest::RunTest(const FString&)
{
	using namespace PHBaseStatsLevelTests;
	struct FCase { float Authored; int32 Minimum; int32 Maximum; int32 Expected; };
	const FCase Cases[] = {{2.49f, 0, 100, 2}, {2.5f, 0, 100, 3}, {-8.0f, 0, 100, 0},
		{250.0f, 0, 100, 100}, {1.0f, 5, 20, 5}};
	for (const FCase& Case : Cases)
	{
		FCharacterFixture Fixture;
		if (!Fixture.Initialize(*this))
		{
			return false;
		}
		Fixture.Progression->MinLevel = Case.Minimum;
		Fixture.Progression->MaxLevel = Case.Maximum;
		Fixture.BeginProgression();
		Fixture.Stats->InitializeFromDataAsset(MakeData(Case.Authored));
		Fixture.CheckLevel(*this, Case.Expected, FString::Printf(TEXT("Authored level %.2f"), Case.Authored));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBaseStatsDelayedStartingLevelTest,
	"ProjectHunter.Stats.BaseStatsLevel.DelayedDataUpdatesXPAndGAS", PHBaseStatsLevelTests::TestFlags)

bool FPHBaseStatsDelayedStartingLevelTest::RunTest(const FString&)
{
	using namespace PHBaseStatsLevelTests;
	FCharacterFixture Fixture;
	if (!Fixture.Initialize(*this))
	{
		return false;
	}
	Fixture.Progression->Level = 2;
	Fixture.BeginProgression();
	Fixture.CheckLevel(*this, 2, TEXT("Startup without stats data"));
	TestFalse(TEXT("Missing data leaves the initial seed pending"), Fixture.Progression->HasSeededStartingLevel());
	const int64 OriginalXPCost = Fixture.Progression->XPToNextLevel;
	Fixture.Progression->CurrentXP = 2;
	Fixture.Stats->InitializeFromDataAsset(MakeData(7.0f));
	Fixture.CheckLevel(*this, 7, TEXT("Data arriving after progression startup"));
	TestTrue(TEXT("The delayed level changes the XP cost"), Fixture.Progression->XPToNextLevel != OriginalXPCost);
	TestEqual(TEXT("Seeding does not spend accumulated XP"), Fixture.Progression->CurrentXP, int64{2});
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBaseStatsReinitializationPreservesEarnedLevelTest,
	"ProjectHunter.Stats.BaseStatsLevel.ReinitializationPreservesEarnedLevel", PHBaseStatsLevelTests::TestFlags)

bool FPHBaseStatsReinitializationPreservesEarnedLevelTest::RunTest(const FString&)
{
	using namespace PHBaseStatsLevelTests;
	FCharacterFixture Fixture;
	if (!Fixture.Initialize(*this))
	{
		return false;
	}
	UBaseStatsData* Data = MakeData(3.0f);
	Fixture.Stats->InitializeFromDataAsset(Data);
	Fixture.BeginProgression();
	Fixture.Progression->AwardExperience(Fixture.Progression->XPToNextLevel);
	Fixture.CheckLevel(*this, 4, TEXT("Level earned through the XP pipeline"));
	TestEqual(TEXT("The earned level awards stat points"), Fixture.Progression->UnspentStatPoints,
		Fixture.Progression->StatPointsPerLevel);
	TestEqual(TEXT("The earned level awards skill points"), Fixture.Progression->UnspentSkillPoints,
		Fixture.Progression->SkillPointsPerLevel);
	Fixture.Stats->InitializeFromDataAsset(Data);
	Fixture.CheckLevel(*this, 4, TEXT("Reinitialization with the original starting data"));
	TestEqual(TEXT("Reinitialization does not award extra stat points"), Fixture.Progression->UnspentStatPoints,
		Fixture.Progression->StatPointsPerLevel);
	TestEqual(TEXT("Reinitialization leaves earned XP at zero"), Fixture.Progression->CurrentXP, int64{0});

	FCharacterFixture LateDataFixture;
	if (!LateDataFixture.Initialize(*this))
	{
		return false;
	}
	LateDataFixture.BeginProgression();
	TestFalse(TEXT("No data leaves the starting-level seed pending"),
		LateDataFixture.Progression->HasSeededStartingLevel());
	LateDataFixture.Progression->AwardExperience(LateDataFixture.Progression->XPToNextLevel);
	LateDataFixture.CheckLevel(*this, 1, TEXT("A level earned before stats data is available"));
	TestTrue(TEXT("Earning a level closes the starting-level seed window"),
		LateDataFixture.Progression->HasSeededStartingLevel());
	LateDataFixture.Stats->InitializeFromDataAsset(MakeData(7.0f));
	LateDataFixture.CheckLevel(*this, 1, TEXT("First stats data cannot replace an already earned level"));
	TestEqual(TEXT("Late first data retains earned stat points"), LateDataFixture.Progression->UnspentStatPoints,
		LateDataFixture.Progression->StatPointsPerLevel);
	TestEqual(TEXT("Late first data retains earned skill points"), LateDataFixture.Progression->UnspentSkillPoints,
		LateDataFixture.Progression->SkillPointsPerLevel);
	TestEqual(TEXT("Late first data does not fabricate XP"), LateDataFixture.Progression->CurrentXP, int64{0});
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBaseStatsLevelDerivedVitalsTest,
	"ProjectHunter.Stats.BaseStatsLevel.DerivedVitalsFollowLevelWithoutStacking", PHBaseStatsLevelTests::TestFlags)

bool FPHBaseStatsLevelDerivedVitalsTest::RunTest(const FString&)
{
	using namespace PHBaseStatsLevelTests;
	FCharacterFixture Fixture;
	if (!Fixture.Initialize(*this))
	{
		return false;
	}
	UBaseStatsData* Data = MakeData(0.0f, true, true);
	Fixture.Stats->InitializeFromDataAsset(Data);
	Fixture.BeginProgression();

	const auto CheckVitals = [this, &Fixture](const int32 Level)
	{
		// The three native max-vital MMCs each add 12 per level; fixture primaries are zero.
		const float LevelBonus = 12.0f * static_cast<float>(Level);
		TestEqual(TEXT("MaxHealth retains its authored base plus the level bonus"),
			Fixture.Attributes->GetMaxHealth(), 120.0f + LevelBonus, Tolerance);
		TestEqual(TEXT("MaxMana retains its authored base plus the level bonus"),
			Fixture.Attributes->GetMaxMana(), 80.0f + LevelBonus, Tolerance);
		TestEqual(TEXT("MaxStamina retains its authored base plus the level bonus"),
			Fixture.Attributes->GetMaxStamina(), 90.0f + LevelBonus, Tolerance);
		TestEqual(TEXT("There is exactly one derived-vitals effect"),
			Fixture.ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), 1);
	};
	CheckVitals(0);
	Fixture.Progression->AwardExperience(Fixture.Progression->XPToNextLevel);
	Fixture.CheckLevel(*this, 1, TEXT("First earned level"));
	CheckVitals(1);
	Fixture.Stats->InitializeFromDataAsset(Data);
	Fixture.CheckLevel(*this, 1, TEXT("Reinitialized level with derived effects"));
	CheckVitals(1);
	Fixture.Progression->LevelUp();
	Fixture.CheckLevel(*this, 2, TEXT("Second level through the public level-up API"));
	CheckVitals(2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHConfiguredBaseStatsStartingLevelTest,
	"ProjectHunter.Stats.BaseStatsLevel.ConfiguredAssetPassesThrough", PHBaseStatsLevelTests::TestFlags)

bool FPHConfiguredBaseStatsStartingLevelTest::RunTest(const FString&)
{
	using namespace PHBaseStatsLevelTests;
	UBaseStatsData* Data = LoadObject<UBaseStatsData>(nullptr,
		TEXT("/Game/ProjectHunter/Gameplay/Stats/DA_BaseStats.DA_BaseStats"));
	if (!TestNotNull(TEXT("The project's configured DA_BaseStats loads"), Data))
	{
		return false;
	}
	float AuthoredLevel = 0.0f;
	if (!TestTrue(TEXT("DA_BaseStats explicitly authors PlayerLevel"), Data->GetStatValue(TEXT("PlayerLevel"), AuthoredLevel)) ||
		!TestTrue(TEXT("The configured starting level is finite"), FMath::IsFinite(AuthoredLevel)))
	{
		return false;
	}
	const bool bPackageWasDirty = Data->GetOutermost()->IsDirty();
	AddInfo(FString::Printf(TEXT("DA_BaseStats authors PlayerLevel=%g."), AuthoredLevel));
	FCharacterFixture Fixture;
	if (!Fixture.Initialize(*this))
	{
		return false;
	}
	Fixture.BeginProgression();
	Fixture.Stats->InitializeFromDataAsset(Data);
	const int32 Expected = FMath::RoundToInt(FMath::Clamp(AuthoredLevel,
		static_cast<float>(Fixture.Progression->MinLevel), static_cast<float>(Fixture.Progression->MaxLevel)));
	Fixture.CheckLevel(*this, Expected, TEXT("Actual DA_BaseStats starting level"));
	float RetainedLevel = 0.0f;
	TestTrue(TEXT("Reading the starting level retains the authored row"), Data->GetStatValue(TEXT("PlayerLevel"), RetainedLevel));
	TestEqual(TEXT("Initialization does not edit the authored level"), RetainedLevel, AuthoredLevel);
	TestEqual(TEXT("Initialization does not dirty the source asset"), Data->GetOutermost()->IsDirty(), bPackageWasDirty);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBaseStatsWithoutProgressionTest,
	"ProjectHunter.Stats.BaseStatsLevel.WithoutProgressionInitializesGASDirectly", PHBaseStatsLevelTests::TestFlags)

bool FPHBaseStatsWithoutProgressionTest::RunTest(const FString&)
{
	using namespace PHBaseStatsLevelTests;
	FCharacterFixture Fixture;
	if (!Fixture.Initialize(*this, false))
	{
		return false;
	}
	TestNull(TEXT("The actor has no progression owner"),
		Fixture.Owner->FindComponentByClass<UCharacterProgressionManager>());
	Fixture.Stats->InitializeFromDataAsset(MakeData(6.0f));
	TestEqual(TEXT("Authored PlayerLevel still initializes actors without a progression component"),
		Fixture.Attributes->GetPlayerLevel(), 6.0f, Tolerance);
	Fixture.Stats->InitializeFromDataAsset(MakeData(0.0f));
	TestEqual(TEXT("Such actors can also explicitly start at level zero"),
		Fixture.Attributes->GetPlayerLevel(), 0.0f, Tolerance);
	return true;
}

#endif

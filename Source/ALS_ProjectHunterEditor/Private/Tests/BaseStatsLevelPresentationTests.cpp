// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Effects/HunterGE_DerivedPrimaryVitals.h"
#include "AbilitySystem/HunterAbilitySystemComponent.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "Character/PHBaseCharacter.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/AutomationTest.h"
#include "Progression/Components/CharacterProgressionManager.h"
#include "Stats/Components/StatsManager.h"
#include "Stats/Data/BaseStatsData.h"
#include "Tests/AutomationCommon.h"
#include "UI/HUD/HunterHUD_XPWidget.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace PHBaseStatsLevelPresentationTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	UBaseStatsData* MakeData(const float Level)
	{
		UBaseStatsData* Data = NewObject<UBaseStatsData>();
		Data->SourceAttributeSetClass = UHunterAttributeSet::StaticClass();
		const TMap<FName, float> Values = {{TEXT("PlayerLevel"), Level}, {TEXT("MaxHealth"), 120.0f},
			{TEXT("MaxMana"), 80.0f}, {TEXT("MaxStamina"), 90.0f}, {TEXT("Strength"), 0.0f},
			{TEXT("Intelligence"), 0.0f}, {TEXT("Endurance"), 0.0f},
			{TEXT("XPGainMultiplier"), 1.0f}, {TEXT("XPPenalty"), 1.0f}};
		for (const TPair<FName, float>& Value : Values)
		{
			FStatInitializationEntry& Row = Data->BaseAttributes.AddDefaulted_GetRef();
			Row.StatName = Value.Key;
			Row.BaseValue = Value.Value;
			Row.bOverrideValue = true;
		}
		Data->InitializationEffects.Add(UHunterGE_DerivedPrimaryVitals::StaticClass());
		return Data;
	}

	struct FCharacterFixture
	{
		TStrongObjectPtr<UBlueprint> TransientChild;
		FTestWorldWrapper TestWorld;
		APHBaseCharacter* Character = nullptr;
		UHunterAbilitySystemComponent* ASC = nullptr;
		UCharacterProgressionManager* Progression = nullptr;
		UStatsManager* Stats = nullptr;

		~FCharacterFixture()
		{
			if (ASC)
			{
				ASC->RemoveActiveEffects(FGameplayEffectQuery());
			}
		}

		bool Initialize(FAutomationTestBase& Test)
		{
			// The native character is abstract. This unsaved child adds no gameplay logic or content.
			const FName BlueprintName = MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(),
				TEXT("BP_BaseStatsLevelTest"));
			TransientChild.Reset(FKismetEditorUtilities::CreateBlueprint(APHBaseCharacter::StaticClass(),
				GetTransientPackage(), BlueprintName, BPTYPE_Normal));
			if (!Test.TestNotNull(TEXT("An empty transient character Blueprint was created"), TransientChild.Get()))
			{
				return false;
			}
			TransientChild->SetFlags(RF_Transient);
			TransientChild->ClearFlags(RF_Standalone);
			FKismetEditorUtilities::CompileBlueprint(TransientChild.Get(),
				EBlueprintCompileOptions::SkipSave | EBlueprintCompileOptions::SkipGarbageCollection |
				EBlueprintCompileOptions::SkipFiBSearchMetaUpdate);
			if (!Test.TestTrue(TEXT("The transient child compiled"),
				TransientChild->Status == BS_UpToDate || TransientChild->Status == BS_UpToDateWithWarnings) ||
				!Test.TestNotNull(TEXT("Compilation produced a character class"), TransientChild->GeneratedClass.Get()) ||
				!Test.TestFalse(TEXT("The generated child is concrete without changing class flags"),
					TransientChild->GeneratedClass->HasAnyClassFlags(CLASS_Abstract)))
			{
				return false;
			}
			if (!TestWorld.CreateTestWorld(EWorldType::GamePreview))
			{
				TestWorld.ForwardErrorMessages(&Test);
				return false;
			}
			Test.TestNull(TEXT("Preview setup creates no game instance or persistence subsystems"),
				TestWorld.GetTestWorld()->GetGameInstance());
			FActorSpawnParameters Parameters;
			Parameters.bDeferConstruction = true;
			Parameters.ObjectFlags = RF_Transient;
			Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Character = TestWorld.GetTestWorld()->SpawnActor<APHBaseCharacter>(
				TransientChild->GeneratedClass, FTransform::Identity, Parameters);
			if (!Test.TestNotNull(TEXT("The transient child spawned"), Character))
			{
				return false;
			}
			Character->AutoPossessPlayer = EAutoReceiveInput::Disabled;
			Character->AutoPossessAI = EAutoPossessAI::Disabled;
			Character->FinishSpawning(FTransform::Identity);
			if (!Character->IsActorInitialized())
			{
				Character->PreInitializeComponents();
				Character->InitializeComponents();
				Character->PostInitializeComponents();
			}
			ASC = Cast<UHunterAbilitySystemComponent>(Character->GetAbilitySystemComponent());
			Progression = Character->GetProgressionManager();
			Stats = Character->GetStatsManager();
			if (!Test.TestNotNull(TEXT("The character has its native ASC"), ASC) ||
				!Test.TestNotNull(TEXT("The character has its progression owner"), Progression) ||
				!Test.TestNotNull(TEXT("The character has its stats manager"), Stats))
			{
				return false;
			}
			// APawn's null-PlayerState notification is safe; no possession or BeginPlay is needed.
			Character->OnRep_PlayerState();
			return Test.TestTrue(TEXT("The character has authoritative gameplay state"), Character->HasAuthority()) &&
				Test.TestNotNull(TEXT("The real actor path registered its AttributeSet"), ASC->GetSet<UHunterAttributeSet>()) &&
				Test.TestFalse(TEXT("The actor never began play"), Character->HasActorBegunPlay()) &&
				Test.TestNull(TEXT("No controller was created"), Character->GetController());
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBaseStatsCharacterRefreshTest,
	"ProjectHunter.Stats.BaseStatsLevel.CharacterAbilitySystemRefreshPreservesZero",
	PHBaseStatsLevelPresentationTests::TestFlags)

bool FPHBaseStatsCharacterRefreshTest::RunTest(const FString&)
{
	using namespace PHBaseStatsLevelPresentationTests;
	FCharacterFixture Fixture;
	if (!Fixture.Initialize(*this))
	{
		return false;
	}
	Fixture.Stats->InitializeFromDataAsset(MakeData(0.0f));
	for (int32 Refresh = 0; Refresh < 3; ++Refresh)
	{
		Fixture.Character->OnRep_PlayerState();
		TestEqual(TEXT("Refreshing actor info preserves the level-zero progression owner"), Fixture.Progression->Level, 0);
		TestEqual(TEXT("Refreshing actor info preserves level zero in GAS"),
			Fixture.ASC->GetNumericAttribute(UHunterAttributeSet::GetPlayerLevelAttribute()), 0.0f);
		TestEqual(TEXT("Actor refresh leaves zero-level health at the authored base"),
			Fixture.ASC->GetNumericAttribute(UHunterAttributeSet::GetMaxHealthAttribute()), 120.0f);
		TestEqual(TEXT("Actor refresh does not add another derived-vitals effect"),
			Fixture.ASC->GetGameplayEffectCount(UHunterGE_DerivedPrimaryVitals::StaticClass(), Fixture.ASC, false), 1);
	}
	TestFalse(TEXT("Verification never enters character BeginPlay"), Fixture.Character->HasActorBegunPlay());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBaseStatsLateSeedPresentationTest,
	"ProjectHunter.Stats.BaseStatsLevel.XPWidgetTracksLateSeedAndRelease",
	PHBaseStatsLevelPresentationTests::TestFlags)

bool FPHBaseStatsLateSeedPresentationTest::RunTest(const FString&)
{
	using namespace PHBaseStatsLevelPresentationTests;
	FCharacterFixture Fixture;
	if (!Fixture.Initialize(*this))
	{
		return false;
	}
	Fixture.Progression->CurrentXP = 2;
	TStrongObjectPtr<UHunterHUD_XPWidget> Widget(NewObject<UHunterHUD_XPWidget>(GetTransientPackage()));
	Widget->InitializeForCharacter(Fixture.Character);
	TestTrue(TEXT("The native widget binds without a viewport"), Widget->IsBoundToCharacter());
	TestEqual(TEXT("The initial widget snapshot uses the owner's level"), Widget->GetCurrentLevel(), 0);
	TestEqual(TEXT("The initial widget snapshot includes current XP"), Widget->GetCurrentXP(), int64{2});

	Fixture.Stats->InitializeFromDataAsset(MakeData(7.0f));
	TestEqual(TEXT("Late seeding refreshes the existing widget level"), Widget->GetCurrentLevel(), 7);
	TestEqual(TEXT("Late seeding refreshes the existing widget threshold"),
		Widget->GetXPToNextLevel(), Fixture.Progression->GetXPForLevel(8));
	TestEqual(TEXT("Seeding does not spend the cached XP"), Widget->GetCurrentXP(), int64{2});
	TestEqual(TEXT("Starting at a higher level awards no stat points"), Fixture.Progression->UnspentStatPoints, 0);
	TestEqual(TEXT("Starting at a higher level awards no skill points"), Fixture.Progression->UnspentSkillPoints, 0);

	Widget->ReleaseCharacter();
	TestFalse(TEXT("Release clears the character binding"), Widget->IsBoundToCharacter());
	Fixture.Progression->MaxLevel = 8;
	Fixture.Progression->AwardExperience(Fixture.Progression->XPToNextLevel - Fixture.Progression->CurrentXP);
	TestEqual(TEXT("The real XP pipeline still advances the released character"), Fixture.Progression->Level, 8);
	TestEqual(TEXT("The released widget no longer observes level changes"), Widget->GetCurrentLevel(), 7);
	TestEqual(TEXT("The released widget no longer observes XP changes"), Widget->GetCurrentXP(), int64{2});

	Widget->InitializeForCharacter(Fixture.Character);
	TestEqual(TEXT("Rebinding snapshots the current earned level"), Widget->GetCurrentLevel(), 8);
	TestEqual(TEXT("Rebinding snapshots current XP"), Widget->GetCurrentXP(), int64{0});
	TestEqual(TEXT("At the level cap the rebound widget has no next-level XP cost"), Widget->GetXPToNextLevel(), int64{0});
	TestEqual(TEXT("At the level cap the rebound XP bar is full"), Widget->GetXPFillPercent(), 1.0f);
	Widget->ReleaseCharacter();
	return true;
}

#endif

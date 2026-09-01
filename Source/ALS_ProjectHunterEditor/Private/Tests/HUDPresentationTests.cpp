// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/HunterAbilitySystemComponent.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Character/PHBaseCharacter.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/AutomationTest.h"
#include "Progression/Components/CharacterProgressionManager.h"
#include "Stats/Components/StatsManager.h"
#include "Stats/Data/BaseStatsData.h"
#include "Tests/AutomationCommon.h"
#include "TimerManager.h"
#include "UI/HUD/HunterHUDResourceWidget.h"
#include "UI/HUD/HunterMainHUDWidget.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "WidgetBlueprint.h"

namespace PHHUDPresentationTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	UWidgetBlueprint* LoadHUD()
	{
		return LoadObject<UWidgetBlueprint>(nullptr,
			TEXT("/Game/ProjectHunter/UI/HUD/WBP_HunterHUD.WBP_HunterHUD"));
	}

	struct FCharacterFixture
	{
		TStrongObjectPtr<UBlueprint> CharacterBlueprint;
		FTestWorldWrapper TestWorld;
		APHBaseCharacter* Character = nullptr;
		UHunterAbilitySystemComponent* ASC = nullptr;
		UCharacterProgressionManager* Progression = nullptr;

		bool Initialize(FAutomationTestBase& Test)
		{
			CharacterBlueprint.Reset(FKismetEditorUtilities::CreateBlueprint(APHBaseCharacter::StaticClass(),
				GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(),
					TEXT("BP_HUDPresentationTest")), BPTYPE_Normal));
			if (!Test.TestNotNull(TEXT("A transient character Blueprint exists"), CharacterBlueprint.Get()))
			{
				return false;
			}
			CharacterBlueprint->SetFlags(RF_Transient);
			CharacterBlueprint->ClearFlags(RF_Standalone);
			FKismetEditorUtilities::CompileBlueprint(CharacterBlueprint.Get(),
				EBlueprintCompileOptions::SkipSave | EBlueprintCompileOptions::SkipGarbageCollection |
				EBlueprintCompileOptions::SkipFiBSearchMetaUpdate);
			if (!Test.TestNotNull(TEXT("The transient character class compiled"), CharacterBlueprint->GeneratedClass.Get()) ||
				!TestWorld.CreateTestWorld(EWorldType::GamePreview))
			{
				TestWorld.ForwardErrorMessages(&Test);
				return false;
			}
			FActorSpawnParameters Parameters;
			Parameters.bDeferConstruction = true;
			Parameters.ObjectFlags = RF_Transient;
			Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Character = TestWorld.GetTestWorld()->SpawnActor<APHBaseCharacter>(
				CharacterBlueprint->GeneratedClass, FTransform::Identity, Parameters);
			if (!Test.TestNotNull(TEXT("The real character base can be exercised without a gameplay map"), Character))
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
			Character->OnRep_PlayerState();
			ASC = Cast<UHunterAbilitySystemComponent>(Character->GetAbilitySystemComponent());
			Progression = Character->GetProgressionManager();
			return Test.TestNotNull(TEXT("The real character owns its ASC"), ASC) &&
				Test.TestNotNull(TEXT("The real character owns its progression component"), Progression) &&
				Test.TestNotNull(TEXT("The existing initialization path registers the AttributeSet"),
					ASC->GetSet<UHunterAttributeSet>());
		}

		void Seed(const int32 Level, const float MaxHealth, const float Health)
		{
			UBaseStatsData* Data = NewObject<UBaseStatsData>();
			Data->SourceAttributeSetClass = UHunterAttributeSet::StaticClass();
			const TMap<FName, float> Values = {{TEXT("PlayerLevel"), static_cast<float>(Level)},
				{TEXT("MaxHealth"), MaxHealth}, {TEXT("MaxMana"), 400.0f}, {TEXT("MaxStamina"), 100.0f},
				{TEXT("XPGainMultiplier"), 1.0f}, {TEXT("XPPenalty"), 1.0f}};
			for (const TPair<FName, float>& Value : Values)
			{
				FStatInitializationEntry& Row = Data->BaseAttributes.AddDefaulted_GetRef();
				Row.StatName = Value.Key;
				Row.BaseValue = Value.Value;
				Row.bOverrideValue = true;
			}
			Character->GetStatsManager()->InitializeFromDataAsset(Data);
			ASC->SetNumericAttributeBase(UHunterAttributeSet::GetHealthAttribute(), Health);
			ASC->SetNumericAttributeBase(UHunterAttributeSet::GetManaAttribute(), 300.0f);
			ASC->SetNumericAttributeBase(UHunterAttributeSet::GetStaminaAttribute(), 50.0f);
		}
	};

	UTextBlock* FindText(UHunterMainHUDWidget* HUD, const FName Name)
	{
		return HUD && HUD->WidgetTree ? Cast<UTextBlock>(HUD->WidgetTree->FindWidget(Name)) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHHUDStaminaPresentationTest,
	"ProjectHunter.HUD.Presentation.PreservesAuthoredStaminaAndBindings",
	PHHUDPresentationTests::TestFlags)

bool FPHHUDStaminaPresentationTest::RunTest(const FString&)
{
	using namespace PHHUDPresentationTests;
	UWidgetBlueprint* Blueprint = LoadHUD();
	if (!TestNotNull(TEXT("The authored HUD loads"), Blueprint) ||
		!TestNotNull(TEXT("The authoring tree exists"), Blueprint->WidgetTree.Get()))
	{
		return false;
	}
	const UHunterHUDResourceWidget* AuthoredStamina = Cast<UHunterHUDResourceWidget>(
		Blueprint->WidgetTree->FindWidget(TEXT("Stamina")));
	if (!TestNotNull(TEXT("The existing Stamina instance remains in the authoring tree"), AuthoredStamina))
	{
		return false;
	}
	FTestWorldWrapper TestWorld;
	if (!TestWorld.CreateTestWorld(EWorldType::GamePreview))
	{
		TestWorld.ForwardErrorMessages(this);
		return false;
	}
	UHunterMainHUDWidget* HUD = CreateWidget<UHunterMainHUDWidget>(TestWorld.GetTestWorld(), Blueprint->GeneratedClass.Get());
	if (!TestNotNull(TEXT("The original HUD class creates"), HUD))
	{
		return false;
	}
	TSharedPtr<SWidget> AliveSlate = HUD->TakeWidget();
	UHunterHUDResourceWidget* Stamina = HUD->GetStaminaWidget();
	if (!TestNotNull(TEXT("The original Stamina binding resolves"), Stamina))
	{
		return false;
	}
	TestEqual(TEXT("Stamina width survives PreConstruct"), Stamina->BarWidthOverride, AuthoredStamina->BarWidthOverride);
	TestEqual(TEXT("Stamina height survives PreConstruct"), Stamina->BarHeightOverride, AuthoredStamina->BarHeightOverride);
	TestTrue(TEXT("Stamina keeps its authored fill direction"), Stamina->BarFillType == AuthoredStamina->BarFillType);
	TestEqual(TEXT("Stamina keeps its authored color"), Stamina->CurrentFillColor, AuthoredStamina->CurrentFillColor);
	TestEqual(TEXT("Stamina keeps its authored interpolation"), Stamina->FillInterpSpeed, AuthoredStamina->FillInterpSpeed);
	TestEqual(TEXT("Stamina keeps its authored image-style switch"),
		Stamina->bApplyProgressBarImageStyle, AuthoredStamina->bApplyProgressBarImageStyle);
	TestEqual(TEXT("Stamina keeps its authored fill image"),
		Stamina->ProgressBarImageStyle.FillImage.GetResourceObject(),
		AuthoredStamina->ProgressBarImageStyle.FillImage.GetResourceObject());
	if (TestNotNull(TEXT("The live Stamina remains parented"), Stamina->GetParent()) &&
		TestNotNull(TEXT("The authored Stamina remains parented"), AuthoredStamina->GetParent()))
	{
		TestEqual(TEXT("Stamina remains under the same parent"),
			Stamina->GetParent()->GetFName(), AuthoredStamina->GetParent()->GetFName());
	}
	const UOverlaySlot* AuthoredSlot = Cast<UOverlaySlot>(AuthoredStamina->Slot);
	const UOverlaySlot* LiveSlot = Cast<UOverlaySlot>(Stamina->Slot);
	if (TestNotNull(TEXT("Stamina retains its authored Overlay slot"), AuthoredSlot) &&
		TestNotNull(TEXT("The live Stamina has the same slot type"), LiveSlot))
	{
		TestTrue(TEXT("Stamina padding is unchanged"), LiveSlot->GetPadding() == AuthoredSlot->GetPadding());
		TestTrue(TEXT("Stamina horizontal placement is unchanged"), LiveSlot->GetHorizontalAlignment() == AuthoredSlot->GetHorizontalAlignment());
		TestTrue(TEXT("Stamina vertical placement is unchanged"), LiveSlot->GetVerticalAlignment() == AuthoredSlot->GetVerticalAlignment());
	}
	for (UHunterHUDResourceWidget* Resource : {HUD->GetHealthWidget(), HUD->GetManaWidget(), Stamina})
	{
		if (!TestNotNull(TEXT("Every existing resource implementation resolves"), Resource))
		{
			return false;
		}
		for (const FName Name : {FName(TEXT("Bar_Current")), FName(TEXT("Bar_DamageLag")), FName(TEXT("Bar_Reserved"))})
		{
			TestNotNull(FString::Printf(TEXT("%s retains %s"), *Resource->GetName(), *Name.ToString()),
				Cast<UProgressBar>(Resource->WidgetTree->FindWidget(Name)));
		}
	}
	TestNotNull(TEXT("The requested level label exists"), FindText(HUD, TEXT("PlayerLevelText")));
	TestNotNull(TEXT("The requested current/max health label exists"), FindText(HUD, TEXT("HealthValueText")));
	for (const FName LabelName : {FName(TEXT("PlayerLevelText")), FName(TEXT("HealthValueText"))})
	{
		if (const UTextBlock* Label = FindText(HUD, LabelName))
		{
			TestNotNull(TEXT("Header fonts retain a serializable asset reference"), Label->GetFont().FontObject.Get());
		}
	}

	// Reconstruct with a deliberate designer change: the parent must not replace it with a preset.
	Stamina->SetSize(73.0f, 233.0f);
	AliveSlate.Reset();
	HUD->ReleaseSlateResources(true);
	AliveSlate = HUD->TakeWidget();
	TestEqual(TEXT("A custom Stamina width survives reconstruction"), Stamina->BarWidthOverride, 73.0f);
	TestEqual(TEXT("A custom Stamina height survives reconstruction"), Stamina->BarHeightOverride, 233.0f);
	const USizeBox* BarSize = Cast<USizeBox>(Stamina->WidgetTree->FindWidget(TEXT("BarSize")));
	if (TestNotNull(TEXT("Stamina still uses its existing SizeBox"), BarSize))
	{
		TestEqual(TEXT("The real SizeBox reflects the editable width"), BarSize->GetWidthOverride(), 73.0f);
		TestEqual(TEXT("The real SizeBox reflects the editable height"), BarSize->GetHeightOverride(), 233.0f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHHUDLivePresentationTest,
	"ProjectHunter.HUD.Presentation.LiveValuesRebindAndDelayedAttributes",
	PHHUDPresentationTests::TestFlags)

bool FPHHUDLivePresentationTest::RunTest(const FString&)
{
	using namespace PHHUDPresentationTests;
	FCharacterFixture First;
	FCharacterFixture Second;
	if (!First.Initialize(*this) || !Second.Initialize(*this))
	{
		return false;
	}
	First.Seed(3, 900.0f, 750.0f);
	Second.Seed(8, 600.0f, 450.0f);
	// The gameplay effects add level scaling to authored base pools. Compare presentation
	// against the actual ASC result, without duplicating or changing that calculation.
	const float FirstMaximum = First.ASC->GetNumericAttribute(UHunterAttributeSet::GetMaxEffectiveHealthAttribute());
	const float SecondMaximum = Second.ASC->GetNumericAttribute(UHunterAttributeSet::GetMaxEffectiveHealthAttribute());
	const auto HealthLabel = [](float Current, float Maximum)
	{
		return FString::Printf(TEXT("%.0f / %.0f"), Current, Maximum);
	};
	UWidgetBlueprint* Blueprint = LoadHUD();
	if (!TestNotNull(TEXT("The real player HUD Blueprint loads"), Blueprint))
	{
		return false;
	}
	UHunterMainHUDWidget* HUD = CreateWidget<UHunterMainHUDWidget>(First.TestWorld.GetTestWorld(), Blueprint->GeneratedClass.Get());
	if (!TestNotNull(TEXT("The real player HUD creates"), HUD))
	{
		return false;
	}
	UTextBlock* LevelText = FindText(HUD, TEXT("PlayerLevelText"));
	UTextBlock* HealthText = FindText(HUD, TEXT("HealthValueText"));
	UHunterHUDResourceWidget* Health = HUD->GetHealthWidget();
	UHunterHUDResourceWidget* Mana = HUD->GetManaWidget();
	UHunterHUDResourceWidget* Stamina = HUD->GetStaminaWidget();
	if (!TestNotNull(TEXT("Level text is bound"), LevelText) || !TestNotNull(TEXT("Health text is bound"), HealthText) ||
		!TestNotNull(TEXT("Health is bound"), Health) || !TestNotNull(TEXT("Mana is bound"), Mana) ||
		!TestNotNull(TEXT("Stamina is bound"), Stamina))
	{
		return false;
	}
	Stamina->SetSize(71.0f, 211.0f);
	// Initialization may precede Slate, or a child may already have its first snapshot.
	Health->InitializeForCharacter(First.Character);
	HUD->BindToCharacter(First.Character);
	TSharedPtr<SWidget> AliveSlate = HUD->TakeWidget();
	TestEqual(TEXT("Initial label reads the existing progression owner"), LevelText->GetText().ToString(), FString(TEXT("Level: 3")));
	TestEqual(TEXT("Initial label reads actual current/effective maximum health"),
		HealthText->GetText().ToString(), HealthLabel(750.0f, FirstMaximum));
	TestEqual(TEXT("Mana uses its existing effective max"), Mana->GetMaxValue(),
		First.ASC->GetNumericAttribute(UHunterAttributeSet::GetMaxEffectiveManaAttribute()));
	TestEqual(TEXT("Mana uses the existing mana attribute"), Mana->GetCurrentValue(), 300.0f);
	const float PriorDisplay = Health->GetDisplayFillPercent();
	First.ASC->SetNumericAttributeBase(UHunterAttributeSet::GetHealthAttribute(), 300.0f);
	TestEqual(TEXT("Health text changes on the existing attribute event, before a visual frame"),
		HealthText->GetText().ToString(), HealthLabel(300.0f, FirstMaximum));
	TestEqual(TEXT("Gameplay value changes immediately"), Health->GetCurrentValue(), 300.0f);
	TestEqual(TEXT("Interpolation remains separate from the current health value"), Health->GetDisplayFillPercent(), PriorDisplay);
	TestTrue(TEXT("The bar target uses the live percentage"), FMath::IsNearlyEqual(Health->GetFillPercent(), 300.0f / FirstMaximum));
	First.ASC->SetNumericAttributeBase(UHunterAttributeSet::GetFlatReservedHealthAttribute(), 100.0f);
	TestEqual(TEXT("The label follows the bar's effective maximum when reservation changes"),
		HealthText->GetText().ToString(), HealthLabel(300.0f,
			First.ASC->GetNumericAttribute(UHunterAttributeSet::GetMaxEffectiveHealthAttribute())));
	First.ASC->SetNumericAttributeBase(UHunterAttributeSet::GetManaAttribute(), 80.0f);
	TestEqual(TEXT("Mana changes arrive without HUD polling"), Mana->GetCurrentValue(), 80.0f);
	First.Progression->AwardExperience(First.Progression->XPToNextLevel - First.Progression->CurrentXP);
	TestEqual(TEXT("An earned level arrives through the existing progression event"),
		LevelText->GetText().ToString(), FString(TEXT("Level: 4")));
	TestEqual(TEXT("Binding does not reset a custom Stamina width"), Stamina->BarWidthOverride, 71.0f);
	TestEqual(TEXT("Binding does not reset a custom Stamina height"), Stamina->BarHeightOverride, 211.0f);

	// Simulate possession before the replacement pawn's replicated AttributeSet is available.
	UHunterAttributeSet* SecondAttributes = const_cast<UHunterAttributeSet*>(Second.ASC->GetSet<UHunterAttributeSet>());
	Second.ASC->RemoveSpawnedAttribute(SecondAttributes);
	HUD->BindToCharacter(Second.Character);
	TestTrue(TEXT("No stale health number is shown while attributes are unavailable"), HealthText->GetText().IsEmpty());
	TestEqual(TEXT("Level can initialize independently of resource availability"),
		LevelText->GetText().ToString(), FString(TEXT("Level: 8")));
	Second.ASC->AddAttributeSetSubobject(SecondAttributes);
	FTimerManager& Timers = First.TestWorld.GetTestWorld()->GetTimerManager();
	Timers.Tick(0.0f);
	{
		// Advance only the test timer manager; no gameplay world/BeginPlay is needed.
		TGuardValue<uint64> FrameGuard(GFrameCounter, GFrameCounter + 1);
		Timers.Tick(0.2f);
	}
	TestEqual(TEXT("The existing resource retry publishes its first valid snapshot to the header"),
		HealthText->GetText().ToString(), HealthLabel(450.0f, SecondMaximum));
	First.ASC->SetNumericAttributeBase(UHunterAttributeSet::GetHealthAttribute(), 50.0f);
	First.Progression->AwardExperience(First.Progression->XPToNextLevel - First.Progression->CurrentXP);
	TestEqual(TEXT("The old pawn cannot change the rebound health label"),
		HealthText->GetText().ToString(), HealthLabel(450.0f, SecondMaximum));
	TestEqual(TEXT("The old progression owner cannot change the rebound level label"),
		LevelText->GetText().ToString(), FString(TEXT("Level: 8")));
	Second.ASC->SetNumericAttributeBase(UHunterAttributeSet::GetHealthAttribute(), -100.0f);
	TestEqual(TEXT("Zero health displays from the real resource pipeline"), HealthText->GetText().ToString(), HealthLabel(0.0f, SecondMaximum));
	TestEqual(TEXT("Displayed target remains clamped at zero"), Health->GetFillPercent(), 0.0f);
	Second.ASC->SetNumericAttributeBase(UHunterAttributeSet::GetHealthAttribute(), 9999.0f);
	TestEqual(TEXT("Displayed target remains clamped at one"), Health->GetFillPercent(), 1.0f);
	HUD->BindToCharacter(nullptr);
	TestTrue(TEXT("Releasing a pawn clears health text"), HealthText->GetText().IsEmpty());
	TestTrue(TEXT("Releasing a pawn clears level text"), LevelText->GetText().IsEmpty());
	Second.ASC->SetNumericAttributeBase(UHunterAttributeSet::GetHealthAttribute(), 200.0f);
	TestTrue(TEXT("An old attribute source cannot repaint a released label"), HealthText->GetText().IsEmpty());
	HUD->BindToCharacter(Second.Character);
	TestEqual(TEXT("A released HUD can bind again"), HealthText->GetText().ToString(), HealthLabel(200.0f, SecondMaximum));
	AliveSlate.Reset();
	TestTrue(TEXT("Destroying the bound Slate tree clears health text"), HealthText->GetText().IsEmpty());
	TestTrue(TEXT("Destroying the bound Slate tree clears level text"), LevelText->GetText().IsEmpty());
	Second.ASC->SetNumericAttributeBase(UHunterAttributeSet::GetHealthAttribute(), 100.0f);
	TestTrue(TEXT("Old sources cannot repaint after NativeDestruct"), HealthText->GetText().IsEmpty());
	TestFalse(TEXT("Neither fixture enters gameplay BeginPlay"), First.Character->HasActorBegunPlay() || Second.Character->HasActorBegunPlay());
	return true;
}

#endif

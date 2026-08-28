#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "Combat/Components/CombatManager.h"
#include "Combat/Processors/CombatRecoveryProcessor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace PHCombatRecoveryTests
{
	constexpr float Tolerance = 0.01f;
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	UWorld* FindTestWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World()
				&& (Context.WorldType == EWorldType::Editor
					|| Context.WorldType == EWorldType::Game
					|| Context.WorldType == EWorldType::PIE))
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	AActor* CreateRecoveryActor(
		UWorld* World,
		UHunterAttributeSet*& OutAttributes,
		UCombatRecoveryProcessor*& OutProcessor)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Actor, TEXT("RecoveryTestASC"));
		Actor->AddInstanceComponent(ASC);
		ASC->RegisterComponent();

		OutAttributes = NewObject<UHunterAttributeSet>(Actor, TEXT("RecoveryTestAttributes"));
		ASC->AddAttributeSetSubobject(OutAttributes);

		UCombatManager* Manager = NewObject<UCombatManager>(Actor, TEXT("RecoveryTestCombatManager"));
		Actor->AddInstanceComponent(Manager);
		Manager->RegisterComponent();
		OutProcessor = Manager->GetRecoveryProcessor();
		return Actor;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatRecoveryProcessorTest,
	"ProjectHunter.Combat.Recovery.LeechRecoupAndShieldRecharge",
	PHCombatRecoveryTests::TestFlags)

bool FPHCombatRecoveryProcessorTest::RunTest(const FString&)
{
	using namespace PHCombatRecoveryTests;

	TestEqual(
		TEXT("Maximum leech rate is a percentage of effective maximum"),
		UCombatRecoveryProcessor::CalculateLeechRateCap(100.f, 20.f),
		20.f,
		Tolerance);

	UWorld* World = FindTestWorld();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}

	UHunterAttributeSet* Attributes = nullptr;
	UCombatRecoveryProcessor* Processor = nullptr;
	AActor* Actor = CreateRecoveryActor(World, Attributes, Processor);
	if (!TestNotNull(TEXT("Recovery processor exists"), Processor))
	{
		World->DestroyActor(Actor);
		return false;
	}

	Attributes->InitMaxHealth(100.f);
	Attributes->InitMaxEffectiveHealth(100.f);
	Attributes->InitHealth(0.f);
	Attributes->InitMaxLifeLeechRatePercent(20.f);
	Processor->QueueLeech(ECombatRecoveryResource::Health, 100.f, 1.f);
	Processor->Advance(1.f);
	TestEqual(TEXT("Leech is timed and capped at 20 percent per second"), Attributes->GetHealth(), 20.f, Tolerance);

	Processor->Shutdown();
	Attributes->InitHealth(0.f);
	Attributes->InitMaxLifeLeechRatePercent(0.f);
	Processor->QueueLeech(ECombatRecoveryResource::Health, 100.f, 1.f);
	Processor->Advance(1.f);
	TestEqual(TEXT("Zero maximum leech rate prevents recovery"), Attributes->GetHealth(), 0.f, Tolerance);
	TestEqual(TEXT("Zero maximum leech rate clears its pending instance"), Processor->GetPendingRecoveryCount(), 0);

	Processor->Shutdown();
	Attributes->InitHealth(0.f);
	Processor->QueueRecoup(ECombatRecoveryResource::Health, 40.f, 4.f);
	Processor->Advance(1.f);
	TestEqual(TEXT("Recoup restores evenly over its duration without the leech cap"), Attributes->GetHealth(), 10.f, Tolerance);

	Processor->Shutdown();
	Attributes->InitMaxArcaneShield(100.f);
	Attributes->InitMaxEffectiveArcaneShield(100.f);
	Attributes->InitArcaneShield(0.f);
	Attributes->InitArcaneShieldRechargeDelay(2.f);
	Attributes->InitArcaneShieldRechargeRate(20.f);
	Processor->NotifyHitDamageTaken();
	Processor->Advance(1.f);
	TestEqual(TEXT("Shield does not recharge during its delay"), Attributes->GetArcaneShield(), 0.f, Tolerance);
	Processor->Advance(1.5f);
	TestEqual(TEXT("Shield recharges for the time left after the delay"), Attributes->GetArcaneShield(), 10.f, Tolerance);

	Processor->Shutdown();
	World->DestroyActor(Actor);
	return true;
}

#endif

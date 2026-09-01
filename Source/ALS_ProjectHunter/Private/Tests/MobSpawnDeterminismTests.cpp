// Automation coverage for AUDIT F-03: mob spawn placement used global randomness.
//
// The manager already had a deterministic composition stream for pack sizes,
// mob types and per-monster seeds, but decided *where* a mob stands with
// FMath::RandRange. A floor therefore replayed while the mobs on it did not.
//
// Placement now draws from its own stream, derived from the encounter seed
// rather than shared with composition. These tests pin down both halves of that:
// placement replays, and placement retries cannot shift a composition roll.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "AI/Library/Structs/MobStructs.h"
#include "Character/PHBaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Tests/PHMobPlacementProbe.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace PHMobSpawnDeterminismTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	constexpr int32 DrawCount = 24;

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

	/**
	 * A manager whose placement depends on nothing but its own stream: the nav,
	 * ground and collision gates are off, so GetRandomSpawnLocation returns the
	 * raw box sample and two probes cannot agree by accident of the level.
	 */
	APHMobPlacementProbe* MakeProbe(UWorld* World, const int32 EncounterSeed)
	{
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		APHMobPlacementProbe* Probe = World->SpawnActor<APHMobPlacementProbe>(
			FVector::ZeroVector, FRotator::ZeroRotator, Parameters);
		if (!Probe)
		{
			return nullptr;
		}

		Probe->EncounterSeedOverride = EncounterSeed;
		Probe->bAutoActivate = false;
		Probe->bUseNavCheck = false;
		Probe->bUseGroundCheck = false;
		Probe->bUseCollisionCheck = false;
		Probe->bUseSmartSpawnPlacement = false;
		Probe->MinDistanceFromPlayer = 0.0f;
		Probe->MaxDistanceFromPlayer = 0.0f;

		if (Probe->SpawnArea)
		{
			Probe->SpawnArea->SetBoxExtent(FVector(1500.0f, 1200.0f, 300.0f));
		}

		// Two entries with different weights, so a composition roll has an
		// observable outcome rather than always returning index zero.
		FMobTypeEntry Common;
		Common.MobClass = APHBaseCharacter::StaticClass();
		Common.SpawnWeight = 70;

		FMobTypeEntry Rare;
		Rare.MobClass = APHBaseCharacter::StaticClass();
		Rare.SpawnWeight = 30;

		Probe->MobTypes = { Common, Rare };

		Probe->CacheSpawnBox();
		return Probe;
	}

	/** Consecutive placement draws. Returns false if the manager refused one. */
	bool DrawPlacements(APHMobPlacementProbe* Probe, const int32 Count, TArray<FVector>& Out)
	{
		Out.Reset();
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FVector Location = FVector::ZeroVector;
			if (!Probe->DrawSpawnLocation(Location))
			{
				return false;
			}
			Out.Add(Location);
		}
		return true;
	}

	void DrawMobTypes(APHMobPlacementProbe* Probe, const int32 Count, TArray<int32>& Out)
	{
		Out.Reset();
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Out.Add(Probe->DrawMobTypeIndex());
		}
	}

	bool AllIdentical(const TArray<FVector>& Values)
	{
		for (int32 Index = 1; Index < Values.Num(); ++Index)
		{
			if (!Values[Index].Equals(Values[0]))
			{
				return false;
			}
		}
		return true;
	}
}

// ---------------------------------------------------------------------------
// The same encounter seed puts mobs in the same places
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHMobPlacementReplaysForOneSeedTest,
	"ProjectHunter.AI.MobSpawn.PlacementReplaysForOneSeed",
	PHMobSpawnDeterminismTests::TestFlags)

bool FPHMobPlacementReplaysForOneSeedTest::RunTest(const FString&)
{
	using namespace PHMobSpawnDeterminismTests;

	UWorld* World = FindTestWorld();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}

	APHMobPlacementProbe* First = MakeProbe(World, 20260831);
	APHMobPlacementProbe* Second = MakeProbe(World, 20260831);
	if (!TestNotNull(TEXT("First manager spawns"), First)
		|| !TestNotNull(TEXT("Second manager spawns"), Second))
	{
		return false;
	}

	TArray<FVector> FirstRun;
	TArray<FVector> SecondRun;
	const bool bFirstDrawn = DrawPlacements(First, DrawCount, FirstRun);
	const bool bSecondDrawn = DrawPlacements(Second, DrawCount, SecondRun);

	World->DestroyActor(First);
	World->DestroyActor(Second);

	if (!TestTrue(TEXT("Both managers produced a full set of positions"),
		bFirstDrawn && bSecondDrawn))
	{
		return false;
	}

	TestEqual(TEXT("Both managers drew the same number of positions"),
		SecondRun.Num(), FirstRun.Num());

	for (int32 Index = 0; Index < FirstRun.Num(); ++Index)
	{
		TestTrue(
			FString::Printf(TEXT("Position %d replays for the same encounter seed"), Index),
			FirstRun[Index].Equals(SecondRun[Index]));
	}

	// Guards the failure mode where a broken stream is reproducible only because
	// it stopped varying at all.
	TestFalse(TEXT("Positions still vary within one manager"), AllIdentical(FirstRun));

	return true;
}

// ---------------------------------------------------------------------------
// Different encounters still get different places
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHMobPlacementDiffersBetweenSeedsTest,
	"ProjectHunter.AI.MobSpawn.PlacementDiffersBetweenSeeds",
	PHMobSpawnDeterminismTests::TestFlags)

bool FPHMobPlacementDiffersBetweenSeedsTest::RunTest(const FString&)
{
	using namespace PHMobSpawnDeterminismTests;

	UWorld* World = FindTestWorld();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}

	APHMobPlacementProbe* First = MakeProbe(World, 20260831);
	APHMobPlacementProbe* Second = MakeProbe(World, 20260901);
	if (!TestNotNull(TEXT("First manager spawns"), First)
		|| !TestNotNull(TEXT("Second manager spawns"), Second))
	{
		return false;
	}

	TArray<FVector> FirstRun;
	TArray<FVector> SecondRun;
	const bool bDrawn = DrawPlacements(First, DrawCount, FirstRun)
		&& DrawPlacements(Second, DrawCount, SecondRun);

	World->DestroyActor(First);
	World->DestroyActor(Second);

	if (!TestTrue(TEXT("Both managers produced a full set of positions"), bDrawn))
	{
		return false;
	}

	bool bAnyDifferent = false;
	for (int32 Index = 0; Index < FirstRun.Num(); ++Index)
	{
		if (!FirstRun[Index].Equals(SecondRun[Index]))
		{
			bAnyDifferent = true;
			break;
		}
	}

	TestTrue(TEXT("A different encounter seed produces a different placement run"),
		bAnyDifferent);

	return true;
}

// ---------------------------------------------------------------------------
// Placement retries must not shift the composition rolls
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHMobPlacementDoesNotDisturbCompositionTest,
	"ProjectHunter.AI.MobSpawn.PlacementDoesNotDisturbComposition",
	PHMobSpawnDeterminismTests::TestFlags)

bool FPHMobPlacementDoesNotDisturbCompositionTest::RunTest(const FString&)
{
	using namespace PHMobSpawnDeterminismTests;

	// This is the reason placement gets its own stream instead of sharing the
	// encounter stream. A spawn attempt is retried a variable number of times
	// depending on navmesh, collision and player distance, so a shared stream
	// would let the level geometry decide which mob type came next.
	UWorld* World = FindTestWorld();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}

	APHMobPlacementProbe* Undisturbed = MakeProbe(World, 4242);
	APHMobPlacementProbe* Interleaved = MakeProbe(World, 4242);
	if (!TestNotNull(TEXT("Undisturbed manager spawns"), Undisturbed)
		|| !TestNotNull(TEXT("Interleaved manager spawns"), Interleaved))
	{
		return false;
	}

	TArray<int32> Expected;
	DrawMobTypes(Undisturbed, DrawCount, Expected);

	// The same composition rolls, with a different number of placement draws
	// wedged between each one.
	TArray<int32> Actual;
	bool bPlacementSucceeded = true;
	for (int32 Index = 0; Index < DrawCount; ++Index)
	{
		TArray<FVector> Discarded;
		bPlacementSucceeded &= DrawPlacements(Interleaved, 1 + (Index % 5), Discarded);
		Actual.Add(Interleaved->DrawMobTypeIndex());
	}

	World->DestroyActor(Undisturbed);
	World->DestroyActor(Interleaved);

	if (!TestTrue(TEXT("Interleaved placement draws all succeeded"), bPlacementSucceeded))
	{
		return false;
	}

	TestEqual(TEXT("Both managers rolled the same number of mob types"),
		Actual.Num(), Expected.Num());

	for (int32 Index = 0; Index < Expected.Num(); ++Index)
	{
		TestEqual(
			FString::Printf(TEXT("Mob type roll %d is unaffected by placement draws"), Index),
			Actual[Index], Expected[Index]);
	}

	return true;
}

// ---------------------------------------------------------------------------
// Resetting the encounter also rewinds placement
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHMobResetRewindsPlacementTest,
	"ProjectHunter.AI.MobSpawn.ResetRewindsPlacement",
	PHMobSpawnDeterminismTests::TestFlags)

bool FPHMobResetRewindsPlacementTest::RunTest(const FString&)
{
	using namespace PHMobSpawnDeterminismTests;

	// A manager reused across floors calls ResetEncounterStream. If that rewound
	// composition but not placement, the second floor would compose identically
	// and still place differently.
	UWorld* World = FindTestWorld();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}

	APHMobPlacementProbe* Probe = MakeProbe(World, 777);
	if (!TestNotNull(TEXT("Manager spawns"), Probe))
	{
		return false;
	}

	TArray<FVector> BeforeReset;
	TArray<FVector> AfterReset;
	const bool bDrawnBefore = DrawPlacements(Probe, DrawCount, BeforeReset);

	Probe->ResetEncounterStream();

	const bool bDrawnAfter = DrawPlacements(Probe, DrawCount, AfterReset);

	World->DestroyActor(Probe);

	if (!TestTrue(TEXT("Positions were drawn before and after the reset"),
		bDrawnBefore && bDrawnAfter))
	{
		return false;
	}

	for (int32 Index = 0; Index < BeforeReset.Num(); ++Index)
	{
		TestTrue(
			FString::Printf(TEXT("Position %d repeats after ResetEncounterStream"), Index),
			BeforeReset[Index].Equals(AfterReset[Index]));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

// Automation coverage for the combat damage pipeline.
//
// Both calculators are plain static functions over a const UHunterAttributeSet,
// an FAnimationDamageInfo and a seeded FRandomStream, so they can be driven
// without a world, an ASC, or a spawned actor. Attribute values are seeded with
// the generated InitX() setters, which write the FGameplayAttributeData directly
// and therefore need no ability system component.
//
// These tests exist to pin down the locked design decisions (minimum 1 physical
// damage, parry negating ailments, Souls-style block with no random chance) and
// the defects fixed in the Phase 1 correctness pass, so a later refactor cannot
// silently undo them.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Combat/Calculators/CombatOutgoingDamageCalculator.h"
#include "Combat/Components/CombatManager.h"
#include "Combat/Resolvers/CombatIncomingDamageResolver.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Stats/Data/BaseStatsData.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace PHCombatTests
{
	constexpr float Tolerance = 0.01f;

	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	/** Fresh attribute set. The constructor seeds every multiplier to its neutral 1.0. */
	TStrongObjectPtr<UHunterAttributeSet> MakeAttributes()
	{
		return TStrongObjectPtr<UHunterAttributeSet>(
			NewObject<UHunterAttributeSet>(GetTransientPackage()));
	}

	/** Attacker whose only damage source is an exact, non-random physical hit. */
	TStrongObjectPtr<UHunterAttributeSet> MakeFlatPhysicalAttacker(const float Damage)
	{
		TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeAttributes();
		Attrs->InitMinPhysicalDamage(Damage);
		Attrs->InitMaxPhysicalDamage(Damage);
		return Attrs;
	}

	/** Damage info that does not crit, so packets stay deterministic. */
	FAnimationDamageInfo MakeNonCritInfo()
	{
		FAnimationDamageInfo Info;
		Info.Crit.bCanCrit = false;
		return Info;
	}

	float PacketTotal(const FCombatDamagePacket& Packet)
	{
		return Packet.Physical + Packet.Fire + Packet.Ice
			+ Packet.Lightning + Packet.Light + Packet.Corruption;
	}
}

// ---------------------------------------------------------------------------
// Runtime data: neutral multiplier defaults and legacy asset migration
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatBaseStatsNeutralMultiplierTest,
	"ProjectHunter.Combat.RuntimeData.BaseStatsNeutralMultipliers",
	PHCombatTests::TestFlags)

bool FPHCombatBaseStatsNeutralMultiplierTest::RunTest(const FString&)
{
	UBaseStatsData* BaseStats = LoadObject<UBaseStatsData>(
		nullptr,
		TEXT("/Game/ProjectHunter/Data/Stats/DA_BaseStats.DA_BaseStats"));
	if (!TestNotNull(TEXT("DA_BaseStats loads"), BaseStats))
	{
		return false;
	}

	auto TestStat = [this, BaseStats](const TCHAR* Name, const float Expected)
	{
		float Actual = 0.f;
		TestTrue(*FString::Printf(TEXT("%s is authored"), Name), BaseStats->GetStatValue(Name, Actual));
		TestEqual(*FString::Printf(TEXT("%s has a usable neutral default"), Name), Actual, Expected, PHCombatTests::Tolerance);
	};

	TestStat(TEXT("GlobalMoreDamage"), 1.f);
	TestStat(TEXT("PhysicalMoreDamage"), 1.f);
	TestStat(TEXT("GlobalDamageTakenMultiplier"), 1.f);
	TestStat(TEXT("PhysicalDamageTakenMultiplier"), 1.f);
	TestStat(TEXT("CritMultiplier"), 1.5f);
	TestStat(TEXT("SpellsCritMultiplier"), 1.5f);
	TestEqual(
		TEXT("DA_BaseStats has the current schema"),
		BaseStats->StatsSchemaVersion,
		UBaseStatsData::CurrentStatsSchemaVersion);
	return true;
}

// ---------------------------------------------------------------------------
// Manager integration: preserve Blueprint call-site compatibility
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatDefenderManagerRoutingTest,
	"ProjectHunter.Combat.Manager.DefenderManagerRoutesToAttacker",
	PHCombatTests::TestFlags)

bool FPHCombatDefenderManagerRoutingTest::RunTest(const FString&)
{
	UWorld* World = nullptr;
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() &&
				(Context.WorldType == EWorldType::Editor || Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE))
			{
				World = Context.World();
				break;
			}
		}
	}
	if (!TestNotNull(TEXT("Transient test world"), World))
	{
		return false;
	}

	auto CreateCombatActor = [World](const TCHAR* Name, UHunterAttributeSet*& OutAttributes, UCombatManager*& OutManager)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = FName(Name);
		AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParameters);

		UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Actor, TEXT("ASC"));
		Actor->AddInstanceComponent(ASC);
		ASC->RegisterComponent();

		OutAttributes = NewObject<UHunterAttributeSet>(Actor, TEXT("Attributes"));
		ASC->AddAttributeSetSubobject(OutAttributes);

		OutManager = NewObject<UCombatManager>(Actor, TEXT("CombatManager"));
		Actor->AddInstanceComponent(OutManager);
		OutManager->RegisterComponent();
		return Actor;
	};

	UHunterAttributeSet* AttackerAttributes = nullptr;
	UCombatManager* AttackerManager = nullptr;
	AActor* Attacker = CreateCombatActor(TEXT("RoutingAttacker"), AttackerAttributes, AttackerManager);

	UHunterAttributeSet* DefenderAttributes = nullptr;
	UCombatManager* DefenderManager = nullptr;
	AActor* Defender = CreateCombatActor(TEXT("RoutingDefender"), DefenderAttributes, DefenderManager);

	AttackerAttributes->InitMinPhysicalDamage(10.f);
	AttackerAttributes->InitMaxPhysicalDamage(10.f);
	DefenderAttributes->InitMaxHealth(100.f);
	DefenderAttributes->InitMaxEffectiveHealth(100.f);
	DefenderAttributes->InitHealth(100.f);

	FAnimationDamageInfo Info = PHCombatTests::MakeNonCritInfo();
	FCombatResolveResult Result;
	const bool bApplied = DefenderManager->ApplyHit(Attacker, Defender, Info, Result);

	TestTrue(TEXT("A call made on the defender manager is routed and applied"), bApplied);
	TestEqual(TEXT("The routed hit resolves ten damage"), Result.DamageToHealth, 10.f, PHCombatTests::Tolerance);
	TestEqual(TEXT("The defender loses health"), DefenderAttributes->GetHealth(), 90.f, PHCombatTests::Tolerance);
	TestTrue(TEXT("CombatManager is replicated for owning-client RPC forwarding"), AttackerManager->GetIsReplicated());

	World->DestroyActor(Attacker);
	World->DestroyActor(Defender);
	return true;
}

// ---------------------------------------------------------------------------
// Outgoing: base damage
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatOutgoingBaseDamageTest,
	"ProjectHunter.Combat.Outgoing.BaseDamage",
	PHCombatTests::TestFlags)

bool FPHCombatOutgoingBaseDamageTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	// A fixed min==max range removes the roll, so the result is exact.
	const TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeFlatPhysicalAttacker(100.f);
	FRandomStream Stream(1234);

	const FCombatDamagePacket Packet =
		FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), MakeNonCritInfo(), Stream);

	TestEqual(TEXT("Physical equals the weapon roll"), Packet.Physical, 100.f, Tolerance);
	TestEqual(TEXT("No other damage type is produced"), PacketTotal(Packet) - Packet.Physical, 0.f, Tolerance);
	TestFalse(TEXT("Hit did not crit"), Packet.bCrit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatWeaponEffectivenessTest,
	"ProjectHunter.Combat.Outgoing.WeaponEffectiveness",
	PHCombatTests::TestFlags)

bool FPHCombatWeaponEffectivenessTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	const TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeFlatPhysicalAttacker(100.f);
	FRandomStream Stream(1);

	FAnimationDamageInfo Info = MakeNonCritInfo();
	Info.WeaponDamageEffectivenessPercent = 50.f;

	const FCombatDamagePacket Half =
		FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), Info, Stream);
	TestEqual(TEXT("50% weapon effectiveness halves weapon damage"), Half.Physical, 50.f, Tolerance);

	// A spell that ignores the weapon entirely still deals its own base damage.
	Info.WeaponDamageEffectivenessPercent = 0.f;
	Info.SkillBaseDamage.Fire = 40.f;

	const FCombatDamagePacket SpellPacket =
		FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), Info, Stream);
	TestEqual(TEXT("Zero weapon effectiveness removes weapon damage"), SpellPacket.Physical, 0.f, Tolerance);
	TestEqual(TEXT("Skill base damage still applies"), SpellPacket.Fire, 40.f, Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatAddedDamageTest,
	"ProjectHunter.Combat.Outgoing.AddedDamage",
	PHCombatTests::TestFlags)

bool FPHCombatAddedDamageTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	const TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeFlatPhysicalAttacker(100.f);
	Attrs->InitPhysicalFlatDamage(50.f);
	FRandomStream Stream(1);

	FAnimationDamageInfo Info = MakeNonCritInfo();
	const FCombatDamagePacket Full =
		FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), Info, Stream);
	TestEqual(TEXT("Flat added damage joins the weapon roll"), Full.Physical, 150.f, Tolerance);

	Info.AddedDamageEffectivenessPercent = 50.f;
	const FCombatDamagePacket Scaled =
		FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), Info, Stream);
	TestEqual(TEXT("Added damage effectiveness scales only the flat portion"), Scaled.Physical, 125.f, Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatDeterministicRangeTest,
	"ProjectHunter.Combat.Outgoing.DeterministicRange",
	PHCombatTests::TestFlags)

bool FPHCombatDeterministicRangeTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	const TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeAttributes();
	Attrs->InitMinPhysicalDamage(10.f);
	Attrs->InitMaxPhysicalDamage(20.f);

	FRandomStream StreamA(4242);
	FRandomStream StreamB(4242);
	const FCombatDamagePacket A =
		FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), MakeNonCritInfo(), StreamA);
	const FCombatDamagePacket B =
		FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), MakeNonCritInfo(), StreamB);

	TestEqual(TEXT("Equal seeds produce identical rolls"), A.Physical, B.Physical, KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Roll lands inside the weapon range"), A.Physical >= 10.f - Tolerance && A.Physical <= 20.f + Tolerance);

	FRandomStream StreamC(99);
	const FCombatDamagePacket C =
		FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), MakeNonCritInfo(), StreamC);
	TestTrue(TEXT("A different seed still lands inside the range"), C.Physical >= 10.f - Tolerance && C.Physical <= 20.f + Tolerance);
	return true;
}

// ---------------------------------------------------------------------------
// Outgoing: increased / more / less  (W-06)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatIncreasedDamageTest,
	"ProjectHunter.Combat.Outgoing.IncreasedIsAdditive",
	PHCombatTests::TestFlags)

bool FPHCombatIncreasedDamageTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	const TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeFlatPhysicalAttacker(100.f);
	// Two increased sources must sum into one additive pool, not multiply.
	Attrs->InitPhysicalPercentDamage(50.f);
	Attrs->InitGlobalDamages(50.f);
	FRandomStream Stream(1);

	const FCombatDamagePacket Packet =
		FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), MakeNonCritInfo(), Stream);

	// 100 * (1 + 1.00) = 200, not 100 * 1.5 * 1.5 = 225.
	TestEqual(TEXT("Increased percentages add together"), Packet.Physical, 200.f, Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatMoreDamageTest,
	"ProjectHunter.Combat.Outgoing.MoreIsMultiplicative",
	PHCombatTests::TestFlags)

bool FPHCombatMoreDamageTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	const TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeFlatPhysicalAttacker(100.f);
	Attrs->InitGlobalMoreDamage(2.f);
	Attrs->InitPhysicalMoreDamage(1.5f);
	FRandomStream Stream(1);

	const FCombatDamagePacket Packet =
		FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), MakeNonCritInfo(), Stream);

	TestEqual(TEXT("More multipliers compound"), Packet.Physical, 300.f, Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatLessAndZeroMultiplierTest,
	"ProjectHunter.Combat.Outgoing.LessAndZeroMultiplier",
	PHCombatTests::TestFlags)

bool FPHCombatLessAndZeroMultiplierTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	// W-06 regression guard. The old GetNeutralMultiplier() rewrote any value
	// <= 0 to 1.0, so "deals no fire damage" silently dealt full damage.
	{
		const TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeFlatPhysicalAttacker(100.f);
		Attrs->InitPhysicalMoreDamage(0.5f);
		FRandomStream Stream(1);
		const FCombatDamagePacket Packet =
			FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), MakeNonCritInfo(), Stream);
		TestEqual(TEXT("0.5 halves the hit"), Packet.Physical, 50.f, Tolerance);
	}

	{
		const TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeFlatPhysicalAttacker(100.f);
		Attrs->InitPhysicalMoreDamage(0.f);
		FRandomStream Stream(1);
		const FCombatDamagePacket Packet =
			FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), MakeNonCritInfo(), Stream);
		TestEqual(TEXT("A x0 multiplier removes the damage entirely"), Packet.Physical, 0.f, Tolerance);
	}

	{
		const TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeFlatPhysicalAttacker(100.f);
		Attrs->InitGlobalMoreDamage(0.f);
		FRandomStream Stream(1);
		const FCombatDamagePacket Packet =
			FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), MakeNonCritInfo(), Stream);
		TestEqual(TEXT("A global x0 multiplier removes the damage entirely"), Packet.Physical, 0.f, Tolerance);
	}

	return true;
}

// ---------------------------------------------------------------------------
// Outgoing: crit  (W-07)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatCritMultiplierTest,
	"ProjectHunter.Combat.Outgoing.CritMultiplier",
	PHCombatTests::TestFlags)

bool FPHCombatCritMultiplierTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	const TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeFlatPhysicalAttacker(100.f);
	FRandomStream Stream(1);

	FAnimationDamageInfo Info;
	Info.Crit.bCanCrit = true;
	Info.Crit.bForceCrit = true;

	// Constructor default is the 1.5 ratio: 150% damage on crit.
	const FCombatDamagePacket Packet =
		FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), Info, Stream);

	TestTrue(TEXT("Forced crit is flagged"), Packet.bCrit);
	TestEqual(TEXT("Base crit ratio is 1.5"), Packet.Physical, 150.f, Tolerance);
	TestEqual(TEXT("Applied multiplier is reported"), Packet.CritMultiplierApplied, 1.5f, Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatCritUnitsTest,
	"ProjectHunter.Combat.Outgoing.CritUnitsAreConsistent",
	PHCombatTests::TestFlags)

bool FPHCombatCritUnitsTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	// W-07 regression guard: the attribute is an absolute ratio, the animation
	// field is an additive delta, and the two must not be confused.
	{
		const TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeFlatPhysicalAttacker(100.f);
		Attrs->InitCritMultiplier(2.f);
		FRandomStream Stream(1);

		FAnimationDamageInfo Info;
		Info.Crit.bForceCrit = true;
		Info.Crit.CritMultiplier = 0.5f; // +50% on top of the 2.0 base.

		const FCombatDamagePacket Packet =
			FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), Info, Stream);
		TestEqual(TEXT("Animation delta adds to the attribute ratio"), Packet.Physical, 250.f, Tolerance);
	}

	// A spell crit stacks its bonus above neutral rather than replacing the base.
	{
		const TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeFlatPhysicalAttacker(100.f);
		Attrs->InitCritMultiplier(1.5f);
		Attrs->InitSpellsCritMultiplier(2.f); // +1.0 above neutral.
		FRandomStream Stream(1);

		FAnimationDamageInfo Info;
		Info.Crit.bForceCrit = true;
		Info.Tags.bIsSpell = true;

		const FCombatDamagePacket Packet =
			FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), Info, Stream);
		TestEqual(TEXT("Spell crit multiplier stacks as a delta"), Packet.Physical, 250.f, Tolerance);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatDamageOverTimeNeverCritsTest,
	"ProjectHunter.Combat.Outgoing.DamageOverTimeNeverCrits",
	PHCombatTests::TestFlags)

bool FPHCombatDamageOverTimeNeverCritsTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	const TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeFlatPhysicalAttacker(100.f);
	FRandomStream Stream(1);

	FAnimationDamageInfo Info;
	Info.Crit.bForceCrit = true;
	Info.Tags.bIsDamageOverTime = true;

	const FCombatDamagePacket Packet =
		FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), Info, Stream);

	TestFalse(TEXT("Damage over time cannot crit even when forced"), Packet.bCrit);
	TestEqual(TEXT("Damage is unscaled"), Packet.Physical, 100.f, Tolerance);
	return true;
}

// ---------------------------------------------------------------------------
// Outgoing: conversion
//
// These assert the invariants that must hold regardless of how conversion
// scaling is later reworked: a packet never gains damage it was not given, and
// over-allocated conversion scales down instead of duplicating.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatConversionConservesTotalTest,
	"ProjectHunter.Combat.Outgoing.ConversionConservesTotal",
	PHCombatTests::TestFlags)

bool FPHCombatConversionConservesTotalTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	const TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeFlatPhysicalAttacker(100.f);
	Attrs->InitPhysicalToFire(100.f);
	FRandomStream Stream(1);

	const FCombatDamagePacket Packet =
		FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), MakeNonCritInfo(), Stream);

	TestEqual(TEXT("All physical damage moved to fire"), Packet.Physical, 0.f, Tolerance);
	TestEqual(TEXT("Fire received the full amount"), Packet.Fire, 100.f, Tolerance);
	TestEqual(TEXT("Total damage is conserved, never duplicated"), PacketTotal(Packet), 100.f, Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatConversionPartialTest,
	"ProjectHunter.Combat.Outgoing.ConversionPartial",
	PHCombatTests::TestFlags)

bool FPHCombatConversionPartialTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	const TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeFlatPhysicalAttacker(100.f);
	Attrs->InitPhysicalToFire(60.f);
	FRandomStream Stream(1);

	const FCombatDamagePacket Packet =
		FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), MakeNonCritInfo(), Stream);

	TestEqual(TEXT("Unconverted remainder stays physical"), Packet.Physical, 40.f, Tolerance);
	TestEqual(TEXT("Converted portion becomes fire"), Packet.Fire, 60.f, Tolerance);
	TestEqual(TEXT("Total damage is conserved"), PacketTotal(Packet), 100.f, Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatConversionOverAllocationTest,
	"ProjectHunter.Combat.Outgoing.ConversionOverHundredPercent",
	PHCombatTests::TestFlags)

bool FPHCombatConversionOverAllocationTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	// 80% + 80% = 160% requested. This must scale down to 100% total, splitting
	// the hit evenly, rather than producing 160 damage from a 100 damage hit.
	const TStrongObjectPtr<UHunterAttributeSet> Attrs = MakeFlatPhysicalAttacker(100.f);
	Attrs->InitPhysicalToFire(80.f);
	Attrs->InitPhysicalToIce(80.f);
	FRandomStream Stream(1);

	const FCombatDamagePacket Packet =
		FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(Attrs.Get(), MakeNonCritInfo(), Stream);

	TestEqual(TEXT("Over-allocated conversion never gains damage"), PacketTotal(Packet), 100.f, Tolerance);
	TestEqual(TEXT("Fire takes a proportional half"), Packet.Fire, 50.f, Tolerance);
	TestEqual(TEXT("Ice takes a proportional half"), Packet.Ice, 50.f, Tolerance);
	TestEqual(TEXT("Nothing is left unconverted"), Packet.Physical, 0.f, Tolerance);
	return true;
}

// ---------------------------------------------------------------------------
// Incoming: armour and the minimum 1 physical damage rule
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatArmourMitigationTest,
	"ProjectHunter.Combat.Incoming.ArmourMitigation",
	PHCombatTests::TestFlags)

bool FPHCombatArmourMitigationTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	FCombatDamagePacket Packet;
	Packet.Physical = 100.f;

	const TStrongObjectPtr<UHunterAttributeSet> Defender = MakeAttributes();
	Defender->InitArmour(1000.f);

	const FCombatResolveResult Result = FCombatIncomingDamageResolver::MitigateDamagePacket(
		Packet, nullptr, nullptr, nullptr, Defender.Get(), MakeNonCritInfo());

	// Armour / (Armour + 10 * Hit) = 1000 / 2000 = 50% mitigation.
	TestEqual(TEXT("Armour halves a matched hit"), Result.PhysicalTaken, 50.f, Tolerance);

	const TStrongObjectPtr<UHunterAttributeSet> Unarmoured = MakeAttributes();
	const FCombatResolveResult Raw = FCombatIncomingDamageResolver::MitigateDamagePacket(
		Packet, nullptr, nullptr, nullptr, Unarmoured.Get(), MakeNonCritInfo());
	TestEqual(TEXT("No armour means no mitigation"), Raw.PhysicalTaken, 100.f, Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatMinimumOnePhysicalTest,
	"ProjectHunter.Combat.Incoming.MinimumOnePhysicalDamage",
	PHCombatTests::TestFlags)

bool FPHCombatMinimumOnePhysicalTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	// Locked design decision: armour alone can never reduce a landed physical
	// hit to zero. Only hit-response mechanics (parry, i-frames) may do that.
	//
	// Two separate floors are in play and both are asserted here. Armour
	// mitigation is capped at MaxArmourMitigation (90%), so 10% of any hit gets
	// through on its own; the explicit minimum-1 rule only becomes the binding
	// constraint once that 10% would fall below 1, i.e. for hits under 10 damage.
	const TStrongObjectPtr<UHunterAttributeSet> Defender = MakeAttributes();
	Defender->InitArmour(10000000.f);

	// Large hit: the 90% mitigation cap is what limits the reduction.
	{
		FCombatDamagePacket Packet;
		Packet.Physical = 50.f;
		const FCombatResolveResult Result = FCombatIncomingDamageResolver::MitigateDamagePacket(
			Packet, nullptr, nullptr, nullptr, Defender.Get(), MakeNonCritInfo());

		TestTrue(TEXT("A landed physical hit always deals at least 1"),
			Result.PhysicalTaken >= 1.f - KINDA_SMALL_NUMBER);
		TestEqual(TEXT("Armour mitigation is capped at 90%"), Result.PhysicalTaken, 5.f, Tolerance);
		TestTrue(TEXT("Mitigation never increases the hit"), Result.PhysicalTaken <= Packet.Physical);
	}

	// Small hit: 10% would be 0.5, so the explicit minimum-1 floor takes over.
	{
		FCombatDamagePacket Packet;
		Packet.Physical = 5.f;
		const FCombatResolveResult Result = FCombatIncomingDamageResolver::MitigateDamagePacket(
			Packet, nullptr, nullptr, nullptr, Defender.Get(), MakeNonCritInfo());

		TestEqual(TEXT("The minimum-1 floor engages below 10 damage"), Result.PhysicalTaken, 1.f, Tolerance);
	}

	// A hit smaller than 1 is never rounded up past its own size.
	{
		FCombatDamagePacket Packet;
		Packet.Physical = 0.5f;
		const FCombatResolveResult Result = FCombatIncomingDamageResolver::MitigateDamagePacket(
			Packet, nullptr, nullptr, nullptr, Defender.Get(), MakeNonCritInfo());

		TestEqual(TEXT("A sub-1 hit is never inflated by the floor"), Result.PhysicalTaken, 0.5f, Tolerance);
	}

	// Armour can never zero a landed physical hit at any magnitude.
	for (const float Incoming : { 1.f, 2.f, 10.f, 100.f, 1000.f })
	{
		FCombatDamagePacket Packet;
		Packet.Physical = Incoming;
		const FCombatResolveResult Result = FCombatIncomingDamageResolver::MitigateDamagePacket(
			Packet, nullptr, nullptr, nullptr, Defender.Get(), MakeNonCritInfo());

		TestTrue(
			*FString::Printf(TEXT("A %.0f damage hit is never zeroed by armour"), Incoming),
			Result.PhysicalTaken >= 1.f - KINDA_SMALL_NUMBER);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatArmourPiercingTest,
	"ProjectHunter.Combat.Incoming.ArmourPiercing",
	PHCombatTests::TestFlags)

bool FPHCombatArmourPiercingTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	FCombatDamagePacket Packet;
	Packet.Physical = 100.f;

	const TStrongObjectPtr<UHunterAttributeSet> Defender = MakeAttributes();
	Defender->InitArmour(1000.f);

	FAnimationDamageInfo Info = MakeNonCritInfo();
	Info.Piercing.ArmourPiercing = 100.f;

	const FCombatResolveResult Result = FCombatIncomingDamageResolver::MitigateDamagePacket(
		Packet, nullptr, nullptr, nullptr, Defender.Get(), Info);

	TestEqual(TEXT("Full armour piercing removes all armour mitigation"), Result.PhysicalTaken, 100.f, Tolerance);
	return true;
}

// ---------------------------------------------------------------------------
// Incoming: resistances
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatResistanceTest,
	"ProjectHunter.Combat.Incoming.Resistance",
	PHCombatTests::TestFlags)

bool FPHCombatResistanceTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	FCombatDamagePacket Packet;
	Packet.Fire = 100.f;

	const TStrongObjectPtr<UHunterAttributeSet> Defender = MakeAttributes();
	Defender->InitFireResistanceFlatBonus(50.f);

	const FCombatResolveResult Result = FCombatIncomingDamageResolver::MitigateDamagePacket(
		Packet, nullptr, nullptr, nullptr, Defender.Get(), MakeNonCritInfo());

	TestEqual(TEXT("50% fire resistance halves fire damage"), Result.FireTaken, 50.f, Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatNegativeResistanceTest,
	"ProjectHunter.Combat.Incoming.NegativeResistance",
	PHCombatTests::TestFlags)

bool FPHCombatNegativeResistanceTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	FCombatDamagePacket Packet;
	Packet.Fire = 100.f;

	const TStrongObjectPtr<UHunterAttributeSet> Defender = MakeAttributes();
	Defender->InitFireResistanceFlatBonus(-50.f);

	const FCombatResolveResult Result = FCombatIncomingDamageResolver::MitigateDamagePacket(
		Packet, nullptr, nullptr, nullptr, Defender.Get(), MakeNonCritInfo());
	TestEqual(TEXT("Negative resistance amplifies damage"), Result.FireTaken, 150.f, Tolerance);

	// The floor is -100%, so damage can never more than double.
	const TStrongObjectPtr<UHunterAttributeSet> VeryNegative = MakeAttributes();
	VeryNegative->InitFireResistanceFlatBonus(-500.f);
	const FCombatResolveResult Clamped = FCombatIncomingDamageResolver::MitigateDamagePacket(
		Packet, nullptr, nullptr, nullptr, VeryNegative.Get(), MakeNonCritInfo());
	TestEqual(TEXT("Negative resistance is floored at -100%"), Clamped.FireTaken, 200.f, Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatMaxResistanceCapTest,
	"ProjectHunter.Combat.Incoming.MaxResistanceCap",
	PHCombatTests::TestFlags)

bool FPHCombatMaxResistanceCapTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	FCombatDamagePacket Packet;
	Packet.Fire = 100.f;

	// Default cap is 90%, so 200 points of resistance still lets 10% through.
	const TStrongObjectPtr<UHunterAttributeSet> Defender = MakeAttributes();
	Defender->InitFireResistanceFlatBonus(200.f);

	const FCombatResolveResult Capped = FCombatIncomingDamageResolver::MitigateDamagePacket(
		Packet, nullptr, nullptr, nullptr, Defender.Get(), MakeNonCritInfo());
	TestEqual(TEXT("Resistance is capped at the default 90%"), Capped.FireTaken, 10.f, Tolerance);

	// An explicit per-type cap raises the ceiling.
	Defender->InitMaxFireResistance(75.f);
	const FCombatResolveResult LoweredCap = FCombatIncomingDamageResolver::MitigateDamagePacket(
		Packet, nullptr, nullptr, nullptr, Defender.Get(), MakeNonCritInfo());
	TestEqual(TEXT("An explicit max resistance overrides the default"), LoweredCap.FireTaken, 25.f, Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatResistancePenetrationTest,
	"ProjectHunter.Combat.Incoming.ResistancePenetration",
	PHCombatTests::TestFlags)

bool FPHCombatResistancePenetrationTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	FCombatDamagePacket Packet;
	Packet.Fire = 100.f;

	const TStrongObjectPtr<UHunterAttributeSet> Defender = MakeAttributes();
	Defender->InitFireResistanceFlatBonus(50.f);

	FAnimationDamageInfo Info = MakeNonCritInfo();
	Info.Piercing.Fire = 25.f;

	const FCombatResolveResult Result = FCombatIncomingDamageResolver::MitigateDamagePacket(
		Packet, nullptr, nullptr, nullptr, Defender.Get(), Info);

	// 50% resistance minus 25 points of penetration leaves 25%.
	TestEqual(TEXT("Penetration reduces effective resistance"), Result.FireTaken, 75.f, Tolerance);
	return true;
}

// ---------------------------------------------------------------------------
// Incoming: damage taken multipliers and resource routing
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatDamageTakenMultiplierTest,
	"ProjectHunter.Combat.Incoming.DamageTakenMultiplier",
	PHCombatTests::TestFlags)

bool FPHCombatDamageTakenMultiplierTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	FCombatDamagePacket Packet;
	Packet.Physical = 100.f;

	{
		const TStrongObjectPtr<UHunterAttributeSet> Defender = MakeAttributes();
		Defender->InitPhysicalDamageTakenMultiplier(0.5f);
		const FCombatResolveResult Result = FCombatIncomingDamageResolver::MitigateDamagePacket(
			Packet, nullptr, nullptr, nullptr, Defender.Get(), MakeNonCritInfo());
		TestEqual(TEXT("A 0.5 taken multiplier halves incoming damage"), Result.PhysicalTaken, 50.f, Tolerance);
	}

	{
		// W-06 regression guard on the defensive side: full immunity must survive.
		const TStrongObjectPtr<UHunterAttributeSet> Defender = MakeAttributes();
		Defender->InitPhysicalDamageTakenMultiplier(0.f);
		const FCombatResolveResult Result = FCombatIncomingDamageResolver::MitigateDamagePacket(
			Packet, nullptr, nullptr, nullptr, Defender.Get(), MakeNonCritInfo());
		TestEqual(TEXT("A x0 taken multiplier negates the damage"), Result.PhysicalTaken, 0.f, Tolerance);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatArcaneShieldRoutingTest,
	"ProjectHunter.Combat.Incoming.ArcaneShieldRouting",
	PHCombatTests::TestFlags)

bool FPHCombatArcaneShieldRoutingTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	FCombatDamagePacket Packet;
	Packet.Physical = 100.f;

	const TStrongObjectPtr<UHunterAttributeSet> Defender = MakeAttributes();
	Defender->InitMaxHealth(500.f);
	Defender->InitHealth(500.f);
	Defender->InitArcaneShield(30.f);

	const FCombatResolveResult Result = FCombatIncomingDamageResolver::MitigateDamagePacket(
		Packet, nullptr, nullptr, nullptr, Defender.Get(), MakeNonCritInfo());

	TestEqual(TEXT("Arcane shield absorbs first, up to its current value"), Result.DamageToArcaneShield, 30.f, Tolerance);
	TestEqual(TEXT("The remainder falls through to health"), Result.DamageToHealth, 70.f, Tolerance);
	TestEqual(TEXT("Applied damage is the sum of both"), Result.TotalDamageApplied, 100.f, Tolerance);
	return true;
}

// ---------------------------------------------------------------------------
// Incoming: hit response  (C-01, locked decision 4)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatParryNegatesEverythingTest,
	"ProjectHunter.Combat.HitResponse.ParryNegatesDamageAndAilments",
	PHCombatTests::TestFlags)

bool FPHCombatParryNegatesEverythingTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	// Locked design decision: a successful parry blocks the complete attack.
	// No ailment, buildup, stagger or chip damage survives it.
	FCombatResolveResult Result;
	Result.PhysicalTaken = 40.f;
	Result.FireTaken = 30.f;
	Result.IceTaken = 20.f;
	Result.LightningTaken = 10.f;
	Result.LightTaken = 5.f;
	Result.CorruptionTaken = 5.f;
	Result.TotalDamageTaken = 110.f;
	Result.DamageToHealth = 110.f;
	Result.DamageToStamina = 12.f;
	Result.bShouldStagger = true;
	Result.EffectivePoiseDamage = 50.f;

	FCombatIncomingDamageResolver::ApplyHitResponse(EHitResponse::Parry, /*bCanApplyAilments*/ true, Result);

	TestEqual(TEXT("Parry zeroes physical"), Result.PhysicalTaken, 0.f, Tolerance);
	TestEqual(TEXT("Parry zeroes fire"), Result.FireTaken, 0.f, Tolerance);
	TestEqual(TEXT("Parry zeroes ice"), Result.IceTaken, 0.f, Tolerance);
	TestEqual(TEXT("Parry zeroes lightning"), Result.LightningTaken, 0.f, Tolerance);
	TestEqual(TEXT("Parry zeroes light"), Result.LightTaken, 0.f, Tolerance);
	TestEqual(TEXT("Parry zeroes corruption"), Result.CorruptionTaken, 0.f, Tolerance);
	TestEqual(TEXT("Parry zeroes total damage"), Result.TotalDamageTaken, 0.f, Tolerance);
	TestEqual(TEXT("Parry zeroes health damage"), Result.DamageToHealth, 0.f, Tolerance);
	TestEqual(TEXT("Parry costs no stamina"), Result.DamageToStamina, 0.f, Tolerance);
	TestEqual(TEXT("Parry zeroes poise damage"), Result.EffectivePoiseDamage, 0.f, Tolerance);
	TestFalse(TEXT("Parry prevents every ailment"), Result.bShouldApplyAilments);
	TestFalse(TEXT("Parry prevents stagger"), Result.bShouldStagger);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatInvincibleNegatesEverythingTest,
	"ProjectHunter.Combat.HitResponse.InvincibleNegatesDamage",
	PHCombatTests::TestFlags)

bool FPHCombatInvincibleNegatesEverythingTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	FCombatResolveResult Result;
	Result.PhysicalTaken = 80.f;
	Result.TotalDamageTaken = 80.f;
	Result.DamageToHealth = 80.f;
	Result.bWasCrit = true;
	Result.bShouldStagger = true;

	FCombatIncomingDamageResolver::ApplyHitResponse(EHitResponse::Invincible, true, Result);

	TestEqual(TEXT("I-frames zero all damage"), Result.TotalDamageTaken, 0.f, Tolerance);
	TestEqual(TEXT("I-frames zero health damage"), Result.DamageToHealth, 0.f, Tolerance);
	TestFalse(TEXT("I-frames prevent ailments"), Result.bShouldApplyAilments);
	TestFalse(TEXT("I-frames prevent stagger"), Result.bShouldStagger);
	TestFalse(TEXT("A negated hit is not a crit"), Result.bWasCrit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatAttackerCannotClaimDefenceTest,
	"ProjectHunter.Combat.HitResponse.AttackerCannotClaimDefence",
	PHCombatTests::TestFlags)

bool FPHCombatAttackerCannotClaimDefenceTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	// C-01 regression guard. ApplyHit runs on the attacker's component, so a
	// caller-supplied Parry/Invincible must not be trusted. With no defender ASC
	// there is no defender-owned state to justify either, so both downgrade.
	{
		bool bOverrode = false;
		const EHitResponse Resolved = FCombatIncomingDamageResolver::ResolveDefenderHitResponse(
			nullptr, EHitResponse::Parry, bOverrode);
		TestEqual(TEXT("An unbacked parry claim is downgraded"), Resolved, EHitResponse::Normal);
		TestTrue(TEXT("The override is reported"), bOverrode);
	}

	{
		bool bOverrode = false;
		const EHitResponse Resolved = FCombatIncomingDamageResolver::ResolveDefenderHitResponse(
			nullptr, EHitResponse::Invincible, bOverrode);
		TestEqual(TEXT("An unbacked invincibility claim is downgraded"), Resolved, EHitResponse::Normal);
		TestTrue(TEXT("The override is reported"), bOverrode);
	}

	// Normal and Blocked carry no defensive assertion, so they pass through.
	{
		bool bOverrode = false;
		const EHitResponse Resolved = FCombatIncomingDamageResolver::ResolveDefenderHitResponse(
			nullptr, EHitResponse::Normal, bOverrode);
		TestEqual(TEXT("Normal passes through"), Resolved, EHitResponse::Normal);
		TestFalse(TEXT("Normal is not an override"), bOverrode);
	}

	{
		bool bOverrode = false;
		const EHitResponse Resolved = FCombatIncomingDamageResolver::ResolveDefenderHitResponse(
			nullptr, EHitResponse::Blocked, bOverrode);
		TestEqual(TEXT("Blocked passes through"), Resolved, EHitResponse::Blocked);
		TestFalse(TEXT("Blocked is not an override"), bOverrode);
	}

	return true;
}

// ---------------------------------------------------------------------------
// Incoming: block is Souls-style, never a random chance
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatBlockRequiresGuardStateTest,
	"ProjectHunter.Combat.Incoming.BlockRequiresGuardState",
	PHCombatTests::TestFlags)

bool FPHCombatBlockRequiresGuardStateTest::RunTest(const FString&)
{
	using namespace PHCombatTests;

	// Blocking depends on actual guard state and facing, never on a roll. A
	// defender with high BlockStrength but no guard state blocks nothing, and
	// the result is identical across repeated evaluations.
	FCombatDamagePacket Packet;
	Packet.Physical = 100.f;

	const TStrongObjectPtr<UHunterAttributeSet> Defender = MakeAttributes();
	Defender->InitBlockStrength(80.f);

	float FirstTaken = -1.f;
	for (int32 Iteration = 0; Iteration < 16; ++Iteration)
	{
		const FCombatResolveResult Result = FCombatIncomingDamageResolver::MitigateDamagePacket(
			Packet, nullptr, nullptr, nullptr, Defender.Get(), MakeNonCritInfo());

		TestFalse(TEXT("No guard state means no block"), Result.bWasBlocked);
		if (Iteration == 0)
		{
			FirstTaken = Result.PhysicalTaken;
		}
		else
		{
			TestEqual(TEXT("Block resolution is deterministic, not a chance roll"),
				Result.PhysicalTaken, FirstTaken, KINDA_SMALL_NUMBER);
		}
	}

	TestEqual(TEXT("Damage passes through unblocked"), FirstTaken, 100.f, Tolerance);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "GameplayEffect.h"
#include "Item/Library/Structs/ItemAttributeStructs.h"
#include "Stats/Library/FunctionLibraries/EquipmentStatsApplier.h"

namespace EquipmentStatsApplierTests
{
	constexpr float Tolerance = 0.001f;

	/** Magnitude of the modifier targeting Attribute, or a miss. */
	bool FindModifierMagnitude(
		const UGameplayEffect& Effect,
		const FGameplayAttribute& Attribute,
		float& OutMagnitude)
	{
		for (const FGameplayModifierInfo& Modifier : Effect.Modifiers)
		{
			if (Modifier.Attribute != Attribute)
			{
				continue;
			}

			if (Modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.f, OutMagnitude))
			{
				return true;
			}
		}

		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGlobalRangeAffixAppliesBothEndpointsTest,
	"ProjectHunter.Item.Affix.GlobalRangeAffixAppliesBothEndpoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGlobalRangeAffixAppliesBothEndpointsTest::RunTest(const FString& Parameters)
{
	using namespace EquipmentStatsApplierTests;

	// "Adds 5-12 Fire Damage" on a ring. Accessories have no local resolver, so
	// this has to go through the global path - which used to apply the lower
	// endpoint only while the tooltip still advertised the full range.
	FPHAttributeData AddedFire;
	AddedFire.AffixID = TEXT("AddedFireDamage");
	AddedFire.ModifiedLocation = EAffixScope::AS_Global;
	AddedFire.ModifiedAttribute = UHunterAttributeSet::GetMinFireDamageAttribute();
	AddedFire.ModifyType = EModifyType::MT_AddRange;
	AddedFire.RolledStatValue = 5.f;
	AddedFire.RolledSecondaryStatValue = 12.f;

	UGameplayEffect* Effect = NewObject<UGameplayEffect>();
	TestTrue(TEXT("The range affix produces modifiers"),
		FEquipmentStatsApplier::ApplyStatModifier(Effect, AddedFire, AddedFire.ModifiedAttribute));

	TestEqual(TEXT("A global range affix emits one modifier per endpoint"),
		Effect->Modifiers.Num(), 2);

	float MinMagnitude = 0.f;
	if (TestTrue(TEXT("The lower endpoint reaches the Min attribute"),
		FindModifierMagnitude(*Effect, UHunterAttributeSet::GetMinFireDamageAttribute(), MinMagnitude)))
	{
		TestEqual(TEXT("Lower endpoint value"), MinMagnitude, 5.f, Tolerance);
	}

	float MaxMagnitude = 0.f;
	if (TestTrue(TEXT("The upper endpoint reaches the Max attribute"),
		FindModifierMagnitude(*Effect, UHunterAttributeSet::GetMaxFireDamageAttribute(), MaxMagnitude)))
	{
		TestEqual(TEXT("Upper endpoint value"), MaxMagnitude, 12.f, Tolerance);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNonRangeAffixEmitsOneModifierTest,
	"ProjectHunter.Item.Affix.NonRangeAffixEmitsOneModifier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNonRangeAffixEmitsOneModifierTest::RunTest(const FString& Parameters)
{
	using namespace EquipmentStatsApplierTests;

	// The pairing must not leak into ordinary affixes.
	FPHAttributeData FlatFire;
	FlatFire.AffixID = TEXT("FlatFireDamage");
	FlatFire.ModifiedLocation = EAffixScope::AS_Global;
	FlatFire.ModifiedAttribute = UHunterAttributeSet::GetMinFireDamageAttribute();
	FlatFire.ModifyType = EModifyType::MT_Add;
	FlatFire.RolledStatValue = 5.f;
	FlatFire.RolledSecondaryStatValue = 12.f;

	UGameplayEffect* Effect = NewObject<UGameplayEffect>();
	TestTrue(TEXT("The flat affix produces a modifier"),
		FEquipmentStatsApplier::ApplyStatModifier(Effect, FlatFire, FlatFire.ModifiedAttribute));

	TestEqual(TEXT("A non-range affix emits exactly one modifier"), Effect->Modifiers.Num(), 1);

	float Magnitude = 0.f;
	if (TestTrue(TEXT("It targets the authored attribute"),
		FindModifierMagnitude(*Effect, UHunterAttributeSet::GetMinFireDamageAttribute(), Magnitude)))
	{
		TestEqual(TEXT("Using the primary rolled value"), Magnitude, 5.f, Tolerance);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnpairableRangeAffixStillAppliesTest,
	"ProjectHunter.Item.Affix.UnpairableRangeAffixStillApplies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnpairableRangeAffixStillAppliesTest::RunTest(const FString& Parameters)
{
	using namespace EquipmentStatsApplierTests;

	// A range affix authored against something that is not a damage Min has no
	// second attribute to carry its upper endpoint. It warns, but the primary
	// value must still land rather than the whole affix being dropped.
	AddExpectedError(TEXT("upper endpoint cannot be applied"),
		EAutomationExpectedErrorFlags::Contains, 0);

	FPHAttributeData OddRange;
	OddRange.AffixID = TEXT("OddRange");
	OddRange.ModifiedLocation = EAffixScope::AS_Global;
	OddRange.ModifiedAttribute = UHunterAttributeSet::GetAttackSpeedAttribute();
	OddRange.ModifyType = EModifyType::MT_AddRange;
	OddRange.RolledStatValue = 3.f;
	OddRange.RolledSecondaryStatValue = 9.f;

	UGameplayEffect* Effect = NewObject<UGameplayEffect>();
	TestTrue(TEXT("The affix still applies"),
		FEquipmentStatsApplier::ApplyStatModifier(Effect, OddRange, OddRange.ModifiedAttribute));
	TestEqual(TEXT("Only the primary endpoint is emitted"), Effect->Modifiers.Num(), 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

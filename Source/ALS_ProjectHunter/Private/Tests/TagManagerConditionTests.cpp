#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

#include "AbilitySystemComponent.h"
#include "Tags/Components/TagManager.h"
#include "Tags/Helpers/TagConditionEvaluator.h"
#include "Tags/PHGameplayTags.h"

namespace PHTagManagerTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHTagManagerPreservesExternalTagCountsTest,
	"ProjectHunter.Tags.Manager.PreservesExternalTagCounts",
	PHTagManagerTests::TestFlags)

bool FPHTagManagerPreservesExternalTagCountsTest::RunTest(const FString&)
{
	TStrongObjectPtr<UAbilitySystemComponent> FirstASC(NewObject<UAbilitySystemComponent>(GetTransientPackage()));
	TStrongObjectPtr<UAbilitySystemComponent> SecondASC(NewObject<UAbilitySystemComponent>(GetTransientPackage()));
	TStrongObjectPtr<UTagManager> Manager(NewObject<UTagManager>(GetTransientPackage()));
	Manager->Initialize(FirstASC.Get());

	const FGameplayTag TestTag = FPHGameplayTags::Get().Condition_TakingDamage;
	FirstASC->AddLooseGameplayTag(TestTag);
	Manager->AddTag(TestTag);
	TestEqual(TEXT("Manager adds its own contribution beside an external source"), FirstASC->GetTagCount(TestTag), 2);

	Manager->RemoveTag(TestTag);
	TestEqual(TEXT("Removing the managed state preserves the external source"), FirstASC->GetTagCount(TestTag), 1);

	Manager->RemoveTag(TestTag);
	TestEqual(TEXT("Repeated removal cannot consume an external source"), FirstASC->GetTagCount(TestTag), 1);

	Manager->AddTag(TestTag);
	Manager->Initialize(SecondASC.Get());
	TestEqual(TEXT("Switching ASCs removes only the manager contribution from the old ASC"), FirstASC->GetTagCount(TestTag), 1);
	TestEqual(TEXT("Transient external state is not copied to the new ASC"), SecondASC->GetTagCount(TestTag), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHTagManagerExternalCombatSourceTest,
	"ProjectHunter.Tags.Manager.ExternalCombatSourceDoesNotLatch",
	PHTagManagerTests::TestFlags)

bool FPHTagManagerExternalCombatSourceTest::RunTest(const FString&)
{
	TStrongObjectPtr<UAbilitySystemComponent> ASC(NewObject<UAbilitySystemComponent>(GetTransientPackage()));
	TStrongObjectPtr<UTagManager> Manager(NewObject<UTagManager>(GetTransientPackage()));
	Manager->Initialize(ASC.Get());

	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	ASC->AddLooseGameplayTag(Tags.Condition_InCombat);
	Manager->RefreshBaseConditionTags();
	TestTrue(TEXT("An external combat source keeps the derived combat condition active"), Manager->HasTag(Tags.Condition_InCombat));
	TestFalse(TEXT("Out of combat is disabled while an external combat source exists"), Manager->HasTag(Tags.Condition_OutOfCombat));

	ASC->RemoveLooseGameplayTag(Tags.Condition_InCombat);
	Manager->RefreshBaseConditionTags();
	TestFalse(TEXT("The manager contribution does not latch after the external source ends"), Manager->HasTag(Tags.Condition_InCombat));
	TestTrue(TEXT("Out of combat returns after the final combat source ends"), Manager->HasTag(Tags.Condition_OutOfCombat));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHTagResourceConditionHysteresisTest,
	"ProjectHunter.Tags.Conditions.ResourceHysteresis",
	PHTagManagerTests::TestFlags)

bool FPHTagResourceConditionHysteresisTest::RunTest(const FString&)
{
	const FTagConditionThresholds Thresholds;

	FTagResourceConditionState State = FTagConditionEvaluator::EvaluateResource(
		35.0f, 100.0f, false, false, Thresholds);
	TestTrue(TEXT("Low resource enters at the configured boundary"), State.bLow);
	TestFalse(TEXT("Low resource cannot also be full"), State.bFull);

	State = FTagConditionEvaluator::EvaluateResource(38.0f, 100.0f, State.bLow, State.bFull, Thresholds);
	TestTrue(TEXT("Low resource remains active inside the exit hysteresis band"), State.bLow);

	State = FTagConditionEvaluator::EvaluateResource(41.0f, 100.0f, State.bLow, State.bFull, Thresholds);
	TestFalse(TEXT("Low resource exits above the configured exit boundary"), State.bLow);

	State = FTagConditionEvaluator::EvaluateResource(100.0f, 100.0f, false, false, Thresholds);
	TestTrue(TEXT("Full resource enters at maximum"), State.bFull);

	State = FTagConditionEvaluator::EvaluateResource(99.7f, 100.0f, State.bLow, State.bFull, Thresholds);
	TestTrue(TEXT("Full resource remains active inside the exit hysteresis band"), State.bFull);

	State = FTagConditionEvaluator::EvaluateResource(99.4f, 100.0f, State.bLow, State.bFull, Thresholds);
	TestFalse(TEXT("Full resource exits below the configured exit boundary"), State.bFull);

	State = FTagConditionEvaluator::EvaluateResource(0.0f, 0.0f, true, true, Thresholds);
	TestFalse(TEXT("Invalid maximum clears low state"), State.bLow);
	TestFalse(TEXT("Invalid maximum clears full state"), State.bFull);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHTagMovementConditionHysteresisTest,
	"ProjectHunter.Tags.Conditions.MovementHysteresis",
	PHTagManagerTests::TestFlags)

bool FPHTagMovementConditionHysteresisTest::RunTest(const FString&)
{
	const FTagConditionThresholds Thresholds;
	bool bMoving = FTagConditionEvaluator::EvaluateMovement(FMath::Square(21.0f), false, false, Thresholds);
	TestTrue(TEXT("Movement starts above the start speed"), bMoving);

	bMoving = FTagConditionEvaluator::EvaluateMovement(FMath::Square(10.0f), true, bMoving, Thresholds);
	TestTrue(TEXT("Movement remains active inside the stop hysteresis band"), bMoving);

	bMoving = FTagConditionEvaluator::EvaluateMovement(FMath::Square(4.0f), true, bMoving, Thresholds);
	TestFalse(TEXT("Movement stops below the stop speed"), bMoving);

	bMoving = FTagConditionEvaluator::EvaluateMovement(FMath::Square(10.0f), true, bMoving, Thresholds);
	TestFalse(TEXT("Stationary state does not restart inside the hysteresis band"), bMoving);
	return true;
}

#endif

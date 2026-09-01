// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tags/Components/TagManager.h"
#include "Tests/PHDeadStateProbe.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDeadStateReportsDeathTest,
	"ProjectHunter.Character.Death.MarkingTheTagReportsDeath",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FPHDeadStateReportsDeathTest::RunTest(const FString&)
{
	// The gap this covers cost a working feature: the AI death graph ragdolled, destroyed its
	// controller and set the dead tag, but never called NotifyDeath. Nothing broadcast OnDeath, so
	// the mob manager never raised OnMobDied, no kill was counted, and a floor could never be
	// cleared. The character now listens for the tag being set, and this asserts the signal it
	// listens to actually fires.
	TStrongObjectPtr<UTagManager> Tags(NewObject<UTagManager>());
	TStrongObjectPtr<UPHDeadStateProbe> Probe(NewObject<UPHDeadStateProbe>());

	Tags->OnDeadStateChanged.AddDynamic(Probe.Get(), &UPHDeadStateProbe::OnDeadStateChanged);

	Tags->SetDeadState(true);
	TestEqual(TEXT("Marking the dead state reports it once"), Probe->Calls, 1);
	TestTrue(TEXT("It reports that the character died"), Probe->LastValue);

	// Clearing it has to be distinguishable, or a revive or a pooled reuse would read as a death.
	Tags->SetDeadState(false);
	TestEqual(TEXT("Clearing the dead state also reports"), Probe->Calls, 2);
	TestFalse(TEXT("It reports that the character is no longer dead"), Probe->LastValue);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

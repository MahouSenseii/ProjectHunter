// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/PHMenuPreviewEditorLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHAuthoredMenuPresentationTest,
	"ProjectHunter.Menu.Presentation.AuthoredBindingsTabsAndInventorySelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPHAuthoredMenuPresentationTest::RunTest(const FString&)
{
	return TestTrue(TEXT("Actual menu bindings, native owner listeners, tab clicks, selection and cached cells remain functional"),
		UPHMenuPreviewEditorLibrary::ValidateSystemMenu());
}

#endif

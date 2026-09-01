// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

// The corner-accent material could not be verified from a render: it is
// time-driven, and a single offscreen frame shows nothing whether it is wired
// correctly or not. These cases assert the wiring instead - that the accent
// image is bound, that it has a dynamic material, and that the two parameters
// the material needs from its widget actually arrive.
//
// That is the defect this covers: the material was assigned but never given a
// MaskValue or an AspectRatio, so its squares matched neither the window's
// shape nor its proportions.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Engine/Blueprint.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PreviewScene.h"
#include "UI/Menu/Widgets/PHMenuRootWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace PHMenuAccentTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	const TCHAR* MenuRootPath = TEXT("/Game/ProjectHunter/UI/Widgets/Menus/System/WBP_SystemMenuRoot");
	const TCHAR* MaskPath = TEXT("/Game/ProjectHunter/UI/Widgets/Menus/System/T_SystemPanel_Mask");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHMenuAccentMaterialTest,
	"ProjectHunter.Menu.Accents.MaterialReceivesMaskAndAspect",
	PHMenuAccentTests::TestFlags)

bool FPHMenuAccentMaterialTest::RunTest(const FString&)
{
	using namespace PHMenuAccentTests;

	UBlueprint* MenuBlueprint = LoadObject<UBlueprint>(nullptr, MenuRootPath);
	if (!TestNotNull(TEXT("WBP_SystemMenuRoot loads"), MenuBlueprint)
		|| !TestNotNull(TEXT("It has a generated class"), MenuBlueprint->GeneratedClass.Get()))
	{
		return false;
	}

	FPreviewScene Scene{FPreviewScene::ConstructionValues()
		.SetEditor(false).SetTransactional(false)
		.SetCreateDefaultLighting(false).AllowAudioPlayback(false)};

	TStrongObjectPtr<UPHMenuRootWidget> Root(
		CreateWidget<UPHMenuRootWidget>(Scene.GetWorld(), MenuBlueprint->GeneratedClass.Get()));
	if (!TestNotNull(TEXT("The menu root constructs"), Root.Get()))
	{
		return false;
	}

	// Building the Slate widget is what runs NativeConstruct, which is where the
	// dynamic material is created.
	TSharedPtr<SWidget> Slate = Root->TakeWidget();

	UImage* Accents = Cast<UImage>(Root->GetWidgetFromName(TEXT("ScanlineSweep")));
	if (!TestNotNull(TEXT("The authored accent image exists and is an Image"), Accents))
	{
		return false;
	}

	UMaterialInstanceDynamic* Dynamic = Cast<UMaterialInstanceDynamic>(
		Accents->GetBrush().GetResourceObject());
	if (!TestNotNull(TEXT("The accent image has a dynamic material instance"), Dynamic))
	{
		// Without a dynamic instance nothing can be pushed at the material, which
		// is exactly the original defect.
		return false;
	}

	UTexture* Mask = nullptr;
	const bool bHasMask = Dynamic->GetTextureParameterValue(FName(TEXT("MaskValue")), Mask);
	TestTrue(TEXT("The material exposes MaskValue"), bHasMask);
	if (TestNotNull(TEXT("MaskValue is set"), Mask))
	{
		TestEqual(TEXT("MaskValue is the panel silhouette, so accents follow the window shape"),
			Mask->GetPathName(), FString(MaskPath) + TEXT(".T_SystemPanel_Mask"));
	}

	// The aspect is pushed on the first tick with a real size. NativeTick is
	// protected, so drive it the way Slate does rather than widening the widget's
	// API for a test.
	const FGeometry Geometry = FGeometry::MakeRoot(FVector2D(1920.0, 1080.0), FSlateLayoutTransform());
	Slate->Tick(Geometry, FPlatformTime::Seconds(), 0.016f);

	float Aspect = 0.0f;
	const bool bHasAspect = Dynamic->GetScalarParameterValue(FName(TEXT("AspectRatio")), Aspect);
	TestTrue(TEXT("The material exposes AspectRatio"), bHasAspect);

	// 1080/1920. Without this the material's squares render as rectangles.
	TestEqual(TEXT("AspectRatio matches the widget it is drawn on"), Aspect, 0.5625f, 0.001f);

	Slate.Reset();
	Root->ReleaseSlateResources(true);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/Menu/Library/Enums/MenuEnums.h"
#include "UI/Menu/Widgets/PHSettingsMenuPageWidget.h"
#include "PHMenuPreviewEditorLibrary.generated.h"

/** Explicit, read-only inspection and transient previews of the existing system menu. */
UCLASS()
class ALS_PROJECTHUNTEREDITOR_API UPHMenuPreviewEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Captures all eleven authored menu widget contracts and settings without compiling or saving them. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Project Hunter|Menu Editor")
	static bool InspectMenuContracts(const FString& OutputJSONPath);

	/** Exercises the real tabs, panel bindings, inventory selection and page caching in a transient world. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Project Hunter|Menu Editor")
	static bool ValidateSystemMenu();

	/**
	 * Renders the actual menu with project viewport DPI and authored DA_BaseStats values.
	 * UI-only: the world-character area stays transparent. Requires a real RHI.
	 * Writes a PNG and a sibling JSON describing the identical, transient preview fixture.
	 *
	 * PageToShow selects which tab is open in the capture. It defaults to
	 * Equipment so existing callers render exactly what they did before.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Project Hunter|Menu Editor")
	static bool RenderSystemMenu(const FString& OutputPNGPath, int32 Width = 1920, int32 Height = 1080,
		EMenuType PageToShow = EMenuType::MT_Equipment,
		EPHSettingsSection SettingsSection = EPHSettingsSection::SS_Gameplay);
};

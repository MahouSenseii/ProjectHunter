// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PHHUDEditorLibrary.generated.h"

/** Explicit editor operations for the existing player HUD; never run automatically. */
UCLASS()
class ALS_PROJECTHUNTEREDITOR_API UPHHUDEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Writes the authored template tree and Blueprint contract inventory without saving assets. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Project Hunter|HUD Editor")
	static bool InspectPlayerHUD(const FString& OutputJSONPath);

	/** Backs up and edits only the player HUD and, if absent, imports the supplied emblem texture. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Project Hunter|HUD Editor")
	static bool ApplyHealthManaLayout(const FString& EmblemSourcePath);

	/** Adds the run-status/mission panel and existing floor-banner class without restyling resources. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Project Hunter|HUD Editor")
	static bool ApplyFloorAndMissionLayout();

	/** Renders the generated HUD. Sample values affect this transient preview only. Requires a real RHI. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Project Hunter|HUD Editor")
	static bool RenderPlayerHUD(const FString& OutputPNGPath, int32 Width = 1920, int32 Height = 1080,
		float HealthPercent = 0.85f, float ManaPercent = 0.7f, int32 PreviewLevel = 1,
		float PreviewHealthCurrent = 7500.0f, float PreviewHealthMax = 8600.0f,
		int32 PreviewFloor = 0, int32 PreviewRemainingEnemies = 0, int32 PreviewEnemyTarget = 0,
		float PreviewBannerTime = -1.0f);
};

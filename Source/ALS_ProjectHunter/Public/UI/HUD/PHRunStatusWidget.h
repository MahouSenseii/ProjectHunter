// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Tower/Library/Structs/RunStructs.h"
#include "PHRunStatusWidget.generated.h"

class UPanelWidget;
class UTextBlock;

/** Displays the existing run snapshot. Owns no objectives, kill counts, or mission state. */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UPHRunStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HUD|Run")
	void ApplyRunSnapshot(ERunState State, const FRunSessionData& Session);

	/** Add presentation widgets here from an existing mission owner; snapshot updates preserve them. */
	UFUNCTION(BlueprintPure, Category = "HUD|Missions")
	UPanelWidget* GetMissionContainer() const;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Run", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FloorLabelText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Run", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RemainingEnemiesText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Run", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FloorMissionText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Missions", meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> MissionEntries;
};

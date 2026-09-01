// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PHPassiveTreeSettings.generated.h"

class UPHPassiveTreeDataAsset;

/** Project-wide default passive graph. Individual character components may override it. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Project Hunter Passive Tree"))
class ALS_PROJECTHUNTER_API UPHPassiveTreeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Passive Tree")
	TSoftObjectPtr<UPHPassiveTreeDataAsset> DefaultTree;
};

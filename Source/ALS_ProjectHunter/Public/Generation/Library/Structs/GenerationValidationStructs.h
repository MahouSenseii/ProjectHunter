// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Generation/Library/Enums/GenerationValidationEnums.h"
#include "GenerationValidationStructs.generated.h"

/** A structural error. The code identifies the affected collection or layout-level field. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHGenerationIssue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Generation|Validation")
	EPHGenerationIssueCode Code = EPHGenerationIssueCode::None;

	/** Array index in the affected collection, or INDEX_NONE for a layout-level issue. */
	UPROPERTY(BlueprintReadOnly, Category = "Generation|Validation")
	int32 ElementIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Generation|Validation")
	FString Message;
};

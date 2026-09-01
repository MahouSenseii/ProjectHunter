// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Generation/Library/Structs/GeneratedLayoutStructs.h"
#include "Generation/Library/Structs/GenerationValidationStructs.h"
#include "PHGenerationValidationLibrary.generated.h"

/** Stateless logical validation; never constructs a world or changes the run owner. */
UCLASS()
class ALS_PROJECTHUNTER_API UPHGenerationValidationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Replaces OutIssues and returns true only for a structurally valid, reachable layout.
	 * Bounds may overlap; clearance, physical traversal, and generator-specific constraints
	 * need later checks. Connectivity runs after local errors are fixed to avoid cascades.
	 */
	UFUNCTION(BlueprintCallable, Category = "Generation|Validation")
	static bool ValidateLayout(const FPHGeneratedLayout& Layout, TArray<FPHGenerationIssue>& OutIssues);
};

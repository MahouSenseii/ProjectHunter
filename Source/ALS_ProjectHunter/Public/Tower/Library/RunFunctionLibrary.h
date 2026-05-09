// Tower/Library/RunFunctionLibrary.h
// Blueprint-callable static helpers for the run system.
// Use these to query run state without taking a direct dependency on URunSubsystem.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Tower/Library/RunEnumLibrary.h"
#include "Tower/Library/RunStructs.h"
#include "RunFunctionLibrary.generated.h"

class URunSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogRunFunctionLibrary, Log, All);

/**
 * Static utility functions for querying the run system.
 *
 * Prefer these over getting URunSubsystem directly when you only need
 * a single value — avoids subsystem casts scattered across the codebase.
 */
UCLASS()
class ALS_PROJECTHUNTER_API URunFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// ═══════════════════════════════════════════════
	// SUBSYSTEM ACCESS
	// ═══════════════════════════════════════════════

	/**
	 * Returns the URunSubsystem from the provided world context.
	 * Returns nullptr if no game instance or subsystem is available.
	 */
	UFUNCTION(BlueprintPure, Category = "Run|Utility",
		meta = (WorldContext = "WorldContextObject"))
	static URunSubsystem* GetRunSubsystem(const UObject* WorldContextObject);

	// ═══════════════════════════════════════════════
	// STATE QUERIES
	// ═══════════════════════════════════════════════

	/** Returns the current run state without needing a subsystem reference. */
	UFUNCTION(BlueprintPure, Category = "Run|Utility",
		meta = (WorldContext = "WorldContextObject"))
	static ERunState GetRunState(const UObject* WorldContextObject);

	/** True if a run is currently active. */
	UFUNCTION(BlueprintPure, Category = "Run|Utility",
		meta = (WorldContext = "WorldContextObject"))
	static bool IsRunActive(const UObject* WorldContextObject);

	/** Returns the current floor number. Returns 0 if no run is active. */
	UFUNCTION(BlueprintPure, Category = "Run|Utility",
		meta = (WorldContext = "WorldContextObject"))
	static int32 GetCurrentFloor(const UObject* WorldContextObject);

	/** Returns total kills this run. Returns 0 if no run is active. */
	UFUNCTION(BlueprintPure, Category = "Run|Utility",
		meta = (WorldContext = "WorldContextObject"))
	static int32 GetTotalKills(const UObject* WorldContextObject);

	// ═══════════════════════════════════════════════
	// FORMATTING
	// ═══════════════════════════════════════════════

	/**
	 * Formats a raw seconds value into a MM:SS display string.
	 * e.g. 125.4f → "02:05"
	 * Use for the in-run timer HUD and the end-of-run screen.
	 */
	UFUNCTION(BlueprintPure, Category = "Run|Utility")
	static FString FormatRunTime(float Seconds);
};

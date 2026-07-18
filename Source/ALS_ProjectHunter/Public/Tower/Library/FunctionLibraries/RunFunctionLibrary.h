#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Tower/Library/Enums/RunEnumLibrary.h"
#include "Tower/Library/Structs/RunStructs.h"
#include "RunFunctionLibrary.generated.h"

class URunSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogRunFunctionLibrary, Log, All);

UCLASS()
class ALS_PROJECTHUNTER_API URunFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Run|Utility",
		meta = (WorldContext = "WorldContextObject"))
	static URunSubsystem* GetRunSubsystem(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Run|Utility",
		meta = (WorldContext = "WorldContextObject"))
	static ERunState GetRunState(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Run|Utility",
		meta = (WorldContext = "WorldContextObject"))
	static bool IsRunActive(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Run|Utility",
		meta = (WorldContext = "WorldContextObject"))
	static int32 GetCurrentFloor(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Run|Utility",
		meta = (WorldContext = "WorldContextObject"))
	static int32 GetTotalKills(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Run|Utility")
	static FString FormatRunTime(float Seconds);
};

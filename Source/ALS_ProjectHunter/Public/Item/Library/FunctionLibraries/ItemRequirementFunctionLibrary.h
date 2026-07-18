// Item/Library/FunctionLibraries/ItemRequirementFunctionLibrary.h

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/Structs/ItemRequirementStructs.h"
#include "ItemRequirementFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API UItemRequirementFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Item|Requirements", meta = (
		DisplayName = "Meets Item Requirements",
		Keywords = "requirements meet stat check hunter"))
	static bool MeetsItemRequirements(
		const FItemStatRequirement& Requirements,
		int32 HunterLevel,
		int32 Strength,
		int32 Dexterity,
		int32 Intelligence,
		int32 Endurance,
		int32 Affliction,
		int32 Luck,
		int32 Covenant);

	UFUNCTION(BlueprintPure, Category = "Item|Requirements", meta = (
		DisplayName = "Get Required Level",
		Keywords = "requirements level stat"))
	static int32 GetRequiredLevel(const FItemStatRequirement& Requirements);
};

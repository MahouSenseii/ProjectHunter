// Item/Library/Structs/ItemRequirementStructs.h
#pragma once

#include "CoreMinimal.h"
#include "ItemRequirementStructs.generated.h"

USTRUCT(BlueprintType)
struct FItemStatRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredStrength = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredDexterity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredIntelligence = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredEndurance = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredAffliction = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredLuck = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredCovenant = 0;

	FItemStatRequirement() = default;

	bool MeetsRequirements(
		int32 Level,
		int32 Strength,
		int32 Dexterity,
		int32 Intelligence,
		int32 Endurance,
		int32 Affliction,
		int32 Luck,
		int32 Covenant) const
	{
		return Level >= RequiredLevel
			&& Strength >= RequiredStrength
			&& Dexterity >= RequiredDexterity
			&& Intelligence >= RequiredIntelligence
			&& Endurance >= RequiredEndurance
			&& Affliction >= RequiredAffliction
			&& Luck >= RequiredLuck
			&& Covenant >= RequiredCovenant;
	}
};

// Item/Library/Structs/ItemRequirementStructs.h
#pragma once

#include "CoreMinimal.h"
#include "ItemRequirementStructs.generated.h"

UENUM(BlueprintType)
enum class EItemRequirementType : uint8
{
	Level,
	Strength,
	Dexterity,
	Intelligence,
	Endurance,
	Affliction,
	Luck,
	Covenant
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FItemRequirementFailure
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Item|Requirements")
	EItemRequirementType RequirementType = EItemRequirementType::Level;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Requirements")
	float CurrentValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Requirements")
	float RequiredValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Requirements")
	float MissingValue = 0.0f;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FItemRequirementStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Item|Requirements")
	EItemRequirementType RequirementType = EItemRequirementType::Level;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Requirements")
	float CurrentValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Requirements")
	float RequiredValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Requirements")
	float MissingValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Requirements")
	bool bMet = false;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FItemRequirementCheckResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Item|Requirements")
	bool bItemValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Requirements")
	bool bStatsAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Requirements")
	bool bMeetsRequirements = false;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Requirements")
	TArray<FItemRequirementFailure> Failures;

	/** One entry for every authored non-zero requirement, including requirements that pass. */
	UPROPERTY(BlueprintReadOnly, Category = "Item|Requirements")
	TArray<FItemRequirementStatus> Checks;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FItemStatRequirement
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

	FItemRequirementCheckResult EvaluateRequirements(
		float Level,
		float Strength,
		float Dexterity,
		float Intelligence,
		float Endurance,
		float Affliction,
		float Luck,
		float Covenant) const;

	bool MeetsRequirements(
		int32 Level,
		int32 Strength,
		int32 Dexterity,
		int32 Intelligence,
		int32 Endurance,
		int32 Affliction,
		int32 Luck,
		int32 Covenant) const;
};

// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StatsMenuStructs.generated.h"

/** How a stat's number should read. Percent and seconds are not interchangeable. */
UENUM(BlueprintType)
enum class EPHStatFormat : uint8
{
	SF_Integer   UMETA(DisplayName = "Integer"),
	SF_Decimal   UMETA(DisplayName = "Decimal"),
	SF_Percent   UMETA(DisplayName = "Percent"),
	SF_Seconds   UMETA(DisplayName = "Seconds")
};

/**
 * One line on the stats page.
 *
 * The tag is the identity: the value is resolved through
 * FPHGameplayTags::GetAttributeFromTag, so adding a stat to the page needs a
 * row here and no code. An unmapped tag is skipped rather than shown as zero,
 * because a zero is indistinguishable from a real value the player earned.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHStatRowDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Row")
	FGameplayTag AttributeTag;

	/** Leave empty to derive the label from the tag's last element. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Row")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Row")
	EPHStatFormat Format = EPHStatFormat::SF_Integer;

	/**
	 * Attribute name passed to UCharacterProgressionManager::SpendStatPoint.
	 * Only the primary attributes are spendable; leave None elsewhere and the
	 * row renders without a spend control.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Row")
	FName SpendableAttributeName = NAME_None;
};

/** A titled, collapsible block of stat rows. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHStatGroupDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Group")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Group")
	bool bStartExpanded = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Group")
	TArray<FPHStatRowDef> Rows;
};

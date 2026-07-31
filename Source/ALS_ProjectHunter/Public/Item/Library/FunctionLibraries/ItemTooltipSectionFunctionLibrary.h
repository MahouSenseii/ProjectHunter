#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/Structs/ItemTooltipStructs.h"
#include "ItemTooltipSectionFunctionLibrary.generated.h"

class UItemInstance;
struct FBaseArmorStats;
struct FBaseWeaponStats;
struct FConsumableData;
struct FItemBase;
struct FItemDurability;
struct FItemStatRequirement;
struct FPHAttributeData;

UCLASS()
class ALS_PROJECTHUNTER_API UItemTooltipSectionFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static FItemTooltipSection MakeTooltipSection(
		EItemTooltipSectionType Type,
		const FText& Heading,
		bool bShowHeading = true);

	static void AddSectionIfAny(FItemTooltipData& TooltipData, FItemTooltipSection& Section);
	static void AddAffixSection(FItemTooltipData& TooltipData, EItemTooltipSectionType Type, const FText& Heading, const TArray<FPHAttributeData>& Affixes);
	static void AddWeaponStatsSection(
		FItemTooltipData& TooltipData,
		const FBaseWeaponStats& Stats,
		const FItemDurability& Durability);
	static void AddArmorStatsSection(
		FItemTooltipData& TooltipData,
		const FBaseArmorStats& Stats,
		const FItemDurability& Durability);
	static void AddRequirementsSection(FItemTooltipData& TooltipData, const FItemStatRequirement& Requirements);
	static void AddRunesSection(FItemTooltipData& TooltipData, const UItemInstance* Item, const FItemBase& Base);
	static void AddConsumableSection(FItemTooltipData& TooltipData, const UItemInstance* Item, const FConsumableData& ConsumableData);
	static void AddDetailsSection(FItemTooltipData& TooltipData, const UItemInstance* Item);
	static void AddDescriptionSection(FItemTooltipData& TooltipData, const FItemBase& Base);
};

// Item/Library/FunctionLibraries/ItemEnumFunctionLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/Enums/AffixEnums.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "ItemEnumFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API UItemEnumFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Item|Types")
	static bool IsValidItemType(EItemType Type);

	UFUNCTION(BlueprintPure, Category = "Item|Types")
	static bool IsValidItemSubType(EItemSubType SubType);

	UFUNCTION(BlueprintPure, Category = "Item|Types")
	static bool IsWeaponItemSubType(EItemSubType SubType);

	UFUNCTION(BlueprintPure, Category = "Item|Types")
	static bool IsArmorItemSubType(EItemSubType SubType);

	UFUNCTION(BlueprintPure, Category = "Item|Types")
	static bool IsAccessoryItemSubType(EItemSubType SubType);

	UFUNCTION(BlueprintPure, Category = "Item|Types")
	static bool IsConsumableItemSubType(EItemSubType SubType);

	UFUNCTION(BlueprintPure, Category = "Item|Types")
	static bool IsItemSubTypeAllowedForItemType(EItemType ItemType, EItemSubType SubType);

	UFUNCTION(BlueprintPure, Category = "Item|Display")
	static FLinearColor GetItemRarityColor(EItemRarity Rarity);

	UFUNCTION(BlueprintPure, Category = "Item|Display")
	static FText GetItemRarityDisplayName(EItemRarity Rarity);

	UFUNCTION(BlueprintPure, Category = "Item|Utility")
	static EDefenseType DamageTypeToResistance(EDamageType DamageType);

	UFUNCTION(BlueprintPure, Category = "Item|Utility")
	static FText GetItemTypeName(EItemType ItemType);

	UFUNCTION(BlueprintPure, Category = "Item|Utility")
	static FText GetItemSubTypeName(EItemSubType SubType);

	static EAttachmentRule ToEngineRule(EPHAttachmentRule Rule);

	UFUNCTION(BlueprintPure, Category = "Item|Affixes")
	static int32 GetRankPointsValue(ERankPoints Points);

	UFUNCTION(BlueprintPure, Category = "Item|Affixes")
	static int32 GetAffixRarityWeight(EAffixRarity Rarity);

	UFUNCTION(BlueprintPure, Category = "Item|Affixes")
	static FLinearColor GetAffixTierColor(EAffixColorTier Tier);

	UFUNCTION(BlueprintPure, Category = "Item|Affixes")
	static FString GetModifyTypeSymbol(EModifyType ModifyType);
};

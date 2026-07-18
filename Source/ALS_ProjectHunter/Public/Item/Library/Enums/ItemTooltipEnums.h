// Item/Library/Enums/ItemTooltipEnums.h
#pragma once

#include "CoreMinimal.h"
#include "ItemTooltipEnums.generated.h"

UENUM(BlueprintType)
enum class EItemTooltipLineStyle : uint8
{
	Normal      UMETA(DisplayName = "Normal"),
	Header      UMETA(DisplayName = "Header"),
	Property    UMETA(DisplayName = "Property"),
	Stat        UMETA(DisplayName = "Stat"),
	Affix       UMETA(DisplayName = "Affix"),
	Warning     UMETA(DisplayName = "Warning"),
	Corrupted   UMETA(DisplayName = "Corrupted"),
	Description UMETA(DisplayName = "Description")
};

UENUM(BlueprintType)
enum class EItemTooltipSectionType : uint8
{
	Details      UMETA(DisplayName = "Details"),
	BaseStats    UMETA(DisplayName = "Base Stats"),
	Requirements UMETA(DisplayName = "Requirements"),
	Implicits    UMETA(DisplayName = "Implicits"),
	Prefixes     UMETA(DisplayName = "Prefixes"),
	Suffixes      UMETA(DisplayName = "Suffixes"),
	Crafted      UMETA(DisplayName = "Crafted"),
	Enchants     UMETA(DisplayName = "Enchants"),
	Unique       UMETA(DisplayName = "Unique"),
	Corruption   UMETA(DisplayName = "Corruption"),
	Runes        UMETA(DisplayName = "Runes"),
	Durability   UMETA(DisplayName = "Durability"),
	Consumable   UMETA(DisplayName = "Consumable"),
	Description  UMETA(DisplayName = "Description")
};

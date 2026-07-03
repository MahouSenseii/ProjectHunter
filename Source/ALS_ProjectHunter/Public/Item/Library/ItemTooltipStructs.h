#pragma once

#include "CoreMinimal.h"
#include "Item/Library/ItemEnums.h"
#include "ItemTooltipStructs.generated.h"

class UMaterialInstance;

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

USTRUCT(BlueprintType)
struct FItemTooltipLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	FText Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	FLinearColor TextColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	EItemTooltipLineStyle Style = EItemTooltipLineStyle::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	bool bUseValueColumn = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	bool bEmphasized = false;
};

USTRUCT(BlueprintType)
struct FItemTooltipSection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	EItemTooltipSectionType SectionType = EItemTooltipSectionType::Details;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	FText Heading;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	TArray<FItemTooltipLine> Lines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	bool bShowHeading = true;

	bool HasDisplayableLines() const
	{
		return Lines.Num() > 0;
	}
};

USTRUCT(BlueprintType)
struct FItemTooltipData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	bool bHasItem = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	FText BaseItemName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	FText RarityName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	FText ItemTypeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	FText ItemSubTypeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	EItemRarity Rarity = EItemRarity::IR_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	FLinearColor RarityColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	FLinearColor BorderColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	FLinearColor HeaderColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	TObjectPtr<UMaterialInstance> IconMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	int32 ItemLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	int32 ItemValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	float TotalWeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	bool bIdentified = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	bool bStackable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	bool bCorrupted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Tooltip")
	TArray<FItemTooltipSection> Sections;

	bool HasSections() const
	{
		return Sections.Num() > 0;
	}
};

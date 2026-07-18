#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/ItemInstance.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "Item/Library/Structs/ItemTooltipStructs.h"
#include "ItemTooltipWidget.generated.h"

UCLASS()
class UItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void UpdateTooltip(UItemInstance* Item);

	UFUNCTION(BlueprintCallable, Category = "Tooltip")
	void ClearTooltip();

	UFUNCTION(BlueprintPure, Category = "Tooltip")
	FItemTooltipData GetTooltipData() const { return TooltipData; }

protected:
	/**
	 * Fired after the C++ population pass so a Blueprint child can extend the
	 * tooltip without replacing the base logic.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tooltip")
	void OnTooltipUpdated(UItemInstance* Item);

	// Blueprint children can create rows from Sections/Lines without blank stat widgets.
	UFUNCTION(BlueprintImplementableEvent, Category = "Tooltip")
	void OnTooltipDataUpdated(const FItemTooltipData& InTooltipData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Tooltip")
	void OnTooltipCleared();

	UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
	FItemTooltipData TooltipData;

	UPROPERTY(meta = (BindWidget))
	class UBorder* HeaderBorder;

	UPROPERTY(meta = (BindWidget))
	class UImage* ItemIconImage;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ItemNameText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ItemTypeText;

	UPROPERTY(meta = (BindWidget))
	class UBorder* BaseStatsBox;

	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* BaseStatsContainer;

	UPROPERTY(meta = (BindWidget))
	class UBorder* AffixesBox;

	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* AffixesContainer;

	UPROPERTY(meta = (BindWidget))
	class UBorder* LoreBox;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* LoreText;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeF = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeE = FLinearColor::White;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeD = FLinearColor(0.3f, 0.9f, 0.3f, 1.0f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeC = FLinearColor(0.4f, 0.6f, 1.0f, 1.0f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeB = FLinearColor(0.7f, 0.3f, 0.9f, 1.0f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeA = FLinearColor(1.0f, 0.7f, 0.0f, 1.0f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeS = FLinearColor(1.0f, 0.3f, 0.0f, 1.0f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeSS = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeUnkown = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeCorrupted = FLinearColor(0.5f, 0.0f, 0.3f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Colors")
	FLinearColor AffixColor = FLinearColor(0.5f, 0.8f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Colors")
	FLinearColor BaseStatColor = FLinearColor(0.9f, 0.9f, 0.9f);

	UPROPERTY(EditDefaultsOnly, Category = "Colors")
	FLinearColor LoreColor = FLinearColor(0.9f, 0.6f, 0.3f);

private:
	void SetGradeVisuals(EItemRarity Grade);
	FLinearColor GetGradeColor(EItemRarity Grade) const;
	void PopulateBaseStats(UItemInstance* Item);
	void PopulateAffixes(UItemInstance* Item);
	void PopulateLore(UItemInstance* Item);
	UTextBlock* CreateStatTextBlock(const FString& Text, FLinearColor Color);
};

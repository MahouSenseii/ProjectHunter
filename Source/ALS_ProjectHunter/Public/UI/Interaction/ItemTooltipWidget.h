#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/ItemInstance.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "UI/Library/PHUIStyle.h"
#include "Item/Library/Structs/ItemTooltipStructs.h"
#include "ItemTooltipWidget.generated.h"

class UBorder;
class UImage;
class UItemTooltipSectionWidget;
class UTextBlock;
class UVerticalBox;
class UWidgetAnimation;

UCLASS()
class UItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void UpdateTooltip(UItemInstance* Item);

	/** Make the tooltip visible and play its optional entrance animation. */
	UFUNCTION(BlueprintCallable, Category = "Tooltip|Animation")
	void ShowAnimated();

	/** Play the optional exit animation, then collapse the tooltip. */
	UFUNCTION(BlueprintCallable, Category = "Tooltip|Animation")
	void HideAnimated();

	UFUNCTION(BlueprintCallable, Category = "Tooltip")
	void ClearTooltip();

	UFUNCTION(BlueprintPure, Category = "Tooltip")
	FItemTooltipData GetTooltipData() const { return TooltipData; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

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
	TObjectPtr<UBorder> HeaderBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemIconImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemTypeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> SectionsContainer;

	UPROPERTY(EditDefaultsOnly, Category = "Tooltip|Widgets")
	TSubclassOf<UItemTooltipSectionWidget> SectionWidgetClass;

	/** Optional UMG animation named exactly OpenCloseAnimation in WBP_ItemTooltip. */
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> OpenCloseAnimation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeF = PHUIStyle::GradeF;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeE = PHUIStyle::GradeE;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeD = PHUIStyle::GradeD;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeC = PHUIStyle::GradeC;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeB = PHUIStyle::GradeB;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeA = PHUIStyle::GradeA;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeS = PHUIStyle::GradeS;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeSS = PHUIStyle::GradeSS;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeUnkown = PHUIStyle::GradeUnknown;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeCorrupted = PHUIStyle::GradeCorrupted;

private:
	TWeakObjectPtr<UItemInstance> DisplayedItem;
	bool bCloseRequested = false;

	UFUNCTION()
	void HandleCloseAnimationFinished();

	void SetGradeVisuals(EItemRarity Grade);
	FLinearColor GetGradeColor(EItemRarity Grade) const;
	void PopulateSections();
	void AddFallbackSection(const FItemTooltipSection& Section);
	UTextBlock* CreateFallbackTextBlock(
		const FText& Text,
		FLinearColor Color,
		ETextJustify::Type Justification);
};

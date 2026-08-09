#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/Library/Structs/ItemTooltipStructs.h"
#include "ItemTooltipSectionWidget.generated.h"

class UItemTooltipLineWidget;
class UTextBlock;
class UVerticalBox;

UCLASS(Abstract, Blueprintable)
class ALS_PROJECTHUNTER_API UItemTooltipSectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Item|Tooltip")
	void SetSectionData(const FItemTooltipSection& InSectionData, FLinearColor InHeadingColor);

	UFUNCTION(BlueprintPure, Category = "Item|Tooltip")
	FItemTooltipSection GetSectionData() const { return SectionData; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Item|Tooltip")
	FItemTooltipSection SectionData;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Tooltip")
	FLinearColor HeadingColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "Item|Tooltip")
	TSubclassOf<UItemTooltipLineWidget> LineWidgetClass;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeadingText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> LinesContainer;

	UFUNCTION(BlueprintImplementableEvent, Category = "Item|Tooltip")
	void OnSectionDataUpdated(const FItemTooltipSection& InSectionData);

private:
	void PopulateLines();
	void AddFallbackLine(const FItemTooltipLine& Line);
};

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/Library/Structs/ItemTooltipStructs.h"
#include "ItemTooltipLineWidget.generated.h"

class UTextBlock;

UCLASS(Abstract, Blueprintable)
class ALS_PROJECTHUNTER_API UItemTooltipLineWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Item|Tooltip")
	void SetLineData(const FItemTooltipLine& InLineData);

	UFUNCTION(BlueprintPure, Category = "Item|Tooltip")
	FItemTooltipLine GetLineData() const { return LineData; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Item|Tooltip")
	FItemTooltipLine LineData;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CenteredText;

	UFUNCTION(BlueprintImplementableEvent, Category = "Item|Tooltip")
	void OnLineDataUpdated(const FItemTooltipLine& InLineData);

private:
	void RefreshLine();
};

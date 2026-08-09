#include "UI/Interaction/ItemTooltipLineWidget.h"

#include "Components/TextBlock.h"

void UItemTooltipLineWidget::SetLineData(const FItemTooltipLine& InLineData)
{
	LineData = InLineData;
	RefreshLine();
	OnLineDataUpdated(LineData);
}

void UItemTooltipLineWidget::RefreshLine()
{
	const FSlateColor TextColor(LineData.TextColor);
	const bool bUseValueColumn = LineData.bUseValueColumn && !LineData.Value.IsEmpty();

	if (LabelText)
	{
		LabelText->SetText(LineData.Label);
		LabelText->SetColorAndOpacity(TextColor);
		LabelText->SetVisibility(bUseValueColumn || !CenteredText
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (ValueText)
	{
		ValueText->SetText(LineData.Value);
		ValueText->SetColorAndOpacity(TextColor);
		ValueText->SetVisibility(bUseValueColumn
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (CenteredText)
	{
		CenteredText->SetText(LineData.Label);
		CenteredText->SetColorAndOpacity(TextColor);
		CenteredText->SetVisibility(bUseValueColumn
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}
}

// RequirementsBox.h
#pragma once

#include "CoreMinimal.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Library/PHItemStructLibrary.h"
#include "UI/Widgets/PHUserWidget.h"
#include "RequirementsBox.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API URequirementsBox : public UPHUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeConstruct() override;
	UPROPERTY(EditAnywhere, Category = "Item")
	FEquippableItemData ItemData;

	// Main container box that will hold all dynamically created requirement boxes
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* RequirementsBox;

	// Style settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Styling")
	FLinearColor RequirementMetColor = FLinearColor::Green;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Styling")
	FLinearColor RequirementNotMetColor = FLinearColor::Red;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Styling")
	FSlateFontInfo RequirementFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Styling")
	FMargin RequirementPadding = FMargin(10.0f, 0.0f);

public:
	void SetItemRequirements(const FEquippableItemData& PassedItemData, APHBaseCharacter* Character);

private:
	void AddRequirementElement(const FString& StatName, float RequiredValue, float CurrentValue);
};
// EquippableToolTip.h

#pragma once

#include "CoreMinimal.h"
#include "Library/PHItemEnumLibrary.h"
#include "Components/SizeBox.h"
#include "UI/ToolTip/PHBaseToolTip.h"
#include "UI/ToolTip/EquippableStatsBox.h"
#include "EquippableToolTip.generated.h"

class UBaseItem;

USTRUCT(BlueprintType)
struct FStatCheckInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemStats StatType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USizeBox* SizeBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextBlock* TextBlock;
};

USTRUCT(BlueprintType)
struct FStatRequirementInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemRequiredStatsCategory StatType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USizeBox* SizeBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextBlock* TextBlock;
};

UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UEquippableToolTip : public UPHBaseToolTip
{
	GENERATED_BODY()
    
public:
	virtual void NativeConstruct() override;
	virtual void InitializeToolTip() override;
    
	
	virtual void SetItemInfo( UItemDefinitionAsset* Definition,  FItemInstanceData& InstanceData) override;

protected:
	/** Create affix display boxes from instance data */
	void CreateAffixBox();
    
	/** Update stats display */
	void UpdateStatsDisplay();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* LoreText;
    
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UVerticalBox* AffixBoxContainer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UEquippableStatsBox* StatsBox;
};
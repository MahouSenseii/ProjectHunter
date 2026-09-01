// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "UI/Menu/Widgets/PHMenuPageWidgetBase.h"
#include "PHPassiveTreeMenuPageWidget.generated.h"

class SPHPassiveTreeGraph;
class STextBlock;
class UCharacterProgressionManager;
class UPHPassiveTreeComponent;
class UPHPassiveTreeDataAsset;
struct FPHPassiveNodeDefinition;

/** Native System-window page for the pannable, zoomable, searchable Hunter Paths graph. */
UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UPHPassiveTreeMenuPageWidget : public UPHMenuPageWidgetBase
{
	GENERATED_BODY()

public:
	UPHPassiveTreeDataAsset* GetTreeData() const;
	bool IsNodeAllocated(FName NodeID) const;
	bool IsRandomStartNode(FName NodeID) const;
	bool CanAllocateNode(FName NodeID, FText& OutReason) const;
	void RequestAllocateNode(FName NodeID);
	void ShowNodeDetails(const FPHPassiveNodeDefinition* Node);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;

private:
	friend class SPHPassiveTreeGraph;

	UFUNCTION()
	void HandleTreeChanged();

	UFUNCTION()
	void HandleAllocationRejected(FName NodeID, FText Reason);

	UFUNCTION()
	void HandleProgressionChanged();

	void RefreshView();
	void SetSearchText(const FText& Text);
	FReply ZoomIn();
	FReply ZoomOut();
	FReply ResetView();

	TWeakObjectPtr<UPHPassiveTreeComponent> PassiveTree;
	TWeakObjectPtr<UCharacterProgressionManager> Progression;

	TSharedPtr<SPHPassiveTreeGraph> GraphWidget;
	TSharedPtr<STextBlock> PointsText;
	TSharedPtr<STextBlock> AllocatedCountText;
	TSharedPtr<STextBlock> SearchCountText;
	TSharedPtr<STextBlock> ZoomText;
	TSharedPtr<STextBlock> DetailTitleText;
	TSharedPtr<STextBlock> DetailBodyText;
	TSharedPtr<STextBlock> StatusText;
};

// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "UI/Menu/Library/Structs/StatsMenuStructs.h"
#include "UI/Menu/Widgets/PHMenuPageWidgetBase.h"
#include "PHStatsMenuPageWidget.generated.h"

class UButton;
class UCharacterProgressionManager;
class UPanelWidget;
class UTextBlock;
class UVerticalBox;

/**
 * The Stats page.
 *
 * Reads only. Values come from the character's ability system through the
 * gameplay-tag attribute registry, and spending a point is a request into
 * UCharacterProgressionManager - this page owns no progression state and
 * calculates no stat itself.
 *
 * Unlike the equipment page, whose layout is an authored Blueprint, this page
 * builds its rows from StatGroups. That is what keeps it data-driven: the 374
 * registered secondary attributes are reachable by adding rows, not code.
 * A Blueprint child may still supply StatsContainer to control where the rows
 * land, and may override StatGroups entirely.
 */
UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UPHStatsMenuPageWidget : public UPHMenuPageWidgetBase
{
	GENERATED_BODY()

public:
	UPHStatsMenuPageWidget();

	/** Rebuilds every value from the live ability system. */
	UFUNCTION(BlueprintCallable, Category = "Stats Menu")
	void RefreshStats();

	/** Requests one point into a primary attribute. True when it was spent. */
	UFUNCTION(BlueprintCallable, Category = "Stats Menu")
	bool RequestSpendStatPoint(FName AttributeName);

	UFUNCTION(BlueprintPure, Category = "Stats Menu")
	int32 GetUnspentStatPoints() const;

protected:
	/**
	 * Populates the widget tree before Slate is built from it.
	 *
	 * These pages construct their own rows, and a Blueprint shell has an empty
	 * tree. Assigning RootWidget from NativeConstruct is too late - Slate has
	 * already been built from the empty tree by then, and the page renders
	 * blank.
	 */
	virtual TSharedRef<SWidget> RebuildWidget() override;

	virtual void NativeConstruct() override;
	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;

	/** Curated by default; a Blueprint child may replace this wholesale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats Menu|Config")
	TArray<FPHStatGroupDef> StatGroups;

	/** Optional authored host. When absent the page builds its own column. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Stats Menu")
	TObjectPtr<UPanelWidget> StatsContainer;

	UFUNCTION(BlueprintImplementableEvent, Category = "Stats Menu|Events")
	void OnStatsRefreshed();

private:
	/** One built row, kept so refresh writes values without rebuilding the tree. */
	struct FStatRowWidgets
	{
		FGameplayTag Tag;
		EPHStatFormat Format = EPHStatFormat::SF_Integer;
		FName SpendableAttributeName = NAME_None;
		TWeakObjectPtr<UTextBlock> ValueText;
		TWeakObjectPtr<UButton> SpendButton;
	};

	/** A group's header, its rows, and the box holding them. */
	struct FStatGroupWidgets
	{
		TWeakObjectPtr<UVerticalBox> RowBox;
		TWeakObjectPtr<UTextBlock> Caret;
		bool bExpanded = true;
		TArray<FStatRowWidgets> Rows;
	};

	void BuildDefaultGroups();
	void BuildWidgets();
	void BindProgressionDelegates();
	void UnbindProgressionDelegates();
	UCharacterProgressionManager* GetProgression() const;
	bool TryReadAttribute(const FGameplayTag& Tag, float& OutValue) const;
	void RefreshHeader();
	void ApplyGroupExpansion(int32 GroupIndex);

	static FText FormatValue(float Value, EPHStatFormat Format);
	static FText LabelForTag(const FGameplayTag& Tag, const FText& Explicit);

	UFUNCTION()
	void HandleSectionToggled();

	UFUNCTION()
	void HandleSpendClicked();

	UFUNCTION()
	void HandleLevelUp(int32 NewLevel, int32 StatPointsAwarded, int32 SkillPointsAwarded);

	UFUNCTION()
	void HandleStatPointSpent(FName AttributeName, int32 RemainingPoints);

	UFUNCTION()
	void HandleXPGained(int64 FinalXP, int64 BaseXP, float TotalMultiplier);

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> BuiltColumn = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LevelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ExperienceText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PointsText = nullptr;

	/** Parallel to StatGroups. Weak pointers: the widget tree owns the widgets. */
	TArray<FStatGroupWidgets> GroupWidgets;

	/** Maps a clicked control back to its group or row without a lambda capture. */
	TMap<TWeakObjectPtr<UButton>, int32> HeaderButtonToGroup;
	TMap<TWeakObjectPtr<UButton>, FName> SpendButtonToAttribute;

	bool bWidgetsBuilt = false;
};

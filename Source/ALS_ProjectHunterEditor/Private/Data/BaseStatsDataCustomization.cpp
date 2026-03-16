#include "Data/BaseStatsDataCustomization.h"

#include "AttributeSet.h"
#include "Data/BaseStatsData.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace BaseStatsCustomizationPrivate
{
	FString ActiveSearchText;
	TMap<FName, bool> CategoryExpansionStates;

	struct FVisibleStatRow
	{
		int32 Index = INDEX_NONE;
		const FStatInitializationEntry* Entry = nullptr;
		TSharedPtr<IPropertyHandle> OverrideHandle;
		TSharedPtr<IPropertyHandle> ValueHandle;
	};

	struct FCategoryGroup
	{
		FName NormalizedCategory = NAME_None;
		FName MainCategory = NAME_None;
		FName SubCategory = NAME_None;

		bool HasSubCategory() const
		{
			return SubCategory != NAME_None;
		}

		FString BuildDetailsPath() const
		{
			return HasSubCategory()
				? FString::Printf(TEXT("All Attributes|%s|%s"), *MainCategory.ToString(), *SubCategory.ToString())
				: FString::Printf(TEXT("All Attributes|%s"), *MainCategory.ToString());
		}

		FText GetDisplayText() const
		{
			return HasSubCategory() ? FText::FromName(SubCategory) : FText::FromName(MainCategory);
		}
	};

	bool GetBoolValue(const TSharedPtr<IPropertyHandle>& Handle)
	{
		if (!Handle.IsValid())
		{
			return false;
		}

		bool Value = false;
		Handle->GetValue(Value);
		return Value;
	}

	bool PassesSearchFilter(const FStatInitializationEntry& Entry)
	{
		const FString SearchText = ActiveSearchText.TrimStartAndEnd();
		return SearchText.IsEmpty() || Entry.BuildSearchString().Contains(SearchText, ESearchCase::IgnoreCase);
	}

	FText GetDisplayNameText(const FStatInitializationEntry& Entry)
	{
		return Entry.DisplayName.IsEmpty()
			? FText::FromString(FName::NameToDisplayString(Entry.StatName.ToString(), false))
			: Entry.DisplayName;
	}

	FText GetStatTypeText(EHunterStatType StatType)
	{
		return StaticEnum<EHunterStatType>()->GetDisplayNameTextByValue(static_cast<int64>(StatType));
	}

	FString BuildCategoryDisplayString(const FParsedStatCategory& ParsedCategory)
	{
		return ParsedCategory.HasSubCategory()
			? FString::Printf(TEXT("%s / %s"), *ParsedCategory.MainCategory.ToString(), *ParsedCategory.SubCategory.ToString())
			: ParsedCategory.MainCategory.ToString();
	}

	TSharedRef<SWidget> BuildIconWidget(const FStatInitializationEntry& Entry)
	{
		const FLinearColor AccentColor = UBaseStatsData::GetStatTypeColor(Entry.StatType);
		const FText TooltipText = Entry.Tooltip.IsEmpty() ? GetDisplayNameText(Entry) : Entry.Tooltip;

		if (Entry.IconName != NAME_None)
		{
			if (const FSlateBrush* Brush = FAppStyle::GetOptionalBrush(Entry.IconName))
			{
				if (Brush->DrawAs != ESlateBrushDrawType::NoDrawType)
				{
					return SNew(SBox)
						.WidthOverride(16.0f)
						.HeightOverride(16.0f)
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
							.Image(Brush)
							.ColorAndOpacity(AccentColor)
							.ToolTipText(TooltipText)
						];
				}
			}
		}

		return SNew(SBox)
			.WidthOverride(10.0f)
			.HeightOverride(10.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.75f))
				.ToolTipText(TooltipText)
			];
	}

	TSharedRef<SWidget> BuildTypeBadgeWidget(const FStatInitializationEntry& Entry)
	{
		const FLinearColor TypeColor = UBaseStatsData::GetStatTypeColor(Entry.StatType);

		return SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(TypeColor.CopyWithNewOpacity(0.18f))
			.Padding(FMargin(6.0f, 2.0f))
			[
				SNew(STextBlock)
				.Text(GetStatTypeText(Entry.StatType))
				.ColorAndOpacity(TypeColor)
				.Font(IDetailLayoutBuilder::GetDetailFont())
			];
	}

	void SortCategories(TArray<FName>& Categories)
	{
		Categories.Sort([](FName Left, FName Right)
		{
			const FParsedStatCategory LeftCategory = UBaseStatsData::ParseCategoryPath(Left);
			const FParsedStatCategory RightCategory = UBaseStatsData::ParseCategoryPath(Right);
			const int32 LeftPriority = UBaseStatsData::GetCategorySortPriority(LeftCategory.MainCategory);
			const int32 RightPriority = UBaseStatsData::GetCategorySortPriority(RightCategory.MainCategory);
			if (LeftPriority != RightPriority)
			{
				return LeftPriority < RightPriority;
			}

			const int32 MainCompare = LeftCategory.MainCategory.ToString().Compare(RightCategory.MainCategory.ToString(), ESearchCase::IgnoreCase);
			if (MainCompare != 0)
			{
				return MainCompare < 0;
			}

			const FString LeftSubCategory = LeftCategory.SubCategory.ToString();
			const FString RightSubCategory = RightCategory.SubCategory.ToString();
			const bool bLeftHasSubCategory = !LeftSubCategory.IsEmpty();
			const bool bRightHasSubCategory = !RightSubCategory.IsEmpty();
			if (bLeftHasSubCategory != bRightHasSubCategory)
			{
				return !bLeftHasSubCategory;
			}

			return LeftSubCategory.Compare(RightSubCategory, ESearchCase::IgnoreCase) < 0;
		});
	}

	FText BuildSummaryText(const UBaseStatsData* StatsData, int32 TotalCount, int32 VisibleCount)
	{
		const FString AttributeSetName = StatsData ? GetNameSafe(StatsData->SourceAttributeSetClass.Get()) : TEXT("None");
		return FText::FromString(FString::Printf(TEXT("Source: %s  |  Visible: %d / %d"), *AttributeSetName, VisibleCount, TotalCount));
	}
}

TSharedRef<IDetailCustomization> FBaseStatsDataCustomization::MakeInstance()
{
	return MakeShared<FBaseStatsDataCustomization>();
}

void FBaseStatsDataCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
	DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);

	UBaseStatsData* StatsData = CustomizedObjects.Num() > 0 ? Cast<UBaseStatsData>(CustomizedObjects[0].Get()) : nullptr;

	const TSharedRef<IPropertyHandle> BaseAttributesHandle =
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UBaseStatsData, BaseAttributes));

	DetailBuilder.HideProperty(BaseAttributesHandle);

	const TSharedPtr<IPropertyHandleArray> ArrayHandle = BaseAttributesHandle->AsArray();
	IDetailCategoryBuilder& RootCategory =
		DetailBuilder.EditCategory(TEXT("All Attributes"), FText::FromString(TEXT("All Attributes")), ECategoryPriority::Important);

	const TSharedPtr<IPropertyUtilities> PropertyUtilities = DetailBuilder.GetPropertyUtilities();

	RootCategory.AddCustomRow(FText::FromString(TEXT("Stat Search")))
		.WholeRowContent()
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SSearchBox)
				.HintText(FText::FromString(TEXT("Search stats by name, category, tooltip, or type")))
				.HintText(FText::FromString(BaseStatsCustomizationPrivate::ActiveSearchText))
				.OnTextChanged_Lambda([PropertyUtilities](const FText& NewText)
				{
					BaseStatsCustomizationPrivate::ActiveSearchText = NewText.ToString();
					if (PropertyUtilities.IsValid())
					{
						PropertyUtilities->RequestRefresh();
					}
				})
			]
		];

	if (!ArrayHandle.IsValid() || !StatsData)
	{
		RootCategory.AddCustomRow(FText::FromString(TEXT("MissingStatsData")))
			.WholeRowContent()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Unable to build the reflected attribute editor for this asset.")))
				.ColorAndOpacity(FLinearColor(0.85f, 0.62f, 0.30f, 1.0f))
			];
		return;
	}

	uint32 NumElements = 0;
	ArrayHandle->GetNumElements(NumElements);

	const TArray<FStatInitializationEntry>& Entries = StatsData->GetBaseAttributes();
	const int32 RowCount = FMath::Min<int32>(static_cast<int32>(NumElements), Entries.Num());

	TMap<FName, TArray<BaseStatsCustomizationPrivate::FVisibleStatRow>> RowsByCategory;
	TMap<FName, BaseStatsCustomizationPrivate::FCategoryGroup> CategoryGroups;
	TArray<FName> OrderedCategories;
	int32 VisibleStatCount = 0;

	for (int32 Index = 0; Index < RowCount; ++Index)
	{
		const FStatInitializationEntry& Entry = Entries[Index];
		if (!Entry.IsValid() || !BaseStatsCustomizationPrivate::PassesSearchFilter(Entry))
		{
			continue;
		}

		const TSharedRef<IPropertyHandle> EntryHandle = ArrayHandle->GetElement(Index);
		const TSharedPtr<IPropertyHandle> OverrideHandle =
			EntryHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FStatInitializationEntry, bOverrideValue));
		const TSharedPtr<IPropertyHandle> ValueHandle =
			EntryHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FStatInitializationEntry, BaseValue));

		BaseStatsCustomizationPrivate::FVisibleStatRow Row;
		Row.Index = Index;
		Row.Entry = &Entry;
		Row.OverrideHandle = OverrideHandle;
		Row.ValueHandle = ValueHandle;

		const FParsedStatCategory ParsedCategory = UBaseStatsData::ParseCategoryPath(Entry.Category);
		const FName Category = ParsedCategory.NormalizedCategory;
		if (!RowsByCategory.Contains(Category))
		{
			OrderedCategories.Add(Category);
			// Reuse the same normalized path as runtime so whitespace variants collapse into one branch.
			BaseStatsCustomizationPrivate::FCategoryGroup& CategoryGroup = CategoryGroups.FindOrAdd(Category);
			CategoryGroup.NormalizedCategory = Category;
			CategoryGroup.MainCategory = ParsedCategory.MainCategory;
			CategoryGroup.SubCategory = ParsedCategory.SubCategory;
		}

		RowsByCategory.FindOrAdd(Category).Add(MoveTemp(Row));
		++VisibleStatCount;
	}

	RootCategory.AddCustomRow(FText::FromString(TEXT("StatSummaryRefresh")))
		.WholeRowContent()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(FMargin(8.0f, 6.0f))
			[
				SNew(STextBlock)
				.Text(BaseStatsCustomizationPrivate::BuildSummaryText(StatsData, Entries.Num(), VisibleStatCount))
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		];

	if (VisibleStatCount == 0)
	{
		const FString SearchText = BaseStatsCustomizationPrivate::ActiveSearchText.TrimStartAndEnd();
		const FString EmptyState = SearchText.IsEmpty()
			? TEXT("No reflected stats are available for this AttributeSet.")
			: FString::Printf(TEXT("No stats matched \"%s\"."), *SearchText);

		RootCategory.AddCustomRow(FText::FromString(TEXT("EmptySearchResult")))
			.WholeRowContent()
			[
				SNew(STextBlock)
				.Text(FText::FromString(EmptyState))
				.ColorAndOpacity(FLinearColor(0.72f, 0.72f, 0.72f, 1.0f))
			];
		return;
	}

	BaseStatsCustomizationPrivate::SortCategories(OrderedCategories);

	for (int32 CategoryIndex = 0; CategoryIndex < OrderedCategories.Num(); ++CategoryIndex)
	{
		const FName Category = OrderedCategories[CategoryIndex];
		const BaseStatsCustomizationPrivate::FCategoryGroup& CategoryGroup = CategoryGroups.FindChecked(Category);
		const FString CategoryPath = CategoryGroup.BuildDetailsPath();
		IDetailCategoryBuilder& CategoryBuilder =
			DetailBuilder.EditCategory(*CategoryPath, CategoryGroup.GetDisplayText(), ECategoryPriority::Default);

		CategoryBuilder.RestoreExpansionState(false);
		CategoryBuilder.SetSortOrder(CategoryIndex);

		const bool bIsSearching = !BaseStatsCustomizationPrivate::ActiveSearchText.TrimStartAndEnd().IsEmpty();
		const bool* SavedExpansionState = BaseStatsCustomizationPrivate::CategoryExpansionStates.Find(Category);
		const bool bExpanded = bIsSearching
			? true
			: (SavedExpansionState ? *SavedExpansionState : true);

		CategoryBuilder.InitiallyCollapsed(!bExpanded);
		CategoryBuilder.OnExpansionChanged(FOnBooleanValueChanged::CreateLambda([Category](bool bNowExpanded)
		{
			BaseStatsCustomizationPrivate::CategoryExpansionStates.Add(Category, bNowExpanded);
		}));

		const TArray<BaseStatsCustomizationPrivate::FVisibleStatRow>& CategoryRows = RowsByCategory.FindChecked(Category);

		for (const BaseStatsCustomizationPrivate::FVisibleStatRow& Row : CategoryRows)
		{
			const FStatInitializationEntry& Entry = *Row.Entry;
			const FLinearColor TypeColor = UBaseStatsData::GetStatTypeColor(Entry.StatType);
			const FText DisplayName = BaseStatsCustomizationPrivate::GetDisplayNameText(Entry);
			const FText Tooltip = Entry.Tooltip.IsEmpty() ? DisplayName : Entry.Tooltip;
			const FParsedStatCategory ParsedCategory = UBaseStatsData::ParseCategoryPath(Entry.Category);
			const FString CategoryDisplayString = BaseStatsCustomizationPrivate::BuildCategoryDisplayString(ParsedCategory);

			CategoryBuilder.AddCustomRow(DisplayName)
				.NameContent()
				.MinDesiredWidth(300.0f)
				.MaxDesiredWidth(520.0f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						BaseStatsCustomizationPrivate::BuildIconWidget(Entry)
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(DisplayName)
							.ToolTipText(Tooltip)
							.Font(IDetailLayoutBuilder::GetDetailFontBold())
							.ColorAndOpacity(TypeColor)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("%s  |  %s"), *Entry.StatName.ToString(), *CategoryDisplayString)))
							.ToolTipText(Tooltip)
							.Font(IDetailLayoutBuilder::GetDetailFont())
							.ColorAndOpacity(FLinearColor(0.62f, 0.62f, 0.66f, 1.0f))
						]
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(8.0f, 0.0f, 0.0f, 0.0f)
					[
						BaseStatsCustomizationPrivate::BuildTypeBadgeWidget(Entry)
					]
				]
				.ValueContent()
				.MinDesiredWidth(320.0f)
				.MaxDesiredWidth(520.0f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
					.VAlign(VAlign_Center)
					[
						Row.OverrideHandle.IsValid()
							? Row.OverrideHandle->CreatePropertyValueWidget()
							: SNullWidget::NullWidget
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.IsEnabled_Lambda([OverrideHandle = Row.OverrideHandle]()
						{
							return BaseStatsCustomizationPrivate::GetBoolValue(OverrideHandle);
						})
						[
							Row.ValueHandle.IsValid()
								? Row.ValueHandle->CreatePropertyValueWidget()
								: SNullWidget::NullWidget
						]
					]
				];
		}
	}
}

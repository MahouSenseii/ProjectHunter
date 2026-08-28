#include "Data/BaseStatsDataCustomization.h"

#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "Stats/Data/BaseStatsData.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IPropertyUtilities.h"
#include "UObject/UObjectGlobals.h"
#include "PropertyHandle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
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

	/**
	 * Three levels of noise. bOverrideValue on its own is not a useful filter:
	 * ResetToBaseline stamps roughly thirty required multipliers plus the
	 * starter block, so "authored" is mostly rows nobody chose. Edited compares
	 * against the baseline value and shows only what actually differs.
	 */
	enum class EStatListMode : uint8
	{
		Edited,
		Authored,
		All
	};

	EStatListMode ListMode = EStatListMode::Edited;

	/**
	 * Rows revealed through the picker. Without this, adding a stat that
	 * happens to sit at its baseline value would author it and then
	 * immediately hide it again.
	 */
	TSet<FName> PinnedStats;

	FString GetListModeLabel(EStatListMode Mode)
	{
		switch (Mode)
		{
		case EStatListMode::Edited:   return TEXT("Edited only");
		case EStatListMode::Authored: return TEXT("All authored");
		default:                      return TEXT("Every reflected stat");
		}
	}

	/** Authored, and either off-baseline or with no baseline to compare to. */
	bool IsEditedFromBaseline(const FStatInitializationEntry& Entry)
	{
		if (!Entry.bOverrideValue)
		{
			return false;
		}

		float BaselineValue = 0.0f;
		if (!UBaseStatsData::GetBaselineValueForStat(Entry.StatName, BaselineValue))
		{
			return true;
		}

		return !FMath::IsNearlyEqual(Entry.BaseValue, BaselineValue);
	}

	/** Search inside the add-stat picker, kept apart from the main list search. */
	FString PickerSearchText;

	/** Attributes some InitializationEffect modifies. Rebuilt on every refresh. */
	TSet<FName> EffectDrivenAttributes;

	/**
	 * The subset modified with EGameplayModOp::Override. Those cannot be beaten
	 * by an authored value - the effect applies after initialization and simply
	 * replaces it - which is the usual reason an authored number "does nothing".
	 */
	TSet<FName> EffectOverriddenAttributes;

	struct FPickerEntry
	{
		int32 Index = INDEX_NONE;
		FName StatName;
		FText DisplayName;
		FText Category;
	};

	/**
	 * Same modifier walk FStatsInitializer uses to decide whether to skip an
	 * effect, so what this panel flags is exactly what the runtime will drop.
	 */
	void GatherEffectDrivenAttributes(const UBaseStatsData* StatsData)
	{
		EffectDrivenAttributes.Reset();
		EffectOverriddenAttributes.Reset();
		if (!StatsData)
		{
			return;
		}

		for (const TSubclassOf<UGameplayEffect>& EffectClass : StatsData->InitializationEffects)
		{
			if (!EffectClass)
			{
				continue;
			}

			const UGameplayEffect* EffectCDO = EffectClass->GetDefaultObject<UGameplayEffect>();
			if (!EffectCDO)
			{
				continue;
			}

			for (const FGameplayModifierInfo& Modifier : EffectCDO->Modifiers)
			{
				if (!Modifier.Attribute.IsValid())
				{
					continue;
				}

				if (const FProperty* AttributeProperty = Modifier.Attribute.GetUProperty())
				{
					const FName AttributeName = AttributeProperty->GetFName();
					EffectDrivenAttributes.Add(AttributeName);
					if (Modifier.ModifierOp == EGameplayModOp::Override)
					{
						EffectOverriddenAttributes.Add(AttributeName);
					}
				}
			}
		}
	}

	/** An authored row that an effect also drives blocks that whole effect. */
	bool IsBlockingInitializationEffect(const FStatInitializationEntry& Entry)
	{
		// Matches FindAuthoredStatConflicts: an Additive modifier stacks on the
		// authored base instead of replacing it, so it blocks nothing.
		return Entry.bOverrideValue && EffectOverriddenAttributes.Contains(Entry.StatName);
	}

	FText BuildInitializationStatusText(const UBaseStatsData* StatsData, bool& bOutIsWarning)
	{
		bOutIsWarning = false;
		if (!StatsData)
		{
			return FText::GetEmpty();
		}

		int32 EffectCount = 0;
		for (const TSubclassOf<UGameplayEffect>& EffectClass : StatsData->InitializationEffects)
		{
			EffectCount += EffectClass ? 1 : 0;
		}

		if (EffectCount == 0)
		{
			return FText::FromString(
				TEXT("No initialization effects. Every authored value below is applied as-is."));
		}

		TArray<FName> Authored;
		TArray<FName> AuthoredAndOverridden;
		for (const FStatInitializationEntry& Entry : StatsData->GetBaseAttributes())
		{
			if (!Entry.bOverrideValue || !EffectDrivenAttributes.Contains(Entry.StatName))
			{
				continue;
			}

			Authored.Add(Entry.StatName);
			if (EffectOverriddenAttributes.Contains(Entry.StatName))
			{
				AuthoredAndOverridden.Add(Entry.StatName);
			}
		}
		Authored.Sort(FNameLexicalLess());

		const auto Join = [](const TArray<FName>& Names)
		{
			return FString::JoinBy(Names, TEXT(", "), [](const FName& N) { return N.ToString(); });
		};

		if (StatsData->bSkipInitializationEffectsThatModifyAuthoredStats)
		{
			if (Authored.IsEmpty())
			{
				return FText::FromString(FString::Printf(
					TEXT("Skip guard ON. %d initialization effect(s) will apply; no authored row blocks them."),
					EffectCount));
			}

			bOutIsWarning = true;
			return FText::FromString(FString::Printf(
				TEXT("Skip guard ON. Any effect touching these authored rows is skipped ENTIRELY, so your ")
				TEXT("values win but nothing that effect derives is calculated: %s"),
				*Join(Authored)));
		}

		if (AuthoredAndOverridden.IsEmpty())
		{
			return FText::FromString(FString::Printf(
				TEXT("Skip guard OFF. %d initialization effect(s) will apply on top of the authored values."),
				EffectCount));
		}

		AuthoredAndOverridden.Sort(FNameLexicalLess());
		bOutIsWarning = true;
		return FText::FromString(FString::Printf(
			TEXT("Skip guard OFF. Initialization effects run AFTER these values and Override them, so ")
			TEXT("editing them here changes nothing at runtime: %s.  ")
			TEXT("To author them instead, re-tick \"Skip Initialization Effects That Modify Authored Stats\", ")
			TEXT("or clear Initialization Effects."),
			*Join(AuthoredAndOverridden)));
	}


	FText GetDisplayNameText(const FStatInitializationEntry& Entry)
	{
		return Entry.DisplayName.IsEmpty()
			? FText::FromString(FName::NameToDisplayString(Entry.StatName.ToString(), false))
			: Entry.DisplayName;
	}

	void SetOverrideThroughHandle(const TSharedPtr<IPropertyHandle>& OverrideHandle, bool bNewValue)
	{
		if (OverrideHandle.IsValid())
		{
			OverrideHandle->SetValue(bNewValue);
		}
	}

	bool ShouldListEntry(const FStatInitializationEntry& Entry)
	{
		if (ListMode == EStatListMode::All || PinnedStats.Contains(Entry.StatName))
		{
			return true;
		}

		return ListMode == EStatListMode::Authored
			? Entry.bOverrideValue
			: IsEditedFromBaseline(Entry);
	}

	bool PickerEntryMatchesSearch(const FPickerEntry& Entry)
	{
		const FString Search = PickerSearchText.TrimStartAndEnd();
		if (Search.IsEmpty())
		{
			return true;
		}

		return Entry.StatName.ToString().Contains(Search)
			|| Entry.DisplayName.ToString().Contains(Search)
			|| Entry.Category.ToString().Contains(Search);
	}

	/**
	 * Searchable list of every stat that is not authored yet. Choosing one ticks
	 * its override, which is what moves it into the main list.
	 */
	TSharedRef<SWidget> BuildAddStatMenu(
		const UBaseStatsData* StatsData,
		TSharedPtr<IPropertyHandleArray> ArrayHandle,
		TSharedPtr<IPropertyUtilities> PropertyUtilities)
	{
		using FEntryPtr = TSharedPtr<FPickerEntry>;

		TSharedRef<TArray<FEntryPtr>> AllEntries = MakeShared<TArray<FEntryPtr>>();
		TSharedRef<TArray<FEntryPtr>> VisibleEntries = MakeShared<TArray<FEntryPtr>>();

		if (StatsData)
		{
			const TArray<FStatInitializationEntry>& Entries = StatsData->GetBaseAttributes();
			for (int32 Index = 0; Index < Entries.Num(); ++Index)
			{
				const FStatInitializationEntry& Entry = Entries[Index];
				// Anything already on screen would be a no-op to "add".
				if (!Entry.IsValid() || ShouldListEntry(Entry))
				{
					continue;
				}

				FEntryPtr Item = MakeShared<FPickerEntry>();
				Item->Index = Index;
				Item->StatName = Entry.StatName;
				Item->DisplayName = GetDisplayNameText(Entry);
				Item->Category = FText::FromName(Entry.Category);
				AllEntries->Add(Item);
			}
		}

		AllEntries->Sort([](const FEntryPtr& A, const FEntryPtr& B)
		{
			return A->StatName.LexicalLess(B->StatName);
		});

		const auto RefreshVisible = [AllEntries, VisibleEntries]()
		{
			VisibleEntries->Reset();
			for (const FEntryPtr& Item : *AllEntries)
			{
				if (PickerEntryMatchesSearch(*Item))
				{
					VisibleEntries->Add(Item);
				}
			}
		};
		RefreshVisible();

		TSharedRef<SListView<FEntryPtr>> ListView = SNew(SListView<FEntryPtr>)
			.ListItemsSource(&VisibleEntries.Get())
			.SelectionMode(ESelectionMode::Single)
			.OnGenerateRow_Lambda([](FEntryPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
			{
				return SNew(STableRow<FEntryPtr>, OwnerTable)
					.Padding(FMargin(6.0f, 3.0f))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(Item->DisplayName)
							.Font(IDetailLayoutBuilder::GetDetailFontBold())
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("%s  |  %s"),
								*Item->StatName.ToString(), *Item->Category.ToString())))
							.Font(IDetailLayoutBuilder::GetDetailFont())
							.ColorAndOpacity(FLinearColor(0.62f, 0.62f, 0.66f, 1.0f))
						]
					];
			})
			.OnSelectionChanged_Lambda(
				[ArrayHandle, PropertyUtilities](FEntryPtr Item, ESelectInfo::Type SelectInfo)
			{
				// Navigating with the keyboard also fires this; only a click commits.
				if (!Item.IsValid() || SelectInfo == ESelectInfo::Direct || !ArrayHandle.IsValid())
				{
					return;
				}

				const TSharedRef<IPropertyHandle> EntryHandle = ArrayHandle->GetElement(Item->Index);
				SetOverrideThroughHandle(
					EntryHandle->GetChildHandle(
						GET_MEMBER_NAME_CHECKED(FStatInitializationEntry, bOverrideValue)),
					true);

				// Pinned so it stays visible even while it still holds the
				// baseline value the user is about to change.
				PinnedStats.Add(Item->StatName);

				FSlateApplication::Get().DismissAllMenus();
				if (PropertyUtilities.IsValid())
				{
					PropertyUtilities->RequestForceRefresh();
				}
			});

		return SNew(SBox)
			.WidthOverride(360.0f)
			.HeightOverride(420.0f)
			.Padding(FMargin(4.0f))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(SSearchBox)
					.HintText(FText::FromString(TEXT("Filter unauthored stats")))
					.InitialText(FText::FromString(PickerSearchText))
					.OnTextChanged_Lambda([RefreshVisible, ListView](const FText& NewText)
					{
						PickerSearchText = NewText.ToString();
						RefreshVisible();
						ListView->RequestListRefresh();
					})
				]

				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					ListView
				]
			];
	}

	/** Flips bOverrideValue through the handle so undo and dirtying come free. */

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

FBaseStatsDataCustomization::~FBaseStatsDataCustomization()
{
	if (PropertyChangedHandle.IsValid())
	{
		FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(PropertyChangedHandle);
	}
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

	// Rebuild whenever anything edits this asset - notably the CallInEditor
	// buttons, which rewrite BaseAttributes without going through a handle.
	CustomizedObject = StatsData;
	WeakPropertyUtilities = PropertyUtilities;
	if (!PropertyChangedHandle.IsValid())
	{
		PropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddLambda(
			[this](UObject* Object, FPropertyChangedEvent&)
			{
				if (!Object || Object != CustomizedObject.Get())
				{
					return;
				}

				if (const TSharedPtr<IPropertyUtilities> Utilities = WeakPropertyUtilities.Pin())
				{
					Utilities->RequestForceRefresh();
				}
			});
	}

	RootCategory.AddCustomRow(FText::FromString(TEXT("Stat Search")))
		.WholeRowContent()
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SSearchBox)
				.HintText(FText::FromString(TEXT("Search stats by name, category, tooltip, or type")))
				// InitialText, not HintText: the panel is rebuilt on every change,
				// so without this the box loses what was typed into it.
				.InitialText(FText::FromString(BaseStatsCustomizationPrivate::ActiveSearchText))
				.DelayChangeNotificationsWhileTyping(true)
				.OnTextChanged_Lambda([PropertyUtilities](const FText& NewText)
				{
					BaseStatsCustomizationPrivate::ActiveSearchText = NewText.ToString();
					if (PropertyUtilities.IsValid())
					{
						PropertyUtilities->RequestForceRefresh();
					}
				})
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SComboButton)
				.ToolTipText(FText::FromString(
					TEXT("Edited only: rows whose value differs from what ResetToBaseline would set. "
					     "All authored: every row with Override Value ticked, defaults included. "
					     "Every reflected stat: the full AttributeSet.")))
				.ButtonContent()
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.Text_Lambda([]()
					{
						return FText::FromString(FString::Printf(TEXT("Showing: %s"),
							*BaseStatsCustomizationPrivate::GetListModeLabel(
								BaseStatsCustomizationPrivate::ListMode)));
					})
				]
				.OnGetMenuContent_Lambda([PropertyUtilities]()
				{
					TSharedRef<SVerticalBox> Menu = SNew(SVerticalBox);
					for (const BaseStatsCustomizationPrivate::EStatListMode Mode : {
							BaseStatsCustomizationPrivate::EStatListMode::Edited,
							BaseStatsCustomizationPrivate::EStatListMode::Authored,
							BaseStatsCustomizationPrivate::EStatListMode::All })
					{
						Menu->AddSlot()
						.AutoHeight()
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "SimpleButton")
							.HAlign(HAlign_Left)
							.OnClicked_Lambda([Mode, PropertyUtilities]()
							{
								BaseStatsCustomizationPrivate::ListMode = Mode;
								FSlateApplication::Get().DismissAllMenus();
								if (PropertyUtilities.IsValid())
								{
									PropertyUtilities->RequestForceRefresh();
								}
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Font(IDetailLayoutBuilder::GetDetailFont())
								.Text(FText::FromString(
									BaseStatsCustomizationPrivate::GetListModeLabel(Mode)))
							]
						];
					}
					return Menu;
				})
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.ToolTipText(FText::FromString(
						TEXT("Rebuild the list. Editing a value can change whether a row counts as "
						     "edited, and a value commit does not rebuild the panel on its own.")))
					.OnClicked_Lambda([PropertyUtilities]()
					{
						if (PropertyUtilities.IsValid())
						{
							PropertyUtilities->RequestForceRefresh();
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Font(IDetailLayoutBuilder::GetDetailFont())
						.Text(FText::FromString(TEXT("Refresh")))
					]
				]
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

	BaseStatsCustomizationPrivate::GatherEffectDrivenAttributes(StatsData);

	RootCategory.AddCustomRow(FText::FromString(TEXT("Add Stat")))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SComboButton)
				.ToolTipText(FText::FromString(
					TEXT("Pick an attribute that is not authored yet. Choosing one ticks its "
					     "Override Value, which is what moves it into the list above.")))
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Add stat to override...")))
					.Font(IDetailLayoutBuilder::GetDetailFont())
				]
				.OnGetMenuContent_Lambda(
					[WeakStatsData = TWeakObjectPtr<UBaseStatsData>(StatsData), ArrayHandle, PropertyUtilities]()
				{
					return BaseStatsCustomizationPrivate::BuildAddStatMenu(
						WeakStatsData.Get(), ArrayHandle, PropertyUtilities);
				})
			]
		];

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
		if (!Entry.IsValid()
			|| !BaseStatsCustomizationPrivate::PassesSearchFilter(Entry)
			|| !BaseStatsCustomizationPrivate::ShouldListEntry(Entry))
		{
			continue;
		}

		const TSharedRef<IPropertyHandle> EntryHandle = ArrayHandle->GetElement(Index);
		const TSharedPtr<IPropertyHandle> OverrideHandle =
			EntryHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FStatInitializationEntry, bOverrideValue));
		const TSharedPtr<IPropertyHandle> ValueHandle =
			EntryHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FStatInitializationEntry, BaseValue));

		if (OverrideHandle.IsValid() && PropertyUtilities.IsValid())
		{
			// Ticking or unticking a row changes whether it belongs in the
			// current list, so the panel has to be rebuilt to reflect it.
			OverrideHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda(
				[PropertyUtilities]() { PropertyUtilities->RequestForceRefresh(); }));
		}

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

	{
		bool bStatusIsWarning = false;
		const FText StatusText =
			BaseStatsCustomizationPrivate::BuildInitializationStatusText(StatsData, bStatusIsWarning);

		if (!StatusText.IsEmpty())
		{
			RootCategory.AddCustomRow(FText::FromString(TEXT("InitializationStatus")))
				.WholeRowContent()
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(FMargin(8.0f, 6.0f))
					[
						SNew(STextBlock)
						.Text(StatusText)
						.Font(IDetailLayoutBuilder::GetDetailFont())
						.AutoWrapText(true)
						.ColorAndOpacity(bStatusIsWarning
							? FLinearColor(0.95f, 0.62f, 0.22f, 1.0f)
							: FLinearColor(0.66f, 0.74f, 0.66f, 1.0f))
					]
				];
		}
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
		FString EmptyState;
		if (!SearchText.IsEmpty())
		{
			EmptyState = FString::Printf(TEXT("No stats matched \"%s\"."), *SearchText);
		}
		else if (BaseStatsCustomizationPrivate::ListMode
			== BaseStatsCustomizationPrivate::EStatListMode::Edited)
		{
			EmptyState = TEXT("Nothing differs from the baseline yet. Use \"Add stat to override...\" to "
			                  "pick one, or switch Showing to \"All authored\" to see the defaults too.");
		}
		else
		{
			EmptyState = TEXT("No reflected stats are available for this AttributeSet.");
		}

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

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(8.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("BLOCKS EFFECT")))
						.ToolTipText(FText::FromString(
							TEXT("An InitializationEffect also drives this attribute. While Override Value is "
							     "ticked, that whole effect is skipped - not just this one stat.")))
						.Font(IDetailLayoutBuilder::GetDetailFontBold())
						.ColorAndOpacity(FLinearColor(0.95f, 0.55f, 0.18f, 1.0f))
						.Visibility(BaseStatsCustomizationPrivate::IsBlockingInitializationEffect(Entry)
							? EVisibility::Visible
							: EVisibility::Collapsed)
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

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(6.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.ToolTipText(FText::FromString(
							TEXT("Stop authoring this stat. The value is kept, so re-adding it restores the number.")))
						.OnClicked_Lambda([OverrideHandle = Row.OverrideHandle,
							StatName = Entry.StatName, PropertyUtilities]()
						{
							BaseStatsCustomizationPrivate::PinnedStats.Remove(StatName);
							BaseStatsCustomizationPrivate::SetOverrideThroughHandle(OverrideHandle, false);
							if (PropertyUtilities.IsValid())
							{
								PropertyUtilities->RequestForceRefresh();
							}
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("X")))
							.Font(IDetailLayoutBuilder::GetDetailFontBold())
							.ColorAndOpacity(FLinearColor(0.80f, 0.55f, 0.55f, 1.0f))
						]
					]
				];
		}
	}
}

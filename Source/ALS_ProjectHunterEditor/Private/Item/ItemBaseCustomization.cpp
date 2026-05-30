#include "Item/ItemBaseCustomization.h"

#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "IPropertyUtilities.h"
#include "Item/Library/ItemStructs.h"
#include "PropertyHandle.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IPropertyTypeCustomization> FItemBaseCustomization::MakeInstance()
{
	return MakeShared<FItemBaseCustomization>();
}

void FItemBaseCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	(void)StructCustomizationUtils;

	HeaderRow
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			StructPropertyHandle->CreatePropertyValueWidget()
		];
}

void FItemBaseCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	ItemTypeHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FItemBase, ItemType));
	ItemSubTypeHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FItemBase, ItemSubType));

	if (ItemTypeHandle.IsValid())
	{
		TWeakPtr<IPropertyUtilities> WeakPropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();
		ItemTypeHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([WeakPropertyUtilities]()
		{
			if (const TSharedPtr<IPropertyUtilities> PropertyUtilities = WeakPropertyUtilities.Pin())
			{
				PropertyUtilities->RequestRefresh();
			}
		}));
	}

	RefreshSubTypeOptions();

	uint32 ChildCount = 0;
	StructPropertyHandle->GetNumChildren(ChildCount);

	for (uint32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
	{
		TSharedPtr<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex);
		if (!ChildHandle.IsValid())
		{
			continue;
		}

		const FProperty* ChildProperty = ChildHandle->GetProperty();
		if (ChildProperty && ChildProperty->GetFName() == GET_MEMBER_NAME_CHECKED(FItemBase, ItemSubType))
		{
			ChildBuilder.AddCustomRow(ChildHandle->GetPropertyDisplayName())
				.NameContent()
				[
					ChildHandle->CreatePropertyNameWidget()
				]
				.ValueContent()
				.MinDesiredWidth(220.0f)
				[
					SNew(SComboBox<TSharedPtr<EItemSubType>>)
					.OptionsSource(&SubTypeOptions)
					.OnGenerateWidget(this, &FItemBaseCustomization::MakeSubTypeOptionWidget)
					.OnSelectionChanged(this, &FItemBaseCustomization::HandleSubTypeSelectionChanged)
					[
						SNew(STextBlock)
						.Text(this, &FItemBaseCustomization::GetSelectedSubTypeText)
						.Font(IDetailLayoutBuilder::GetDetailFont())
					]
				];
			continue;
		}

		ChildBuilder.AddProperty(ChildHandle.ToSharedRef());
	}
}

void FItemBaseCustomization::RefreshSubTypeOptions()
{
	SubTypeOptions.Reset();
	SubTypeOptions.Add(MakeShared<EItemSubType>(EItemSubType::IST_None));

	const EItemType ItemType = GetItemTypeValue();
	const UEnum* SubTypeEnum = StaticEnum<EItemSubType>();
	if (!SubTypeEnum)
	{
		return;
	}

	for (int32 EnumIndex = 0; EnumIndex < SubTypeEnum->NumEnums(); ++EnumIndex)
	{
		if (SubTypeEnum->HasMetaData(TEXT("Hidden"), EnumIndex))
		{
			continue;
		}

		const int64 EnumValue = SubTypeEnum->GetValueByIndex(EnumIndex);
		if (EnumValue < 0 || EnumValue > TNumericLimits<uint8>::Max())
		{
			continue;
		}

		const EItemSubType SubType = static_cast<EItemSubType>(EnumValue);
		if (SubType == EItemSubType::IST_None)
		{
			continue;
		}

		if (IsItemSubTypeAllowedForItemType(ItemType, SubType))
		{
			SubTypeOptions.Add(MakeShared<EItemSubType>(SubType));
		}
	}
}

TSharedRef<SWidget> FItemBaseCustomization::MakeSubTypeOptionWidget(TSharedPtr<EItemSubType> Option) const
{
	return SNew(STextBlock)
		.Text(Option.IsValid() ? GetItemSubTypeText(*Option) : FText::GetEmpty())
		.Font(IDetailLayoutBuilder::GetDetailFont());
}

FText FItemBaseCustomization::GetSelectedSubTypeText() const
{
	const EItemType ItemType = GetItemTypeValue();
	const EItemSubType SubType = GetItemSubTypeValue();
	const FText SubTypeText = GetItemSubTypeText(SubType);

	if (SubType != EItemSubType::IST_None && !IsItemSubTypeAllowedForItemType(ItemType, SubType))
	{
		return FText::FromString(FString::Printf(
			TEXT("%s (invalid for %s)"),
			*SubTypeText.ToString(),
			*GetItemTypeText(ItemType).ToString()));
	}

	return SubTypeText;
}

void FItemBaseCustomization::HandleSubTypeSelectionChanged(
	TSharedPtr<EItemSubType> NewSelection,
	ESelectInfo::Type SelectInfo) const
{
	(void)SelectInfo;

	if (!ItemSubTypeHandle.IsValid() || !NewSelection.IsValid())
	{
		return;
	}

	ItemSubTypeHandle->SetValue(static_cast<uint8>(*NewSelection));
}

EItemType FItemBaseCustomization::GetItemTypeValue() const
{
	uint8 RawValue = static_cast<uint8>(EItemType::IT_None);
	if (ItemTypeHandle.IsValid())
	{
		ItemTypeHandle->GetValue(RawValue);
	}

	return static_cast<EItemType>(RawValue);
}

EItemSubType FItemBaseCustomization::GetItemSubTypeValue() const
{
	uint8 RawValue = static_cast<uint8>(EItemSubType::IST_None);
	if (ItemSubTypeHandle.IsValid())
	{
		ItemSubTypeHandle->GetValue(RawValue);
	}

	return static_cast<EItemSubType>(RawValue);
}

FText FItemBaseCustomization::GetItemTypeText(EItemType ItemType)
{
	if (const UEnum* ItemTypeEnum = StaticEnum<EItemType>())
	{
		return ItemTypeEnum->GetDisplayNameTextByValue(static_cast<int64>(ItemType));
	}

	return FText::FromString(TEXT("None"));
}

FText FItemBaseCustomization::GetItemSubTypeText(EItemSubType ItemSubType)
{
	if (const UEnum* SubTypeEnum = StaticEnum<EItemSubType>())
	{
		return SubTypeEnum->GetDisplayNameTextByValue(static_cast<int64>(ItemSubType));
	}

	return FText::FromString(TEXT("None"));
}

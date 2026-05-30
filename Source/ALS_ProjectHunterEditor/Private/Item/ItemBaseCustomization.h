#pragma once

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyTypeCustomization.h"
#include "Item/Library/ItemEnums.h"
#include "Types/SlateEnums.h"
#include "Widgets/SWidget.h"

class IPropertyHandle;

class FItemBaseCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	virtual void CustomizeChildren(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	void RefreshSubTypeOptions();
	TSharedRef<SWidget> MakeSubTypeOptionWidget(TSharedPtr<EItemSubType> Option) const;
	FText GetSelectedSubTypeText() const;
	void HandleSubTypeSelectionChanged(TSharedPtr<EItemSubType> NewSelection, ESelectInfo::Type SelectInfo) const;

	EItemType GetItemTypeValue() const;
	EItemSubType GetItemSubTypeValue() const;
	static FText GetItemTypeText(EItemType ItemType);
	static FText GetItemSubTypeText(EItemSubType ItemSubType);

	TSharedPtr<IPropertyHandle> ItemTypeHandle;
	TSharedPtr<IPropertyHandle> ItemSubTypeHandle;
	TArray<TSharedPtr<EItemSubType>> SubTypeOptions;
};

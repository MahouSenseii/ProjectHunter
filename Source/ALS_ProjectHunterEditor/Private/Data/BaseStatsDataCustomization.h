#pragma once

#include "IDetailCustomization.h"
#include "UObject/WeakObjectPtr.h"

class IPropertyUtilities;

class FBaseStatsDataCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual ~FBaseStatsDataCustomization() override;

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	/**
	 * The stat rows are custom rows built from a snapshot of BaseAttributes, so
	 * a CallInEditor button that rewrites the array leaves the panel showing the
	 * pre-click list. PostEditChange only invalidates values; rebuilding needs a
	 * force refresh driven from here.
	 */
	FDelegateHandle PropertyChangedHandle;

	TWeakObjectPtr<UObject> CustomizedObject;
	TWeakPtr<IPropertyUtilities> WeakPropertyUtilities;
};

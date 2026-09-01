// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"
#include "Progression/Data/PHPassiveTreeDataAsset.h"
#include "AssetDefinition_PHPassiveTree.generated.h"

/** Sends a double-click on a passive tree asset to the graph editor instead of the generic property view. */
UCLASS()
class UAssetDefinition_PHPassiveTree : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	virtual FText GetAssetDisplayName() const override;
	virtual TSoftClassPtr<UObject> GetAssetClass() const override;
	virtual FLinearColor GetAssetColor() const override;
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
};

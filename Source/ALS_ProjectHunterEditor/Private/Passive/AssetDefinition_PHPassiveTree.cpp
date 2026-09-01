// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "Passive/AssetDefinition_PHPassiveTree.h"

#include "Passive/PHPassiveTreeAssetEditor.h"

#define LOCTEXT_NAMESPACE "AssetDefinition_PHPassiveTree"

FText UAssetDefinition_PHPassiveTree::GetAssetDisplayName() const
{
	return LOCTEXT("DisplayName", "Passive Tree");
}

TSoftClassPtr<UObject> UAssetDefinition_PHPassiveTree::GetAssetClass() const
{
	return UPHPassiveTreeDataAsset::StaticClass();
}

FLinearColor UAssetDefinition_PHPassiveTree::GetAssetColor() const
{
	return FLinearColor(0.06f, 0.26f, 0.45f);
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_PHPassiveTree::GetAssetCategories() const
{
	static const FAssetCategoryPath Categories[] = {EAssetCategoryPaths::Gameplay};
	return Categories;
}

EAssetCommandResult UAssetDefinition_PHPassiveTree::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UPHPassiveTreeDataAsset* Tree : OpenArgs.LoadObjects<UPHPassiveTreeDataAsset>())
	{
		const TSharedRef<FPHPassiveTreeAssetEditor> Editor = MakeShared<FPHPassiveTreeAssetEditor>();
		Editor->InitPassiveTreeEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, Tree);
	}
	return EAssetCommandResult::Handled;
}

#undef LOCTEXT_NAMESPACE

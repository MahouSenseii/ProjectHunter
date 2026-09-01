// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "UObject/StrongObjectPtr.h"

class IDetailsView;
class SGraphEditor;
class UEdGraph;
class UEdGraphNode;
class UPHPassiveTreeDataAsset;

/**
 * Node-graph editor for a passive tree.
 *
 * The asset stays the source of truth; the graph is a view rebuilt from it on open and written back
 * on every change. That keeps the runtime, the existing Python authoring tool, and the plain details
 * panel all working unchanged - opening this editor is not a one-way migration.
 *
 * Positions are the whole point: dragging a node writes its Position, so a designer can never again
 * put a node somewhere different from where they meant to type.
 */
class FPHPassiveTreeAssetEditor : public FAssetEditorToolkit, public FEditorUndoClient
{
public:
	virtual ~FPHPassiveTreeAssetEditor() override;

	void InitPassiveTreeEditor(
		EToolkitMode::Type Mode,
		const TSharedPtr<IToolkitHost>& InitToolkitHost,
		UPHPassiveTreeDataAsset* InAsset);

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void SaveAsset_Execute() override;
	virtual void OnClose() override;

	// FEditorUndoClient
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

private:
	TSharedRef<SDockTab> SpawnGraphTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnNodeDetailsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTreeDetailsTab(const FSpawnTabArgs& Args);

	/** Asset -> graph. Runs on open and after an undo, which can rewrite Nodes underneath us. */
	void RebuildGraphFromAsset();

	/** Graph -> asset. Every edit path funnels through here so the two never drift. */
	void SyncAssetFromGraph();

	void BindGraphCommands();
	void OnGraphChanged(const FEdGraphEditAction& Action);
	void OnGraphSelectionChanged(const TSet<UObject*>& NewSelection);
	void OnNodeDetailsChanged(const FPropertyChangedEvent& Event);
	void ExtendToolbar();
	void ValidateTree();

	void DeleteSelectedNodes();
	bool CanDeleteNodes() const;
	void CopySelectedNodes();
	bool CanCopyNodes() const;
	void CutSelectedNodes();
	void PasteNodes();
	bool CanPasteNodes() const;
	void DuplicateNodes();
	void SelectAllNodes();

	TObjectPtr<UPHPassiveTreeDataAsset> Asset = nullptr;
	TStrongObjectPtr<UEdGraph> Graph;
	TSharedPtr<SGraphEditor> GraphEditor;
	TSharedPtr<IDetailsView> NodeDetailsView;
	TSharedPtr<IDetailsView> TreeDetailsView;
	FDelegateHandle GraphChangedHandle;

	/** Guards the asset->graph rebuild from being mistaken for a designer edit. */
	bool bRebuildingGraph = false;
};

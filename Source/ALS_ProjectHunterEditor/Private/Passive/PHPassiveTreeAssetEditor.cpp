// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "Passive/PHPassiveTreeAssetEditor.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphUtilities.h"
#include "Editor.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GraphEditor.h"
#include "Styling/AppStyle.h"
#include "Textures/SlateIcon.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "Passive/PHPassiveTreeGraphNode.h"
#include "Passive/PHPassiveTreeGraphSchema.h"
#include "Progression/Data/PHPassiveTreeDataAsset.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "PHPassiveTreeAssetEditor"

namespace PHPassiveTreeEditorTabs
{
	static const FName GraphTab(TEXT("PHPassiveTree_Graph"));
	static const FName NodeDetailsTab(TEXT("PHPassiveTree_NodeDetails"));
	static const FName TreeDetailsTab(TEXT("PHPassiveTree_TreeDetails"));
	static const FName AppIdentifier(TEXT("PHPassiveTreeEditorApp"));
}

FPHPassiveTreeAssetEditor::~FPHPassiveTreeAssetEditor()
{
	GEditor->UnregisterForUndo(this);
	if (Graph.IsValid() && GraphChangedHandle.IsValid())
	{
		Graph->RemoveOnGraphChangedHandler(GraphChangedHandle);
	}
}

void FPHPassiveTreeAssetEditor::InitPassiveTreeEditor(
	const EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost,
	UPHPassiveTreeDataAsset* InAsset)
{
	Asset = InAsset;
	check(Asset);

	Graph.Reset(NewObject<UEdGraph>(GetTransientPackage(), UEdGraph::StaticClass(), NAME_None, RF_Transactional));
	Graph->Schema = UPHPassiveTreeGraphSchema::StaticClass();
	RebuildGraphFromAsset();
	GraphChangedHandle = Graph->AddOnGraphChangedHandler(
		FOnGraphChanged::FDelegate::CreateSP(this, &FPHPassiveTreeAssetEditor::OnGraphChanged));

	FPropertyEditorModule& PropertyEditor =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

	FDetailsViewArgs NodeArgs;
	NodeArgs.bAllowSearch = true;
	NodeArgs.bHideSelectionTip = true;
	NodeArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	NodeDetailsView = PropertyEditor.CreateDetailView(NodeArgs);
	NodeDetailsView->OnFinishedChangingProperties().AddSP(
		this, &FPHPassiveTreeAssetEditor::OnNodeDetailsChanged);

	FDetailsViewArgs TreeArgs;
	TreeArgs.bAllowSearch = true;
	TreeArgs.bHideSelectionTip = true;
	TreeArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	TreeDetailsView = PropertyEditor.CreateDetailView(TreeArgs);
	TreeDetailsView->SetObject(Asset);

	BindGraphCommands();

	const TSharedRef<FTabManager::FLayout> Layout =
		FTabManager::NewLayout(TEXT("PHPassiveTreeEditor_Layout_v1"))
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.72f)
				->AddTab(PHPassiveTreeEditorTabs::GraphTab, ETabState::OpenedTab)
				->SetHideTabWell(true)
			)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.28f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->AddTab(PHPassiveTreeEditorTabs::NodeDetailsTab, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.4f)
					->AddTab(PHPassiveTreeEditorTabs::TreeDetailsTab, ETabState::OpenedTab)
				)
			)
		);

	InitAssetEditor(
		Mode, InitToolkitHost, PHPassiveTreeEditorTabs::AppIdentifier, Layout,
		/*bCreateDefaultStandaloneMenu*/ true, /*bCreateDefaultToolbar*/ true, Asset);

	ExtendToolbar();
	RegenerateMenusAndToolbars();
	GEditor->RegisterForUndo(this);
}

FName FPHPassiveTreeAssetEditor::GetToolkitFName() const
{
	return FName(TEXT("PHPassiveTreeEditor"));
}

FText FPHPassiveTreeAssetEditor::GetBaseToolkitName() const
{
	return LOCTEXT("ToolkitName", "Passive Tree Editor");
}

FString FPHPassiveTreeAssetEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("TabPrefix", "PassiveTree ").ToString();
}

FLinearColor FPHPassiveTreeAssetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.06f, 0.26f, 0.45f, 0.5f);
}

void FPHPassiveTreeAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	const TSharedRef<FWorkspaceItem> Category = InTabManager->AddLocalWorkspaceMenuCategory(
		LOCTEXT("WorkspaceMenu", "Passive Tree Editor"));
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(PHPassiveTreeEditorTabs::GraphTab,
			FOnSpawnTab::CreateSP(this, &FPHPassiveTreeAssetEditor::SpawnGraphTab))
		.SetDisplayName(LOCTEXT("GraphTab", "Graph"))
		.SetGroup(Category);

	InTabManager->RegisterTabSpawner(PHPassiveTreeEditorTabs::NodeDetailsTab,
			FOnSpawnTab::CreateSP(this, &FPHPassiveTreeAssetEditor::SpawnNodeDetailsTab))
		.SetDisplayName(LOCTEXT("NodeDetailsTab", "Selected Node"))
		.SetGroup(Category);

	InTabManager->RegisterTabSpawner(PHPassiveTreeEditorTabs::TreeDetailsTab,
			FOnSpawnTab::CreateSP(this, &FPHPassiveTreeAssetEditor::SpawnTreeDetailsTab))
		.SetDisplayName(LOCTEXT("TreeDetailsTab", "Tree Settings"))
		.SetGroup(Category);
}

void FPHPassiveTreeAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(PHPassiveTreeEditorTabs::GraphTab);
	InTabManager->UnregisterTabSpawner(PHPassiveTreeEditorTabs::NodeDetailsTab);
	InTabManager->UnregisterTabSpawner(PHPassiveTreeEditorTabs::TreeDetailsTab);
}

TSharedRef<SDockTab> FPHPassiveTreeAssetEditor::SpawnGraphTab(const FSpawnTabArgs& Args)
{
	SGraphEditor::FGraphEditorEvents Events;
	Events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(
		this, &FPHPassiveTreeAssetEditor::OnGraphSelectionChanged);

	FGraphAppearanceInfo Appearance;
	Appearance.CornerText = LOCTEXT("GraphCorner", "HUNTER PATHS");

	GraphEditor = SNew(SGraphEditor)
		.AdditionalCommands(ToolkitCommands)
		.IsEditable(true)
		.Appearance(Appearance)
		.GraphToEdit(Graph.Get())
		.GraphEvents(Events);

	return SNew(SDockTab)
		.Label(LOCTEXT("GraphTabLabel", "Graph"))
		[
			GraphEditor.ToSharedRef()
		];
}

TSharedRef<SDockTab> FPHPassiveTreeAssetEditor::SpawnNodeDetailsTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("NodeDetailsTabLabel", "Selected Node"))
		[
			NodeDetailsView.ToSharedRef()
		];
}

TSharedRef<SDockTab> FPHPassiveTreeAssetEditor::SpawnTreeDetailsTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("TreeDetailsTabLabel", "Tree Settings"))
		[
			TreeDetailsView.ToSharedRef()
		];
}

void FPHPassiveTreeAssetEditor::RebuildGraphFromAsset()
{
	if (!Asset || !Graph.IsValid())
	{
		return;
	}

	TGuardValue<bool> Guard(bRebuildingGraph, true);
	Graph->Nodes.Reset();

	TMap<FName, UPHPassiveTreeGraphNode*> ByID;
	ByID.Reserve(Asset->Nodes.Num());
	for (const FPHPassiveNodeDefinition& Definition : Asset->Nodes)
	{
		UPHPassiveTreeGraphNode* Node = NewObject<UPHPassiveTreeGraphNode>(Graph.Get());
		Node->SetFlags(RF_Transactional);
		Node->CreateNewGuid();
		Node->Definition = Definition;
		Node->ApplyGraphPosition();
		Node->AllocateDefaultPins();
		Graph->Nodes.Add(Node);
		// A duplicate ID is an authoring error the validator reports; first-wins keeps wiring total.
		ByID.FindOrAdd(Definition.NodeID, Node);
	}

	for (UEdGraphNode* RawNode : Graph->Nodes)
	{
		UPHPassiveTreeGraphNode* Node = CastChecked<UPHPassiveTreeGraphNode>(RawNode);
		UEdGraphPin* ParentPin = Node->GetParentPin();
		if (!ParentPin)
		{
			continue;
		}
		for (const FName ParentID : Node->Definition.RequiredNodeIDs)
		{
			UPHPassiveTreeGraphNode* const* Parent = ByID.Find(ParentID);
			if (Parent && *Parent != Node)
			{
				if (UEdGraphPin* ChildPin = (*Parent)->GetChildPin())
				{
					ParentPin->MakeLinkTo(ChildPin);
				}
			}
		}
	}
}

void FPHPassiveTreeAssetEditor::SyncAssetFromGraph()
{
	if (!Asset || !Graph.IsValid() || bRebuildingGraph)
	{
		return;
	}

	Asset->Modify();
	Asset->Nodes.Reset();
	Asset->Nodes.Reserve(Graph->Nodes.Num());
	for (UEdGraphNode* RawNode : Graph->Nodes)
	{
		if (UPHPassiveTreeGraphNode* Node = Cast<UPHPassiveTreeGraphNode>(RawNode))
		{
			// The canvas owns position and connections; whatever is in the details panel loses.
			Node->SyncDefinitionFromGraph();
			Asset->Nodes.Add(Node->Definition);
		}
	}

	Asset->InvalidateNodeIndex();
	Asset->MarkPackageDirty();
	if (TreeDetailsView.IsValid())
	{
		TreeDetailsView->ForceRefresh();
	}
}

void FPHPassiveTreeAssetEditor::BindGraphCommands()
{
	const FGenericCommands& Commands = FGenericCommands::Get();

	ToolkitCommands->MapAction(Commands.Delete,
		FExecuteAction::CreateSP(this, &FPHPassiveTreeAssetEditor::DeleteSelectedNodes),
		FCanExecuteAction::CreateSP(this, &FPHPassiveTreeAssetEditor::CanDeleteNodes));
	ToolkitCommands->MapAction(Commands.Copy,
		FExecuteAction::CreateSP(this, &FPHPassiveTreeAssetEditor::CopySelectedNodes),
		FCanExecuteAction::CreateSP(this, &FPHPassiveTreeAssetEditor::CanCopyNodes));
	ToolkitCommands->MapAction(Commands.Cut,
		FExecuteAction::CreateSP(this, &FPHPassiveTreeAssetEditor::CutSelectedNodes),
		FCanExecuteAction::CreateSP(this, &FPHPassiveTreeAssetEditor::CanDeleteNodes));
	ToolkitCommands->MapAction(Commands.Paste,
		FExecuteAction::CreateSP(this, &FPHPassiveTreeAssetEditor::PasteNodes),
		FCanExecuteAction::CreateSP(this, &FPHPassiveTreeAssetEditor::CanPasteNodes));
	ToolkitCommands->MapAction(Commands.Duplicate,
		FExecuteAction::CreateSP(this, &FPHPassiveTreeAssetEditor::DuplicateNodes),
		FCanExecuteAction::CreateSP(this, &FPHPassiveTreeAssetEditor::CanCopyNodes));
	ToolkitCommands->MapAction(Commands.SelectAll,
		FExecuteAction::CreateSP(this, &FPHPassiveTreeAssetEditor::SelectAllNodes));
}

void FPHPassiveTreeAssetEditor::ExtendToolbar()
{
	const TSharedRef<FExtender> Extender = MakeShared<FExtender>();
	Extender->AddToolBarExtension(
		TEXT("Asset"), EExtensionHook::After, ToolkitCommands,
		FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& Builder)
		{
			Builder.BeginSection(TEXT("PassiveTree"));
			Builder.AddToolBarButton(
				FUIAction(FExecuteAction::CreateSP(this, &FPHPassiveTreeAssetEditor::ValidateTree)),
				NAME_None,
				LOCTEXT("Validate", "Validate"),
				LOCTEXT("ValidateTooltip",
					"Check the graph for islands, missing connections, and unusable random-start rules."),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Blueprints")));
			Builder.EndSection();
		}));
	AddToolbarExtender(Extender);
}

void FPHPassiveTreeAssetEditor::ValidateTree()
{
	SyncAssetFromGraph();
	if (!Asset)
	{
		return;
	}

	TArray<FText> Errors;
	const bool bValid = Asset->ValidateTree(Errors);

	FNotificationInfo Info(bValid
		? FText::Format(LOCTEXT("ValidPassed", "Passive tree is valid. {0} node(s)."),
			FText::AsNumber(Asset->Nodes.Num()))
		: FText::Format(LOCTEXT("ValidFailed", "Passive tree has {0} problem(s). See the Output Log."),
			FText::AsNumber(Errors.Num())));
	Info.ExpireDuration = 6.0f;
	FSlateNotificationManager::Get().AddNotification(Info)
		->SetCompletionState(bValid ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);

	for (const FText& Error : Errors)
	{
		UE_LOG(LogTemp, Warning, TEXT("Passive tree '%s': %s"), *Asset->GetName(), *Error.ToString());
	}
}

void FPHPassiveTreeAssetEditor::OnGraphChanged(const FEdGraphEditAction& Action)
{
	SyncAssetFromGraph();
}

void FPHPassiveTreeAssetEditor::OnGraphSelectionChanged(const TSet<UObject*>& NewSelection)
{
	// Also the cheapest reliable moment to capture node positions: dragging a node does not raise a
	// graph-changed event, but the selection always settles afterwards.
	SyncAssetFromGraph();

	TArray<UObject*> Selected;
	for (UObject* Object : NewSelection)
	{
		if (Cast<UPHPassiveTreeGraphNode>(Object))
		{
			Selected.Add(Object);
		}
	}

	if (NodeDetailsView.IsValid())
	{
		NodeDetailsView->SetObjects(Selected);
	}
}

void FPHPassiveTreeAssetEditor::OnNodeDetailsChanged(const FPropertyChangedEvent& Event)
{
	SyncAssetFromGraph();
	if (GraphEditor.IsValid())
	{
		// A renamed node has to redraw its title.
		GraphEditor->NotifyGraphChanged();
	}
}

void FPHPassiveTreeAssetEditor::SaveAsset_Execute()
{
	SyncAssetFromGraph();
	FAssetEditorToolkit::SaveAsset_Execute();
}

void FPHPassiveTreeAssetEditor::OnClose()
{
	SyncAssetFromGraph();
	FAssetEditorToolkit::OnClose();
}

void FPHPassiveTreeAssetEditor::PostUndo(bool bSuccess)
{
	// Push the restored graph back to the asset rather than rebuilding the graph from it. The graph
	// and its nodes are the transactional objects, so undo has already restored them - including node
	// positions, which are recorded on the node by the move transaction and never on the asset.
	// Rebuilding the other way would overwrite an undone move with the stale position.
	SyncAssetFromGraph();
	if (GraphEditor.IsValid())
	{
		GraphEditor->ClearSelectionSet();
		GraphEditor->NotifyGraphChanged();
	}
	if (NodeDetailsView.IsValid())
	{
		NodeDetailsView->SetObjects(TArray<UObject*>());
	}
}

void FPHPassiveTreeAssetEditor::PostRedo(bool bSuccess)
{
	PostUndo(bSuccess);
}

void FPHPassiveTreeAssetEditor::DeleteSelectedNodes()
{
	if (!GraphEditor.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DeleteNodes", "Delete Passive Nodes"));
	Graph->Modify();

	const FGraphPanelSelectionSet Selection = GraphEditor->GetSelectedNodes();
	GraphEditor->ClearSelectionSet();
	for (UObject* Object : Selection)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(Object); Node && Node->CanUserDeleteNode())
		{
			Node->Modify();
			Node->DestroyNode();
		}
	}

	SyncAssetFromGraph();
	GraphEditor->NotifyGraphChanged();
}

bool FPHPassiveTreeAssetEditor::CanDeleteNodes() const
{
	return GraphEditor.IsValid() && GraphEditor->GetSelectedNodes().Num() > 0;
}

bool FPHPassiveTreeAssetEditor::CanCopyNodes() const
{
	return CanDeleteNodes();
}

void FPHPassiveTreeAssetEditor::CopySelectedNodes()
{
	if (!GraphEditor.IsValid())
	{
		return;
	}

	FGraphPanelSelectionSet Selection = GraphEditor->GetSelectedNodes();
	for (UObject* Object : Selection)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(Object))
		{
			Node->PrepareForCopying();
		}
	}

	FString Exported;
	FEdGraphUtilities::ExportNodesToText(Selection, Exported);
	FPlatformApplicationMisc::ClipboardCopy(*Exported);
}

void FPHPassiveTreeAssetEditor::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedNodes();
}

bool FPHPassiveTreeAssetEditor::CanPasteNodes() const
{
	FString Clipboard;
	FPlatformApplicationMisc::ClipboardPaste(Clipboard);
	return Graph.IsValid() && FEdGraphUtilities::CanImportNodesFromText(Graph.Get(), Clipboard);
}

void FPHPassiveTreeAssetEditor::PasteNodes()
{
	if (!GraphEditor.IsValid() || !Graph.IsValid())
	{
		return;
	}

	FString Clipboard;
	FPlatformApplicationMisc::ClipboardPaste(Clipboard);

	const FScopedTransaction Transaction(LOCTEXT("PasteNodes", "Paste Passive Nodes"));
	Graph->Modify();
	GraphEditor->ClearSelectionSet();

	TSet<UEdGraphNode*> Pasted;
	FEdGraphUtilities::ImportNodesFromText(Graph.Get(), Clipboard, Pasted);

	// Offset the block so it does not land exactly on the nodes it was copied from, then re-key it:
	// a pasted node carrying its original NodeID would collide with the node it came from, and the ID
	// is the save key, so the collision would be silent and permanent.
	const FVector2D PasteLocation = GraphEditor->GetPasteLocation2f();
	FVector2D Average = FVector2D::ZeroVector;
	for (const UEdGraphNode* Node : Pasted)
	{
		Average += FVector2D(Node->NodePosX, Node->NodePosY);
	}
	if (!Pasted.IsEmpty())
	{
		Average /= Pasted.Num();
	}

	for (UEdGraphNode* Node : Pasted)
	{
		Node->NodePosX = FMath::RoundToInt(Node->NodePosX - Average.X + PasteLocation.X);
		Node->NodePosY = FMath::RoundToInt(Node->NodePosY - Average.Y + PasteLocation.Y);
		Node->CreateNewGuid();
		if (UPHPassiveTreeGraphNode* PassiveNode = Cast<UPHPassiveTreeGraphNode>(Node))
		{
			PassiveNode->Definition.NodeID = UPHPassiveTreeGraphNode::MakeUniqueNodeID(Graph.Get());
			PassiveNode->SyncDefinitionFromGraph();
		}
		GraphEditor->SetNodeSelection(Node, true);
	}

	SyncAssetFromGraph();
	GraphEditor->NotifyGraphChanged();
}

void FPHPassiveTreeAssetEditor::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

void FPHPassiveTreeAssetEditor::SelectAllNodes()
{
	if (GraphEditor.IsValid())
	{
		GraphEditor->SelectAllNodes();
	}
}

#undef LOCTEXT_NAMESPACE

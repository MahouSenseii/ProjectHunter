// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "UI/PHMenuPreviewEditorLibrary.h"

#include "AbilitySystem/HunterAbilitySystemComponent.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Character/PHBaseCharacter.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/WidgetSwitcher.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/UserInterfaceSettings.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Inventory/Components/InventoryManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Layout/Children.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "MovieScene.h"
#include "PreviewScene.h"
#include "Progression/Components/CharacterProgressionManager.h"
#include "Progression/Components/PHPassiveTreeComponent.h"
#include "Progression/Data/PHPassiveTreeDataAsset.h"
#include "Progression/Settings/PHPassiveTreeSettings.h"
#include "GraphEditor.h"
#include "SGraphPanel.h"
#include "Passive/PHPassiveTreeGraphNode.h"
#include "Passive/PHPassiveTreeGraphSchema.h"
#include "RenderingThread.h"
#include "RHIGlobals.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/JsonSerializer.h"
#include "Slate/WidgetRenderer.h"
#include "Stats/Components/StatsManager.h"
#include "Stats/Data/BaseStatsData.h"
#include "TextureCompiler.h"
#include "UI/HUD/HunterHUD.h"
#include "UI/HUD/HunterHUDResourceWidget.h"
#include "UI/Menu/Widgets/PHEquipmentMenuPageWidget.h"
#include "UI/Menu/Widgets/PHEquipmentMenuPanelWidget.h"
#include "UI/Menu/Widgets/PHEquipmentSlotWidget.h"
#include "UI/Menu/Widgets/PHInventoryMenuPanelWidget.h"
#include "UI/Menu/Widgets/PHInventorySlotWidget.h"
#include "UI/Menu/Widgets/PHMenuRootWidget.h"
#include "UI/Menu/Widgets/PHSettingsMenuPageWidget.h"
#include "UI/Menu/Widgets/PHMenuTabBarWidget.h"
#include "UI/Menu/Widgets/PHMenuTabButtonWidget.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

DEFINE_LOG_CATEGORY_STATIC(LogPHMenuPreviewEditor, Log, All);

namespace PHMenuPreviewEditor
{
	constexpr const TCHAR* RootPath = TEXT("/Game/ProjectHunter/UI/Widgets/Menus/System/WBP_SystemMenuRoot.WBP_SystemMenuRoot");
	constexpr const TCHAR* StatsPath = TEXT("/Game/ProjectHunter/Gameplay/Stats/DA_BaseStats.DA_BaseStats");
	const TCHAR* const MenuPaths[] = {
		RootPath,
		TEXT("/Game/ProjectHunter/UI/Widgets/Menus/System/WBP_SystemMenuHeader.WBP_SystemMenuHeader"),
		TEXT("/Game/ProjectHunter/UI/Widgets/Menus/System/WBP_SystemMenuTab.WBP_SystemMenuTab"),
		TEXT("/Game/ProjectHunter/UI/Widgets/Menus/Equipment/WBP_EquipmentPage.WBP_EquipmentPage"),
		TEXT("/Game/ProjectHunter/UI/Widgets/Menus/Equipment/WBP_EquipmentPanel.WBP_EquipmentPanel"),
		TEXT("/Game/ProjectHunter/UI/Widgets/Menus/Equipment/WBP_EquipmentSlot.WBP_EquipmentSlot"),
		TEXT("/Game/ProjectHunter/UI/Widgets/Menus/Equipment/WBP_InventoryPanel.WBP_InventoryPanel"),
		TEXT("/Game/ProjectHunter/UI/Widgets/Menus/Equipment/WBP_InventorySlot.WBP_InventorySlot"),
		TEXT("/Game/ProjectHunter/UI/Widgets/Menus/Equipment/WBP_CharacterPreview.WBP_CharacterPreview"),
		TEXT("/Game/ProjectHunter/UI/Widgets/Menus/Equipment/WBP_VitalsPanel.WBP_VitalsPanel"),
		TEXT("/Game/ProjectHunter/UI/Widgets/Menus/Equipment/WBP_VitalBar.WBP_VitalBar")
	};

	bool Fail(const FString& Message)
	{
		UE_LOG(LogPHMenuPreviewEditor, Error, TEXT("%s"), *Message);
		return false;
	}

	FString ExportProperty(UObject* Object, const FName Name)
	{
		FString Value;
		if (Object)
		{
			if (const FProperty* Property = FindFProperty<FProperty>(Object->GetClass(), Name))
			{
				Property->ExportText_InContainer(0, Value, Object, nullptr, Object, PPF_None);
			}
		}
		return Value;
	}

	TSharedRef<FJsonObject> Properties(UObject* Object)
	{
		TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
		if (Object)
		{
			for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
			{
				if (!It->HasAnyPropertyFlags(CPF_Transient | CPF_SkipSerialization))
				{
					Result->SetStringField(It->GetName(), ExportProperty(Object, It->GetFName()));
				}
			}
		}
		return Result;
	}

	TSharedRef<FJsonObject> Configuration(UObject* Object)
	{
		TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
		for (const TCHAR* Name : {
			TEXT("MenuEntries"), TEXT("DefaultMenuType"), TEXT("DefaultPageWidgetClass"),
			TEXT("bBuildHeaderFromMenuEnum"), TEXT("TabWidgetClass"), TEXT("CloseMenuKeys"),
			TEXT("bDropItemsToWorldOnMissedDrop"), TEXT("EquipmentSlotOrder"),
			TEXT("EquipmentSlotWidgetClass"), TEXT("InventorySlotWidgetClass"),
			TEXT("bAutoBuildEquipmentSlotWidgets"), TEXT("bAutoBuildInventorySlotWidgets"),
			TEXT("bIncludeEmptyInventorySlots"), TEXT("GridColumns"), TEXT("InventoryCellSize"),
			TEXT("ConnectedEquipmentSlot"), TEXT("bEnableDragAndDrop"),
			TEXT("bDropToWorldWhenDraggedOutOfMenu"), TEXT("DragVisualWidgetClass"), TEXT("DragVisualSize"),
			TEXT("bShowTooltipOnHover"), TEXT("bTooltipFollowsMouse"), TEXT("bFocusMenuCameraOnHover"),
			TEXT("bShowItemNameInSlot"), TEXT("bHideSlotNameWhenOccupied"), TEXT("DragMouseButton"),
			TEXT("DragSensitivity"), TEXT("WheelZoomStep"), TEXT("bResetTurntableOnDoubleClick"),
			TEXT("ResourceType"), TEXT("CurrentAttribute"), TEXT("MaxAttribute"), TEXT("ReservedAttribute"),
			TEXT("BarFillType"), TEXT("FillInterpSpeed"), TEXT("DamageLagDelay"), TEXT("DamageLagInterpSpeed"),
			TEXT("bManageVisibility"), TEXT("bAutoHideNonPlayerBar"), TEXT("StaminaHideWhenFullThreshold"),
			TEXT("NonPlayerAutoHideDelay"), TEXT("bApplyProgressBarImageStyle"), TEXT("bUseLayeredBarBackgrounds"),
			TEXT("NormalColor"), TEXT("HoveredColor"), TEXT("SelectedColor"),
			TEXT("NormalTextColor"), TEXT("SelectedTextColor"), TEXT("BarWidthOverride"), TEXT("BarHeightOverride")})
		{
			if (Object && FindFProperty<FProperty>(Object->GetClass(), Name))
			{
				Result->SetStringField(Name, ExportProperty(Object, Name));
			}
		}
		if (Object && Object->IsA<UHunterHUDResourceWidget>())
		{
			Result->SetStringField(TEXT("Visibility"), ExportProperty(Object, TEXT("Visibility")));
		}
		return Result;
	}

	FString DiskHash(UObject* Asset)
	{
		if (!Asset) { return FString(); }
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		TArray<uint8> Bytes;
		return FFileHelper::LoadFileToArray(Bytes, *Filename)
			? FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString() : FString();
	}

	bool WriteJSON(const FString& Filename, const TSharedRef<FJsonObject>& Object)
	{
		if (Filename.IsEmpty())
		{
			return Fail(TEXT("A JSON output filename is required."));
		}
		const FString FullPath = FPaths::ConvertRelativePathToFull(Filename);
		FString Text;
		return FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Text)) &&
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(FullPath), true) &&
			FFileHelper::SaveStringToFile(Text, *FullPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	TSharedRef<FJsonObject> GraphContracts(UWidgetBlueprint* Blueprint)
	{
		TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
		Result->SetStringField(TEXT("bindings"), ExportProperty(Blueprint, TEXT("Bindings")));
		Result->SetStringField(TEXT("widget_variable_guids"), ExportProperty(Blueprint, TEXT("WidgetVariableNameToGuidMap")));
		TSharedRef<FJsonObject> NativeBindings = MakeShared<FJsonObject>();
		for (TFieldIterator<FProperty> It(Blueprint->ParentClass); It; ++It)
		{
			if (It->HasMetaData(TEXT("BindWidget")) || It->HasMetaData(TEXT("BindWidgetOptional")))
			{
				TSharedRef<FJsonObject> Binding = MakeShared<FJsonObject>();
				Binding->SetBoolField(TEXT("optional"), It->HasMetaData(TEXT("BindWidgetOptional")));
				Binding->SetStringField(TEXT("type"), It->GetCPPType());
				UWidget* Widget = Blueprint->WidgetTree->FindWidget(It->GetFName());
				Binding->SetStringField(TEXT("authored_class"), Widget ? Widget->GetClass()->GetPathName() : TEXT("None"));
				NativeBindings->SetObjectField(It->GetName(), Binding);
			}
		}
		Result->SetObjectField(TEXT("native_widget_bindings"), NativeBindings);
		TArray<UEdGraph*> Graphs;
		Blueprint->GetAllGraphs(Graphs);
		Graphs.Sort([](const UEdGraph& A, const UEdGraph& B) { return A.GetPathName() < B.GetPathName(); });
		TArray<TSharedPtr<FJsonValue>> GraphValues;
		for (UEdGraph* Graph : Graphs)
		{
			TSharedRef<FJsonObject> GraphValue = MakeShared<FJsonObject>();
			GraphValue->SetStringField(TEXT("name"), Graph->GetName());
			GraphValue->SetStringField(TEXT("guid"), Graph->GraphGuid.ToString());
			GraphValue->SetStringField(TEXT("schema"), ExportProperty(Graph, TEXT("Schema")));
			TArray<TSharedPtr<FJsonValue>> Nodes;
			for (const UEdGraphNode* Node : Graph->Nodes)
			{
				if (!Node) { continue; }
				TSharedRef<FJsonObject> NodeValue = MakeShared<FJsonObject>();
				NodeValue->SetStringField(TEXT("name"), Node->GetName());
				NodeValue->SetStringField(TEXT("class"), Node->GetClass()->GetPathName());
				NodeValue->SetStringField(TEXT("guid"), Node->NodeGuid.ToString());
				NodeValue->SetObjectField(TEXT("properties"), Properties(const_cast<UEdGraphNode*>(Node)));
				TArray<TSharedPtr<FJsonValue>> Pins;
				for (const UEdGraphPin* Pin : Node->Pins)
				{
					if (!Pin) { continue; }
					TSharedRef<FJsonObject> PinValue = MakeShared<FJsonObject>();
					PinValue->SetStringField(TEXT("name"), Pin->PinName.ToString());
					PinValue->SetStringField(TEXT("id"), Pin->PinId.ToString());
					PinValue->SetStringField(TEXT("persistent_guid"), Pin->PersistentGuid.ToString());
					PinValue->SetNumberField(TEXT("direction"), static_cast<int32>(Pin->Direction.GetValue()));
					FString Type;
					FEdGraphPinType::StaticStruct()->ExportText(Type, &Pin->PinType, nullptr, nullptr, PPF_None, nullptr);
					PinValue->SetStringField(TEXT("type"), Type);
					PinValue->SetStringField(TEXT("default"), Pin->DefaultValue);
					PinValue->SetStringField(TEXT("default_object"), GetPathNameSafe(Pin->DefaultObject.Get()));
					PinValue->SetStringField(TEXT("default_text"), Pin->DefaultTextValue.ToString());
					PinValue->SetStringField(TEXT("autogenerated_default"), Pin->AutogeneratedDefaultValue);
					PinValue->SetStringField(TEXT("parent_pin"), Pin->ParentPin ? Pin->ParentPin->PinId.ToString() : FString());
					PinValue->SetBoolField(TEXT("orphaned"), Pin->bOrphanedPin);
					TArray<FString> Links;
					for (const UEdGraphPin* Link : Pin->LinkedTo)
					{
						if (Link)
						{
							Links.Add(Link->GetOwningNode()->NodeGuid.ToString() + TEXT(":") + Link->PinId.ToString());
						}
					}
					Links.Sort();
					TArray<TSharedPtr<FJsonValue>> LinkValues;
					for (const FString& Link : Links) { LinkValues.Add(MakeShared<FJsonValueString>(Link)); }
					PinValue->SetArrayField(TEXT("links"), LinkValues);
					Pins.Add(MakeShared<FJsonValueObject>(PinValue));
				}
				NodeValue->SetArrayField(TEXT("pins"), Pins);
				Nodes.Add(MakeShared<FJsonValueObject>(NodeValue));
			}
			GraphValue->SetArrayField(TEXT("nodes"), Nodes);
			GraphValues.Add(MakeShared<FJsonValueObject>(GraphValue));
		}
		Result->SetArrayField(TEXT("graphs"), GraphValues);
		TArray<TSharedPtr<FJsonValue>> Animations;
		for (UWidgetAnimation* Animation : Blueprint->Animations)
		{
			if (!Animation) { continue; }
			TSharedRef<FJsonObject> Value = MakeShared<FJsonObject>();
			Value->SetStringField(TEXT("name"), Animation->GetName());
			Value->SetStringField(TEXT("path"), Animation->GetPathName());
			Value->SetStringField(TEXT("movie_scene"), GetPathNameSafe(Animation->GetMovieScene()));
			Value->SetStringField(TEXT("bindings"), ExportProperty(Animation, TEXT("AnimationBindings")));
			Value->SetNumberField(TEXT("start_time"), Animation->GetStartTime());
			Value->SetNumberField(TEXT("end_time"), Animation->GetEndTime());
			Animations.Add(MakeShared<FJsonValueObject>(Value));
		}
		Result->SetArrayField(TEXT("animations"), Animations);
		return Result;
	}

	void CollectWidgets(UWidget* Root, TArray<UWidget*>& OutWidgets)
	{
		if (!Root) { return; }
		OutWidgets.AddUnique(Root);
		for (int32 Index = 0; Index < OutWidgets.Num(); ++Index)
		{
			UWidget* Widget = OutWidgets[Index];
			if (UUserWidget* UserWidget = Cast<UUserWidget>(Widget); UserWidget && UserWidget->WidgetTree)
			{
				if (UWidget* TreeRoot = UserWidget->WidgetTree->RootWidget) { OutWidgets.AddUnique(TreeRoot); }
			}
			if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
			{
				for (UWidget* Child : Panel->GetAllChildren()) { OutWidgets.AddUnique(Child); }
			}
		}
	}

	struct FMenuFixture
	{
		TStrongObjectPtr<UBlueprint> CharacterBlueprint;
		TStrongObjectPtr<UBaseStatsData> StatsData;
		FPreviewScene Scene{FPreviewScene::ConstructionValues().SetEditor(false).SetTransactional(false)
			.SetCreateDefaultLighting(false).AllowAudioPlayback(false)};
		TStrongObjectPtr<UPHMenuRootWidget> Root;
		TSharedPtr<SWidget> Slate;
		APHBaseCharacter* Character = nullptr;
		UHunterAbilitySystemComponent* ASC = nullptr;
		UInventoryManager* Inventory = nullptr;
		UCharacterProgressionManager* Progression = nullptr;
		UPHPassiveTreeComponent* Passives = nullptr;

		~FMenuFixture()
		{
			if (Root.IsValid()) { Root->ReleaseCharacter(); }
			Slate.Reset();
			if (Root.IsValid()) { Root->ReleaseSlateResources(true); }
			Root.Reset();
		}

		bool Initialize(EMenuType PageToShow = EMenuType::MT_Equipment,
			EPHSettingsSection SettingsSection = EPHSettingsSection::SS_Gameplay)
		{
			UWidgetBlueprint* MenuBlueprint = LoadObject<UWidgetBlueprint>(nullptr, RootPath);
			UBaseStatsData* AuthoredStats = LoadObject<UBaseStatsData>(nullptr, StatsPath);
			if (!MenuBlueprint || !MenuBlueprint->GeneratedClass || !AuthoredStats ||
				!MenuBlueprint->GeneratedClass->IsChildOf(UPHMenuRootWidget::StaticClass()))
			{
				return Fail(TEXT("The existing WBP_SystemMenuRoot and DA_BaseStats must load with their original native classes."));
			}
			CharacterBlueprint.Reset(FKismetEditorUtilities::CreateBlueprint(APHBaseCharacter::StaticClass(),
				GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(),
					TEXT("BP_MenuPresentationPreview")), BPTYPE_Normal));
			if (!CharacterBlueprint.IsValid()) { return Fail(TEXT("Could not create the transient menu fixture class.")); }
			CharacterBlueprint->SetFlags(RF_Transient);
			CharacterBlueprint->ClearFlags(RF_Standalone);
			FKismetEditorUtilities::CompileBlueprint(CharacterBlueprint.Get(), EBlueprintCompileOptions::SkipSave |
				EBlueprintCompileOptions::SkipGarbageCollection | EBlueprintCompileOptions::SkipFiBSearchMetaUpdate);
			if (!CharacterBlueprint->GeneratedClass) { return Fail(TEXT("The transient character class did not compile.")); }
			FActorSpawnParameters Parameters;
			Parameters.bDeferConstruction = true;
			Parameters.ObjectFlags = RF_Transient;
			Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Character = Scene.GetWorld()->SpawnActor<APHBaseCharacter>(
				CharacterBlueprint->GeneratedClass.Get(), FTransform::Identity, Parameters);
			if (!Character) { return Fail(TEXT("Could not create the isolated character data owner.")); }
			Character->AutoPossessPlayer = EAutoReceiveInput::Disabled;
			Character->AutoPossessAI = EAutoPossessAI::Disabled;
			Character->FinishSpawning(FTransform::Identity);
			if (!Character->IsActorInitialized())
			{
				Character->PreInitializeComponents();
				Character->InitializeComponents();
				Character->PostInitializeComponents();
			}
			UStatsManager* Stats = Character->GetStatsManager();
			Inventory = Character->FindComponentByClass<UInventoryManager>();
			const FObjectPropertyBase* StatsProperty = Stats ? FindFProperty<FObjectPropertyBase>(Stats->GetClass(), TEXT("StatsData")) : nullptr;
			if (!StatsProperty || !Inventory) { return Fail(TEXT("The native character's stats and inventory components must exist.")); }
			StatsData.Reset(DuplicateObject<UBaseStatsData>(AuthoredStats, GetTransientPackage()));
			if (!StatsData.IsValid()) { return Fail(TEXT("Could not copy authored stats into the transient preview.")); }
			StatsData->SetFlags(RF_Transient);
			StatsData->ClearFlags(RF_Standalone);
			// Configure only this transient owner, then use its normal initialization/listener path.
			StatsProperty->SetObjectPropertyValue_InContainer(Stats, StatsData.Get());
			Character->OnRep_PlayerState();
			ASC = Cast<UHunterAbilitySystemComponent>(Character->GetAbilitySystemComponent());
			Progression = Character->FindComponentByClass<UCharacterProgressionManager>();
			Passives = Character->FindComponentByClass<UPHPassiveTreeComponent>();
			if (!ASC || !ASC->GetSet<UHunterAttributeSet>() || !Stats->HasInitializedStats())
			{
				return Fail(TEXT("The existing ability-system/stat initialization did not supply the menu fixture."));
			}
			if (PageToShow == EMenuType::MT_PassiveTree)
			{
				if (!Progression || !Passives)
				{
					return Fail(TEXT("The native character must supply progression and passive-tree owners."));
				}
				// Show real allocated, available, and locked states without touching game saves.
				Progression->UnspentPassivePoints = 6;
				Progression->TotalPassivePoints = 6;

				// A fixed seed so the preview is reproducible, but a genuine roll, so the shot shows
				// what a real Hunter's opening looks like rather than a hand-picked branch.
				if (Passives->RollRandomStart(20260831).IsNone())
				{
					return Fail(TEXT("The passive preview could not roll a random start."));
				}

				// Grown outward from wherever the roll landed rather than from a hardcoded list, so the
				// preview stays valid if the tree or the roll changes.
				const UPHPassiveTreeDataAsset* PreviewTree = Passives->GetTreeData();
				if (!PreviewTree)
				{
					return Fail(TEXT("The passive preview has no tree to grow from."));
				}
				for (int32 Step = 0; Step < 3; ++Step)
				{
					FName NextNodeID = NAME_None;
					FText UnusedReason;
					for (const FPHPassiveNodeDefinition& Node : PreviewTree->Nodes)
					{
						if (Passives->CanAllocateNode(Node.NodeID, UnusedReason))
						{
							NextNodeID = Node.NodeID;
							break;
						}
					}
					if (NextNodeID.IsNone() || !Passives->AllocateNode(NextNodeID))
					{
						break;
					}
				}
			}
			Root.Reset(CreateWidget<UPHMenuRootWidget>(Scene.GetWorld(), MenuBlueprint->GeneratedClass.Get()));
			if (!Root.IsValid()) { return Fail(TEXT("Could not instantiate the actual generated system menu.")); }
			Slate = Root->TakeWidget();
			Root->InitializeForCharacter(Character);
			Root->OpenMenu(PageToShow);

			// Drive the settings sub-tab through its own API rather than the
			// asset's default, so the capture exercises the real code path.
			if (UPHSettingsMenuPageWidget* SettingsPage =
				Cast<UPHSettingsMenuPageWidget>(Root->GetActivePage()))
			{
				SettingsPage->ShowSection(SettingsSection);
			}
			return Root->GetActivePage() != nullptr || Fail(TEXT("The authored menu did not create its equipment page."));
		}
	};

	bool ValidateFixture(FMenuFixture& Fixture)
	{
		UPHMenuRootWidget* Root = Fixture.Root.Get();
		UPHEquipmentMenuPageWidget* Page = Cast<UPHEquipmentMenuPageWidget>(Root->GetActivePage());
		if (!Page || !Root->WidgetTree || !Page->WidgetTree ||
			!Root->WidgetTree->FindWidget<UButton>(TEXT("CloseButton")) ||
			!Root->WidgetTree->FindWidget<UPHMenuTabBarWidget>(TEXT("TabBar")) ||
			!Root->WidgetTree->FindWidget<UWidgetSwitcher>(TEXT("ContentSwitcher")))
		{
			return Fail(TEXT("The actual menu's root/page native parents or required control bindings changed."));
		}
		UPHInventoryMenuPanelWidget* InventoryPanel = Page->WidgetTree->FindWidget<UPHInventoryMenuPanelWidget>(TEXT("InventoryPanel"));
		if (!InventoryPanel || !Page->WidgetTree->FindWidget<UPHEquipmentMenuPanelWidget>(TEXT("EquipmentPanel")) ||
			InventoryPanel->GetInventorySlots().Num() != Fixture.Inventory->GetSlotCount())
		{
			return Fail(TEXT("The equipment page must bind its original panels and render the existing inventory owner's slots."));
		}
		TArray<UWidget*> Widgets;
		CollectWidgets(Root, Widgets);
		UPHInventorySlotWidget* FirstCell = nullptr;
		UPHMenuTabButtonWidget* SettingsTab = nullptr;
		UPHMenuTabButtonWidget* EquipmentTab = nullptr;
		UClass* AuthoredPreviewClass = LoadClass<UUserWidget>(nullptr,
			TEXT("/Game/ProjectHunter/UI/Widgets/Menus/Equipment/WBP_CharacterPreview.WBP_CharacterPreview_C"));
		int32 InventoryCellCount = 0;
		int32 EquipmentCellCount = 0;
		bool bHasCharacterPreview = false;
		for (UWidget* Widget : Widgets)
		{
			if (UPHInventorySlotWidget* Cell = Cast<UPHInventorySlotWidget>(Widget))
			{
				++InventoryCellCount;
				if (!FirstCell) { FirstCell = Cell; }
			}
			EquipmentCellCount += Widget->IsA<UPHEquipmentSlotWidget>() ? 1 : 0;
			// Preserve the authored preview class; presentation validation must not require reparenting it.
			bHasCharacterPreview |= Widget->GetFName() == TEXT("WBP_CharacterPreview") && Widget->GetClass() == AuthoredPreviewClass;
			if (UPHMenuTabButtonWidget* Tab = Cast<UPHMenuTabButtonWidget>(Widget))
			{
				if (Tab->GetMenuType() == EMenuType::MT_Settings) { SettingsTab = Tab; }
				if (Tab->GetMenuType() == EMenuType::MT_Equipment) { EquipmentTab = Tab; }
			}
			if (UHunterHUDResourceWidget* Vital = Cast<UHunterHUDResourceWidget>(Widget))
			{
				if (Vital->GetBoundCharacter() != Fixture.Character || !Vital->WidgetTree ||
					!Vital->WidgetTree->FindWidget<UProgressBar>(TEXT("Bar_Current")))
				{
					return Fail(TEXT("A menu vital lost its existing character listener or progress-bar binding."));
				}
			}
		}
		if (!FirstCell || InventoryCellCount != Fixture.Inventory->GetSlotCount() ||
			EquipmentCellCount != Page->GetEquipmentSlots().Num() || !bHasCharacterPreview ||
			!SettingsTab || !EquipmentTab || !EquipmentTab->IsSelected())
		{
			return Fail(TEXT("Generated menu cells, selected tab, or the existing character-preview widget are missing."));
		}
		FirstCell->SelectSlot();
		const int32 SlotIndex = FirstCell->GetSlotData().SlotIndex;
		if (InventoryPanel->GetSelectedInventorySlotIndex() != SlotIndex || Page->GetSelectedInventorySlotIndex() != SlotIndex ||
			FirstCell->RequestEquip(EEquipmentSlot::ES_MainHand))
		{
			return Fail(TEXT("Inventory selection must reach the panel/page and an empty cell must reject equipping."));
		}
		for (UPHMenuTabButtonWidget* Tab : {SettingsTab, EquipmentTab})
		{
			UButton* Button = Tab->WidgetTree ? Tab->WidgetTree->FindWidget<UButton>(TEXT("TabButton")) : nullptr;
			if (!Button) { return Fail(TEXT("An authored tab lost its TabButton binding.")); }
			Button->OnClicked.Broadcast();
			if (Root->GetActiveMenuType() != Tab->GetMenuType() || !Tab->IsSelected())
			{
				return Fail(TEXT("The real button-to-tab-to-root selection delegate chain failed."));
			}
		}
		InventoryPanel->RefreshInventoryData();
		TArray<UWidget*> RefreshedWidgets;
		CollectWidgets(Root, RefreshedWidgets);
		if (Root->GetActivePage() != Page || !RefreshedWidgets.Contains(FirstCell))
		{
			return Fail(TEXT("Changing pages or refreshing inventory replaced the existing cached page/cells."));
		}
		InventoryPanel->ClearSelection();
		return true;
	}

	void FreezeSlateTicks(const TSharedRef<SWidget>& Widget)
	{
		Widget->SetCanTick(false);
		FChildren* Children = Widget->GetChildren();
		for (int32 Index = 0; Index < Children->Num(); ++Index) { FreezeSlateTicks(Children->GetChildAt(Index)); }
	}
}

bool UPHMenuPreviewEditorLibrary::InspectMenuContracts(const FString& OutputJSONPath)
{
	using namespace PHMenuPreviewEditor;
	TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("scope"), TEXT("Read-only loaded menu contracts; no explicit asset compilation, mutation or save."));
	Report->SetStringField(TEXT("menu_host"), AHunterHUD::StaticClass()->GetPathName());
	Report->SetStringField(TEXT("inventory_owner"), UInventoryManager::StaticClass()->GetPathName());
	Report->SetStringField(TEXT("equipment_owner"), UEquipmentManager::StaticClass()->GetPathName());
	Report->SetStringField(TEXT("stats_owner"), UStatsManager::StaticClass()->GetPathName());
	TArray<TSharedPtr<FJsonValue>> Assets;
	for (const TCHAR* Path : MenuPaths)
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, Path);
		if (!Blueprint || !Blueprint->WidgetTree || !Blueprint->GeneratedClass)
		{
			return Fail(FString::Printf(TEXT("Missing authored menu Blueprint/tree/generated class: %s"), Path));
		}
		TSharedRef<FJsonObject> Asset = MakeShared<FJsonObject>();
		Asset->SetStringField(TEXT("asset"), Blueprint->GetPathName());
		Asset->SetStringField(TEXT("parent_class"), GetPathNameSafe(Blueprint->ParentClass));
		const UClass* NativeParent = Blueprint->ParentClass;
		while (NativeParent && !NativeParent->HasAnyClassFlags(CLASS_Native)) { NativeParent = NativeParent->GetSuperClass(); }
		Asset->SetStringField(TEXT("native_parent"), GetPathNameSafe(NativeParent));
		Asset->SetStringField(TEXT("generated_class"), Blueprint->GeneratedClass->GetPathName());
		Asset->SetStringField(TEXT("compile_status"), UEnum::GetValueAsString(Blueprint->Status.GetValue()));
		Asset->SetBoolField(TEXT("compile_ok"), Blueprint->Status == BS_UpToDate || Blueprint->Status == BS_UpToDateWithWarnings);
		Asset->SetStringField(TEXT("disk_sha1"), DiskHash(Blueprint));
		Asset->SetBoolField(TEXT("package_dirty"), Blueprint->GetOutermost()->IsDirty());
		Asset->SetObjectField(TEXT("contracts"), GraphContracts(Blueprint));
		Asset->SetObjectField(TEXT("configuration"), Configuration(Blueprint->GeneratedClass->GetDefaultObject()));
		TArray<UWidget*> Widgets;
		Blueprint->WidgetTree->GetAllWidgets(Widgets);
		Widgets.Sort([](const UWidget& A, const UWidget& B) { return A.GetName() < B.GetName(); });
		TArray<TSharedPtr<FJsonValue>> WidgetValues;
		for (UWidget* Widget : Widgets)
		{
			TSharedRef<FJsonObject> Value = MakeShared<FJsonObject>();
			Value->SetStringField(TEXT("name"), Widget->GetName());
			Value->SetStringField(TEXT("class"), Widget->GetClass()->GetPathName());
			Value->SetStringField(TEXT("parent"), GetNameSafe(Widget->GetParent()));
			Value->SetObjectField(TEXT("configuration"), Configuration(Widget));
			Value->SetObjectField(TEXT("properties"), Properties(Widget));
			if (Widget->Slot)
			{
				Value->SetStringField(TEXT("slot_class"), Widget->Slot->GetClass()->GetPathName());
				Value->SetObjectField(TEXT("slot_properties"), Properties(Widget->Slot));
			}
			WidgetValues.Add(MakeShared<FJsonValueObject>(Value));
		}
		Asset->SetArrayField(TEXT("widgets"), WidgetValues);
		Assets.Add(MakeShared<FJsonValueObject>(Asset));
	}
	Report->SetArrayField(TEXT("assets"), Assets);
	if (!WriteJSON(OutputJSONPath, Report)) { return Fail(TEXT("Could not write the menu contract report.")); }
	UE_LOG(LogPHMenuPreviewEditor, Display, TEXT("Inspected %d authored menu widget contracts without saving assets: %s"), Assets.Num(), *OutputJSONPath);
	return true;
}

bool UPHMenuPreviewEditorLibrary::ValidateSystemMenu()
{
	using namespace PHMenuPreviewEditor;
	if (!FSlateApplication::IsInitialized()) { return Fail(TEXT("Menu validation requires initialized Slate.")); }
	FMenuFixture Fixture;
	if (!Fixture.Initialize() || !ValidateFixture(Fixture)) { return false; }
	UE_LOG(LogPHMenuPreviewEditor, Display, TEXT("Actual menu bindings, generated cells, selection, tab clicks and cached-page reuse passed in an isolated transient world."));
	return true;
}

bool UPHMenuPreviewEditorLibrary::RenderSystemMenu(const FString& OutputPNGPath, const int32 Width, const int32 Height,
	const EMenuType PageToShow, const EPHSettingsSection SettingsSection)
{
	using namespace PHMenuPreviewEditor;
	if (OutputPNGPath.IsEmpty() || !FApp::CanEverRender() || !FSlateApplication::IsInitialized() ||
		!GIsRHIInitialized || GUsingNullRHI || Width <= 0 || Height <= 0 || Width > 8192 || Height > 8192)
	{
		return Fail(TEXT("Menu PNG rendering requires an output path, initialized Slate, a real RHI (omit -NullRHI), and dimensions from 1 to 8192."));
	}
	FMenuFixture Fixture;
	if (!Fixture.Initialize(PageToShow, SettingsSection)) { return false; }
	const float DPIScale = GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(FIntPoint(Width, Height));
	TArray<UWidget*> Widgets;
	CollectWidgets(Fixture.Root.Get(), Widgets);
	TArray<UTexture*> Textures;
	auto AddTexture = [&Textures](const FSlateBrush& Brush)
	{
		if (UTexture* Texture = Cast<UTexture>(Brush.GetResourceObject())) { Textures.AddUnique(Texture); }
	};
	int32 InventoryCells = 0;
	int32 EquipmentCells = 0;
	for (UWidget* Widget : Widgets)
	{
		InventoryCells += Widget->IsA<UPHInventorySlotWidget>() ? 1 : 0;
		EquipmentCells += Widget->IsA<UPHEquipmentSlotWidget>() ? 1 : 0;
		if (UImage* Image = Cast<UImage>(Widget)) { AddTexture(Image->GetBrush()); }
		// UE 5.7 exposes Border's brush as Background (there is no GetBrush accessor).
		if (UBorder* Border = Cast<UBorder>(Widget)) { AddTexture(Border->Background); }
		if (UButton* Button = Cast<UButton>(Widget))
		{
			AddTexture(Button->GetStyle().Normal);
			AddTexture(Button->GetStyle().Hovered);
			AddTexture(Button->GetStyle().Pressed);
		}
		if (UProgressBar* Bar = Cast<UProgressBar>(Widget))
		{
			AddTexture(Bar->GetWidgetStyle().BackgroundImage);
			AddTexture(Bar->GetWidgetStyle().FillImage);
		}
	}
	if (InventoryCells != Fixture.Inventory->GetSlotCount() || EquipmentCells == 0)
	{
		return Fail(TEXT("The real configured equipment/inventory cells must be present before a menu preview is exported."));
	}
	FTextureCompilingManager::Get().FinishCompilation(Textures);
	// Freeze only these transient widgets. No game/editor world is advanced to make the image.
	FreezeSlateTicks(Fixture.Slate.ToSharedRef());
	TStrongObjectPtr<UTextureRenderTarget2D> Target(FWidgetRenderer::CreateTargetFor(FVector2D(Width, Height), TF_Bilinear, false));
	if (!Target.IsValid()) { return Fail(TEXT("Could not create the menu preview render target.")); }
	FWidgetRenderer* Renderer = new FWidgetRenderer(true, true);
	if (!Renderer->GetSlateRenderer())
	{
		BeginCleanup(Renderer);
		return Fail(TEXT("Slate did not supply the menu preview renderer."));
	}
	// Slate applies the same display gamma as a viewport. The force-linear target
	// avoids a second GPU conversion; PNG export treats its byte pixels as sRGB.
	// Disabling Slate gamma here would export linear bytes as sRGB and darken the UI.
	Renderer->DrawWidget(Target.Get(), Fixture.Slate.ToSharedRef(), DPIScale, FVector2D(Width, Height), 0.0f);
	FlushRenderingCommands();
	FBufferArchive PNG;
	const bool bExported = FImageUtils::ExportRenderTarget2DAsPNG(Target.Get(), PNG);
	BeginCleanup(Renderer);
	FlushRenderingCommands();
	const FString FullPath = FPaths::ConvertRelativePathToFull(OutputPNGPath);
	if (!bExported || !IFileManager::Get().MakeDirectory(*FPaths::GetPath(FullPath), true) || !FFileHelper::SaveArrayToFile(PNG, *FullPath))
	{
		return Fail(TEXT("Could not export the actual menu as a PNG."));
	}
	TSharedRef<FJsonObject> Metadata = MakeShared<FJsonObject>();
	Metadata->SetStringField(TEXT("preview_mode"), TEXT("UI-only; the transparent center normally reveals the live world character. No character image or duplicate camera was created."));
	Metadata->SetStringField(TEXT("menu_asset"), RootPath);
	Metadata->SetStringField(TEXT("stats_source"), StatsPath);
	Metadata->SetStringField(TEXT("stats_source_sha1"), DiskHash(LoadObject<UBaseStatsData>(nullptr, StatsPath)));
	Metadata->SetNumberField(TEXT("width"), Width);
	Metadata->SetNumberField(TEXT("height"), Height);
	Metadata->SetNumberField(TEXT("viewport_dpi_scale"), DPIScale);
	Metadata->SetNumberField(TEXT("inventory_cells"), InventoryCells);
	Metadata->SetNumberField(TEXT("equipment_cells"), EquipmentCells);
	Metadata->SetNumberField(TEXT("inventory_items"), Fixture.Inventory->GetItemCount());
	if (Fixture.Progression && Fixture.Passives)
	{
		Metadata->SetNumberField(TEXT("passive_points"), Fixture.Progression->UnspentPassivePoints);
		Metadata->SetNumberField(TEXT("allocated_passives"), Fixture.Passives->AllocatedNodeIDs.Num());
		Metadata->SetStringField(TEXT("random_start"), Fixture.Passives->RandomStartNodeID.ToString());
	}
	TSharedRef<FJsonObject> Values = MakeShared<FJsonObject>();
	for (const FGameplayAttribute& Attribute : {UHunterAttributeSet::GetHealthAttribute(), UHunterAttributeSet::GetMaxEffectiveHealthAttribute(),
		UHunterAttributeSet::GetManaAttribute(), UHunterAttributeSet::GetMaxEffectiveManaAttribute(),
		UHunterAttributeSet::GetStaminaAttribute(), UHunterAttributeSet::GetMaxEffectiveStaminaAttribute(), UHunterAttributeSet::GetPlayerLevelAttribute()})
	{
		Values->SetNumberField(Attribute.GetName(), Fixture.ASC->GetNumericAttribute(Attribute));
	}
	Metadata->SetObjectField(TEXT("authoritative_fixture_values"), Values);
	if (!WriteJSON(FPaths::ChangeExtension(FullPath, TEXT("json")), Metadata)) { return Fail(TEXT("PNG was written but preview metadata could not be saved.")); }
	UE_LOG(LogPHMenuPreviewEditor, Display, TEXT("Rendered actual system menu UI-only at %dx%d, viewport DPI %.3f, %d inventory/%d equipment cells: %s"),
		Width, Height, DPIScale, InventoryCells, EquipmentCells, *FullPath);
	return true;
}

bool UPHMenuPreviewEditorLibrary::RenderPassiveTreeGraph(
	const FString& OutputPNGPath, const int32 Width, const int32 Height)
{
	using namespace PHMenuPreviewEditor;
	if (OutputPNGPath.IsEmpty() || !FApp::CanEverRender() || !FSlateApplication::IsInitialized() ||
		!GIsRHIInitialized || GUsingNullRHI || Width <= 0 || Height <= 0 || Width > 8192 || Height > 8192)
	{
		return Fail(TEXT("Graph PNG rendering requires an output path, initialized Slate, a real RHI (omit -NullRHI), and dimensions from 1 to 8192."));
	}

	UPHPassiveTreeDataAsset* Tree = GetDefault<UPHPassiveTreeSettings>()->DefaultTree.LoadSynchronous();
	if (!Tree || Tree->Nodes.IsEmpty()) { return Fail(TEXT("No default passive tree is configured to preview.")); }

	// Built the same way the asset editor builds it, so the capture shows the real editor visuals.
	TStrongObjectPtr<UEdGraph> Graph(NewObject<UEdGraph>(
		GetTransientPackage(), UEdGraph::StaticClass(), NAME_None, RF_Transactional));
	Graph->Schema = UPHPassiveTreeGraphSchema::StaticClass();

	TMap<FName, UPHPassiveTreeGraphNode*> ByID;
	for (const FPHPassiveNodeDefinition& Definition : Tree->Nodes)
	{
		UPHPassiveTreeGraphNode* Node = NewObject<UPHPassiveTreeGraphNode>(Graph.Get());
		Node->CreateNewGuid();
		Node->Definition = Definition;
		Node->ApplyGraphPosition();
		Node->AllocateDefaultPins();
		Graph->Nodes.Add(Node);
		ByID.FindOrAdd(Definition.NodeID, Node);
	}
	for (UEdGraphNode* RawNode : Graph->Nodes)
	{
		UPHPassiveTreeGraphNode* Node = CastChecked<UPHPassiveTreeGraphNode>(RawNode);
		UEdGraphPin* ParentPin = Node->GetParentPin();
		if (!ParentPin) { continue; }
		for (const FName ParentID : Node->Definition.RequiredNodeIDs)
		{
			UPHPassiveTreeGraphNode* const* Parent = ByID.Find(ParentID);
			if (Parent && *Parent != Node)
			{
				if (UEdGraphPin* ChildPin = (*Parent)->GetChildPin()) { ParentPin->MakeLinkTo(ChildPin); }
			}
		}
	}

	// Without this the capture can catch placeholder mips and the chamfer reads as a blurry blob.
	TArray<UTexture*> Textures;
	for (const TCHAR* TexturePath : {
		TEXT("/Game/ProjectHunter/UI/Widgets/Menus/System/T_SystemPanel_Fill.T_SystemPanel_Fill"),
		TEXT("/Game/ProjectHunter/UI/Widgets/Menus/System/T_SystemPanel_Frame.T_SystemPanel_Frame")})
	{
		if (UTexture* Texture = LoadObject<UTexture>(nullptr, TexturePath)) { Textures.AddUnique(Texture); }
	}
	FTextureCompilingManager::Get().FinishCompilation(Textures);

	// Editable, not read-only: a read-only panel stamps a large READ-ONLY watermark over the capture.
	const TSharedRef<SGraphEditor> GraphEditor = SNew(SGraphEditor)
		.IsEditable(true)
		.GraphToEdit(Graph.Get());

	TStrongObjectPtr<UTextureRenderTarget2D> Target(
		FWidgetRenderer::CreateTargetFor(FVector2D(Width, Height), TF_Bilinear, false));
	if (!Target.IsValid()) { return Fail(TEXT("Could not create the graph preview render target.")); }
	FWidgetRenderer* Renderer = new FWidgetRenderer(true, true);
	if (!Renderer->GetSlateRenderer())
	{
		BeginCleanup(Renderer);
		return Fail(TEXT("Slate did not supply the graph preview renderer."));
	}

	// The panel frames itself rather than being positioned by hand: it only learns its own node
	// extents once it has a geometry and has spawned node widgets, and ZoomToFit is deferred to a
	// later tick and then eased, so the view needs several ticks to settle before the capture.
	GraphEditor->NotifyGraphChanged();
	Renderer->DrawWidget(Target.Get(), GraphEditor, 1.0f, FVector2D(Width, Height), 0.1f);
	FlushRenderingCommands();

	// The panel builds its node widgets from an active timer, and active timers only fire for widgets
	// living in a real Slate window - a render target's virtual window never ticks them. Driving
	// Update() by hand is what makes an offline capture of a graph possible at all.
	if (SGraphPanel* Panel = GraphEditor->GetGraphPanel())
	{
		Panel->Update();
	}
	Renderer->DrawWidget(Target.Get(), GraphEditor, 1.0f, FVector2D(Width, Height), 0.1f);
	FlushRenderingCommands();

	// ZoomToFit is deferred to a tick this virtual window never delivers, so the view is set
	// directly. RestoreViewSettings applies immediately, unlike SetViewLocation.
	FBox2D Bounds(ForceInit);
	for (const FPHPassiveNodeDefinition& Definition : Tree->Nodes)
	{
		if (FMath::IsFinite(Definition.Position.X) && FMath::IsFinite(Definition.Position.Y))
		{
			Bounds += Definition.Position;
		}
	}
	// Padded by a node footprint so edge nodes are not clipped by their own width, then asked for
	// less zoom than would exactly fit: the panel snaps to discrete levels and can snap upward, which
	// would crop the outermost nodes.
	const FVector2D Extent = Bounds.GetSize() + FVector2D(560.0, 320.0);
	const float Zoom = static_cast<float>(FMath::Clamp(
		FMath::Min(Width / FMath::Max(Extent.X, 1.0), Height / FMath::Max(Extent.Y, 1.0)) * 0.75, 0.05, 1.0));
	if (SGraphPanel* Panel = GraphEditor->GetGraphPanel())
	{
		// The panel snaps to its own discrete zoom levels, so the requested zoom is rarely the one
		// applied. Centring against the requested value would offset the whole tree; the applied value
		// is read back and the offset recomputed from that.
		Panel->RestoreViewSettings(FVector2f::ZeroVector, Zoom);
		const float AppliedZoom = FMath::Max(Panel->GetZoomAmount(), UE_KINDA_SMALL_NUMBER);
		Panel->RestoreViewSettings(
			FVector2f(Bounds.GetCenter() - FVector2D(Width, Height) * 0.5 / AppliedZoom), AppliedZoom);
	}

	for (int32 Settle = 0; Settle < 4; ++Settle)
	{
		Renderer->DrawWidget(Target.Get(), GraphEditor, 1.0f, FVector2D(Width, Height), 0.1f);
		FlushRenderingCommands();
	}

	FBufferArchive PNG;
	const bool bExported = FImageUtils::ExportRenderTarget2DAsPNG(Target.Get(), PNG);
	BeginCleanup(Renderer);
	FlushRenderingCommands();

	const FString FullPath = FPaths::ConvertRelativePathToFull(OutputPNGPath);
	if (!bExported || !IFileManager::Get().MakeDirectory(*FPaths::GetPath(FullPath), true) ||
		!FFileHelper::SaveArrayToFile(PNG, *FullPath))
	{
		return Fail(TEXT("Could not export the passive graph as a PNG."));
	}

	UE_LOG(LogPHMenuPreviewEditor, Display,
		TEXT("Rendered the passive graph editor at %dx%d, %d nodes: %s"),
		Width, Height, Graph->Nodes.Num(), *FullPath);
	return true;
}

// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "UI/PHHUDEditorLibrary.h"
#include "PHHUDEditorInternals.h"

#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Factories/TextureFactory.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Layout/Children.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "PreviewScene.h"
#include "RenderingThread.h"
#include "ScopedTransaction.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Slate/WidgetRenderer.h"
#include "TextureCompiler.h"
#include "UI/HUD/HunterHUDResourceWidget.h"
#include "UI/HUD/HunterMainHUDWidget.h"
#include "UI/HUD/PHFloorBannerWidget.h"
#include "UI/HUD/PHRunStatusWidget.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"
#include "Widgets/SWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogPHHUDEditor, Log, All);

namespace PHHUDEditor
{
	constexpr const TCHAR* HUDPath = TEXT("/Game/ProjectHunter/UI/HUD/WBP_HunterHUD.WBP_HunterHUD");
	constexpr const TCHAR* ResourcePath = TEXT("/Game/ProjectHunter/UI/Widgets/WBP_BaseProgressBar.WBP_BaseProgressBar");
	constexpr const TCHAR* EmblemPath = TEXT("/Game/ProjectHunter/Textures/HUD/T_HunterEmblem.T_HunterEmblem");

	bool Fail(const FString& Message)
	{
		UE_LOG(LogPHHUDEditor, Error, TEXT("%s"), *Message);
		return false;
	}

	FString EvidenceDirectory()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation/PH-HUD-TopLeft"));
	}

	FString DiskHash(const FString& Filename)
	{
		TArray<uint8> Bytes;
		return FFileHelper::LoadFileToArray(Bytes, *Filename)
			? FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString() : FString();
	}

	FString ExportProperty(UObject* Object, const FName PropertyName)
	{
		FString Result;
		if (const FProperty* Property = FindFProperty<FProperty>(Object->GetClass(), PropertyName))
		{
			Property->ExportText_InContainer(0, Result, Object, nullptr, Object, PPF_None);
		}
		return Result;
	}

	TSharedRef<FJsonObject> DescribeProperties(UObject* Object)
	{
		TSharedRef<FJsonObject> Properties = MakeShared<FJsonObject>();
		for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
		{
			if (!It->HasAnyPropertyFlags(CPF_Transient | CPF_SkipSerialization))
			{
				Properties->SetStringField(It->GetName(), ExportProperty(Object, It->GetFName()));
			}
		}
		return Properties;
	}

	FString SavedObjectHash(UObject* Object)
	{
		TArray<uint8> Bytes;
		FMemoryWriter Writer(Bytes, true);
		FObjectAndNameAsStringProxyArchive Archive(Writer, false);
		Object->Serialize(Archive);
		return FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString();
	}

	TSharedRef<FJsonObject> DescribeObject(UObject* Object)
	{
		TSharedRef<FJsonObject> Description = MakeShared<FJsonObject>();
		Description->SetStringField(TEXT("name"), Object->GetName());
		Description->SetStringField(TEXT("path"), Object->GetPathName());
		Description->SetStringField(TEXT("class"), Object->GetClass()->GetPathName());
		Description->SetStringField(TEXT("serialized_sha1"), SavedObjectHash(Object));
		Description->SetObjectField(TEXT("properties"), DescribeProperties(Object));
		return Description;
	}

	FString JSONText(const TSharedRef<FJsonObject>& Object)
	{
		FString Text;
		FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Text));
		return Text;
	}

	bool WriteJSON(const FString& Filename, const TSharedRef<FJsonObject>& Object)
	{
		if (Filename.IsEmpty() || !IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true))
		{
			return Fail(TEXT("Cannot create the requested HUD inspection output directory."));
		}
		return FFileHelper::SaveStringToFile(JSONText(Object), *Filename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	TSharedRef<FJsonObject> ContractInventory(UWidgetBlueprint* Blueprint)
	{
		TSharedRef<FJsonObject> Inventory = MakeShared<FJsonObject>();
		Inventory->SetStringField(TEXT("bindings"), ExportProperty(Blueprint, TEXT("Bindings")));
		Inventory->SetStringField(TEXT("animations"), ExportProperty(Blueprint, TEXT("Animations")));
		TArray<UEdGraph*> Graphs;
		Blueprint->GetAllGraphs(Graphs);
		TArray<TSharedPtr<FJsonValue>> GraphDescriptions;
		for (UEdGraph* Graph : Graphs)
		{
			TSharedRef<FJsonObject> Description = MakeShared<FJsonObject>();
			Description->SetStringField(TEXT("name"), Graph->GetName());
			Description->SetStringField(TEXT("guid"), Graph->GraphGuid.ToString());
			TArray<TSharedPtr<FJsonValue>> Nodes;
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (Node)
				{
					TSharedRef<FJsonObject> NodeDescription = MakeShared<FJsonObject>();
					NodeDescription->SetStringField(TEXT("name"), Node->GetName());
					NodeDescription->SetStringField(TEXT("class"), Node->GetClass()->GetPathName());
					NodeDescription->SetStringField(TEXT("guid"), Node->NodeGuid.ToString());
					Nodes.Add(MakeShared<FJsonValueObject>(NodeDescription));
				}
			}
			Description->SetArrayField(TEXT("nodes"), Nodes);
			GraphDescriptions.Add(MakeShared<FJsonValueObject>(Description));
		}
		Inventory->SetArrayField(TEXT("graphs"), GraphDescriptions);
		return Inventory;
	}

	TSharedRef<FJsonObject> ResourceSnapshot(UHunterHUDResourceWidget* Resource)
	{
		TSharedRef<FJsonObject> Result = DescribeObject(Resource);
		Result->SetStringField(TEXT("parent"), GetPathNameSafe(Resource->GetParent()));
		if (Resource->Slot)
		{
			Result->SetObjectField(TEXT("slot"), DescribeObject(Resource->Slot));
		}
		TArray<UObject*> Children;
		GetObjectsWithOuter(Resource, Children, true);
		Children.Sort([](const UObject& A, const UObject& B) { return A.GetPathName() < B.GetPathName(); });
		TArray<TSharedPtr<FJsonValue>> Subobjects;
		for (UObject* Child : Children)
		{
			if (!Child->HasAnyFlags(RF_Transient))
			{
				Subobjects.Add(MakeShared<FJsonValueObject>(DescribeObject(Child)));
			}
		}
		Result->SetArrayField(TEXT("subobjects"), Subobjects);
		return Result;
	}

	TSharedRef<FJsonObject> Inspection(UWidgetBlueprint* Blueprint)
	{
		TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
		Result->SetStringField(TEXT("asset"), Blueprint->GetPathName());
		Result->SetStringField(TEXT("native_parent"), GetPathNameSafe(Blueprint->ParentClass));
		Result->SetBoolField(TEXT("package_dirty"), Blueprint->GetOutermost()->IsDirty());
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		Result->SetStringField(TEXT("disk_filename"), FPaths::ConvertRelativePathToFull(Filename));
		Result->SetStringField(TEXT("disk_sha1"), DiskHash(Filename));
		Result->SetObjectField(TEXT("contracts"), ContractInventory(Blueprint));
		Result->SetStringField(TEXT("widget_variable_guids"), ExportProperty(Blueprint, TEXT("WidgetVariableNameToGuidMap")));
		TArray<TSharedPtr<FJsonValue>> Widgets;
		if (Blueprint->WidgetTree)
		{
			Result->SetStringField(TEXT("root"), GetPathNameSafe(Blueprint->WidgetTree->RootWidget));
			Blueprint->WidgetTree->ForEachWidget([&Widgets](UWidget* Widget)
			{
				TSharedRef<FJsonObject> Description = DescribeObject(Widget);
				Description->SetStringField(TEXT("parent"), GetPathNameSafe(Widget->GetParent()));
				if (Widget->Slot)
				{
					Description->SetObjectField(TEXT("slot"), DescribeObject(Widget->Slot));
				}
				if (UHunterHUDResourceWidget* Resource = Cast<UHunterHUDResourceWidget>(Widget))
				{
					Description->SetObjectField(TEXT("resource_snapshot"), ResourceSnapshot(Resource));
				}
				Widgets.Add(MakeShared<FJsonValueObject>(Description));
			});
		}
		Result->SetArrayField(TEXT("widgets"), Widgets);
		return Result;
	}

	UWidgetBlueprint* LoadHUD()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, HUDPath);
		if (!Blueprint || !Blueprint->ParentClass || !Blueprint->ParentClass->IsChildOf(UHunterMainHUDWidget::StaticClass()) ||
			!Blueprint->WidgetTree || !Blueprint->WidgetTree->RootWidget)
		{
			Fail(TEXT("The existing WBP_HunterHUD must retain its HunterMainHUDWidget parent and authored root tree."));
			return nullptr;
		}
		return Blueprint;
	}

	bool BackupHUD(UWidgetBlueprint* Blueprint)
	{
		const FString Source = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		const FString Hash = DiskHash(Source);
		if (Hash.IsEmpty())
		{
			return Fail(TEXT("Cannot read the existing player HUD package for a verified backup."));
		}
		const FString Directory = EvidenceDirectory() / TEXT("Backups") / Hash;
		if (!IFileManager::Get().MakeDirectory(*Directory, true))
		{
			return Fail(TEXT("Cannot create the HUD backup directory; no assets were changed."));
		}
		for (const TCHAR* Extension : {TEXT(".uasset"), TEXT(".uexp"), TEXT(".ubulk")})
		{
			const FString SourcePart = FPaths::ChangeExtension(Source, Extension);
			if (!IFileManager::Get().FileExists(*SourcePart))
			{
				continue;
			}
			const FString Destination = Directory / FPaths::GetCleanFilename(SourcePart);
			if (!IFileManager::Get().FileExists(*Destination) &&
				IFileManager::Get().Copy(*Destination, *SourcePart, false, false) != COPY_OK)
			{
				return Fail(TEXT("Backing up a HUD package file failed; no assets were changed."));
			}
			if (DiskHash(Destination) != DiskHash(SourcePart))
			{
				return Fail(TEXT("HUD backup hash verification failed; no assets were changed."));
			}
		}
		UE_LOG(LogPHHUDEditor, Display, TEXT("Preserved HUD backup %s (SHA1 %s)."), *Directory, *Hash);
		const FString InspectionPath = Directory / TEXT("AuthoredInspection.json");
		return IFileManager::Get().FileExists(*InspectionPath) || WriteJSON(InspectionPath, Inspection(Blueprint));
	}

	bool SaveNamedAsset(UObject* Asset)
	{
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		Args.bSlowTask = false;
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		return UPackage::SavePackage(Asset->GetOutermost(), Asset, *Filename, Args);
	}

	UTexture2D* GetOrImportEmblem(const FString& SourcePath, bool& bWasImported)
	{
		bWasImported = false;
		if (!IFileManager::Get().FileExists(*SourcePath) || !FPaths::GetExtension(SourcePath).Equals(TEXT("png"), ESearchCase::IgnoreCase))
		{
			Fail(TEXT("Supply the existing HunterEmblem PNG source file; no substitute image is generated."));
			return nullptr;
		}
		if (UTexture2D* Existing = LoadObject<UTexture2D>(nullptr, EmblemPath, nullptr, LOAD_NoWarn))
		{
			const FMD5Hash SourceHash = FMD5Hash::HashFile(*SourcePath);
			if (Existing->AssetImportData && Existing->AssetImportData->GetSourceData().SourceFiles.ContainsByPredicate(
				[&SourceHash](const FAssetImportInfo::FSourceFile& Source) { return Source.FileHash == SourceHash; }))
			{
				const FString ExistingFilename = FPackageName::LongPackageNameToFilename(
					Existing->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
				// A failed Apply may have imported it in this editor session without saving yet.
				bWasImported = !IFileManager::Get().FileExists(*ExistingFilename);
				return Existing;
			}
			Fail(TEXT("An existing T_HunterEmblem does not match the supplied source PNG. It was not overwritten."));
			return nullptr;
		}
		UTextureFactory* Factory = NewObject<UTextureFactory>();
		Factory->LODGroup = TEXTUREGROUP_UI;
		Factory->CompressionSettings = TC_EditorIcon;
		Factory->MipGenSettings = TMGS_NoMipmaps;
		Factory->NoAlpha = false;
		Factory->bCreateMaterial = false;
		UAssetImportTask* Task = NewObject<UAssetImportTask>();
		Task->Filename = FPaths::ConvertRelativePathToFull(SourcePath);
		Task->DestinationPath = TEXT("/Game/ProjectHunter/Textures/HUD");
		Task->DestinationName = TEXT("T_HunterEmblem");
		Task->bAutomated = true;
		Task->bAsync = false;
		Task->bSave = false;
		Task->bReplaceExisting = false;
		Task->Factory = Factory;
		FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().ImportAssetTasks({Task});
		for (UObject* Imported : Task->GetObjects())
		{
			if (UTexture2D* Texture = Cast<UTexture2D>(Imported); Texture && Texture->GetPathName() == EmblemPath)
			{
				bWasImported = true;
				return Texture;
			}
		}
		Fail(TEXT("The texture factory did not return the expected T_HunterEmblem asset; the HUD was not saved."));
		return nullptr;
	}

	FSlateBrush BarBrush(const FLinearColor& Color, const FLinearColor& Outline, float Radius, float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Color);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius), FSlateColor(Outline), OutlineWidth);
		return Brush;
	}

	void StyleResource(UHunterHUDResourceWidget* Resource, bool bHealth)
	{
		Resource->Modify();
		Resource->CurrentFillColor = bHealth ? FLinearColor(0.52f, 0.95f, 0.025f, 1.0f) : FLinearColor(0.01f, 0.72f, 1.0f, 1.0f);
		Resource->BarFillType = EProgressBarFillType::LeftToRight;
		Resource->BarWidthOverride = 0.0f;
		Resource->BarHeightOverride = 0.0f;
		Resource->FillInterpSpeed = bHealth ? 16.0f : 18.0f;
		Resource->DamageLagDelay = bHealth ? 0.30f : 0.10f;
		Resource->DamageLagInterpSpeed = 8.0f;
		Resource->bApplyProgressBarImageStyle = true;
		Resource->bUseLayeredBarBackgrounds = true;
		const FLinearColor Outline = bHealth ? FLinearColor(0.56f, 0.64f, 0.55f, 0.9f) : FLinearColor(0.32f, 0.60f, 0.72f, 0.9f);
		Resource->ProgressBarImageStyle.SetBackgroundImage(BarBrush(FLinearColor(0.016f, 0.022f, 0.035f, 0.82f), Outline, bHealth ? 1.0f : 3.0f, 1.0f));
		Resource->ProgressBarImageStyle.SetFillImage(BarBrush(FLinearColor::White, FLinearColor::Transparent, bHealth ? 1.0f : 3.0f, 0.0f));
	}

	template <typename WidgetType>
	WidgetType* NamedWidget(UWidgetTree* Tree, const TCHAR* Name)
	{
		WidgetType* Widget = Cast<WidgetType>(Tree->FindWidget(Name));
		if (!Widget)
		{
			Widget = Tree->ConstructWidget<WidgetType>(WidgetType::StaticClass(), Name);
		}
		Widget->Modify();
		return Widget;
	}

	void SetTextStyle(UTextBlock* Text, int32 Size)
	{
		Text->bIsVariable = true;
		// The native TextBlock font references a cooked UFont asset; CoreStyle's transient
		// composite font cannot be serialized into a Widget Blueprint.
		FSlateFontInfo Font = GetDefault<UTextBlock>()->GetFont();
		Font.TypefaceFontName = TEXT("Regular");
		Font.Size = Size;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.96f, 1.0f, 1.0f)));
		Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f));
		Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
	}
}

bool UPHHUDEditorLibrary::InspectPlayerHUD(const FString& OutputJSONPath)
{
	UWidgetBlueprint* Blueprint = PHHUDEditor::LoadHUD();
	return Blueprint && PHHUDEditor::WriteJSON(FPaths::ConvertRelativePathToFull(OutputJSONPath), PHHUDEditor::Inspection(Blueprint));
}

bool UPHHUDEditorLibrary::ApplyHealthManaLayout(const FString& EmblemSourcePath)
{
	using namespace PHHUDEditor;
	UWidgetBlueprint* Blueprint = LoadHUD();
	if (!Blueprint)
	{
		return false;
	}
	UWidgetTree* Tree = Blueprint->WidgetTree;
	UWidget* OriginalRoot = Tree->RootWidget;
	if (!OriginalRoot->IsA<UOverlay>() && !OriginalRoot->IsA<UCanvasPanel>())
	{
		return Fail(TEXT("This utility supports the existing Overlay/Canvas root only; it will not replace the HUD root."));
	}
	UHunterHUDResourceWidget* Health = nullptr;
	UHunterHUDResourceWidget* Mana = nullptr;
	UHunterHUDResourceWidget* Stamina = nullptr;
	int32 HealthCount = 0, ManaCount = 0, StaminaCount = 0;
	Tree->ForEachWidget([&](UWidget* Widget)
	{
		if (UHunterHUDResourceWidget* Resource = Cast<UHunterHUDResourceWidget>(Widget))
		{
			switch (Resource->ResourceType)
			{
			case EHunterResourceType::Health: Health = Resource; ++HealthCount; break;
			case EHunterResourceType::Mana: Mana = Resource; ++ManaCount; break;
			case EHunterResourceType::Stamina: Stamina = Resource; ++StaminaCount; break;
			default: break;
			}
		}
	});
	if (HealthCount != 1 || StaminaCount != 1 || ManaCount > 1 || !Health || !Stamina ||
		Health->GetFName() != TEXT("Health") || Stamina->GetFName() != TEXT("Stamina") ||
		(Mana && Mana->GetFName() != TEXT("Mana") && Mana->GetFName() != TEXT("ManaWidget")))
	{
		return Fail(TEXT("Expected exactly the existing Health and Stamina widgets and at most one recognized Mana widget. No asset was changed."));
	}
	const TPair<FName, UClass*> LayoutWidgets[] = {
		{TEXT("HealthManaGroup"), UHorizontalBox::StaticClass()}, {TEXT("HunterEmblemSize"), USizeBox::StaticClass()},
		{TEXT("HunterEmblemScale"), UScaleBox::StaticClass()}, {TEXT("HunterEmblemImage"), UImage::StaticClass()},
		{TEXT("HealthManaColumnSize"), USizeBox::StaticClass()}, {TEXT("HealthManaColumn"), UVerticalBox::StaticClass()},
		{TEXT("HealthManaHeader"), UHorizontalBox::StaticClass()}, {TEXT("PlayerLevelText"), UTextBlock::StaticClass()},
		{TEXT("HealthValueText"), UTextBlock::StaticClass()}, {TEXT("HealthLayoutSize"), USizeBox::StaticClass()},
		{TEXT("ManaLayoutSize"), USizeBox::StaticClass()}
	};
	const bool bExistingGroup = Tree->FindWidget(TEXT("HealthManaGroup")) != nullptr;
	for (const auto& Expected : LayoutWidgets)
	{
		UObject* ExistingObject = FindObject<UObject>(Tree, *Expected.Key.ToString());
		if (ExistingObject && (!bExistingGroup || !ExistingObject->IsA(Expected.Value)))
		{
			return Fail(FString::Printf(TEXT("Existing widget name %s conflicts with the requested layout; it was preserved."), *Expected.Key.ToString()));
		}
	}
	UWidgetBlueprint* ResourceBlueprint = LoadObject<UWidgetBlueprint>(nullptr, ResourcePath);
	if (!ResourceBlueprint || !ResourceBlueprint->GeneratedClass ||
		!ResourceBlueprint->GeneratedClass->IsChildOf(UHunterHUDResourceWidget::StaticClass()) ||
		ResourceBlueprint->GeneratedClass->HasAnyClassFlags(CLASS_Abstract) ||
		(!Mana && FindObject<UObject>(Tree, TEXT("Mana"))))
	{
		return Fail(TEXT("The existing WBP_BaseProgressBar class or the Mana name is unavailable; no widget was replaced."));
	}
	if (!BackupHUD(Blueprint))
	{
		return false;
	}
	const FString StaminaBefore = JSONText(ResourceSnapshot(Stamina));
	const FString ContractsBefore = JSONText(ContractInventory(Blueprint));
	const TMap<FName, FGuid> OriginalVariableGuids = Blueprint->WidgetVariableNameToGuidMap;
	const auto HasOriginalVariableGuids = [&]()
	{
		for (const TPair<FName, FGuid>& Original : OriginalVariableGuids)
		{
			const FGuid* Current = Blueprint->WidgetVariableNameToGuidMap.Find(Original.Key);
			if (!Current || *Current != Original.Value)
			{
				return false;
			}
		}
		return true;
	};
	UPanelSlot* StaminaSlot = Stamina->Slot;
	UWidget* StaminaParent = Stamina->GetParent();
	bool bImportedEmblem = false;
	UTexture2D* Emblem = GetOrImportEmblem(EmblemSourcePath, bImportedEmblem);
	const FIntPoint EmblemDimensions = Emblem ? Emblem->GetImportedSize() : FIntPoint::ZeroValue;
	if (!Emblem || EmblemDimensions.X <= 0 || EmblemDimensions.Y <= 0)
	{
		return Fail(TEXT("A valid HunterEmblem texture is required; the HUD was not changed."));
	}

	const FScopedTransaction Transaction(NSLOCTEXT("PHHUDEditor", "ApplyLayout", "Apply Project Hunter Health and Mana layout"));
	Blueprint->Modify();
	Tree->Modify();
	OriginalRoot->Modify();
	Health->Modify();
	if (!Mana)
	{
		// Match the UMG palette's template path, avoiding runtime CreateWidget initialization.
		Mana = Cast<UHunterHUDResourceWidget>(Tree->ConstructWidget<UWidget>(ResourceBlueprint->GeneratedClass.Get(), TEXT("Mana")));
		if (!Mana)
		{
			return Fail(TEXT("Could not create the single missing Mana instance; the HUD was not saved."));
		}
		Mana->bIsVariable = true;
		Mana->ResourceType = EHunterResourceType::Mana;
		Mana->CurrentAttribute = FGameplayAttribute();
		Mana->MaxAttribute = FGameplayAttribute();
		Mana->ReservedAttribute = FGameplayAttribute();
	}
	StyleResource(Health, true);
	StyleResource(Mana, false);

	UHorizontalBox* Group = NamedWidget<UHorizontalBox>(Tree, TEXT("HealthManaGroup"));
	USizeBox* EmblemSize = NamedWidget<USizeBox>(Tree, TEXT("HunterEmblemSize"));
	UScaleBox* EmblemScale = NamedWidget<UScaleBox>(Tree, TEXT("HunterEmblemScale"));
	UImage* EmblemImage = NamedWidget<UImage>(Tree, TEXT("HunterEmblemImage"));
	USizeBox* ColumnSize = NamedWidget<USizeBox>(Tree, TEXT("HealthManaColumnSize"));
	UVerticalBox* Column = NamedWidget<UVerticalBox>(Tree, TEXT("HealthManaColumn"));
	UHorizontalBox* Header = NamedWidget<UHorizontalBox>(Tree, TEXT("HealthManaHeader"));
	UTextBlock* LevelText = NamedWidget<UTextBlock>(Tree, TEXT("PlayerLevelText"));
	UTextBlock* HealthText = NamedWidget<UTextBlock>(Tree, TEXT("HealthValueText"));
	USizeBox* HealthSize = NamedWidget<USizeBox>(Tree, TEXT("HealthLayoutSize"));
	USizeBox* ManaSize = NamedWidget<USizeBox>(Tree, TEXT("ManaLayoutSize"));
	Group->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	EmblemSize->SetWidthOverride(96.0f * static_cast<float>(EmblemDimensions.X) / EmblemDimensions.Y);
	EmblemSize->SetHeightOverride(96.0f);
	EmblemScale->SetStretch(EStretch::ScaleToFit);
	// Source dimensions also work in NullRHI authoring, before platform texture data exists.
	EmblemImage->SetBrushFromTexture(Emblem, false);
	FSlateBrush EmblemBrush = EmblemImage->GetBrush();
	EmblemBrush.ImageSize = FVector2D(EmblemDimensions.X, EmblemDimensions.Y);
	EmblemImage->SetBrush(EmblemBrush);
	EmblemImage->SetColorAndOpacity(FLinearColor::White);
	EmblemScale->SetContent(EmblemImage);
	EmblemSize->SetContent(EmblemScale);
	Group->AddChildToHorizontalBox(EmblemSize)->SetVerticalAlignment(VAlign_Top);
	ColumnSize->SetWidthOverride(396.0f);
	ColumnSize->SetContent(Column);
	UHorizontalBoxSlot* ColumnSlot = Group->AddChildToHorizontalBox(ColumnSize);
	ColumnSlot->SetPadding(FMargin(14.0f, 16.0f, 0.0f, 0.0f));
	ColumnSlot->SetVerticalAlignment(VAlign_Top);
	SetTextStyle(LevelText, 16);
	SetTextStyle(HealthText, 14);
	LevelText->SetText(NSLOCTEXT("PHHUDEditor", "UnboundLevel", "Level: --"));
	HealthText->SetText(NSLOCTEXT("PHHUDEditor", "UnboundHealth", "-- / --"));
	Header->AddChildToHorizontalBox(LevelText)->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* ValueSlot = Header->AddChildToHorizontalBox(HealthText);
	ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ValueSlot->SetHorizontalAlignment(HAlign_Right);
	ValueSlot->SetVerticalAlignment(VAlign_Center);
	UVerticalBoxSlot* HeaderSlot = Header->GetParent() == Column
		? CastChecked<UVerticalBoxSlot>(Header->Slot) : Column->AddChildToVerticalBox(Header);
	HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
	HealthSize->ClearWidthOverride();
	HealthSize->SetHeightOverride(10.0f);
	HealthSize->SetContent(Health);
	UVerticalBoxSlot* HealthSlot = HealthSize->GetParent() == Column
		? CastChecked<UVerticalBoxSlot>(HealthSize->Slot) : Column->AddChildToVerticalBox(HealthSize);
	HealthSlot->SetHorizontalAlignment(HAlign_Fill);
	HealthSlot->SetPadding(FMargin(0.0f, 5.0f, 0.0f, 0.0f));
	ManaSize->SetWidthOverride(382.0f);
	ManaSize->SetHeightOverride(7.0f);
	ManaSize->SetContent(Mana);
	UVerticalBoxSlot* ManaSlot = ManaSize->GetParent() == Column
		? CastChecked<UVerticalBoxSlot>(ManaSize->Slot) : Column->AddChildToVerticalBox(ManaSize);
	ManaSlot->SetHorizontalAlignment(HAlign_Left);
	ManaSlot->SetPadding(FMargin(0.0f, 9.0f, 0.0f, 0.0f));
	if (UOverlay* Overlay = Cast<UOverlay>(OriginalRoot))
	{
		UOverlaySlot* Slot = Group->GetParent() == Overlay ? CastChecked<UOverlaySlot>(Group->Slot) : Overlay->AddChildToOverlay(Group);
		Slot->SetHorizontalAlignment(HAlign_Left);
		Slot->SetVerticalAlignment(VAlign_Top);
		Slot->SetPadding(FMargin(28.0f, 24.0f, 0.0f, 0.0f));
	}
	else if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(OriginalRoot))
	{
		UCanvasPanelSlot* Slot = Group->GetParent() == Canvas ? CastChecked<UCanvasPanelSlot>(Group->Slot) : Canvas->AddChildToCanvas(Group);
		Slot->SetAnchors(FAnchors(0.0f, 0.0f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetPosition(FVector2D(28.0f, 24.0f));
		Slot->SetAutoSize(true);
	}
	// UMG tracks all widget names, including non-variable layout panels, for rename/reference repair.
	Tree->ForEachWidget([Blueprint](UWidget* Widget)
	{
		if (!Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
		{
			Blueprint->OnVariableAdded(Widget->GetFName());
		}
	});
	if (Stamina->Slot != StaminaSlot || Stamina->GetParent() != StaminaParent ||
		JSONText(ResourceSnapshot(Stamina)) != StaminaBefore || JSONText(ContractInventory(Blueprint)) != ContractsBefore ||
		!HasOriginalVariableGuids())
	{
		return Fail(TEXT("A protected Stamina or Blueprint contract changed in memory. Nothing was saved; undo or discard this editor session."));
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave |
		EBlueprintCompileOptions::SkipGarbageCollection | EBlueprintCompileOptions::SkipFiBSearchMetaUpdate);
	if ((Blueprint->Status != BS_UpToDate && Blueprint->Status != BS_UpToDateWithWarnings) ||
		Tree != Blueprint->WidgetTree || OriginalRoot != Tree->RootWidget || Tree->FindWidget(TEXT("Health")) != Health ||
		Tree->FindWidget(TEXT("Stamina")) != Stamina || Stamina->Slot != StaminaSlot ||
		JSONText(ResourceSnapshot(Stamina)) != StaminaBefore || JSONText(ContractInventory(Blueprint)) != ContractsBefore ||
		!HasOriginalVariableGuids())
	{
		return Fail(TEXT("HUD compilation or protected-state verification failed. Nothing was saved; undo or discard this editor session."));
	}
	if (bImportedEmblem && !SaveNamedAsset(Emblem))
	{
		return Fail(TEXT("Saving the imported emblem failed; the player HUD was not saved."));
	}
	if (!SaveNamedAsset(Blueprint))
	{
		return Fail(TEXT("Saving the player HUD failed. Its original package remains in the verified backup directory."));
	}
	UE_LOG(LogPHHUDEditor, Display, TEXT("Saved top-left Health/Mana layout. Preserved Health identity, Stamina object/slot/serialized values, bindings, graphs, and animations. No shared resource Blueprint was saved."));
	return WriteJSON(EvidenceDirectory() / TEXT("AuthoredAfterApply.json"), Inspection(Blueprint));
}

bool UPHHUDEditorLibrary::RenderPlayerHUD(const FString& OutputPNGPath, int32 Width, int32 Height,
	float HealthPercent, float ManaPercent, int32 PreviewLevel, float PreviewHealthCurrent, float PreviewHealthMax,
	int32 PreviewFloor, int32 PreviewRemainingEnemies, int32 PreviewEnemyTarget, float PreviewBannerTime)
{
	using namespace PHHUDEditor;
	if (!FApp::CanEverRender() || !FSlateApplication::IsInitialized() || Width <= 0 || Height <= 0 || Width > 8192 || Height > 8192)
	{
		return Fail(TEXT("HUD PNG rendering requires initialized Slate, a real RHI (omit -NullRHI), and dimensions from 1 to 8192."));
	}
	UWidgetBlueprint* Blueprint = LoadHUD();
	if (!Blueprint || !Blueprint->GeneratedClass)
	{
		return false;
	}
	FPreviewScene PreviewScene(FPreviewScene::ConstructionValues().SetCreateDefaultLighting(false).SetCreatePhysicsScene(false).AllowAudioPlayback(false));
	UHunterMainHUDWidget* HUD = CreateWidget<UHunterMainHUDWidget>(PreviewScene.GetWorld(), Blueprint->GeneratedClass.Get());
	if (!HUD)
	{
		return Fail(TEXT("The actual generated player HUD could not be instantiated for rendering."));
	}
	// Use normal construction: designer visibility intentionally reveals collapsed widgets,
	// which would make an idle floor banner appear in the preview.
	TSharedRef<SWidget> SlateWidget = HUD->TakeWidget();
	HUD->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	auto SetPreviewPercent = [](UHunterHUDResourceWidget* Resource, float Percent)
	{
		if (!Resource || !Resource->WidgetTree)
		{
			return false;
		}
		Resource->bManageVisibility = false;
		Resource->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		for (const TCHAR* Name : {TEXT("Bar_Current"), TEXT("Bar_DamageLag"), TEXT("Bar_Reserved")})
		{
			if (UProgressBar* Bar = Resource->WidgetTree->FindWidget<UProgressBar>(Name))
			{
				Bar->SetPercent(FCString::Strcmp(Name, TEXT("Bar_Reserved")) == 0 ? 0.0f : FMath::Clamp(Percent, 0.0f, 1.0f));
			}
		}
		return true;
	};
	if (!SetPreviewPercent(HUD->GetHealthWidget(), HealthPercent) || !SetPreviewPercent(HUD->GetManaWidget(), ManaPercent))
	{
		return Fail(TEXT("Both existing resource widgets must be present before rendering this layout."));
	}
	if (UTextBlock* LevelText = HUD->WidgetTree->FindWidget<UTextBlock>(TEXT("PlayerLevelText")))
	{
		LevelText->SetText(FText::Format(NSLOCTEXT("PHHUDEditor", "PreviewLevel", "Level: {0}"), FText::AsNumber(PreviewLevel)));
	}
	if (UTextBlock* HealthText = HUD->WidgetTree->FindWidget<UTextBlock>(TEXT("HealthValueText")))
	{
		FNumberFormattingOptions Options;
		Options.SetMaximumFractionalDigits(0);
		HealthText->SetText(FText::Format(NSLOCTEXT("PHHUDEditor", "PreviewHealth", "{0} / {1}"),
			FText::AsNumber(PreviewHealthCurrent, &Options), FText::AsNumber(PreviewHealthMax, &Options)));
	}
	if (UImage* Emblem = HUD->WidgetTree->FindWidget<UImage>(TEXT("HunterEmblemImage")))
	{
		if (UTexture* Texture = Cast<UTexture>(Emblem->GetBrush().GetResourceObject()))
		{
			TArray<UTexture*> Textures = {Texture};
			FTextureCompilingManager::Get().FinishCompilation(Textures);
		}
	}
	if (UPHRunStatusWidget* Status = HUD->GetRunStatusWidget())
	{
		FRunSessionData Session;
		Session.Floor.FloorNumber = PreviewFloor;
		Session.Floor.Phase = EFloorPhase::InProgress;
		Session.Floor.ObjectiveTarget = FMath::Max(0, PreviewEnemyTarget);
		Session.Floor.ObjectiveProgress = FMath::Clamp(Session.Floor.ObjectiveTarget - PreviewRemainingEnemies, 0, Session.Floor.ObjectiveTarget);
		Status->ApplyRunSnapshot(PreviewFloor > 0 ? ERunState::Active : ERunState::Inactive, Session);
	}
	if (UPHFloorBannerWidget* Banner = HUD->GetFloorBannerWidget(); Banner && PreviewFloor > 0 && PreviewBannerTime >= 0.0f)
	{
		if (!Banner->GetEntryAnimation())
		{
			return Fail(TEXT("The floor banner's authored opening animation is not bound; no preview was rendered."));
		}
		Banner->ShowFloor(PreviewFloor);
		Banner->FlushAnimations();
		// Start at the requested sample time so UMG queues an actual evaluation there.
		// SetAnimationCurrentTime alone changes only the clock, not the rendered properties.
		Banner->PlayAnimation(Banner->GetEntryAnimation(), PreviewBannerTime);
		Banner->FlushAnimations();
	}
	// Slate's paint traversal can also tick. Freeze only these transient preview widgets after
	// evaluating the requested animation frame, so their live caches cannot overwrite sample values.
	TFunction<void(const TSharedRef<SWidget>&)> FreezePreviewTicks;
	FreezePreviewTicks = [&FreezePreviewTicks](const TSharedRef<SWidget>& Widget)
	{
		Widget->SetCanTick(false);
		FChildren* Children = Widget->GetChildren();
		for (int32 Index = 0; Index < Children->Num(); ++Index)
		{
			FreezePreviewTicks(Children->GetChildAt(Index));
		}
	};
	FreezePreviewTicks(SlateWidget);
	// ExportRenderTarget2DAsPNG converts a linear target to sRGB; avoid applying display gamma twice.
	UTextureRenderTarget2D* Target = FWidgetRenderer::CreateTargetFor(FVector2D(Width, Height), TF_Bilinear, false);
	if (!Target)
	{
		return Fail(TEXT("Slate could not create the HUD preview render target."));
	}
	FWidgetRenderer* Renderer = new FWidgetRenderer(false, true);
	if (!Renderer->GetSlateRenderer())
	{
		BeginCleanup(Renderer);
		return Fail(TEXT("Slate did not supply a renderer; no PNG was produced."));
	}
	Renderer->DrawWidget(Target, SlateWidget, FVector2D(Width, Height), 0.0f);
	FlushRenderingCommands();
	FBufferArchive ImageData;
	const bool bExported = FImageUtils::ExportRenderTarget2DAsPNG(Target, ImageData);
	BeginCleanup(Renderer);
	FlushRenderingCommands();
	if (!bExported || !IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutputPNGPath), true) ||
		!FFileHelper::SaveArrayToFile(ImageData, *OutputPNGPath))
	{
		return Fail(TEXT("The generated HUD could not be exported as a PNG."));
	}
	UE_LOG(LogPHHUDEditor, Display, TEXT("Rendered generated WBP_HunterHUD at %dx%d to %s; sample values were applied only to this transient preview."), Width, Height, *OutputPNGPath);
	return true;
}

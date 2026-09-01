// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "UI/PHHUDEditorLibrary.h"
#include "PHHUDEditorInternals.h"

#include "Animation/MovieScene2DTransformSection.h"
#include "Animation/MovieScene2DTransformTrack.h"
#include "Animation/WidgetAnimation.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "MovieScene.h"
#include "ScopedTransaction.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "UI/HUD/HunterHUDResourceWidget.h"
#include "UI/HUD/PHFloorBannerWidget.h"
#include "UI/HUD/PHRunStatusWidget.h"
#include "WidgetBlueprint.h"

namespace PHHUDRunLayout
{
	const FLinearColor Gold(0.70f, 0.55f, 0.28f, 1.0f);
	const FLinearColor White(0.90f, 0.94f, 1.0f, 1.0f);

	UTextBlock* Text(UWidgetTree* Tree, const TCHAR* Name, const FText& Label, int32 Size, FLinearColor Color)
	{
		UTextBlock* Widget = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		PHHUDEditor::SetTextStyle(Widget, Size);
		Widget->SetText(Label);
		Widget->SetColorAndOpacity(FSlateColor(Color));
		return Widget;
	}

	void Frame(UBorder* Border, const FLinearColor& Outline, const float Opacity)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FLinearColor(0.012f, 0.020f, 0.031f, Opacity));
		Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(2.0f, 2.0f, 2.0f, 2.0f), FSlateColor(Outline), 1.0f);
		Border->SetBrush(Brush);
		Border->SetBrushColor(FLinearColor::White);
		Border->SetPadding(FMargin(14.0f, 10.0f));
	}

	bool Compile(UWidgetBlueprint* Blueprint)
	{
		Blueprint->WidgetTree->ForEachWidget([Blueprint](UWidget* Widget)
		{
			if (!Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
			{
				Blueprint->OnVariableAdded(Widget->GetFName());
			}
		});
		for (UWidgetAnimation* Animation : Blueprint->Animations)
		{
			if (!Blueprint->WidgetVariableNameToGuidMap.Contains(Animation->GetFName()))
			{
				Blueprint->OnVariableAdded(Animation->GetFName());
			}
		}
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave |
			EBlueprintCompileOptions::SkipGarbageCollection | EBlueprintCompileOptions::SkipFiBSearchMetaUpdate);
		return Blueprint->GeneratedClass && (Blueprint->Status == BS_UpToDate || Blueprint->Status == BS_UpToDateWithWarnings);
	}

	UWidgetBlueprint* MakeBlueprint(const TCHAR* PackageName, UClass* Parent, bool& bCreated)
	{
		bCreated = false;
		const FString Name = FPackageName::GetLongPackageAssetName(PackageName);
		if (UObject* Existing = StaticLoadObject(UObject::StaticClass(), nullptr, *(FString(PackageName) + TEXT(".") + Name), nullptr, LOAD_NoWarn))
		{
			UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(Existing);
			return Blueprint && Blueprint->ParentClass == Parent && Blueprint->GeneratedClass ? Blueprint : nullptr;
		}
		UPackage* Package = CreatePackage(PackageName);
		UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(Parent, Package, *Name,
			BPTYPE_Normal, UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
		if (Blueprint)
		{
			bCreated = true;
			FAssetRegistryModule::AssetCreated(Blueprint);
		}
		return Blueprint;
	}

	void BuildStatus(UWidgetBlueprint* Blueprint)
	{
		UWidgetTree* Tree = Blueprint->WidgetTree;
		UVerticalBox* Root = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StatusLayout"));
		Tree->RootWidget = Root;
		UHorizontalBox* CounterRow = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CounterRow"));
		Root->AddChildToVerticalBox(CounterRow);
		UTextBlock* Floor = Text(Tree, TEXT("FloorLabelText"), NSLOCTEXT("PHHUDEditor", "FloorUnbound", "FLOOR --"), 12, Gold);
		CounterRow->AddChildToHorizontalBox(Floor)->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Enemies = Text(Tree, TEXT("RemainingEnemiesText"), NSLOCTEXT("PHHUDEditor", "EnemiesUnbound", "Enemies remaining: --"), 13, White);
		UHorizontalBoxSlot* CountSlot = CounterRow->AddChildToHorizontalBox(Enemies);
		CountSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CountSlot->SetHorizontalAlignment(HAlign_Right);
		CountSlot->SetVerticalAlignment(VAlign_Center);
		UBorder* Missions = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MissionPanel"));
		Frame(Missions, FLinearColor(0.22f, 0.29f, 0.34f, 0.65f), 0.68f);
		Root->AddChildToVerticalBox(Missions)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
		UVerticalBox* Content = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MissionLayout"));
		Missions->SetContent(Content);
		Content->AddChildToVerticalBox(Text(Tree, TEXT("MissionsTitle"), NSLOCTEXT("PHHUDEditor", "Missions", "MISSIONS"), 11, Gold));
		UTextBlock* Objective = Text(Tree, TEXT("FloorMissionText"), NSLOCTEXT("PHHUDEditor", "NoObjective", "No active floor objective"), 15, White);
		Objective->SetAutoWrapText(true);
		Content->AddChildToVerticalBox(Objective)->SetPadding(FMargin(0.0f, 7.0f, 0.0f, 0.0f));
		UVerticalBox* Entries = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MissionEntries"));
		Entries->bIsVariable = true;
		Content->AddChildToVerticalBox(Entries)->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
	}

	void BuildBanner(UWidgetBlueprint* Blueprint)
	{
		UWidgetTree* Tree = Blueprint->WidgetTree;
		UOverlay* Root = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("BannerOverlay"));
		Tree->RootWidget = Root;
		USizeBox* Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BannerSize"));
		Size->SetWidthOverride(520.0f);
		Size->SetHeightOverride(130.0f);
		UOverlaySlot* Slot = Root->AddChildToOverlay(Size);
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetVerticalAlignment(VAlign_Center);
		UBorder* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BannerPanel"));
		Panel->bIsVariable = true;
		Panel->SetVisibility(ESlateVisibility::Collapsed);
		Panel->SetHorizontalAlignment(HAlign_Center);
		Panel->SetVerticalAlignment(VAlign_Center);
		Panel->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Frame(Panel, Gold.CopyWithNewOpacity(0.8f), 0.75f);
		Size->SetContent(Panel);
		UVerticalBox* Content = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BannerContent"));
		Panel->SetContent(Content);
		Content->AddChildToVerticalBox(Text(Tree, TEXT("EnteringText"), NSLOCTEXT("PHHUDEditor", "Entering", "ENTERING"), 12, Gold))->SetHorizontalAlignment(HAlign_Center);
		UTextBlock* Number = Text(Tree, TEXT("FloorNumberText"), NSLOCTEXT("PHHUDEditor", "BannerUnbound", "FLOOR --"), 38, White);
		FSlateFontInfo NumberFont = Number->GetFont();
		NumberFont.TypefaceFontName = TEXT("Bold");
		Number->SetFont(NumberFont);
		UVerticalBoxSlot* NumberSlot = Content->AddChildToVerticalBox(Number);
		NumberSlot->SetHorizontalAlignment(HAlign_Center);
		NumberSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));

		// A normal UMG animation: editable in the Animation timeline, with no gameplay Tick.
		UWidgetAnimation* Animation = NewObject<UWidgetAnimation>(Blueprint, TEXT("FloorOpen"), RF_Transactional);
		Animation->SetDisplayLabel(TEXT("FloorOpen"));
		// Runtime UMG binds by the MovieScene name, not the animation object's name.
		UMovieScene* Scene = NewObject<UMovieScene>(Animation, TEXT("FloorOpen"), RF_Transactional);
		Animation->MovieScene = Scene;
		Scene->SetTickResolutionDirectly(FFrameRate(60, 1));
		Scene->SetDisplayRate(FFrameRate(60, 1));
		Scene->SetPlaybackRange(0, 192);
		const FGuid Binding = Scene->AddPossessable(TEXT("BannerPanel"), UBorder::StaticClass());
		FWidgetAnimationBinding& WidgetBinding = Animation->AnimationBindings.AddDefaulted_GetRef();
		WidgetBinding.WidgetName = Panel->GetFName();
		WidgetBinding.AnimationGuid = Binding;
		UMovieSceneFloatTrack* Fade = Scene->AddTrack<UMovieSceneFloatTrack>(Binding);
		Fade->SetPropertyNameAndPath(TEXT("RenderOpacity"), TEXT("RenderOpacity"));
		UMovieSceneFloatSection* FadeSection = CastChecked<UMovieSceneFloatSection>(Fade->CreateNewSection());
		FadeSection->SetRange(TRange<FFrameNumber>(0, 192));
		Fade->AddSection(*FadeSection);
		FadeSection->GetChannel().AddCubicKey(0, 0.0f);
		FadeSection->GetChannel().AddCubicKey(14, 1.0f);
		FadeSection->GetChannel().AddCubicKey(158, 1.0f);
		FadeSection->GetChannel().AddCubicKey(191, 0.0f);
		UMovieScene2DTransformTrack* Open = Scene->AddTrack<UMovieScene2DTransformTrack>(Binding);
		Open->SetPropertyNameAndPath(TEXT("RenderTransform"), TEXT("RenderTransform"));
		UMovieScene2DTransformSection* OpenSection = CastChecked<UMovieScene2DTransformSection>(Open->CreateNewSection());
		OpenSection->SetRange(TRange<FFrameNumber>(0, 192));
		OpenSection->SetMask(FMovieScene2DTransformMask(EMovieScene2DTransformChannel::Scale));
		OpenSection->Scale[0].AddCubicKey(0, 0.30f);
		OpenSection->Scale[0].AddCubicKey(20, 1.0f);
		OpenSection->Scale[0].AddCubicKey(158, 1.0f);
		OpenSection->Scale[0].AddCubicKey(191, 0.85f);
		OpenSection->Scale[1].AddCubicKey(0, 0.70f);
		OpenSection->Scale[1].AddCubicKey(20, 1.0f);
		OpenSection->Scale[1].AddCubicKey(158, 1.0f);
		OpenSection->Scale[1].AddCubicKey(191, 0.85f);
		Open->AddSection(*OpenSection);
		Blueprint->Animations.Add(Animation);
	}
}

bool UPHHUDEditorLibrary::ApplyFloorAndMissionLayout()
{
	using namespace PHHUDEditor;
	using namespace PHHUDRunLayout;
	UWidgetBlueprint* HUD = LoadHUD();
	UVerticalBox* Column = HUD ? Cast<UVerticalBox>(HUD->WidgetTree->FindWidget(TEXT("HealthManaColumn"))) : nullptr;
	if (!HUD || !Column || (!Cast<UOverlay>(HUD->WidgetTree->RootWidget) && !Cast<UCanvasPanel>(HUD->WidgetTree->RootWidget)))
	{
		return Fail(TEXT("Apply the top-left Health/Mana layout first. The existing HUD root must be Overlay or Canvas."));
	}
	if (!BackupHUD(HUD))
	{
		return false;
	}
	TMap<UHunterHUDResourceWidget*, FString> ProtectedResources;
	for (const TCHAR* Name : {TEXT("Health"), TEXT("Mana"), TEXT("Stamina")})
	{
		UHunterHUDResourceWidget* Resource = Cast<UHunterHUDResourceWidget>(HUD->WidgetTree->FindWidget(Name));
		if (!Resource)
		{
			return Fail(TEXT("All three existing resources must be present; the run layout never creates or styles resources."));
		}
		ProtectedResources.Add(Resource, JSONText(ResourceSnapshot(Resource)));
	}
	const FString ContractsBefore = JSONText(ContractInventory(HUD));
	UWidget* OriginalRoot = HUD->WidgetTree->RootWidget;
	bool bStatusCreated = false;
	bool bBannerCreated = false;
	UWidgetBlueprint* Status = MakeBlueprint(TEXT("/Game/ProjectHunter/UI/HUD/WBP_RunStatus"), UPHRunStatusWidget::StaticClass(), bStatusCreated);
	UWidgetBlueprint* Banner = MakeBlueprint(TEXT("/Game/ProjectHunter/UI/HUD/WBP_FloorBanner"), UPHFloorBannerWidget::StaticClass(), bBannerCreated);
	if (!Status || !Banner)
	{
		return Fail(TEXT("The dedicated run-status/banner asset paths are occupied by incompatible assets. Nothing was saved."));
	}
	if (bStatusCreated) { BuildStatus(Status); }
	if (bBannerCreated) { BuildBanner(Banner); }
	bool bRepairedBannerBinding = false;
	if (!bBannerCreated)
	{
		for (UWidgetAnimation* Animation : Banner->Animations)
		{
			if (Animation->GetFName() == TEXT("FloorOpen") && Animation->MovieScene &&
				Animation->MovieScene->GetFName() == TEXT("FloorOpenTimeline"))
			{
				// Repair only the exact scene name written by the first authoring pass.
				// Keep the designer's curves, duration, tracks, and widget layout intact.
				if (!BackupHUD(Banner)) { return false; }
				Banner->Modify();
				Animation->MovieScene->Modify();
				if (!Animation->MovieScene->Rename(TEXT("FloorOpen"), Animation, REN_DontCreateRedirectors))
				{
					return Fail(TEXT("Could not repair the authored animation binding. Nothing was saved."));
				}
				bRepairedBannerBinding = true;
			}
		}
	}
	if ((bStatusCreated && !Compile(Status)) || ((bBannerCreated || bRepairedBannerBinding) && !Compile(Banner)))
	{
		return Fail(TEXT("The authored run widgets failed compilation. Nothing was saved."));
	}
	const FScopedTransaction Transaction(NSLOCTEXT("PHHUDEditor", "ApplyRunLayout", "Add Project Hunter floor and mission UI"));
	HUD->Modify();
	HUD->WidgetTree->Modify();
	Column->Modify();
	OriginalRoot->Modify();
	UWidget* StatusInstance = HUD->WidgetTree->FindWidget(TEXT("RunStatusWidget"));
	UWidget* BannerInstance = HUD->WidgetTree->FindWidget(TEXT("FloorBannerWidget"));
	if ((StatusInstance && StatusInstance->GetClass() != Status->GeneratedClass) ||
		(BannerInstance && BannerInstance->GetClass() != Banner->GeneratedClass))
	{
		return Fail(TEXT("An incompatible widget already uses a required binding name. Nothing was saved."));
	}
	if (!StatusInstance)
	{
		StatusInstance = HUD->WidgetTree->ConstructWidget<UWidget>(Status->GeneratedClass.Get(), TEXT("RunStatusWidget"));
		StatusInstance->bIsVariable = true;
		UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(StatusInstance);
		Slot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
		Slot->SetHorizontalAlignment(HAlign_Fill);
	}
	if (!BannerInstance)
	{
		BannerInstance = HUD->WidgetTree->ConstructWidget<UWidget>(Banner->GeneratedClass.Get(), TEXT("FloorBannerWidget"));
		BannerInstance->bIsVariable = true;
		if (UOverlay* Overlay = Cast<UOverlay>(OriginalRoot))
		{
			UOverlaySlot* Slot = Overlay->AddChildToOverlay(BannerInstance);
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
		else
		{
			UCanvasPanelSlot* Slot = CastChecked<UCanvasPanel>(OriginalRoot)->AddChildToCanvas(BannerInstance);
			Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			Slot->SetOffsets(FMargin(0.0f));
		}
	}
	if (!Compile(HUD) || HUD->WidgetTree->RootWidget != OriginalRoot || JSONText(ContractInventory(HUD)) != ContractsBefore)
	{
		return Fail(TEXT("Run HUD compile or Blueprint contract verification failed; nothing was saved."));
	}
	for (const TPair<UHunterHUDResourceWidget*, FString>& Resource : ProtectedResources)
	{
		if (JSONText(ResourceSnapshot(Resource.Key)) != Resource.Value)
		{
			return Fail(TEXT("A protected resource changed while adding run UI. Nothing was saved."));
		}
	}
	const bool bSaveStatus = bStatusCreated || !FPackageName::DoesPackageExist(Status->GetOutermost()->GetName());
	const bool bSaveBanner = bBannerCreated || bRepairedBannerBinding || !FPackageName::DoesPackageExist(Banner->GetOutermost()->GetName());
	if ((bSaveStatus && !SaveNamedAsset(Status)) || (bSaveBanner && !SaveNamedAsset(Banner)) || !SaveNamedAsset(HUD))
	{
		return Fail(TEXT("An authored run UI asset could not be saved. The original HUD remains backed up."));
	}
	return InspectPlayerHUD(EvidenceDirectory() / TEXT("AuthoredAfterRunLayout.json"));
}

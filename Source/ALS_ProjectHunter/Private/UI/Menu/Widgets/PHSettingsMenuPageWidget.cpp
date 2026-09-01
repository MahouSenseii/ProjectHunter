// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "UI/Menu/Widgets/PHSettingsMenuPageWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Settings/PHGameSettings.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/Menu/Helpers/MenuRowBuilder.h"
#include "UserSettings/EnhancedInputUserSettings.h"

using namespace PHMenuRowBuilder;

namespace
{
	/** Frame caps offered, in order. 0 means uncapped. */
	const TArray<float> FrameRateOptions = {0.0f, 30.0f, 60.0f, 90.0f, 120.0f, 144.0f, 240.0f};

	int32 WrapIndex(const int32 Index, const int32 Count)
	{
		return Count > 0 ? ((Index % Count) + Count) % Count : 0;
	}

	const TArray<TPair<EPHSettingsSection, const TCHAR*>> SectionOrder = {
		{EPHSettingsSection::SS_Gameplay, TEXT("Gameplay")},
		{EPHSettingsSection::SS_Audio,    TEXT("Audio")},
		{EPHSettingsSection::SS_KeyBinds, TEXT("Key Binds")},
		{EPHSettingsSection::SS_Display,  TEXT("Display")},
	};
}

UGameUserSettings* UPHSettingsMenuPageWidget::Settings()
{
	return GEngine ? GEngine->GetGameUserSettings() : nullptr;
}

UPHGameSettings* UPHSettingsMenuPageWidget::PHSettings()
{
	return UPHGameSettings::Get();
}

FText UPHSettingsMenuPageWidget::QualityLabel(const int32 Level)
{
	switch (Level)
	{
	case 0:  return FText::FromString(TEXT("Low"));
	case 1:  return FText::FromString(TEXT("Medium"));
	case 2:  return FText::FromString(TEXT("High"));
	case 3:  return FText::FromString(TEXT("Epic"));
	case 4:  return FText::FromString(TEXT("Cinematic"));
	default: return FText::FromString(TEXT("Custom"));
	}
}

TSharedRef<SWidget> UPHSettingsMenuPageWidget::RebuildWidget()
{
	CacheResolutions();
	BuildWidgets();
	return Super::RebuildWidget();
}

void UPHSettingsMenuPageWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshSettings();
}

void UPHSettingsMenuPageWidget::CacheResolutions()
{
	AvailableResolutions.Reset();
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(AvailableResolutions);

	// A headless or offscreen process reports none. Offer the current mode
	// rather than an empty control the player cannot move.
	if (AvailableResolutions.IsEmpty())
	{
		if (const UGameUserSettings* GameSettings = Settings())
		{
			AvailableResolutions.Add(GameSettings->GetScreenResolution());
		}
	}

	AvailableResolutions.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return A.X * A.Y < B.X * B.Y;
	});
}

void UPHSettingsMenuPageWidget::BuildWidgets()
{
	if (bWidgetsBuilt || !WidgetTree)
	{
		return;
	}

	UPanelWidget* Host = EnsureRowHost(*WidgetTree, SettingsContainer);
	if (!Host)
	{
		return;
	}

	BuiltColumn = AddPaddedColumn(*WidgetTree, *Host);
	BuildSectionTabs(*BuiltColumn);

	for (const TPair<EPHSettingsSection, const TCHAR*>& Section : SectionOrder)
	{
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		SectionBoxes.Add(Section.Key, Box);
		AddRow(*BuiltColumn, *Box, 4.0f, 4.0f);
	}

	BuildGameplaySection();
	BuildAudioSection();
	BuildKeyBindsSection();
	BuildDisplaySection();

	UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	UButton* Apply = MakeButton(*WidgetTree, FText::FromString(TEXT("APPLY")));
	Apply->OnClicked.AddUniqueDynamic(this, &UPHSettingsMenuPageWidget::HandleApplyClicked);
	if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(Actions->AddChild(Apply)))
	{
		BoxSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	UButton* Revert = MakeButton(*WidgetTree, FText::FromString(TEXT("REVERT")));
	Revert->OnClicked.AddUniqueDynamic(this, &UPHSettingsMenuPageWidget::HandleRevertClicked);
	if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(Actions->AddChild(Revert)))
	{
		BoxSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	UButton* Defaults = MakeButton(*WidgetTree, FText::FromString(TEXT("DEFAULTS")));
	Defaults->OnClicked.AddUniqueDynamic(this, &UPHSettingsMenuPageWidget::HandleDefaultsClicked);
	Actions->AddChild(Defaults);

	AddRow(*BuiltColumn, *Actions, 20.0f, 4.0f);

	StatusText = MakeText(*WidgetTree, FText::GetEmpty(), 12, Palette::Accent, TEXT("Regular"));
	AddRow(*BuiltColumn, *StatusText, 0.0f, 8.0f);

	bWidgetsBuilt = true;
	ShowSection(DefaultSection);
}

void UPHSettingsMenuPageWidget::BuildSectionTabs(UVerticalBox& Column)
{
	UHorizontalBox* TabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	for (const TPair<EPHSettingsSection, const TCHAR*>& Section : SectionOrder)
	{
		UButton* Tab = MakeButton(*WidgetTree, FText::FromString(Section.Value), 13);
		Tab->OnClicked.AddUniqueDynamic(this, &UPHSettingsMenuPageWidget::HandleSectionTabClicked);
		SectionTabs.Add(Section.Key, Tab);

		if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(TabRow->AddChild(Tab)))
		{
			BoxSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
		}
	}

	AddRow(Column, *TabRow, 0.0f, 12.0f);
}

void UPHSettingsMenuPageWidget::BuildGameplaySection()
{
	UVerticalBox* Box = SectionBoxes.FindRef(EPHSettingsSection::SS_Gameplay);
	if (!Box) { return; }

	UTextBlock* Caret = nullptr;
	AddRow(*Box, *MakeSectionHeader(*WidgetTree, FText::FromString(TEXT("INTERFACE")), Caret), 0.0f, 4.0f);

	// Only toggles with a consumer. Mouse sensitivity and invert-look are not
	// here because the look input lives in the ALS plugin and nothing in this
	// project reads a sensitivity value yet - a slider for it would do nothing.
	AddSettingRow(*Box, EPHSettingKind::SK_ShowDamageNumbers,   FText::FromString(TEXT("Show Damage Numbers")));
	AddSettingRow(*Box, EPHSettingKind::SK_ShowFloorBanner,     FText::FromString(TEXT("Show Floor Announcement")));
	AddSettingRow(*Box, EPHSettingKind::SK_ShowEnemyHealthBars, FText::FromString(TEXT("Show Enemy Health Bars")));
}

void UPHSettingsMenuPageWidget::BuildAudioSection()
{
	UVerticalBox* Box = SectionBoxes.FindRef(EPHSettingsSection::SS_Audio);
	if (!Box) { return; }

	UTextBlock* Caret = nullptr;
	AddRow(*Box, *MakeSectionHeader(*WidgetTree, FText::FromString(TEXT("VOLUME")), Caret), 0.0f, 4.0f);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	UTextBlock* Label = MakeText(*WidgetTree, FText::FromString(TEXT("Master Volume")), 14,
		Palette::Dim, TEXT("Regular"));
	if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(Row->AddChild(Label)))
	{
		BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BoxSlot->SetVerticalAlignment(VAlign_Center);
	}

	MasterVolumeSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
	MasterVolumeSlider->SetMinValue(0.0f);
	MasterVolumeSlider->SetMaxValue(1.0f);
	MasterVolumeSlider->SetStepSize(0.05f);
	MasterVolumeSlider->OnValueChanged.AddUniqueDynamic(
		this, &UPHSettingsMenuPageWidget::HandleMasterVolumeChanged);
	if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(Row->AddChild(MasterVolumeSlider)))
	{
		BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BoxSlot->SetVerticalAlignment(VAlign_Center);
		BoxSlot->SetPadding(FMargin(16.0f, 0.0f, 12.0f, 0.0f));
	}

	MasterVolumeValue = MakeText(*WidgetTree, FText::GetEmpty(), 14, Palette::Text);
	MasterVolumeValue->SetJustification(ETextJustify::Right);
	if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(Row->AddChild(MasterVolumeValue)))
	{
		BoxSlot->SetVerticalAlignment(VAlign_Center);
	}

	AddRow(*Box, *Row, 3.0f, 3.0f);

	// Said plainly rather than filling the tab with sliders that do nothing.
	UTextBlock* Note = MakeText(*WidgetTree, FText::FromString(
		TEXT("Music and effect volumes need Sound Classes, which the project does not have yet.")),
		11, Palette::Dim, TEXT("Regular"));
	Note->SetAutoWrapText(true);
	AddRow(*Box, *Note, 12.0f, 4.0f);
}

void UPHSettingsMenuPageWidget::BuildKeyBindsSection()
{
	UVerticalBox* Box = SectionBoxes.FindRef(EPHSettingsSection::SS_KeyBinds);
	if (!Box) { return; }

	UTextBlock* Caret = nullptr;
	AddRow(*Box, *MakeSectionHeader(*WidgetTree, FText::FromString(TEXT("KEY BINDINGS")), Caret), 0.0f, 4.0f);

	// Enhanced Input owns the profile; this lists only what it reports as
	// mappable. Nothing here invents a binding.
	const UEnhancedInputUserSettings* InputSettings = nullptr;
	if (const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (const UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSettings = Subsystem->GetUserSettings();
		}
	}

	const UEnhancedPlayerMappableKeyProfile* Profile =
		InputSettings ? InputSettings->GetActiveKeyProfile() : nullptr;

	if (!Profile)
	{
		UTextBlock* Unavailable = MakeText(*WidgetTree, FText::FromString(
			TEXT("Key bindings are unavailable here: no local player, or no mapping is marked "
			     "player-mappable in the input mapping context.")),
			12, Palette::Dim, TEXT("Regular"));
		Unavailable->SetAutoWrapText(true);
		AddRow(*Box, *Unavailable, 4.0f, 4.0f);
		return;
	}

	for (const TPair<FName, FKeyMappingRow>& Pair : Profile->GetPlayerMappingRows())
	{
		FKeyBindRow BindRow;
		BindRow.MappingName = Pair.Key;

		FText Label = FText::FromName(Pair.Key);
		for (const FPlayerKeyMapping& Mapping : Pair.Value.Mappings)
		{
			if (!Mapping.GetDisplayName().IsEmpty())
			{
				Label = Mapping.GetDisplayName();
				break;
			}
		}

		UTextBlock* KeyText = nullptr;
		UHorizontalBox* Row = MakeStatRow(*WidgetTree, Label, KeyText);
		BindRow.KeyText = KeyText;

		UButton* Rebind = MakeButton(*WidgetTree, FText::FromString(TEXT("REBIND")), 12);
		Rebind->OnClicked.AddUniqueDynamic(this, &UPHSettingsMenuPageWidget::HandleRebindClicked);
		BindRow.RebindButton = Rebind;
		if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(Row->AddChild(Rebind)))
		{
			BoxSlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
			BoxSlot->SetVerticalAlignment(VAlign_Center);
		}

		AddRow(*Box, *Row, 3.0f, 3.0f);
		KeyBindRows.Add(BindRow);
	}
}

void UPHSettingsMenuPageWidget::BuildDisplaySection()
{
	UVerticalBox* Box = SectionBoxes.FindRef(EPHSettingsSection::SS_Display);
	if (!Box) { return; }

	UTextBlock* Caret = nullptr;
	AddRow(*Box, *MakeSectionHeader(*WidgetTree, FText::FromString(TEXT("DISPLAY")), Caret), 0.0f, 4.0f);

	AddSettingRow(*Box, EPHSettingKind::SK_WindowMode,     FText::FromString(TEXT("Window Mode")));
	AddSettingRow(*Box, EPHSettingKind::SK_Resolution,     FText::FromString(TEXT("Resolution")));
	AddSettingRow(*Box, EPHSettingKind::SK_VSync,          FText::FromString(TEXT("VSync")));
	AddSettingRow(*Box, EPHSettingKind::SK_FrameRateLimit, FText::FromString(TEXT("Frame Rate Limit")));

	AddRow(*Box, *MakeSectionHeader(*WidgetTree, FText::FromString(TEXT("GRAPHICS")), Caret), 16.0f, 4.0f);

	AddSettingRow(*Box, EPHSettingKind::SK_ViewDistance,       FText::FromString(TEXT("View Distance")));
	AddSettingRow(*Box, EPHSettingKind::SK_AntiAliasing,       FText::FromString(TEXT("Anti-Aliasing")));
	AddSettingRow(*Box, EPHSettingKind::SK_PostProcessing,     FText::FromString(TEXT("Post Processing")));
	AddSettingRow(*Box, EPHSettingKind::SK_Shadows,            FText::FromString(TEXT("Shadows")));
	AddSettingRow(*Box, EPHSettingKind::SK_GlobalIllumination, FText::FromString(TEXT("Global Illumination")));
	AddSettingRow(*Box, EPHSettingKind::SK_Reflections,        FText::FromString(TEXT("Reflections")));
	AddSettingRow(*Box, EPHSettingKind::SK_Textures,           FText::FromString(TEXT("Textures")));
	AddSettingRow(*Box, EPHSettingKind::SK_Effects,            FText::FromString(TEXT("Effects")));
	AddSettingRow(*Box, EPHSettingKind::SK_Foliage,            FText::FromString(TEXT("Foliage")));
	AddSettingRow(*Box, EPHSettingKind::SK_Shading,            FText::FromString(TEXT("Shading")));
}

void UPHSettingsMenuPageWidget::ShowSection(const EPHSettingsSection Section)
{
	ActiveSection = Section;
	CancelKeyListen();

	for (const TPair<EPHSettingsSection, TObjectPtr<UVerticalBox>>& Pair : SectionBoxes)
	{
		if (UVerticalBox* Box = Pair.Value)
		{
			Box->SetVisibility(Pair.Key == Section
				? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}

	// The active tab reads as selected by dimming the others.
	for (const TPair<EPHSettingsSection, TObjectPtr<UButton>>& Pair : SectionTabs)
	{
		if (UButton* Tab = Pair.Value)
		{
			Tab->SetRenderOpacity(Pair.Key == Section ? 1.0f : 0.55f);
		}
	}

	RefreshSettings();
}

void UPHSettingsMenuPageWidget::HandleSectionTabClicked()
{
	for (const TPair<EPHSettingsSection, TObjectPtr<UButton>>& Pair : SectionTabs)
	{
		const UButton* Tab = Pair.Value;
		if (Tab && Tab->IsHovered())
		{
			ShowSection(Pair.Key);
			return;
		}
	}
}

void UPHSettingsMenuPageWidget::AddSettingRow(UVerticalBox& Column,
	const EPHSettingKind Kind, const FText& Label)
{
	FSettingRow Row;
	Row.Kind = Kind;

	UTextBlock* ValueText = nullptr;
	UHorizontalBox* RowBox = MakeStatRow(*WidgetTree, Label, ValueText);
	Row.ValueText = ValueText;

	// A cycler rather than a dropdown: it needs no authored list widget and
	// works with a controller as well as a mouse.
	UButton* Previous = MakeButton(*WidgetTree, FText::FromString(TEXT("<")), 13);
	Previous->OnClicked.AddUniqueDynamic(this, &UPHSettingsMenuPageWidget::HandleStepClicked);
	if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(RowBox->AddChild(Previous)))
	{
		BoxSlot->SetPadding(FMargin(10.0f, 0.0f, 4.0f, 0.0f));
		BoxSlot->SetVerticalAlignment(VAlign_Center);
	}

	UButton* Next = MakeButton(*WidgetTree, FText::FromString(TEXT(">")), 13);
	Next->OnClicked.AddUniqueDynamic(this, &UPHSettingsMenuPageWidget::HandleStepClicked);
	if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(RowBox->AddChild(Next)))
	{
		BoxSlot->SetVerticalAlignment(VAlign_Center);
	}

	Row.Previous = Previous;
	Row.Next = Next;

	AddRow(Column, *RowBox, 3.0f, 3.0f);
	Rows.Add(Row);
}

void UPHSettingsMenuPageWidget::HandleStepClicked()
{
	// UButton::OnClicked carries no sender; the hovered control is the one
	// that was pressed.
	for (const FSettingRow& Row : Rows)
	{
		if (const UButton* Previous = Row.Previous.Get(); Previous && Previous->IsHovered())
		{
			StepSetting(Row.Kind, -1);
			return;
		}
		if (const UButton* Next = Row.Next.Get(); Next && Next->IsHovered())
		{
			StepSetting(Row.Kind, 1);
			return;
		}
	}
}

void UPHSettingsMenuPageWidget::StepSetting(const EPHSettingKind Kind, const int32 Delta)
{
	// Gameplay toggles live on the project settings object. Direction does not
	// matter for a boolean, so either arrow flips it.
	if (UPHGameSettings* Project = PHSettings())
	{
		switch (Kind)
		{
		case EPHSettingKind::SK_ShowDamageNumbers:
			Project->SetShowDamageNumbers(!Project->GetShowDamageNumbers());
			bDirty = true;
			RefreshSettings();
			return;
		case EPHSettingKind::SK_ShowFloorBanner:
			Project->SetShowFloorBanner(!Project->GetShowFloorBanner());
			bDirty = true;
			RefreshSettings();
			return;
		case EPHSettingKind::SK_ShowEnemyHealthBars:
			Project->SetShowEnemyHealthBars(!Project->GetShowEnemyHealthBars());
			bDirty = true;
			RefreshSettings();
			return;
		default:
			break;
		}
	}

	UGameUserSettings* GameSettings = Settings();
	if (!GameSettings)
	{
		return;
	}

	switch (Kind)
	{
	case EPHSettingKind::SK_WindowMode:
	{
		const int32 Current = static_cast<int32>(GameSettings->GetFullscreenMode());
		GameSettings->SetFullscreenMode(static_cast<EWindowMode::Type>(
			WrapIndex(Current + Delta, EWindowMode::NumWindowModes)));
		break;
	}
	case EPHSettingKind::SK_Resolution:
	{
		if (AvailableResolutions.IsEmpty())
		{
			break;
		}
		const FIntPoint Current = GameSettings->GetScreenResolution();
		int32 Index = AvailableResolutions.IndexOfByKey(Current);
		Index = (Index == INDEX_NONE) ? 0 : Index + Delta;
		GameSettings->SetScreenResolution(
			AvailableResolutions[WrapIndex(Index, AvailableResolutions.Num())]);
		break;
	}
	case EPHSettingKind::SK_VSync:
		GameSettings->SetVSyncEnabled(!GameSettings->IsVSyncEnabled());
		break;
	case EPHSettingKind::SK_FrameRateLimit:
	{
		const float Current = GameSettings->GetFrameRateLimit();
		int32 Index = FrameRateOptions.IndexOfByKey(Current);
		Index = (Index == INDEX_NONE) ? 0 : Index + Delta;
		GameSettings->SetFrameRateLimit(FrameRateOptions[WrapIndex(Index, FrameRateOptions.Num())]);
		break;
	}
	default:
	{
		// The scalability groups all take 0-4.
		const auto Step = [Delta](const int32 Value) { return WrapIndex(Value + Delta, 5); };
		switch (Kind)
		{
		case EPHSettingKind::SK_ViewDistance:       GameSettings->SetViewDistanceQuality(Step(GameSettings->GetViewDistanceQuality())); break;
		case EPHSettingKind::SK_AntiAliasing:       GameSettings->SetAntiAliasingQuality(Step(GameSettings->GetAntiAliasingQuality())); break;
		case EPHSettingKind::SK_PostProcessing:     GameSettings->SetPostProcessingQuality(Step(GameSettings->GetPostProcessingQuality())); break;
		case EPHSettingKind::SK_Shadows:            GameSettings->SetShadowQuality(Step(GameSettings->GetShadowQuality())); break;
		case EPHSettingKind::SK_GlobalIllumination: GameSettings->SetGlobalIlluminationQuality(Step(GameSettings->GetGlobalIlluminationQuality())); break;
		case EPHSettingKind::SK_Reflections:        GameSettings->SetReflectionQuality(Step(GameSettings->GetReflectionQuality())); break;
		case EPHSettingKind::SK_Textures:           GameSettings->SetTextureQuality(Step(GameSettings->GetTextureQuality())); break;
		case EPHSettingKind::SK_Effects:            GameSettings->SetVisualEffectQuality(Step(GameSettings->GetVisualEffectQuality())); break;
		case EPHSettingKind::SK_Foliage:            GameSettings->SetFoliageQuality(Step(GameSettings->GetFoliageQuality())); break;
		case EPHSettingKind::SK_Shading:            GameSettings->SetShadingQuality(Step(GameSettings->GetShadingQuality())); break;
		default: break;
		}
		break;
	}
	}

	bDirty = true;
	RefreshSettings();
}

FText UPHSettingsMenuPageWidget::DescribeSetting(const EPHSettingKind Kind) const
{
	if (const UPHGameSettings* Project = PHSettings())
	{
		const auto OnOff = [](const bool bValue)
		{
			return FText::FromString(bValue ? TEXT("On") : TEXT("Off"));
		};
		switch (Kind)
		{
		case EPHSettingKind::SK_ShowDamageNumbers:   return OnOff(Project->GetShowDamageNumbers());
		case EPHSettingKind::SK_ShowFloorBanner:     return OnOff(Project->GetShowFloorBanner());
		case EPHSettingKind::SK_ShowEnemyHealthBars: return OnOff(Project->GetShowEnemyHealthBars());
		default: break;
		}
	}

	const UGameUserSettings* GameSettings = Settings();
	if (!GameSettings)
	{
		return FText::FromString(TEXT("--"));
	}

	switch (Kind)
	{
	case EPHSettingKind::SK_WindowMode:
		switch (GameSettings->GetFullscreenMode())
		{
		case EWindowMode::Fullscreen:          return FText::FromString(TEXT("Fullscreen"));
		case EWindowMode::WindowedFullscreen:  return FText::FromString(TEXT("Borderless"));
		default:                               return FText::FromString(TEXT("Windowed"));
		}
	case EPHSettingKind::SK_Resolution:
	{
		const FIntPoint Resolution = GameSettings->GetScreenResolution();
		return FText::FromString(FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y));
	}
	case EPHSettingKind::SK_VSync:
		return FText::FromString(GameSettings->IsVSyncEnabled() ? TEXT("On") : TEXT("Off"));
	case EPHSettingKind::SK_FrameRateLimit:
	{
		const float Limit = GameSettings->GetFrameRateLimit();
		return Limit <= 0.0f
			? FText::FromString(TEXT("Uncapped"))
			: FText::FromString(FString::Printf(TEXT("%.0f FPS"), Limit));
	}
	case EPHSettingKind::SK_ViewDistance:       return QualityLabel(GameSettings->GetViewDistanceQuality());
	case EPHSettingKind::SK_AntiAliasing:       return QualityLabel(GameSettings->GetAntiAliasingQuality());
	case EPHSettingKind::SK_PostProcessing:     return QualityLabel(GameSettings->GetPostProcessingQuality());
	case EPHSettingKind::SK_Shadows:            return QualityLabel(GameSettings->GetShadowQuality());
	case EPHSettingKind::SK_GlobalIllumination: return QualityLabel(GameSettings->GetGlobalIlluminationQuality());
	case EPHSettingKind::SK_Reflections:        return QualityLabel(GameSettings->GetReflectionQuality());
	case EPHSettingKind::SK_Textures:           return QualityLabel(GameSettings->GetTextureQuality());
	case EPHSettingKind::SK_Effects:            return QualityLabel(GameSettings->GetVisualEffectQuality());
	case EPHSettingKind::SK_Foliage:            return QualityLabel(GameSettings->GetFoliageQuality());
	case EPHSettingKind::SK_Shading:            return QualityLabel(GameSettings->GetShadingQuality());
	default:                                    return FText::FromString(TEXT("--"));
	}
}

// KEY BINDING

void UPHSettingsMenuPageWidget::HandleRebindClicked()
{
	for (const FKeyBindRow& Row : KeyBindRows)
	{
		const UButton* Button = Row.RebindButton.Get();
		if (Button && Button->IsHovered())
		{
			ListeningMappingName = Row.MappingName;
			// Without focus the page never receives the key press.
			SetKeyboardFocus();
			RefreshKeyBindRows();
			return;
		}
	}
}

void UPHSettingsMenuPageWidget::CancelKeyListen()
{
	if (!ListeningMappingName.IsNone())
	{
		ListeningMappingName = NAME_None;
		RefreshKeyBindRows();
	}
}

bool UPHSettingsMenuPageWidget::ApplyPendingKey(const FKey& Key)
{
	if (ListeningMappingName.IsNone())
	{
		return false;
	}

	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer
		? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	UEnhancedInputUserSettings* InputSettings = Subsystem ? Subsystem->GetUserSettings() : nullptr;
	if (!InputSettings)
	{
		CancelKeyListen();
		return false;
	}

	FMapPlayerKeyArgs Args;
	Args.MappingName = ListeningMappingName;
	Args.Slot = EPlayerMappableKeySlot::First;
	Args.NewKey = Key;

	FGameplayTagContainer FailureReason;
	InputSettings->MapPlayerKey(Args, FailureReason);

	// Enhanced Input applies a rebind immediately, so it is saved immediately
	// rather than staged behind Apply - staging it would misreport state the
	// input system has already changed.
	InputSettings->ApplySettings();
	InputSettings->SaveSettings();

	ListeningMappingName = NAME_None;
	RefreshKeyBindRows();
	return FailureReason.IsEmpty();
}

FReply UPHSettingsMenuPageWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (IsListeningForKey())
	{
		if (InKeyEvent.GetKey() == EKeys::Escape)
		{
			CancelKeyListen();
			return FReply::Handled();
		}
		ApplyPendingKey(InKeyEvent.GetKey());
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UPHSettingsMenuPageWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	// Mouse buttons are bindable too, but only while listening - otherwise this
	// would swallow ordinary clicks on the page.
	if (IsListeningForKey())
	{
		ApplyPendingKey(InMouseEvent.GetEffectingButton());
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UPHSettingsMenuPageWidget::RefreshKeyBindRows()
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer
		? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	const UEnhancedInputUserSettings* InputSettings = Subsystem ? Subsystem->GetUserSettings() : nullptr;
	const UEnhancedPlayerMappableKeyProfile* Profile =
		InputSettings ? InputSettings->GetActiveKeyProfile() : nullptr;

	for (const FKeyBindRow& Row : KeyBindRows)
	{
		UTextBlock* KeyText = Row.KeyText.Get();
		if (!KeyText)
		{
			continue;
		}

		if (Row.MappingName == ListeningMappingName)
		{
			KeyText->SetText(FText::FromString(TEXT("Press any key...   Esc cancels")));
			KeyText->SetColorAndOpacity(FSlateColor(Palette::Accent));
			continue;
		}

		KeyText->SetColorAndOpacity(FSlateColor(Palette::Text));

		FText Display = FText::FromString(TEXT("--"));
		if (Profile)
		{
			if (const FKeyMappingRow* MappingRow = Profile->FindKeyMappingRow(Row.MappingName))
			{
				for (const FPlayerKeyMapping& Mapping : MappingRow->Mappings)
				{
					if (Mapping.GetSlot() == EPlayerMappableKeySlot::First)
					{
						Display = Mapping.GetCurrentKey().GetDisplayName();
						break;
					}
				}
			}
		}
		KeyText->SetText(Display);
	}
}

// LIFECYCLE

void UPHSettingsMenuPageWidget::RefreshSettings()
{
	for (const FSettingRow& Row : Rows)
	{
		if (UTextBlock* ValueText = Row.ValueText.Get())
		{
			ValueText->SetText(DescribeSetting(Row.Kind));
		}
	}

	if (const UPHGameSettings* Project = PHSettings())
	{
		const float Volume = Project->GetMasterVolume();
		if (MasterVolumeSlider && !MasterVolumeSlider->HasMouseCapture())
		{
			MasterVolumeSlider->SetValue(Volume);
		}
		if (MasterVolumeValue)
		{
			MasterVolumeValue->SetText(FText::FromString(
				FString::Printf(TEXT("%.0f%%"), Volume * 100.0f)));
		}
	}

	RefreshKeyBindRows();

	if (StatusText)
	{
		StatusText->SetText(bDirty
			? FText::FromString(TEXT("UNAPPLIED CHANGES"))
			: FText::GetEmpty());
		StatusText->SetVisibility(bDirty ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	OnSettingsRefreshed();
}

void UPHSettingsMenuPageWidget::HandleMasterVolumeChanged(const float NewValue)
{
	if (UPHGameSettings* Project = PHSettings())
	{
		Project->SetMasterVolume(NewValue);
		// Audible while dragging; the value is still only persisted by Apply.
		Project->ApplyAudioSettings();
		bDirty = true;
		RefreshSettings();
	}
}

void UPHSettingsMenuPageWidget::ApplySettings()
{
	if (UGameUserSettings* GameSettings = Settings())
	{
		GameSettings->ApplySettings(/*bCheckForCommandLineOverrides*/ false);
		GameSettings->SaveSettings();
	}
	if (UPHGameSettings* Project = PHSettings())
	{
		Project->Save();
	}
	bDirty = false;
	RefreshSettings();
}

void UPHSettingsMenuPageWidget::RevertSettings()
{
	if (UGameUserSettings* GameSettings = Settings())
	{
		// Re-reads the saved values, discarding anything staged since Apply.
		GameSettings->LoadSettings(/*bForceReload*/ true);
	}
	if (UPHGameSettings* Project = PHSettings())
	{
		// Re-read the saved project values too, not just the engine's.
		Project->LoadConfig();
		Project->ApplyAudioSettings();
	}
	bDirty = false;
	RefreshSettings();
}

void UPHSettingsMenuPageWidget::RestoreDefaults()
{
	if (UGameUserSettings* GameSettings = Settings())
	{
		GameSettings->SetToDefaults();
	}
	if (UPHGameSettings* Project = PHSettings())
	{
		Project->RestoreDefaults();
		Project->ApplyAudioSettings();
	}
	bDirty = true;
	RefreshSettings();
}

void UPHSettingsMenuPageWidget::HandleApplyClicked()    { ApplySettings(); }
void UPHSettingsMenuPageWidget::HandleRevertClicked()   { RevertSettings(); }
void UPHSettingsMenuPageWidget::HandleDefaultsClicked() { RestoreDefaults(); }

// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "UI/Menu/Widgets/PHMenuPageWidgetBase.h"
#include "PHSettingsMenuPageWidget.generated.h"

class UButton;
class UGameUserSettings;
class UPanelWidget;
class UPHGameSettings;
class USlider;
class UTextBlock;
class UVerticalBox;

/** Which sub-tab of the settings page is showing. */
UENUM(BlueprintType)
enum class EPHSettingsSection : uint8
{
	SS_Gameplay  UMETA(DisplayName = "Gameplay"),
	SS_Audio     UMETA(DisplayName = "Audio"),
	SS_KeyBinds  UMETA(DisplayName = "Key Binds"),
	SS_Display   UMETA(DisplayName = "Display")
};

/** One control. Each maps to a real setting with a real consumer. */
UENUM(BlueprintType)
enum class EPHSettingKind : uint8
{
	// Display
	SK_WindowMode      UMETA(DisplayName = "Window Mode"),
	SK_Resolution      UMETA(DisplayName = "Resolution"),
	SK_VSync           UMETA(DisplayName = "VSync"),
	SK_FrameRateLimit  UMETA(DisplayName = "Frame Rate Limit"),
	SK_ViewDistance    UMETA(DisplayName = "View Distance"),
	SK_AntiAliasing    UMETA(DisplayName = "Anti-Aliasing"),
	SK_PostProcessing  UMETA(DisplayName = "Post Processing"),
	SK_Shadows         UMETA(DisplayName = "Shadows"),
	SK_GlobalIllumination UMETA(DisplayName = "Global Illumination"),
	SK_Reflections     UMETA(DisplayName = "Reflections"),
	SK_Textures        UMETA(DisplayName = "Textures"),
	SK_Effects         UMETA(DisplayName = "Effects"),
	SK_Foliage         UMETA(DisplayName = "Foliage"),
	SK_Shading         UMETA(DisplayName = "Shading"),

	// Gameplay
	SK_ShowDamageNumbers   UMETA(DisplayName = "Show Damage Numbers"),
	SK_ShowFloorBanner     UMETA(DisplayName = "Show Floor Banner"),
	SK_ShowEnemyHealthBars UMETA(DisplayName = "Show Enemy Health Bars")
};

/**
 * The Settings page: Gameplay, Audio, Key Binds and Display.
 *
 * Every control here is backed by something real. Display and scalability come
 * from UGameUserSettings; gameplay toggles and master volume from
 * UPHGameSettings, which persists to the same ini; key bindings from
 * Enhanced Input's own user settings, which owns the profile and its save.
 *
 * Changes are staged until Apply so a bad resolution is not committed by
 * scrolling past it. Key rebinds are the exception - Enhanced Input applies
 * those immediately, and this page saves them straight away rather than
 * pretending they are staged.
 */
UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UPHSettingsMenuPageWidget : public UPHMenuPageWidgetBase
{
	GENERATED_BODY()

public:
	/** Re-reads every row from its owning settings object. */
	UFUNCTION(BlueprintCallable, Category = "Settings Menu")
	void RefreshSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings Menu")
	void ApplySettings();

	UFUNCTION(BlueprintCallable, Category = "Settings Menu")
	void RevertSettings();

	/** Engine and project defaults. Still needs Apply. */
	UFUNCTION(BlueprintCallable, Category = "Settings Menu")
	void RestoreDefaults();

	UFUNCTION(BlueprintCallable, Category = "Settings Menu")
	void ShowSection(EPHSettingsSection Section);

	UFUNCTION(BlueprintPure, Category = "Settings Menu")
	bool HasUnappliedChanges() const { return bDirty; }

	/** True while waiting for the player to press a key to bind. */
	UFUNCTION(BlueprintPure, Category = "Settings Menu|Key Binds")
	bool IsListeningForKey() const { return !ListeningMappingName.IsNone(); }

	/** Stops waiting without changing the binding. */
	UFUNCTION(BlueprintCallable, Category = "Settings Menu|Key Binds")
	void CancelKeyListen();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	/** Captures the next key while listening, so rebinding needs no extra widget. */
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** Sub-tab the page opens on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings Menu|Config")
	EPHSettingsSection DefaultSection = EPHSettingsSection::SS_Gameplay;

	/** Optional authored host. Without one the page builds its own column. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings Menu")
	TObjectPtr<UPanelWidget> SettingsContainer;

	UFUNCTION(BlueprintImplementableEvent, Category = "Settings Menu|Events")
	void OnSettingsRefreshed();

private:
	struct FSettingRow
	{
		EPHSettingKind Kind = EPHSettingKind::SK_WindowMode;
		TWeakObjectPtr<UTextBlock> ValueText;
		TWeakObjectPtr<UButton> Previous;
		TWeakObjectPtr<UButton> Next;
	};

	struct FKeyBindRow
	{
		FName MappingName;
		TWeakObjectPtr<UTextBlock> KeyText;
		TWeakObjectPtr<UButton> RebindButton;
	};

	void BuildWidgets();
	void BuildSectionTabs(UVerticalBox& Column);
	void BuildGameplaySection();
	void BuildAudioSection();
	void BuildKeyBindsSection();
	void BuildDisplaySection();

	void AddSettingRow(UVerticalBox& Column, EPHSettingKind Kind, const FText& Label);
	void StepSetting(EPHSettingKind Kind, int32 Delta);
	FText DescribeSetting(EPHSettingKind Kind) const;
	void CacheResolutions();
	void RefreshKeyBindRows();
	bool ApplyPendingKey(const FKey& Key);

	static UGameUserSettings* Settings();
	static UPHGameSettings* PHSettings();
	static FText QualityLabel(int32 Level);

	UFUNCTION()
	void HandleStepClicked();

	UFUNCTION()
	void HandleSectionTabClicked();

	UFUNCTION()
	void HandleRebindClicked();

	UFUNCTION()
	void HandleMasterVolumeChanged(float NewValue);

	UFUNCTION()
	void HandleApplyClicked();

	UFUNCTION()
	void HandleRevertClicked();

	UFUNCTION()
	void HandleDefaultsClicked();

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> BuiltColumn = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USlider> MasterVolumeSlider = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MasterVolumeValue = nullptr;

	UPROPERTY(Transient)
	TMap<EPHSettingsSection, TObjectPtr<UVerticalBox>> SectionBoxes;

	UPROPERTY(Transient)
	TMap<EPHSettingsSection, TObjectPtr<UButton>> SectionTabs;

	TArray<FSettingRow> Rows;
	TArray<FKeyBindRow> KeyBindRows;
	TArray<FIntPoint> AvailableResolutions;

	EPHSettingsSection ActiveSection = EPHSettingsSection::SS_Gameplay;

	/** Mapping awaiting a key press. None when not listening. */
	FName ListeningMappingName;

	bool bWidgetsBuilt = false;
	bool bDirty = false;
};

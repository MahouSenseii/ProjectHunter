#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/PHUserWidget.h"
#include "Components/WidgetSwitcher.h"
#include "Library/WidgetStructLibrary.h"
#include "PHWidgetSwitcher.generated.h"

class UButton;
class UHorizontalBox;
class UCanvasPanel;
class UImage;
class UMenuButton;
class UWidgetManager;

/**
 * Example widget that dynamically creates tabs & buttons
 * from a TArray<FMenuWidgetStuct>.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHWidgetSwitcher : public UPHUserWidget
{
    GENERATED_BODY()

protected:
    // The top-level switcher to which we'll add each tab as a child
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidgetSwitcher> Switcher = nullptr;

    // Your main data source
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Setup")
    TArray<FMenuWidgetStuct> ButtonMapper;

    // Use FMenuButtonArray instead of raw TArray<UMenuButton*>
    UPROPERTY()
    TMap<EWidgetsSwitcher, FMenuButtonArray> TabButtons;

    UPROPERTY()
    TMap<EWidgetsSwitcher, int32> TabFocusedIndex;

    UPROPERTY()
    TMap<EWidgetsSwitcher, UWidget*> SwitcherPanels;

    // If you want custom color styles
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style")
    FLinearColor HoverColor = FLinearColor::Yellow;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style")
    FLinearColor UnHoverColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style")
    FLinearColor ClickedColor = FLinearColor::Blue;

public:
    virtual void NativeConstruct() override;

protected:


    TMap<EWidgetsSwitcher, TArray<FMenuWidgetStuct>> GroupButtonMapperBySwitcher();
    UButton* CreateLBButton();
    UButton* CreateRBButton();
    void BuildTabPanel(EWidgetsSwitcher SwitcherType, TArray<FMenuWidgetStuct>& Buttons);

    // Creates panels + LB/RB + MenuButtons from ButtonMapper
    //void BuildTabsAndButtons();

    // Helper to bind dynamic delegates for a newly created UMenuButton
    void BindMenuButtonDelegates(const UMenuButton* InButton);

    // Called when any "middle" button's OnClicked is triggered
    UFUNCTION()
    void OnAnyButtonClicked();

    // Called when any "middle" button's OnHovered is triggered
    UFUNCTION()
    void OnAnyButtonHovered();

    void BuildTabsAndButtons();
    // Called when any "middle" button's OnUnhovered is triggered
    UFUNCTION()
    void OnAnyButtonUnhovered();

    UWidget* CreateMenuButton(FMenuWidgetStuct& MenuInfo);


    // Called when LB or RB is clicked
    UFUNCTION()
    void OnLBClicked();

    UFUNCTION()
    void OnRBClicked();

    // Helpers to handle color states
    void HoverButton(EWidgets WidgetType) const;
    void
    UnHoverButton(EWidgets WidgetType) const;
    void ClickButton(EWidgets WidgetType) const;
    static void SetButtonColor(const UMenuButton* InButton, const FLinearColor& InColor);

    // Sub-tab focusing
    void FocusButtonOnTab(EWidgetsSwitcher SwitcherType, int32 ButtonIndex);
    void MoveFocusOnTab(EWidgetsSwitcher SwitcherType, int32 Direction);

    UPROPERTY(EditAnywhere, Category = "ButtonSize")
    float ButtonHeight = 60.0f;

    UPROPERTY(EditAnywhere, Category = "ButtonSize")
    float ButtonWidth = 120.0f;
};

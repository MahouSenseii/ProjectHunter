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

UCLASS()
class ALS_PROJECTHUNTER_API UPHWidgetSwitcher : public UPHUserWidget
{
    GENERATED_BODY()

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidgetSwitcher> Switcher = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Setup")
    TArray<FMenuWidgetStuct> ButtonMapper;

    // GC-safe storage
    UPROPERTY()
    TMap<EWidgetsSwitcher, FMenuButtonArray> TabButtons;

    UPROPERTY()
    TMap<EWidgetsSwitcher, int32> TabFocusedIndex;

    UPROPERTY()
    TMap<EWidgetsSwitcher, UWidget*> SwitcherPanels;

    // Performance cache - avoids repeated lookups
    UPROPERTY()
    TMap<TObjectPtr<UButton>, EWidgets> ButtonToWidgetMap;

    UPROPERTY()
    TObjectPtr<UWidgetManager> CachedWidgetManager;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style")
    FLinearColor HoverColor = FLinearColor::Yellow;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style")
    FLinearColor UnHoverColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style")
    FLinearColor ClickedColor = FLinearColor::Blue;

    UPROPERTY(EditAnywhere, Category = "ButtonSize")
    float ButtonHeight = 60.0f;

    UPROPERTY(EditAnywhere, Category = "ButtonSize")
    float ButtonWidth = 120.0f;

public:
    virtual void NativeConstruct() override;

protected:
    // Optimized methods
    TMap<EWidgetsSwitcher, TArray<FMenuWidgetStuct>> GroupButtonMapperBySwitcher();
    UButton* CreateNavigationButton(const FString& TexturePath, bool bIsLeftButton);
    void BuildTabPanel(EWidgetsSwitcher SwitcherType, TArray<FMenuWidgetStuct>& Buttons);
    void BuildTabsAndButtons();
    
    UWidget* CreateMenuButton(FMenuWidgetStuct& MenuInfo);
    void BindMenuButtonDelegates(const UMenuButton* InButton);
    
    // Optimized event handlers with caching
    UFUNCTION()
    void OnAnyButtonClicked();
    
    UFUNCTION()
    void OnAnyButtonHovered();
    
    UFUNCTION()
    void OnAnyButtonUnhovered();

    UFUNCTION()
    void OnLBClicked();

    UFUNCTION()
    void OnRBClicked();

    // Helper methods
    UWidgetManager* GetWidgetManager();
    static void SetButtonColor(const UMenuButton* InButton, const FLinearColor& InColor);
    void FocusButtonOnTab(EWidgetsSwitcher SwitcherType, int32 ButtonIndex);
    void MoveFocusOnTab(EWidgetsSwitcher SwitcherType, int32 Direction);
    
    // Optimized button state management
    void UpdateButtonState(EWidgets WidgetType, const FLinearColor& Color);
    UMenuButton* FindMenuButton(EWidgets WidgetType);
};
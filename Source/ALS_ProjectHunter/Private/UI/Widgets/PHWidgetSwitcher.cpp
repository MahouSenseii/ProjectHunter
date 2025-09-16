#include "UI/Widgets/PHWidgetSwitcher.h"
#include "Blueprint/WidgetTree.h"
#include "Character/Player/PHPlayerController.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/WidgetManager.h"
#include "Library/WidgetStructLibrary.h"
#include "UI/Widgets/MenuButton.h"

void UPHWidgetSwitcher::NativeConstruct()
{
    Super::NativeConstruct();

    if (!Switcher)
    {
        UE_LOG(LogTemp, Error, TEXT("No Switcher bound in UMG!"));
        return;
    }

    // Cache widget manager once
    CachedWidgetManager = GetWidgetManager();
    
    BuildTabsAndButtons();
}

TMap<EWidgetsSwitcher, TArray<FMenuWidgetStuct>> UPHWidgetSwitcher::GroupButtonMapperBySwitcher()
{
    TMap<EWidgetsSwitcher, TArray<FMenuWidgetStuct>> MapperGroups;
    MapperGroups.Reserve(ButtonMapper.Num()); // Pre-allocate
    
    for (const FMenuWidgetStuct& Item : ButtonMapper)
    {
        if (Item.WidgetsSwitcher != EWidgetsSwitcher::WS_None)
        {
            MapperGroups.FindOrAdd(Item.WidgetsSwitcher).Add(Item);
        }
    }
    return MapperGroups;    
}

UButton* UPHWidgetSwitcher::CreateNavigationButton(const FString& TexturePath, bool bIsLeftButton)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    Button->SetBackgroundColor(FLinearColor(0, 0, 0, 0));

    UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    Button->AddChild(Image);

    if (UTexture2D* Texture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *TexturePath)))
    {
        Image->SetBrushFromTexture(Texture);
    }

    // Bind to optimized handlers
    if (bIsLeftButton)
    {
        Button->OnClicked.AddDynamic(this, &UPHWidgetSwitcher::OnLBClicked);
    }
    else
    {
        Button->OnClicked.AddDynamic(this, &UPHWidgetSwitcher::OnRBClicked);
    }

    return Button;
}

void UPHWidgetSwitcher::BuildTabPanel(EWidgetsSwitcher SwitcherType, TArray<FMenuWidgetStuct>& Buttons)
{
    // RESPONSIVE HIERARCHY - Maintains scaling while fixing alignment
    
    // 1. ScaleBox for responsive scaling across devices
    UScaleBox* ScaleBox = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
    ScaleBox->SetStretch(EStretch::ScaleToFit);
    ScaleBox->SetStretchDirection(EStretchDirection::Both);
    
    // 2. SizeBox for size constraints (important for mobile/desktop)
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    SizeBox->ClearWidthOverride();  // Auto-width
    SizeBox->SetHeightOverride(ButtonHeight + 20.0f); // Consistent height with padding
    
    // 3. HorizontalBox for button layout
    UHorizontalBox* HBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    
    // Add LB button
    UButton* LBButton = CreateNavigationButton(TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_LB.T_Gamepad_LB"), true);
    if (UHorizontalBoxSlot* LBSlot = Cast<UHorizontalBoxSlot>(HBox->AddChild(LBButton)))
    {
        LBSlot->SetHorizontalAlignment(HAlign_Center);
        LBSlot->SetVerticalAlignment(VAlign_Center);
        LBSlot->SetPadding(FMargin(5.0f));
    }
    
    TArray<TObjectPtr<UMenuButton>> LocalButtons;
    LocalButtons.Reserve(Buttons.Num());

    // Create menu buttons
    for (FMenuWidgetStuct& Info : Buttons)
    {
        if (UWidget* Widget = CreateMenuButton(Info))
        {
            if (UHorizontalBoxSlot* ButtonSlot = Cast<UHorizontalBoxSlot>(HBox->AddChild(Widget)))
            {
                ButtonSlot->SetHorizontalAlignment(HAlign_Center);
                ButtonSlot->SetVerticalAlignment(VAlign_Center);
                ButtonSlot->SetPadding(FMargin(5.0f));
            }
            
            LocalButtons.Add(Info.MenuButton);

            // Update original ButtonMapper reference
            for (FMenuWidgetStuct& Original : ButtonMapper)
            {
                if (Original.Widget == Info.Widget)
                {
                    Original.MenuButton = Info.MenuButton;
                    break;
                }
            }

            // Cache button mapping for performance
            if (Info.MenuButton && Info.MenuButton->MenuButton)
            {
                ButtonToWidgetMap.Add(Info.MenuButton->MenuButton, Info.Widget);
            }
        }
    }

    // Add RB button
    UButton* RBButton = CreateNavigationButton(TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_RB.T_Gamepad_RB"), false);
    if (UHorizontalBoxSlot* RBSlot = Cast<UHorizontalBoxSlot>(HBox->AddChild(RBButton)))
    {
        RBSlot->SetHorizontalAlignment(HAlign_Center);
        RBSlot->SetVerticalAlignment(VAlign_Center);
        RBSlot->SetPadding(FMargin(5.0f));
    }

    // Assemble hierarchy with proper alignment
    if (UScaleBoxSlot* SizeBoxSlot = Cast<UScaleBoxSlot>(ScaleBox->AddChild(SizeBox)))
    {
        SizeBoxSlot->SetHorizontalAlignment(HAlign_Center);
        SizeBoxSlot->SetVerticalAlignment(VAlign_Center);
    }
    
    SizeBox->AddChild(HBox);

    // Store tab data
    FMenuButtonArray ButtonArray;
    ButtonArray.Buttons = LocalButtons;
    TabButtons.Add(SwitcherType, ButtonArray);
    TabFocusedIndex.Add(SwitcherType, 0);

    // Add the responsive hierarchy to switcher
    Switcher->AddChild(ScaleBox);
    SwitcherPanels.Add(SwitcherType, ScaleBox);
}

void UPHWidgetSwitcher::BuildTabsAndButtons()
{
    TMap<EWidgetsSwitcher, TArray<FMenuWidgetStuct>> GroupedTabs = GroupButtonMapperBySwitcher();
    
    for (auto& Pair : GroupedTabs)
    {
        BuildTabPanel(Pair.Key, Pair.Value);
    }
}

UWidget* UPHWidgetSwitcher::CreateMenuButton(FMenuWidgetStuct& MenuInfo)
{
    if (!MenuInfo.MenuButtonClass)
    {
        UE_LOG(LogTemp, Error, TEXT("MenuButtonClass is null for widget type: %d"), (int32)MenuInfo.Widget);
        return nullptr;
    }

    // Create the menu button widget
    UMenuButton* Button = CreateWidget<UMenuButton>(GetWorld(), MenuInfo.MenuButtonClass);
    if (!Button)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create MenuButton"));
        return nullptr;
    }

    // Configure button properties
    Button->MenuTextBlock->SetText(FText::FromString(MenuInfo.MenuText));
    Button->WidgetType = MenuInfo.Widget;
    Button->WidgetSwitcher = MenuInfo.WidgetsSwitcher;
    
    // Bind delegates and initialize
    BindMenuButtonDelegates(Button);
    MenuInfo.MenuButton = Button;
    Button->PHInitialize();

    // sizing with consistent dimensions
    USizeBox* Wrapper = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    if (!Wrapper)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create SizeBox wrapper"));
        return Button; // Return the button directly if wrapper creation fails
    }

    Wrapper->SetWidthOverride(ButtonWidth);
    Wrapper->SetHeightOverride(ButtonHeight);
    Wrapper->AddChild(Button);

    return Wrapper;
}


void UPHWidgetSwitcher::BindMenuButtonDelegates(const UMenuButton* InButton)
{
    if (!InButton || !InButton->MenuButton)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot bind delegates: Invalid button"));
        return;
    }

    // UButton delegates don't pass parameters - use the original approach but optimized
    InButton->MenuButton->OnClicked.AddDynamic(this, &UPHWidgetSwitcher::OnAnyButtonClicked);
    InButton->MenuButton->OnHovered.AddDynamic(this, &UPHWidgetSwitcher::OnAnyButtonHovered);  
    InButton->MenuButton->OnUnhovered.AddDynamic(this, &UPHWidgetSwitcher::OnAnyButtonUnhovered);
}

// OPTIMIZED EVENT HANDLERS - Use cached mapping for O(1) lookup
void UPHWidgetSwitcher::OnAnyButtonClicked()
{
    // Find which button was clicked using the cached mapping
    for (const auto& Pair : ButtonToWidgetMap)
    {
        if (Pair.Key && Pair.Key->IsHovered()) // The clicked button is currently hovered
        {
            UpdateButtonState(Pair.Value, ClickedColor);
            
            // Handle widget switching
            if (CachedWidgetManager)
            {
                if (const UMenuButton* MenuButton = FindMenuButton(Pair.Value))
                {
                    CachedWidgetManager->Execute_SwitchWidgetTo(
                        OwnerCharacter, Pair.Value, MenuButton->WidgetSwitcher, nullptr);
                    CachedWidgetManager->OpenNewWidget_Implementation(Pair.Value, false);
                }
            }
            break; // Found the clicked button, exit loop
        }
    }
}

void UPHWidgetSwitcher::OnAnyButtonHovered()
{
    // Find which button was hovered using the cached mapping
    for (const auto& Pair : ButtonToWidgetMap)
    {
        if (Pair.Key && Pair.Key->IsHovered())
        {
            UpdateButtonState(Pair.Value, HoverColor);
            break; // Found the hovered button, exit loop
        }
    }
}

void UPHWidgetSwitcher::OnAnyButtonUnhovered()
{
    // Check all buttons and unhover any that are no longer hovered
    for (const auto& Pair : ButtonToWidgetMap)
    {
        if (Pair.Key && !Pair.Key->IsHovered())
        {
            UpdateButtonState(Pair.Value, UnHoverColor);
        }
    }
}

// OPTIMIZED HELPER METHODS
UWidgetManager* UPHWidgetSwitcher::GetWidgetManager()
{
    if (CachedWidgetManager)
    {
        return CachedWidgetManager;
    }

    if (OwnerCharacter)
    {
        // Use consistent controller access
        if (const APHPlayerController* PC = Cast<APHPlayerController>(OwnerCharacter->GetController()))
        {
            CachedWidgetManager = PC->GetWidgetManager();
        }
    }
    
    return CachedWidgetManager;
}

void UPHWidgetSwitcher::UpdateButtonState(EWidgets WidgetType, const FLinearColor& Color)
{
    UMenuButton* Button = FindMenuButton(WidgetType);
    SetButtonColor(Button, Color);
}

UMenuButton* UPHWidgetSwitcher::FindMenuButton(EWidgets WidgetType)
{
    // Cache frequently accessed buttons
    static TMap<EWidgets, TWeakObjectPtr<UMenuButton>> ButtonCache;
    
    if (auto CachedButton = ButtonCache.Find(WidgetType))
    {
        if (CachedButton->IsValid())
        {
            return CachedButton->Get();
        }
    }

    // Fallback to search
    for (const FMenuWidgetStuct& Item : ButtonMapper)
    {
        if (Item.Widget == WidgetType && Item.MenuButton)
        {
            ButtonCache.Add(WidgetType, Item.MenuButton);
            return Item.MenuButton;
        }
    }
    
    return nullptr;
}

void UPHWidgetSwitcher::OnLBClicked()
{
    if (UWidgetManager* Manager = GetWidgetManager())
    {
        const EWidgetsSwitcher CurrentTab = Manager->GetActiveTab_Implementation();
        MoveFocusOnTab(CurrentTab, -1);
    }
}

void UPHWidgetSwitcher::OnRBClicked()
{
    if (UWidgetManager* Manager = GetWidgetManager())
    {
        const EWidgetsSwitcher CurrentTab = Manager->GetActiveTab_Implementation();
        MoveFocusOnTab(CurrentTab, +1);
    }
}

void UPHWidgetSwitcher::SetButtonColor(const UMenuButton* InButton, const FLinearColor& InColor)
{
    if (!InButton || !InButton->MenuTextBlock) return;
    
    InButton->MenuTextBlock->SetColorAndOpacity(InColor);
    // Optionally set background color too
    // InButton->MenuButton->SetBackgroundColor(InColor);
}

void UPHWidgetSwitcher::FocusButtonOnTab(EWidgetsSwitcher SwitcherType, const int32 ButtonIndex)
{
    const FMenuButtonArray* ButtonArrayStruct = TabButtons.Find(SwitcherType);
    if (!ButtonArrayStruct) return;

    const TArray<TObjectPtr<UMenuButton>>& Buttons = ButtonArrayStruct->Buttons;
    if (!Buttons.IsValidIndex(ButtonIndex)) return;

    // Update focus states
    for (int32 i = 0; i < Buttons.Num(); ++i)
    {
        const UMenuButton* ThisButton = Buttons[i].Get();
        if (!ThisButton) continue;

        const FLinearColor Color = (i == ButtonIndex) ? HoverColor : UnHoverColor;
        SetButtonColor(ThisButton, Color);
        
        if (ThisButton->MenuButton)
        {
            ThisButton->MenuButton->SetBackgroundColor(Color);
        }
    }
}

void UPHWidgetSwitcher::MoveFocusOnTab(EWidgetsSwitcher SwitcherType, int32 Direction)
{
    const FMenuButtonArray* ButtonArrayStruct = TabButtons.Find(SwitcherType);
    if (!ButtonArrayStruct) return;

    const TArray<TObjectPtr<UMenuButton>>& Buttons = ButtonArrayStruct->Buttons;
    if (Buttons.Num() == 0) return;

    int32* FocusedIndex = TabFocusedIndex.Find(SwitcherType);
    if (!FocusedIndex) return;

    *FocusedIndex = (*FocusedIndex + Direction + Buttons.Num()) % Buttons.Num();
    FocusButtonOnTab(SwitcherType, *FocusedIndex);
}
#include "UI/Widgets/PHWidgetSwitcher.h"
#include "Blueprint/WidgetTree.h"
#include "Character/Player/PHPlayerController.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/WidgetManager.h"
#include "Library/WidgetStructLibrary.h"
#include "UI/Widgets/MenuButton.h"

void UPHWidgetSwitcher::NativeConstruct()
{
    Super::NativeConstruct();

    if (!Switcher)
    {
        UE_LOG(LogTemp, Warning, TEXT("No Switcher bound in UMG!"));
        return;
    }

    // Build out the tabs and their buttons
    BuildTabsAndButtons();
}

TMap<EWidgetsSwitcher, TArray<FMenuWidgetStuct>> UPHWidgetSwitcher::GroupButtonMapperBySwitcher()
{
    TMap<EWidgetsSwitcher, TArray<FMenuWidgetStuct>> MapperGroups;
    for (const FMenuWidgetStuct& Item : ButtonMapper)
    {
        if (Item.WidgetsSwitcher != EWidgetsSwitcher::WS_None)
        {
            MapperGroups.FindOrAdd(Item.WidgetsSwitcher).Add(Item);
        }
    }
    return MapperGroups;    
}


UButton* UPHWidgetSwitcher::CreateLBButton()
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    Button->SetBackgroundColor(FLinearColor(0, 0, 0, 0));

    UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    Button->AddChild(Image);

    if (UTexture2D* Texture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_LB.T_Gamepad_LB"))))
    {
        Image->SetBrushFromTexture(Texture);
    }

    Button->OnClicked.AddDynamic(this, &UPHWidgetSwitcher::OnLBClicked);
    return Button;
}

UButton* UPHWidgetSwitcher::CreateRBButton()
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    Button->SetBackgroundColor(FLinearColor(0, 0, 0, 0));

    UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    Button->AddChild(Image);

    if (UTexture2D* Texture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_RB.T_Gamepad_RB"))))
    {
        Image->SetBrushFromTexture(Texture);
    }

    Button->OnClicked.AddDynamic(this, &UPHWidgetSwitcher::OnRBClicked);
    return Button;
}


void UPHWidgetSwitcher::BuildTabPanel(EWidgetsSwitcher SwitcherType,  TArray<FMenuWidgetStuct>& Buttons)
{
    UScaleBox* ScaleBox = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
    ScaleBox->SetStretch(EStretch::ScaleToFit);
    ScaleBox->SetStretchDirection(EStretchDirection::Both);

    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    SizeBox->ClearWidthOverride();

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
    UHorizontalBox* HBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    if (UCanvasPanelSlot* HBoxSlot = Cast<UCanvasPanelSlot>(Canvas->AddChild(HBox)))
    {
        HBoxSlot->SetAnchors(FAnchors(0.5f, 0.5f)); // Center anchor
        HBoxSlot->SetAlignment(FVector2D(0.5f, 0.5f)); // Center alignment

        // Explicitly center with zero offsets
        HBoxSlot->SetPosition(FVector2D(0.f, 0.f));
        HBoxSlot->SetSize(FVector2D(0.f, 0.f));
        HBoxSlot->SetAutoSize(true);
    }


    // Add LB
    HBox->AddChild(CreateLBButton());
    TArray<TObjectPtr<UMenuButton>> LocalButtons;

    for (FMenuWidgetStuct& Info : Buttons)
    {
        if (UWidget* Widget = CreateMenuButton(Info))
        {
            HBox->AddChild(Widget);
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
        }
    }

    
    HBox->AddChild(CreateRBButton());

    FMenuButtonArray Struct;
    Struct.Buttons = LocalButtons;

    TabButtons.Add(SwitcherType, Struct);
    TabFocusedIndex.Add(SwitcherType, 0);

    SizeBox->AddChild(Canvas);
    ScaleBox->AddChild(SizeBox);
    Switcher->AddChild(ScaleBox);
    
    SwitcherPanels.Add(SwitcherType, Canvas);
}


void UPHWidgetSwitcher::BuildTabsAndButtons()
{
    for (TMap<EWidgetsSwitcher, TArray<FMenuWidgetStuct>>
        GroupedTabs = GroupButtonMapperBySwitcher(); auto& Pair : GroupedTabs)
    {
        BuildTabPanel(Pair.Key, Pair.Value);
    }
}


void UPHWidgetSwitcher::BindMenuButtonDelegates(const UMenuButton* InButton)
{
    if (!InButton || !InButton->MenuButton)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Binding delegates for button: %s"), *InButton->GetName());

    InButton->MenuButton->OnClicked.AddDynamic(this, &UPHWidgetSwitcher::OnAnyButtonClicked);
    InButton->MenuButton->OnHovered.AddDynamic(this, &UPHWidgetSwitcher::OnAnyButtonHovered);
    InButton->MenuButton->OnUnhovered.AddDynamic(this, &UPHWidgetSwitcher::OnAnyButtonUnhovered);
}

void UPHWidgetSwitcher::OnAnyButtonClicked()
{
    // Identify which button was actually clicked (the hovered one)
    for (const FMenuWidgetStuct& Entry : ButtonMapper)
    {
        if (const UMenuButton* Btn = Entry.MenuButton)
        {
            if (Btn->MenuButton && Btn->MenuButton->IsHovered())
            {
                // Found the clicked button
                ClickButton(Entry.Widget);
                break;
            }
        }
    }
}

void UPHWidgetSwitcher::OnAnyButtonHovered()
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("OnAnyButtonHovered() called"));
    }

    //Un-comment to enable checks for onbutton Hovered 
    //  UE_LOG(LogTemp, Log, TEXT("OnAnyButtonHovered() called"));

    for (const FMenuWidgetStuct& Entry : ButtonMapper)
    {
        if (const UMenuButton* Btn = Entry.MenuButton)
        {
            if (Btn->MenuButton && Btn->MenuButton->IsHovered())
            {
                //Un-comment to enable checks for onbutton Hovered 
                /*  if (GEngine)
                  {
                      GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("Hovered over button: %s"), *Btn->MenuTextBlock->GetText().ToString()));
                  }*/

                UE_LOG(LogTemp, Log, TEXT("Hovered over button: %s"), *Btn->MenuTextBlock->GetText().ToString());
                
                HoverButton(Entry.Widget);
                break;
            }
        }
    }
}




void UPHWidgetSwitcher::OnAnyButtonUnhovered()
{
    for (const FMenuWidgetStuct& Entry : ButtonMapper)
    {
        if (const UMenuButton* Btn = Entry.MenuButton)
        {
            // if it's not hovered now, revert color
            if (!Btn->MenuButton->IsHovered())
            {
                UnHoverButton(Entry.Widget);
            }
        }
    }
}

UWidget* UPHWidgetSwitcher::CreateMenuButton(FMenuWidgetStuct& MenuInfo)
{
    if (!MenuInfo.MenuButtonClass) return nullptr;

    UMenuButton* Button = CreateWidget<UMenuButton>(GetWorld(), MenuInfo.MenuButtonClass);
    if (!Button) return nullptr;

    Button->MenuTextBlock->SetText(FText::FromString(MenuInfo.MenuText));
    Button->WidgetType = MenuInfo.Widget;
    Button->WidgetSwitcher = MenuInfo.WidgetsSwitcher;

    BindMenuButtonDelegates(Button);
    MenuInfo.MenuButton = Button;

    Button->PHInitialize();

    USizeBox* Wrapper = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    Wrapper->SetWidthOverride(ButtonWidth);
    Wrapper->SetHeightOverride(ButtonHeight);
    Wrapper->AddChild(Button);

    return Wrapper;
}

void UPHWidgetSwitcher::HoverButton(const EWidgets WidgetType) const
{
    // Apply hover color to that button
    const UMenuButton* Button = nullptr;
    for (const FMenuWidgetStuct& Item : ButtonMapper)
    {
        if (Item.Widget == WidgetType)
        {
            Button = Item.MenuButton;
            break;
        }
    }
    SetButtonColor(Button, HoverColor);
}

void UPHWidgetSwitcher::UnHoverButton(const EWidgets WidgetType) const
{
    // Apply normal/unhover color
    const UMenuButton* Button = nullptr;
    for (const FMenuWidgetStuct& Item : ButtonMapper)
    {
        if (Item.Widget == WidgetType)
        {
            Button = Item.MenuButton;
            break;
        }
    }
    SetButtonColor(Button, UnHoverColor);
}

void UPHWidgetSwitcher::ClickButton(const EWidgets WidgetType) const
{
    const UMenuButton* Button = nullptr;
    for (const FMenuWidgetStuct& Item : ButtonMapper)
    {
        if (Item.Widget == WidgetType)
        {
            Button = Item.MenuButton;
            if(const APHPlayerController* PC = Cast<APHPlayerController>(Cast<APHPlayerController>(OwnerCharacter->GetController())))
            {
                PC->GetWidgetManager()->Execute_SwitchWidgetTo(OwnerCharacter,
                    WidgetType,Button->WidgetSwitcher, nullptr);
            }
            break;
        }
    }
    SetButtonColor(Button, ClickedColor);

    // Let the WidgetManager know which sub-tab is active
    if (Button && OwnerCharacter)
    {
        if (const APHPlayerController* PC = Cast<APHPlayerController>(OwnerCharacter->GetAlsController()))
        {
            if (UWidgetManager* Manager = PC->GetWidgetManager())
            {
                Manager->OpenNewWidget_Implementation( WidgetType, false);
            }
        }
    }
}

void UPHWidgetSwitcher::OnLBClicked()
{
    if (const APHPlayerController* PC = Cast<APHPlayerController>(OwnerCharacter->GetAlsController()))
    {
        if (UWidgetManager* Manager = PC->GetWidgetManager())
        {
            const EWidgetsSwitcher CurrentTab = Manager->GetActiveTab_Implementation();
            MoveFocusOnTab(CurrentTab, -1);
        }
    }
}

void UPHWidgetSwitcher::OnRBClicked()
{
    if (const APHPlayerController* PC = Cast<APHPlayerController>(OwnerCharacter->GetAlsController()))
    {
        if (UWidgetManager* Manager = PC->GetWidgetManager())
        {
            const EWidgetsSwitcher CurrentTab = Manager->GetActiveTab_Implementation();
            MoveFocusOnTab(CurrentTab, +1);
        }
    }
}

void UPHWidgetSwitcher::SetButtonColor(const UMenuButton* InButton, const FLinearColor& InColor)
{
    if (!InButton) return;
    //InButton->MenuButton->SetBackgroundColor(InColor);
    InButton->MenuTextBlock->SetColorAndOpacity(InColor);
}

// Moves highlight or "focus" to a certain index in the sub-tab's button array
void UPHWidgetSwitcher::FocusButtonOnTab(EWidgetsSwitcher SwitcherType, const int32 ButtonIndex)
{
    if (!TabButtons.Contains(SwitcherType))
    {
        return;
    }

    // Grab the struct first
    const FMenuButtonArray& ButtonArrayStruct = TabButtons[SwitcherType];
    const TArray<TObjectPtr<UMenuButton>>& Buttons = ButtonArrayStruct.Buttons;

    if (Buttons.IsValidIndex(ButtonIndex))
    {
        // Highlight the newly focused button, un-highlight others
        for (int32 i = 0; i < Buttons.Num(); ++i)
        {
            const UMenuButton* ThisButton = Buttons[i].Get();
            if (!ThisButton) 
            {
                continue;
            }

            if (i == ButtonIndex)
            {
                SetButtonColor(ThisButton, HoverColor);
                ThisButton->MenuButton->SetBackgroundColor(HoverColor);
            }
            else
            {
                ThisButton->MenuButton->SetBackgroundColor(UnHoverColor);
                SetButtonColor(ThisButton, UnHoverColor);
            }
        }
    }
}

// Cycles the focus left/right among the "middle" buttons on a sub-tab
void UPHWidgetSwitcher::MoveFocusOnTab(EWidgetsSwitcher SwitcherType, int32 Direction)
{
    if (!TabButtons.Contains(SwitcherType))
    {
        return;
    }

    // Grab the struct first
    const FMenuButtonArray& ButtonArrayStruct = TabButtons[SwitcherType];
    const TArray<TObjectPtr<UMenuButton>>& Buttons = ButtonArrayStruct.Buttons;

    if (Buttons.Num() == 0)
    {
        return;
    }

    int32& FocusedIndex = TabFocusedIndex[SwitcherType];
    FocusedIndex = (FocusedIndex + Direction + Buttons.Num()) % Buttons.Num();

    FocusButtonOnTab(SwitcherType, FocusedIndex);
}

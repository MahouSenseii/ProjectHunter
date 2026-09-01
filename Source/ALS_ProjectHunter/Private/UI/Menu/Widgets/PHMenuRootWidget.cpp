#include "UI/Menu/Widgets/PHMenuRootWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture.h"

#include "Components/Button.h"
#include "UI/HUD/HunterHUD.h"

#include "Components/WidgetSwitcher.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Character/PHBaseCharacter.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Inventory/Components/InventoryManager.h"
#include "UI/Menu/DragDrop/PHItemDragDropOperation.h"
#include "UI/Menu/Widgets/PHMenuPageWidgetBase.h"
#include "UI/Menu/Widgets/PHMenuTabBarWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogPHMenuRootWidget, Log, All);

UPHMenuRootWidget::UPHMenuRootWidget()
{
	DefaultPageWidgetClass = UPHMenuPageWidgetBase::StaticClass();
}

void UPHMenuRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// UUserWidget is not focusable by default, so the HUD's SetWidgetToFocus is
	// silently declined and keyboard focus stays on the game viewport.
	SetIsFocusable(true);

	if (CloseMenuKeys.IsEmpty())
	{
		CloseMenuKeys = { EKeys::Escape, EKeys::Tab };
	}

	SetUpAccentMaterial();

	if (bDropItemsToWorldOnMissedDrop)
	{
		// A UserWidget root is SelfHitTestInvisible by default, which keeps it out
		// of the hit path entirely - drops on the menu background would never
		// reach NativeOnDrop. Children still receive events first and bubble up.
		SetVisibility(ESlateVisibility::Visible);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UPHMenuRootWidget::HandleCloseButtonClicked);
	}

	BuildMenuEntriesFromEnum();

	if (TabBar)
	{
		TabBar->OnMenuTabSelected.AddUniqueDynamic(this, &UPHMenuRootWidget::HandleTabSelected);
		TabBar->InitializeTabs(MenuEntries, ResolveDefaultMenuType());
	}
	else if (MenuEntries.Num() > 0)
	{
		ShowPage(ResolveDefaultMenuType(), EMenuType::MT_None);
	}

	if (MenuEntries.Num() == 0)
	{
		PH_LOG_WARNING(LogPHMenuRootWidget,
			"NativeConstruct: %s has no MenuEntries configured. Fill 'Menu Entries' in the Blueprint class defaults.",
			*GetName());
	}
}

bool UPHMenuRootWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UPHItemDragDropOperation* ItemOperation = Cast<UPHItemDragDropOperation>(InOperation);
	if (!bDropItemsToWorldOnMissedDrop || !ItemOperation || !ItemOperation->IsValidDrag())
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	// Nothing else claimed this drop, so the player released over the menu
	// background - treat it as "throw it away".
	DropOperationToWorld(ItemOperation);
	return true;
}

bool UPHMenuRootWidget::DropOperationToWorld(UPHItemDragDropOperation* Operation)
{
	if (!Operation || !Operation->IsValidDrag())
	{
		return false;
	}

	APHBaseCharacter* Character = GetBoundCharacter();
	UInventoryManager* InventoryManager =
		Character ? Character->FindComponentByClass<UInventoryManager>() : nullptr;
	if (!InventoryManager)
	{
		return false;
	}

	if (Operation->IsFromInventory())
	{
		InventoryManager->DropItemAtSlotToGround(Operation->SourceInventorySlotIndex);
		return true;
	}

	if (Operation->IsFromEquipment())
	{
		UEquipmentManager* EquipmentManager = Character->GetEquipmentManager();
		if (!EquipmentManager || !EquipmentManager->IsSlotOccupied(Operation->SourceEquipmentSlot))
		{
			return false;
		}

		// Unequip into the bag first, then drop that item. Both calls are reliable
		// and ordered, so on a client the drop resolves after the unequip - and if
		// the drop somehow fails the item is in the bag rather than nowhere.
		EquipmentManager->UnequipItem(Operation->SourceEquipmentSlot, /*bMoveToBag=*/true);
		InventoryManager->DropItemToGround(Operation->Item);
		return true;
	}

	return false;
}

void UPHMenuRootWidget::OpenMenu(EMenuType MenuType)
{
	if (MenuType == EMenuType::MT_None)
	{
		MenuType = ResolveDefaultMenuType();
	}

	if (!FindEntry(MenuType))
	{
		MenuType = GetFirstValidMenuType();
	}

	if (MenuType == EMenuType::MT_None)
	{
		return;
	}

	if (TabBar)
	{
		TabBar->SelectTab(MenuType);
 
		if (ActiveMenuType != MenuType || GetActivePage() == nullptr)
		{
			ShowPage(MenuType, ActiveMenuType);
		}
		return;
	}

	if (ActiveMenuType != MenuType || GetActivePage() == nullptr)
	{
		ShowPage(MenuType, ActiveMenuType);
	}
}

UPHMenuPageWidgetBase* UPHMenuRootWidget::GetActivePage() const
{
	return GetPageForMenu(ActiveMenuType);
}

UPHMenuPageWidgetBase* UPHMenuRootWidget::GetPageForMenu(EMenuType MenuType) const
{
	const FMenuEntry* Entry = FindEntry(MenuType);
	return Entry ? Entry->CachedInstance.Get() : nullptr;
}

void UPHMenuRootWidget::SetMenuPageWidgetClass(
	const EMenuType MenuType,
	TSubclassOf<UPHMenuPageWidgetBase> WidgetClass)
{
	if (MenuType == EMenuType::MT_None)
	{
		return;
	}

	FMenuEntry* Entry = FindEntry(MenuType);
	if (!Entry)
	{
		FMenuEntry NewEntry;
		NewEntry.MenuType = MenuType;
		MenuEntries.Add(MoveTemp(NewEntry));
		Entry = &MenuEntries.Last();
	}

	Entry->WidgetClass = WidgetClass;
	Entry->CachedInstance = nullptr;
}

void UPHMenuRootWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	Super::NativeInitializeForCharacter(Character);

	for (FMenuEntry& Entry : MenuEntries)
	{
		if (Entry.CachedInstance)
		{
			Entry.CachedInstance->InitializeForCharacter(Character);
		}
	}
}

void UPHMenuRootWidget::NativeReleaseCharacter()
{
	for (FMenuEntry& Entry : MenuEntries)
	{
		if (Entry.CachedInstance)
		{
			Entry.CachedInstance->ReleaseCharacter();
		}
	}

	Super::NativeReleaseCharacter();
}

void UPHMenuRootWidget::HandleTabSelected(EMenuType NewMenu, EMenuType OldMenu)
{
	ShowPage(NewMenu, OldMenu);
}

void UPHMenuRootWidget::ShowPage(const EMenuType MenuType, const EMenuType OldMenu)
{
	FMenuEntry* Entry = FindEntry(MenuType);
	if (!Entry)
	{
		PH_LOG_WARNING(LogPHMenuRootWidget,
			"ShowPage: no MenuEntry configured for type %d on %s.",
			static_cast<int32>(MenuType), *GetName());
		return;
	}

	UPHMenuPageWidgetBase* Page = GetOrCreatePage(*Entry);
	if (!Page)
	{
		return;
	}

	if (ContentSwitcher)
	{
		ContentSwitcher->SetActiveWidget(Page);
	}

	ActiveMenuType = MenuType;

	OnPageChanged(MenuType, OldMenu);
	OnMenuPageChanged.Broadcast(MenuType, OldMenu);
}

UPHMenuPageWidgetBase* UPHMenuRootWidget::GetOrCreatePage(FMenuEntry& Entry)
{
	if (Entry.CachedInstance)
	{
		return Entry.CachedInstance;
	}

	if (!Entry.WidgetClass)
	{
		PH_LOG_WARNING(LogPHMenuRootWidget,
			"GetOrCreatePage: MenuEntry '%s' (type %d) has no WidgetClass set.",
			*Entry.DisplayName.ToString(), static_cast<int32>(Entry.MenuType));
		return nullptr;
	}

	UPHMenuPageWidgetBase* Page = CreateWidget<UPHMenuPageWidgetBase>(this, Entry.WidgetClass);
	if (!Page)
	{
		PH_LOG_WARNING(LogPHMenuRootWidget,
			"GetOrCreatePage: CreateWidget failed for MenuEntry '%s'.",
			*Entry.DisplayName.ToString());
		return nullptr;
	}

	Entry.CachedInstance = Page;

	if (ContentSwitcher)
	{
		ContentSwitcher->AddChild(Page);
	}

	if (APHBaseCharacter* Character = GetBoundCharacter())
	{
		Page->InitializeForCharacter(Character);
	}

	UE_LOG(LogPHMenuRootWidget, Log,
		TEXT("GetOrCreatePage: created page '%s' for menu type %d."),
		*GetNameSafe(Page), static_cast<int32>(Entry.MenuType));

	return Page;
}

FMenuEntry* UPHMenuRootWidget::FindEntry(const EMenuType MenuType)
{
	for (FMenuEntry& Entry : MenuEntries)
	{
		if (Entry.MenuType == MenuType)
		{
			return &Entry;
		}
	}

	return nullptr;
}

const FMenuEntry* UPHMenuRootWidget::FindEntry(const EMenuType MenuType) const
{
	for (const FMenuEntry& Entry : MenuEntries)
	{
		if (Entry.MenuType == MenuType)
		{
			return &Entry;
		}
	}

	return nullptr;
}

EMenuType UPHMenuRootWidget::GetFirstValidMenuType() const
{
	for (const FMenuEntry& Entry : MenuEntries)
	{
		if (Entry.MenuType != EMenuType::MT_None && Entry.WidgetClass)
		{
			return Entry.MenuType;
		}
	}

	return EMenuType::MT_None;
}

EMenuType UPHMenuRootWidget::ResolveDefaultMenuType() const
{
	const FMenuEntry* DefaultEntry = FindEntry(DefaultMenuType);
	return DefaultEntry && DefaultEntry->WidgetClass
		? DefaultMenuType
		: GetFirstValidMenuType();
}

void UPHMenuRootWidget::BuildMenuEntriesFromEnum()
{
	const UEnum* MenuEnum = StaticEnum<EMenuType>();
	if (!MenuEnum)
	{
		return;
	}

	auto CompleteEntry = [this, MenuEnum](FMenuEntry& Entry)
	{
		if (Entry.DisplayName.IsEmpty())
		{
			Entry.DisplayName = MenuEnum->GetDisplayNameTextByValue(static_cast<int64>(Entry.MenuType));
		}

		if (!Entry.WidgetClass)
		{
			Entry.WidgetClass = DefaultPageWidgetClass;
		}
	};

	if (!bBuildHeaderFromMenuEnum)
	{
		for (FMenuEntry& Entry : MenuEntries)
		{
			CompleteEntry(Entry);
		}
		return;
	}

	TArray<FMenuEntry> OrderedEntries;
	OrderedEntries.Reserve(MenuEnum->NumEnums());

	for (int32 EnumIndex = 0; EnumIndex < MenuEnum->NumEnums(); ++EnumIndex)
	{
		const FString EnumName = MenuEnum->GetNameStringByIndex(EnumIndex);
		const bool bIsGeneratedMax = EnumName.EndsWith(TEXT("_MAX"));
		bool bIsHidden = false;
#if WITH_METADATA
		bIsHidden = MenuEnum->HasMetaData(TEXT("Hidden"), EnumIndex);
#endif

		if (bIsHidden || bIsGeneratedMax)
		{
			continue;
		}

		const int64 EnumValue = MenuEnum->GetValueByIndex(EnumIndex);
		if (EnumValue == INDEX_NONE)
		{
			continue;
		}

		const EMenuType MenuType = static_cast<EMenuType>(EnumValue);
		if (MenuType == EMenuType::MT_None)
		{
			continue;
		}

		FMenuEntry Entry;
		if (const FMenuEntry* ExistingEntry = FindEntry(MenuType))
		{
			Entry = *ExistingEntry;
		}
		else
		{
			Entry.MenuType = MenuType;
		}

		CompleteEntry(Entry);
		OrderedEntries.Add(MoveTemp(Entry));
	}

	MenuEntries = MoveTemp(OrderedEntries);
}

FReply UPHMenuRootWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (CloseMenuKeys.Contains(InKeyEvent.GetKey()))
	{
		RequestCloseMenu();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPHMenuRootWidget::RequestCloseMenu()
{
	if (const APlayerController* PC = GetOwningPlayer())
	{
		if (AHunterHUD* HunterHUD = Cast<AHunterHUD>(PC->GetHUD()))
		{
			HunterHUD->CloseMenu();
		}
	}
}

void UPHMenuRootWidget::HandleCloseButtonClicked()
{
	RequestCloseMenu();
}


void UPHMenuRootWidget::SetUpAccentMaterial()
{
	if (!ScanlineSweep)
	{
		return;
	}

	// A dynamic instance is the only way to give a UMG material per-widget
	// values; the shared material asset cannot know which panel it is drawing.
	AccentMaterial = ScanlineSweep->GetDynamicMaterial();
	if (!AccentMaterial)
	{
		return;
	}

	if (UTexture* Mask = AccentMaskTexture.LoadSynchronous())
	{
		// Confines the accents to the window silhouette. Without it the material
		// samples whatever texture it was authored against and the accents do
		// not follow the panel's shape.
		AccentMaterial->SetTextureParameterValue(AccentMaskParameter, Mask);
	}

	// Seed the aspect immediately so the first frame is not stretched; Tick only
	// corrects it afterwards if the widget resizes.
	CachedAccentSize = FVector2D::ZeroVector;
}

void UPHMenuRootWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!AccentMaterial)
	{
		return;
	}

	// A UI material has no access to its widget's pixel size, so a square in UV
	// space renders as a rectangle on any non-square panel. Pushing the real
	// aspect is what makes the accents square.
	const FVector2D Size = MyGeometry.GetLocalSize();
	if (Size.Equals(CachedAccentSize, 0.5) || Size.X <= 0.0 || Size.Y <= 0.0)
	{
		return;
	}
	CachedAccentSize = Size;

	const float Aspect = bAccentAspectIsHeightOverWidth
		? static_cast<float>(Size.Y / Size.X)
		: static_cast<float>(Size.X / Size.Y);
	AccentMaterial->SetScalarParameterValue(AccentAspectParameter, Aspect);
}

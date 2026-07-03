// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#include "Menu/Widgets/PHMenuRootWidget.h"

#include "Components/WidgetSwitcher.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Menu/Widgets/PHMenuPageWidgetBase.h"
#include "Menu/Widgets/PHMenuTabBarWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogPHMenuRootWidget, Log, All);

void UPHMenuRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TabBar)
	{
		// Bind BEFORE InitializeTabs — the bar auto-selects the first tab during
		// initialization and we need to hear that selection to show its page.
		TabBar->OnMenuTabSelected.AddUniqueDynamic(this, &UPHMenuRootWidget::HandleTabSelected);
		TabBar->InitializeTabs(MenuEntries);
	}
	else if (MenuEntries.Num() > 0)
	{
		// No tab bar in this layout — just show the first configured page.
		ShowPage(GetFirstValidMenuType(), EMenuType::MT_None);
	}

	if (MenuEntries.Num() == 0)
	{
		PH_LOG_WARNING(LogPHMenuRootWidget,
			"NativeConstruct: %s has no MenuEntries configured. Fill 'Menu Entries' in the Blueprint class defaults.",
			*GetName());
	}
}

void UPHMenuRootWidget::OpenMenu(EMenuType MenuType)
{
	if (MenuType == EMenuType::MT_None)
	{
		MenuType = (ActiveMenuType != EMenuType::MT_None)
			? ActiveMenuType            // restore whatever was open last
			: GetFirstValidMenuType();  // first open ever
	}

	if (MenuType == EMenuType::MT_None)
	{
		return;
	}

	if (TabBar)
	{
		// SelectTab dedupes, updates tab visuals, and fires OnMenuTabSelected,
		// which lands in HandleTabSelected → ShowPage.
		TabBar->SelectTab(MenuType);

		// SelectTab early-outs when the type is unchanged; make sure the page
		// exists even on the very first OpenMenu after a same-type no-op.
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

void UPHMenuRootWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	Super::NativeInitializeForCharacter(Character);

	// Rebind every page that already exists; pages created later pick the
	// character up inside GetOrCreatePage.
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

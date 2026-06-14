// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#include "Menu/Widgets/MenuRootWidget.h"

#include "Components/WidgetSwitcher.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Menu/Widgets/MenuBaseWidget.h"
#include "Menu/Widgets/MenuTabBarWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogMenuRootWidget, Log, All);

void UMenuRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TabBar)
	{
		// Bind BEFORE InitializeTabs — the bar auto-selects the first tab during
		// initialization and we need to hear that selection to show its page.
		TabBar->OnMenuTabSelected.AddUniqueDynamic(this, &UMenuRootWidget::HandleTabSelected);
		TabBar->InitializeTabs(MenuEntries);
	}
	else if (MenuEntries.Num() > 0)
	{
		// No tab bar in this layout — just show the first configured page.
		ShowPage(GetFirstValidMenuType(), EMenuType::MT_None);
	}

	if (MenuEntries.Num() == 0)
	{
		PH_LOG_WARNING(LogMenuRootWidget,
			"NativeConstruct: %s has no MenuEntries configured. Fill 'Menu Entries' in the Blueprint class defaults.",
			*GetName());
	}
}

void UMenuRootWidget::OpenMenu(EMenuType MenuType)
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

UMenuBaseWidget* UMenuRootWidget::GetActivePage() const
{
	return GetPageForMenu(ActiveMenuType);
}

UMenuBaseWidget* UMenuRootWidget::GetPageForMenu(EMenuType MenuType) const
{
	const FMenuEntry* Entry = FindEntry(MenuType);
	return Entry ? Entry->CachedInstance.Get() : nullptr;
}

void UMenuRootWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
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

void UMenuRootWidget::NativeReleaseCharacter()
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

void UMenuRootWidget::HandleTabSelected(EMenuType NewMenu, EMenuType OldMenu)
{
	ShowPage(NewMenu, OldMenu);
}

void UMenuRootWidget::ShowPage(const EMenuType MenuType, const EMenuType OldMenu)
{
	FMenuEntry* Entry = FindEntry(MenuType);
	if (!Entry)
	{
		PH_LOG_WARNING(LogMenuRootWidget,
			"ShowPage: no MenuEntry configured for type %d on %s.",
			static_cast<int32>(MenuType), *GetName());
		return;
	}

	UMenuBaseWidget* Page = GetOrCreatePage(*Entry);
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

UMenuBaseWidget* UMenuRootWidget::GetOrCreatePage(FMenuEntry& Entry)
{
	if (Entry.CachedInstance)
	{
		return Entry.CachedInstance;
	}

	if (!Entry.WidgetClass)
	{
		PH_LOG_WARNING(LogMenuRootWidget,
			"GetOrCreatePage: MenuEntry '%s' (type %d) has no WidgetClass set.",
			*Entry.DisplayName.ToString(), static_cast<int32>(Entry.MenuType));
		return nullptr;
	}

	UMenuBaseWidget* Page = CreateWidget<UMenuBaseWidget>(this, Entry.WidgetClass);
	if (!Page)
	{
		PH_LOG_WARNING(LogMenuRootWidget,
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

	UE_LOG(LogMenuRootWidget, Log,
		TEXT("GetOrCreatePage: created page '%s' for menu type %d."),
		*GetNameSafe(Page), static_cast<int32>(Entry.MenuType));

	return Page;
}

FMenuEntry* UMenuRootWidget::FindEntry(const EMenuType MenuType)
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

const FMenuEntry* UMenuRootWidget::FindEntry(const EMenuType MenuType) const
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

EMenuType UMenuRootWidget::GetFirstValidMenuType() const
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

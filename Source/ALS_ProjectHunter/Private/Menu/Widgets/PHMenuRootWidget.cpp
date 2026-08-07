#include "Menu/Widgets/PHMenuRootWidget.h"

#include "Components/WidgetSwitcher.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Menu/Widgets/PHMenuPageWidgetBase.h"
#include "Menu/Widgets/PHMenuTabBarWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogPHMenuRootWidget, Log, All);

UPHMenuRootWidget::UPHMenuRootWidget()
{
	DefaultPageWidgetClass = UPHMenuPageWidgetBase::StaticClass();
}

void UPHMenuRootWidget::NativeConstruct()
{
	Super::NativeConstruct();
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
		if (MenuEnum->HasMetaData(TEXT("Hidden"), EnumIndex))
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

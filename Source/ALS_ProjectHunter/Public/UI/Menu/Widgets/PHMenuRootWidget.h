#pragma once

#include "CoreMinimal.h"
#include "UI/HUD/HunterHUDBaseWidget.h"
#include "UI/Menu/Library/Enums/MenuEnums.h"
#include "UI/Menu/Library/Structs/MenuStructs.h"
#include "PHMenuRootWidget.generated.h"

class APHBaseCharacter;
class UPHMenuPageWidgetBase;
class UPHMenuTabBarWidget;
class UWidgetSwitcher;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMenuPageChanged, EMenuType, NewMenu, EMenuType, OldMenu);

UCLASS(Abstract, BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UPHMenuRootWidget : public UHunterHUDBaseWidget
{
	GENERATED_BODY()

public:
	UPHMenuRootWidget();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OpenMenu(EMenuType MenuType);

	UFUNCTION(BlueprintPure, Category = "Menu")
	EMenuType GetActiveMenuType() const { return ActiveMenuType; }

	UFUNCTION(BlueprintPure, Category = "Menu")
	UPHMenuPageWidgetBase* GetActivePage() const;

	UFUNCTION(BlueprintPure, Category = "Menu")
	UPHMenuPageWidgetBase* GetPageForMenu(EMenuType MenuType) const;

	/** Adds or replaces the page class used by one enum entry. */
	UFUNCTION(BlueprintCallable, Category = "Menu|Config")
	void SetMenuPageWidgetClass(EMenuType MenuType, TSubclassOf<UPHMenuPageWidgetBase> WidgetClass);

	UPROPERTY(BlueprintAssignable, Category = "Menu|Events")
	FOnMenuPageChanged OnMenuPageChanged;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Menu|Events")
	void OnPageChanged(EMenuType NewMenu, EMenuType OldMenu);

	UPROPERTY(EditDefaultsOnly, Category = "Menu|Config")
	TArray<FMenuEntry> MenuEntries;

	/** Page selected whenever the menu is opened without an explicit page. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Config")
	EMenuType DefaultMenuType = EMenuType::MT_Equipment;

	/**
	 * Builds the header in EMenuType declaration order. Existing MenuEntries
	 * provide optional labels, icons, and page-class overrides.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Config")
	bool bBuildHeaderFromMenuEnum = true;

	/** Used for newly added enum pages until a specialized page class is configured. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Config")
	TSubclassOf<UPHMenuPageWidgetBase> DefaultPageWidgetClass;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPHMenuTabBarWidget> TabBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> ContentSwitcher;

	UPROPERTY(BlueprintReadOnly, Category = "Menu")
	EMenuType ActiveMenuType = EMenuType::MT_None;

private:
	UFUNCTION()
	void HandleTabSelected(EMenuType NewMenu, EMenuType OldMenu);

	void ShowPage(EMenuType MenuType, EMenuType OldMenu);
	UPHMenuPageWidgetBase* GetOrCreatePage(FMenuEntry& Entry);
	FMenuEntry* FindEntry(EMenuType MenuType);
	const FMenuEntry* FindEntry(EMenuType MenuType) const;
	EMenuType GetFirstValidMenuType() const;
	EMenuType ResolveDefaultMenuType() const;
	void BuildMenuEntriesFromEnum();
};

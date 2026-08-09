#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Menu/Library/Enums/MenuEnums.h"
#include "UI/Menu/Library/Structs/MenuStructs.h"
#include "PHMenuTabBarWidget.generated.h"

class UPanelWidget;
class UPHMenuTabButtonWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMenuTabSelected, EMenuType, NewMenu, EMenuType, OldMenu);

UCLASS(Abstract, BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UPHMenuTabBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TabBar")
	void InitializeTabs(const TArray<FMenuEntry>& Entries, EMenuType InitialMenuType = EMenuType::MT_None);

	UFUNCTION(BlueprintCallable, Category = "TabBar")
	void SelectTab(EMenuType MenuType);

	UFUNCTION(BlueprintPure, Category = "TabBar")
	EMenuType GetActiveMenuType() const { return ActiveMenuType; }

	UFUNCTION(BlueprintCallable, Category = "TabBar|Config")
	void SetTabWidgetClass(TSubclassOf<UPHMenuTabButtonWidget> InTabWidgetClass)
	{
		TabWidgetClass = InTabWidgetClass;
	}

	UPROPERTY(BlueprintAssignable, Category = "TabBar|Events")
	FOnMenuTabSelected OnMenuTabSelected;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "TabBar|Events")
	void OnActiveTabChanged(EMenuType NewMenu, EMenuType OldMenu);

	UPROPERTY(EditDefaultsOnly, Category = "TabBar|Config")
	TSubclassOf<UPHMenuTabButtonWidget> TabWidgetClass;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> TabContainer;

	UPROPERTY(BlueprintReadOnly, Category = "TabBar")
	EMenuType ActiveMenuType = EMenuType::MT_None;

	UPROPERTY()
	TArray<TObjectPtr<UPHMenuTabButtonWidget>> SpawnedTabs;

private:
	UFUNCTION()
	void HandleTabClicked(EMenuType ClickedType);
};

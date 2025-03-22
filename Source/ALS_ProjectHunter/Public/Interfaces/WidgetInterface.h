// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "Character/PHBaseCharacter.h"
#include "Library/WidgetEnumLibrary.h"
#include "UObject/Interface.h"
#include "WidgetInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ALS_PROJECTHUNTER_API IWidgetInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Widget Manager")
	void SetActiveWidget(EWidgets Widget);
	virtual void SetActiveWidget_Implementation(EWidgets Widget){};

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Widget Manager")
	 EWidgets GetActiveWidget();
	virtual  EWidgets GetActiveWidget_Implementation(){return EWidgets::AW_None;}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Widget Manager")
	void OpenNewWidget(EWidgets Widget, bool IsStorageInArea);
	virtual void OpenNewWidget_Implementation(EWidgets Widget, bool IsStorageInArea){};
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Widget Manager")
	void CloseActiveWidget();
	virtual void CloseActiveWidget_Implementation() {};

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Widget Manager")
	void SwitchWidgetTo(const EWidgets NewWidget,EWidgetsSwitcher Tab, APHBaseCharacter* Vendor);
	virtual void SwitchWidgetTo_Implementation(const EWidgets NewWidget, EWidgetsSwitcher Tab, APHBaseCharacter* Vendor) {};

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Widget Manager")
	void SetActiveTab(EWidgetsSwitcher Tab);
	virtual void SetActiveTab_Implementation(EWidgetsSwitcher Tab) {};

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Widget Manager")
	EWidgetsSwitcher GetActiveTab();
	virtual EWidgetsSwitcher GetActiveTab_Implementation(){return EWidgetsSwitcher::WS_None;}

};

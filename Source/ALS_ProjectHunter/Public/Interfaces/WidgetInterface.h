// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "Character/AI/PHAICharacter.h"
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

	virtual void SetActiveWidget(EWidgets Widget){};
	virtual EWidgets GetActiveWidget(){return EWidgets::AW_None;}
	virtual void CloseActiveWidget() {};
	virtual void OpenNewWidget(EWidgets Widget){};
	virtual void SwitchWidgetTo(const EWidgets NewWidget, APHAICharacter* Vendor) {};
	virtual void SetActiveTab(EWidgets Tab) {};
	virtual EWidgets GetActiveTab(){return EWidgets::AW_None;}
	virtual void SwitchTabTo(EWidgets NewTab) {};
};

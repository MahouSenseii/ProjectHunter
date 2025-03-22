#pragma once
#include "WidgetEnumLibrary.h"
#include "UI/Widgets/MenuButton.h"
#include "WidgetStructLibrary.generated.h"

USTRUCT(BlueprintType)
struct FMenuWidgetStuct
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	TSubclassOf<UMenuButton> MenuButtonClass;
	
	UPROPERTY()
	UMenuButton* MenuButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	EWidgets Widget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	EWidgetsSwitcher WidgetsSwitcher;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	FString MenuText;

	// Default constructor using an initializer list
	FMenuWidgetStuct()
		: MenuButtonClass(nullptr)
		, MenuButton(nullptr)
		, Widget(EWidgets::AW_None)
		, WidgetsSwitcher(EWidgetsSwitcher::WS_None)
		, MenuText("Default Text")
	{
	}
};


USTRUCT(BlueprintType)
struct FMenuButtonArray
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(BlueprintReadOnly) // or EditAnywhere, etc.
	TArray<TObjectPtr<UMenuButton>> Buttons;
};
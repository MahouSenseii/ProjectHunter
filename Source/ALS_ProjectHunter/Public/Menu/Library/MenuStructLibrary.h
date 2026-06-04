#pragma once

#include "CoreMinimal.h"
#include "Menu/Library/MenuEnumLibrary.h"
#include "MenuStructLibrary.generated.h"

class UMenuBaseWidget;
class UTexture2D;

USTRUCT(BlueprintType)
struct FMenuEntry
{
	GENERATED_BODY()

	/** Which menu this entry represents. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	EMenuType MenuType = EMenuType::MT_None;

	/** Label shown on the tab button. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	FText DisplayName;

	/** Icon shown on the tab button. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** Widget class to spawn when this menu is first opened. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TSubclassOf<UMenuBaseWidget> WidgetClass;

	/** Cached live instance — null until first open, then reused. */
	UPROPERTY(Transient)
	TObjectPtr<UMenuBaseWidget> CachedInstance = nullptr;
};





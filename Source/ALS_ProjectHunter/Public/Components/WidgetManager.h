// Copyright@2024 Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/WidgetInterface.h"
#include "WidgetManager.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UWidgetManager : public UActorComponent, public IWidgetInterface
{
	GENERATED_BODY()

public:
	/* ============================= */
	/* === Initialization === */
	/* ============================= */

	// Sets default values for this component's properties
	UWidgetManager();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/* ============================= */
	/* === Widget Management === */
	/* ============================= */

public:
	UFUNCTION(BlueprintCallable)
	void WidgetCheck(APHBaseCharacter* Owner);

	virtual void SetActiveWidget_Implementation(EWidgets Widget) override;
	virtual EWidgets GetActiveWidget_Implementation() override;
	virtual void OpenNewWidget_Implementation(EWidgets Widget, bool bIsStorageInArea) override;
	virtual void CloseActiveWidget_Implementation() override;
	virtual void SwitchWidgetTo_Implementation(const EWidgets NewWidget, EWidgetsSwitcher Tab, APHBaseCharacter* Vendor) override;
	virtual void SetActiveTab_Implementation(EWidgetsSwitcher Tab) override;
	virtual EWidgetsSwitcher GetActiveTab_Implementation() override;

protected:
	/* ============================= */
	/* === Widget Properties === */
	/* ============================= */

	// Reference to the owner character
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APHBaseCharacter> OwnerCharacter = nullptr;

	// The currently active widget type
	UPROPERTY()
	EWidgets ActiveWidget = EWidgets::AW_None;

	// The currently active tab type
	UPROPERTY()
	EWidgetsSwitcher ActiveTab = EWidgetsSwitcher::WS_None;

	// Holds the currently displayed widget instance
	UPROPERTY()
	TObjectPtr<UPHUserWidget> CurrentWidgetInstance = nullptr;

	// Optional: Keep track if the current widget is for storage.
	bool bStorageInArea = false;

	// Stores a reference to a saved widget
	UPROPERTY()
	TObjectPtr<UPHUserWidget> SavedWidget;

	// Map of widget types to widget classes
	UPROPERTY(EditDefaultsOnly, Category = "Widget Manager")
	TMap<EWidgets, TSubclassOf<UPHUserWidget>> WidgetClassMap;

	// Reference to vendor character, if applicable
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APHBaseCharacter> VendorRef;

private:
	/* ============================= */
	/* === Input Handling === */
	/* ============================= */

	// Helper function to handle input modes
	void ConfigureInputForUI(UPHUserWidget* InWidget) const;
	void ConfigureInputForGame() const;

	// Resolves the widget class based on the widget type
	TSubclassOf<UPHUserWidget> ResolveWidgetClass(EWidgets Widget) const;
};
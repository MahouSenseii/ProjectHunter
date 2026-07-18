#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Combat/Library/Structs/CombatStructs.h"
#include "HunterDamagePopupWidget.generated.h"

class UWidgetComponent;

UCLASS(Abstract, BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UHunterDamagePopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HUD|Damage Popup")
	void InitializeDamagePopup(const FCombatDamagePopupData& InPopupData);

	UFUNCTION(BlueprintPure, Category = "HUD|Damage Popup")
	FCombatDamagePopupData GetDamagePopupData() const { return PopupData; }

	UFUNCTION(BlueprintPure, Category = "HUD|Damage Popup")
	FText GetDamageText() const;

	UFUNCTION(BlueprintPure, Category = "HUD|Damage Popup")
	FLinearColor GetDamageColor() const { return PopupData.DisplayColor; }

	UFUNCTION(BlueprintCallable, Category = "HUD|Damage Popup")
	void FinishDamagePopup();

	void SetOwningWorldWidgetComponent(UWidgetComponent* InWidgetComponent);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Damage Popup")
	void OnDamagePopupInitialized(const FCombatDamagePopupData& InPopupData);

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Damage Popup")
	FCombatDamagePopupData PopupData;

	UPROPERTY(Transient)
	TWeakObjectPtr<UWidgetComponent> OwningWorldWidgetComponent;
};

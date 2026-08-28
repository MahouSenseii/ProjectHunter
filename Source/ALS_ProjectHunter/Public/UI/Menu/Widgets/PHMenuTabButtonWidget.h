#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Library/PHUIStyle.h"
#include "UI/Menu/Library/Enums/MenuEnums.h"
#include "UI/Menu/Library/Structs/MenuStructs.h"
#include "PHMenuTabButtonWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTabClicked, EMenuType, MenuType);

UCLASS(Abstract, BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UPHMenuTabButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Tab")
	void SetTabData(const FMenuEntry& Entry);

	UFUNCTION(BlueprintCallable, Category = "Tab")
	void SetSelected(bool bInSelected);

	UFUNCTION(BlueprintPure, Category = "Tab")
	EMenuType GetMenuType() const { return MenuType; }

	UFUNCTION(BlueprintPure, Category = "Tab")
	bool IsSelected() const { return bIsSelected; }

	UPROPERTY(BlueprintAssignable, Category = "Tab|Events")
	FOnTabClicked OnTabClicked;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Tab|Events")
	void OnTabHovered();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tab|Events")
	void OnTabUnhovered();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tab|Events")
	void OnTabSelected();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tab|Events")
	void OnTabDeselected();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> TabButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> TabIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TabLabel;

	UPROPERTY(BlueprintReadOnly, Category = "Tab")
	EMenuType MenuType = EMenuType::MT_None;

	UPROPERTY(BlueprintReadOnly, Category = "Tab")
	bool bIsSelected = false;

	/**
	 * System-window palette. The selected tab inverts to a near-white plate with
	 * dark text, which is how the reference art marks the active window - the
	 * unselected tabs stay as translucent glass so the inversion reads instantly.
	 *
	 * Linear values, not sRGB: the azure is #2E9BE0 converted.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tab|Style")
	FLinearColor NormalColor = FLinearColor(PHUIStyle::AzureDeep.R, PHUIStyle::AzureDeep.G, PHUIStyle::AzureDeep.B, 0.55f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tab|Style")
	FLinearColor HoveredColor = FLinearColor(PHUIStyle::Azure.R, PHUIStyle::Azure.G, PHUIStyle::Azure.B, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tab|Style")
	FLinearColor SelectedColor = FLinearColor(0.900f, 0.960f, 1.000f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tab|Style")
	FLinearColor NormalTextColor = PHUIStyle::TextPrimary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tab|Style")
	FLinearColor SelectedTextColor = PHUIStyle::AzureDeep;

private:
	void ApplySelectionStyle();

	UFUNCTION()
	void HandleButtonClicked();

	UFUNCTION()
	void HandleButtonHovered();

	UFUNCTION()
	void HandleButtonUnhovered();
};

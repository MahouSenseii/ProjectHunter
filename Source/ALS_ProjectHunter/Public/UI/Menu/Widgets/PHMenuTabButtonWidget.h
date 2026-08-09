#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tab|Style")
	FLinearColor NormalColor = FLinearColor(0.02f, 0.12f, 0.16f, 0.88f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tab|Style")
	FLinearColor HoveredColor = FLinearColor(0.10f, 0.55f, 0.65f, 0.90f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tab|Style")
	FLinearColor SelectedColor = FLinearColor(0.12f, 0.78f, 0.88f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tab|Style")
	FLinearColor NormalTextColor = FLinearColor(0.78f, 0.93f, 0.96f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tab|Style")
	FLinearColor SelectedTextColor = FLinearColor::White;

private:
	void ApplySelectionStyle();

	UFUNCTION()
	void HandleButtonClicked();

	UFUNCTION()
	void HandleButtonHovered();

	UFUNCTION()
	void HandleButtonUnhovered();
};

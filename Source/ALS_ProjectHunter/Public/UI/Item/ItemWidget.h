// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Widgets/PHUserWidget.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/Button.h"

#include "ItemWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemoved, UBaseItem*, RemovedItem);

/**
 * UToolTip class represents a tooltip in the user interface.
 */

class UToolTip;
class UInventoryManager;
class UBaseItem;
class UCanvasPanel;
class UEquippableToolTip;
class UConsumableToolTip;

UCLASS()
class ALS_PROJECTHUNTER_API UItemWidget : public UPHUserWidget
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnRemoved OnRemoved;

	UPROPERTY(BlueprintReadOnly, Category = " Components")
	TObjectPtr<UInventoryManager> OwnerInventory;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UCanvasPanel* CanvasPanel;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UBorder* BackgroundBorder;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* BackgroundSizeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UButton* BtnItem;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UImage* ItemImage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UBaseItem> ItemObject;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TileSize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector2D Size;

	UPROPERTY()
	UMaterialInstanceDynamic* CachedDynamicMaterial = nullptr;

	/** UI - ToolTips */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "ToolTip", meta = (AllowPrivateAccess = "true"))
	UUserWidget* CurrentToolTip;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ToolTip", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UEquippableToolTip> EquippableToolTipClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ToolTip", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UConsumableToolTip> ConsumableToolTipClass;
	
public:

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	// Overridden Native functions
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	void InitializeComponents();

	UFUNCTION(BlueprintPure, Category = "Icon")
	FSlateBrush GetIcon() const;

	void
	Refresh();

	UFUNCTION(BlueprintCallable, Category = "Mouse Event")
	void EventHovered();
	void CreateToolTip();

	UFUNCTION(BlueprintCallable, Category = "Mouse Event")
	void EventMouseUnHovered();
	void RemoveToolTip();
	FVector2D CalculateTooltipPosition() const;
	float GetTooltipWidth() const;
	float GetTooltipHeight() const;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/PHBaseCharacter.h"
#include "UI/InteractableWidget.h"
#include "UI/Widgets/PHUserWidget.h"
#include "PHInventoryWidget.generated.h"

class UInventoryGrid;
/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHInventoryWidget : public UPHUserWidget
{
	GENERATED_BODY()

public:
	
	virtual void NativeConstruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	                          UDragDropOperation* InOperation) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool NativeSupportsKeyboardFocus() const override;
	void InitializeOwner();
	virtual bool InitializeWidget(APHBaseCharacter* InOwnerCharacter);
	
	void SetGrid() const;
	void SetupProperties() const;

	UClass* WidgetClass;
	UFUNCTION()
	virtual UInventoryManager* GetInventoryManager() const;
	
	UPROPERTY(BlueprintReadOnly, Category = "Owner")
	TObjectPtr<APHBaseCharacter> Owner;

	FKey MenuToggleKeyBinding = EKeys::O;

	UPROPERTY(BlueprintReadOnly, Category = "Owner")
	TObjectPtr<APlayerController> LocalPlayerController;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UInventoryGrid* InventoryGrid;
	
	UPROPERTY(BlueprintReadWrite)
	UEquipmentManager* EquipmentManager;
	
};

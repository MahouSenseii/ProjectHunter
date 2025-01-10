// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/PHInventoryWidget.h"

#include "Blueprint/DragDropOperation.h"
#include "Components/InventoryManager.h"
#include "UI/Inventory/InventoryGrid.h"

void UPHInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if(!Owner)
	{
		// Initialize the owner.
		InitializeOwner();
		InitializeWidget(Owner);
	}
}

bool UPHInventoryWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	
	const FGeometry InventoryGridGeometry = InventoryGrid->GetCachedGeometry();
	const FVector2D DropPosition = InDragDropEvent.GetScreenSpacePosition();
	// Check if the drop position is within the bounds of the InventoryGrid
	if (InventoryGridGeometry.IsUnderLocation(DropPosition))
	{
		return true;
	}
	
	if (Owner && GetInventoryManager())
	{
		UBaseItem* DroppedItem = Cast<UBaseItem>(InOperation->Payload);
		GetInventoryManager()->DropItemInInventory(DroppedItem);
		return  true;
	}
	return false;
}

FReply UPHInventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

bool UPHInventoryWidget::NativeSupportsKeyboardFocus() const
{
	return true;
}

void UPHInventoryWidget::InitializeOwner()
{
	Owner = Cast<APHBaseCharacter>(GetOwningPlayerPawn());

	// Check if the Owner is valid.
	if (Owner)
	{
		LocalPlayerController = Cast<APlayerController>(Owner->GetController());
		UE_LOG(LogTemp, Warning, TEXT("Owner successfully set."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to set Owner. Make sure the owning pawn is of type AALSBaseCharacter."));
	}
}

bool UPHInventoryWidget::InitializeWidget(APHBaseCharacter* InOwnerCharacter)
{
	if(Owner)
	{
		Owner = InOwnerCharacter;
		// Initialize the grid.
		SetupProperties();
		SetGrid();
		return true;
	}
	return false;
}

void UPHInventoryWidget::SetGrid() const
{
	if (Owner)
	{
		InventoryGrid->OwnerInventory = GetInventoryManager();
		InventoryGrid->SetOwningPlayer(LocalPlayerController);
		InventoryGrid->GridInitialize(GetInventoryManager(), GetInventoryManager()->TileSize);
	}
}

void UPHInventoryWidget::SetupProperties() const
{
}

UInventoryManager* UPHInventoryWidget::GetInventoryManager() const
{
	// Assuming Owner is an AActor* and UInventoryManager is a UActorComponent subclass
	if (Owner)
	{
		return Cast<UInventoryManager>(Owner->GetComponentByClass(UInventoryManager::StaticClass()));
	}
	return nullptr; // Return nullptr if Owner is null or the component is not found
}




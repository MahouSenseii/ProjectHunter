// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/WidgetManager.h"

#include "Character/ALSPlayerController.h"
#include "Components/CanvasPanelSlot.h"

/* ============================= */
/* === Initialization Header === */
/* ============================= */

UWidgetManager::UWidgetManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWidgetManager::BeginPlay()
{
	Super::BeginPlay();
}

/* ============================= */
/* === Widget Management === */
/* ============================= */

void UWidgetManager::WidgetCheck(APHBaseCharacter* Owner)
{
	check(Owner);
	OwnerCharacter = Owner;

	if (Execute_GetActiveWidget(this) != EWidgets::AW_None)
	{
		UE_LOG(LogTemp, Log, TEXT("WidgetCheck: Found active widget %d"), static_cast<int32>(Execute_GetActiveWidget(this)));
		Execute_CloseActiveWidget(this);
	}
	else
	{
		Execute_SwitchWidgetTo(this, EWidgets::AW_Equipment, EWidgetsSwitcher::WS_MainTab, nullptr);
	}
	
}


void UWidgetManager::SetActiveWidget_Implementation(const EWidgets Widget)
{
	ActiveWidget = Widget;
}

EWidgets UWidgetManager::GetActiveWidget_Implementation()
{
	return ActiveWidget;
}

void UWidgetManager::OpenNewWidget_Implementation(EWidgets Widget, const bool bIsStorageInArea)
{
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("UWidgetManager::OpenNewWidget: OwnerCharacter is null"));
		return;
	}

	Execute_CloseActiveWidget(this);
	Execute_SetActiveWidget(this, Widget);
	bStorageInArea = bIsStorageInArea;

	if (const TSubclassOf<UPHUserWidget> WidgetClass = ResolveWidgetClass(Widget); WidgetClass)
	{
		check(WidgetClass);
		if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()); IsValid(PC))
		{
			if (UPHUserWidget* NewWidget = CreateWidget<UPHUserWidget>(PC, WidgetClass); IsValid(NewWidget))
			{
				CurrentWidgetInstance = NewWidget;
				CurrentWidgetInstance->OwnerCharacter = OwnerCharacter;
				CurrentWidgetInstance->Vendor = VendorRef;

				if (AALSPlayerController* AlsPC = Cast<AALSPlayerController>(PC))
				{
					CurrentWidgetInstance->SetWidgetController(AlsPC);
				}

				CurrentWidgetInstance->AddToViewport();

				if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(CurrentWidgetInstance->Slot))
				{
					CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
					CanvasSlot->SetOffsets(FMargin(0.f));
				}
				ConfigureInputForUI(CurrentWidgetInstance);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No widget class found for widget: %d"), static_cast<int32>(Widget));
	}
}

void UWidgetManager::CloseActiveWidget_Implementation()
{
	Execute_SetActiveTab(this, EWidgetsSwitcher::WS_None);
	Execute_SetActiveWidget(this, EWidgets::AW_None);

	if (IsValid(CurrentWidgetInstance))
	{
		CurrentWidgetInstance->RemoveFromParent();
		CurrentWidgetInstance = nullptr;
		ConfigureInputForGame();
	}
}

void UWidgetManager::SwitchWidgetTo_Implementation(const EWidgets NewWidget, EWidgetsSwitcher Tab, APHBaseCharacter* Vendor)
{
	if (Execute_GetActiveWidget(this) != EWidgets::AW_None)
	{
		Execute_CloseActiveWidget(this);
	}

	if (Vendor)
	{
		VendorRef = Vendor;
	}

	if (NewWidget != EWidgets::AW_None)
	{
		Execute_OpenNewWidget(this, NewWidget, false);
		Execute_SetActiveTab(this, Tab);
	}
}

void UWidgetManager::SetActiveTab_Implementation(const EWidgetsSwitcher Tab)
{
	ActiveTab = Tab;
}

EWidgetsSwitcher UWidgetManager::GetActiveTab_Implementation()
{
	return ActiveTab;
}

/* ============================= */
/* === Input Handling === */
/* ============================= */

void UWidgetManager::ConfigureInputForUI(UPHUserWidget* InWidget) const
{
	if (!OwnerCharacter) return;

	if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
	{
		FInputModeGameAndUI InputMode;

		if (IsValid(InWidget))
		{
			const TSharedRef<SWidget> WidgetRef = InWidget->TakeWidget();
			InputMode.SetWidgetToFocus(WidgetRef);
		}
		
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}



void UWidgetManager::ConfigureInputForGame() const
{
	if (!OwnerCharacter) return;
	if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}

/* ============================= */
/* === Widget Class Resolution === */
/* ============================= */

TSubclassOf<UPHUserWidget> UWidgetManager::ResolveWidgetClass(const EWidgets Widget) const
{
	if (const TSubclassOf<UPHUserWidget>* FoundClass = WidgetClassMap.Find(Widget))
	{
		return *FoundClass;
	}
	return nullptr;
}


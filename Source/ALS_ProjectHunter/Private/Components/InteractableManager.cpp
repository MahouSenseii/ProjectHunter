// Copyright@2024 Quentin Davis 


#include "Components/InteractableManager.h"

#include "EnhancedInputSubsystems.h"

#include "Components/TextBlock.h"
#include "Interfaces/InteractionProcessInterface.h"
#include "Library/InteractionEnumLibrary.h"
#include "InputCoreTypes.h"
#include "Character/Player/PHPlayerController.h"
#include "Interfaces/InteractableObjectInterface.h"
#include "UI/InteractableWidget.h"

class UEnhancedInputLocalPlayerSubsystem;

UInteractableManager::UInteractableManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	static ConstructorHelpers::FClassFinder<UInteractableWidget> WidgetFinder(
	TEXT("/Game/PeojectHunter/UI/PopUps/WBP_InteractableWidget")
);

	if (WidgetFinder.Succeeded())
	{
		WidgetClass = WidgetFinder.Class;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find widget class at the specified path."));
	}
}

void UInteractableManager::BeginPlay()
{
	Super::BeginPlay();
	//Call Event Initialize and set appropriate Tags to Component Owners.
	SetIsReplicated(true);
	Initialize();
	GetOwner()->Tags.AddUnique(InteractableTag);
	if(DestroyAfterInteract)
	{
		GetOwner()->Tags.AddUnique(DestroyableTag);
	}
}

void UInteractableManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void UInteractableManager::Initialize()
{
	if (GetOwner()->GetClass()->ImplementsInterface(UInteractableObjectInterface::StaticClass()))
	{
		IInteractableObjectInterface::Execute_BPIInitialize(GetOwner());
	}
}

void UInteractableManager::SetupInteractableReferences(USphereComponent*& InInteractableArea,UWidgetComponent*& InInteractionWidget, const TSet<UPrimitiveComponent*>& InHighlightableObject)
{
	if (!IsValid(InInteractableArea) || !IsValid(InInteractionWidget) || InHighlightableObject.Num() == 0)
	{UE_LOG(LogTemp, Error, TEXT("One or more input parameters are null or empty.")); return;}

	InteractableArea = InInteractableArea;
	InteractionWidget = InInteractionWidget;
	SetupHighlightableObjects(InHighlightableObject);
}

void UInteractableManager::SetupHighlightableObjects(TSet<UPrimitiveComponent*> InHighlightableObject)
{
	// Clear any existing HighlightableObjects
	ObjectsToHighlight.Empty();

	// Loop through each component in the incoming set
	for (UPrimitiveComponent* Component : InHighlightableObject)
	{
		// Check if the component is valid before proceeding
		if (IsValid(Component))
		{
			// Set collision properties and add to HighlightableObjects
			Component->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
			ObjectsToHighlight.Add(Component);
		}
		else
		{
			// Log a warning but continue processing other components
			UE_LOG(LogTemp, Warning, TEXT("Invalid Component in Highlightable Objects."));
			continue;
		}
	}
	
}

void UInteractableManager::ToggleHighlight(bool Highlight, AActor* Interactor)
{
	InteractionWidget->SetVisibility(Highlight);
	for (UPrimitiveComponent* Component : ObjectsToHighlight)
	{
		Component->SetRenderCustomDepth(Highlight);
	}
	if (APHPlayerController* AlsPlayerController = Cast<APHPlayerController>(Interactor))
	{
		SetWidgetLocalOwner(AlsPlayerController);
	}
}

void UInteractableManager::SetWidgetLocalOwner(APlayerController* OwnerPlayerController)
{
	// Ensure the player controller is a local player controller
	if (OwnerPlayerController && OwnerPlayerController->IsLocalPlayerController())
	{
		InteractionWidgetRef = CreateWidget<UInteractableWidget>(OwnerPlayerController, WidgetClass);
		if (InteractionWidgetRef)
		{
			InteractionWidgetRef->Text = InteractionText;
			InteractionWidgetRef->InteractableManager = this;
			InteractionWidgetRef->InteractionDescription->SetText(InteractionText);

			InteractionWidgetRef->InputType = InputType;
			
			if (InteractionWidget)
			{
				InteractionWidget->SetWidget(InteractionWidgetRef);
				InteractionWidget->SetOwnerPlayer(OwnerPlayerController->GetLocalPlayer());
			}
		}
	}
}


void UInteractableManager::PreInteraction(AActor* Interactor)
{
	RefInteractor = Interactor;
	if (GetOwner()->GetClass()->ImplementsInterface(UInteractableObjectInterface::StaticClass()))
	{
		IInteractableObjectInterface::Execute_BPIClientPreInteraction(GetOwner(), Interactor);
	}
	
	switch (InputType)
	{
	case EInteractType::Single:
		if(Interactor->GetClass()->ImplementsInterface(UInteractionProcessInterface::StaticClass()))
		{
			IInteractionProcessInterface::Execute_StartInteractionWithObject(Interactor,this, false);
		}
		break;
	case EInteractType::Holding:
		DurationPress(Interactor);
		break;
	case EInteractType::Mashing:
		MultiplePress();
		break;
	}
}

void UInteractableManager::DurationPress(AActor* Interactor)
{
	// Initialize the timer to periodically check the key status.
	const UWorld* World = GetWorld();
	check(World);
	if (const FTimerManager& TimerManager = World->GetTimerManager(); !TimerManager.IsTimerActive(KeyDownTimer))
	{
		World->GetTimerManager().SetTimer(KeyDownTimer, this, &UInteractableManager::IsKeyDown, 0.05f, true);
	}
	
	if (!PressedInteractionKey.IsValid())
	{
		PressedInteractionKey = EKeys::E; // replace this later to use variable not set key .
	}

	// Attempt to update the PressedInteractionKey if a specific action is detected.
	if (FKey TempKey; GetPressedByAction(ActionToCheckForInteract, Interactor, TempKey))
	{
		// Update the PressedInteractionKey with the one detected by GetPressedByAction.
		PressedInteractionKey = TempKey;
	}
}


void UInteractableManager::IsKeyDown()
{
	// First, check if RefInteractor is valid and implements the interaction interface.
	if (RefInteractor && RefInteractor->GetClass()->ImplementsInterface(UInteractionProcessInterface::StaticClass()))
	{
		// Attempt to get the current interactable object from the RefInteractor.

		const APlayerController* PlayerController = Cast<APlayerController>(RefInteractor);
		if(!PlayerController->IsInputKeyDown(PressedInteractionKey)){ ClearInteractionTimer(); return;}
		
		// Validate the found object and check if it's the same as this manager's owner.
		if (const AActor* FoundInteractableObject = IInteractionProcessInterface::Execute_GetCurrentInteractableObject(RefInteractor);
			IsValid(FoundInteractableObject) && GetOwner() == FoundInteractableObject)
		{
			HoldingInputHandle();
		}
		else
		{
			// If the object is not valid or doesn't match, clear the timer.
			ClearInteractionTimer();
		}
			
	}
	else
	{
		// If RefInteractor is not valid or does not implement the required interface, clear the timer.
		ClearInteractionTimer();
	}
}

;

void UInteractableManager::ClearInteractionTimer()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(KeyDownTimer);
	}
}


bool UInteractableManager::GetPressedByAction(const UInputAction* Action, AActor* Interactor, FKey& OutKey)
{
	const APlayerController* PlayerController = Cast<APlayerController>(Interactor);
	if (PlayerController == nullptr || Action == nullptr) 
	{
		return false; // Return if Controller or Action is null 
	}

	const UEnhancedInputLocalPlayerSubsystem* EnhancedInputSubsystem = PlayerController->GetLocalPlayer()->GetSubsystem<
		UEnhancedInputLocalPlayerSubsystem>();
	if (EnhancedInputSubsystem == nullptr) 
	{ 
		return false; // Return if Subsystem is null
	}

	TArray<FKey> KeysMappedToAction = EnhancedInputSubsystem->QueryKeysMappedToAction(Action);
	for (int32 Index = 0; Index < KeysMappedToAction.Num(); ++Index)
	{
		if (const FKey CurrentKey = KeysMappedToAction[Index]; PlayerController->WasInputKeyJustPressed(CurrentKey))
		{
			OutKey = CurrentKey;
			return true; // Key was just pressed, return true
		}
	}
    
	return false; // No keys were just pressed, return false
}

void UInteractableManager::HoldingInputHandle() 
{
	if (const APlayerController* PlayerController = Cast<APlayerController>(RefInteractor); !PlayerController)
	{
		return;  // Failed to cast to APlayerController.
	}
	if(float HeldDownTime = 0.0f; GetKeyTimeDown(HeldDownTime))
	{
		const float FillValue = (HeldDownTime > MaxKeyTimeDown) ? 0.05f : HeldDownTime;
		OnUpdateHoldingValue.Broadcast(FillValue);
		if(HeldDownTime > MaxKeyTimeDown)
		{
			IInteractionProcessInterface::Execute_StartInteractionWithObject(RefInteractor, this, true);
		}
	}
	else
	{
		OnUpdateHoldingValue.Broadcast(0.0f);
	}

}

bool UInteractableManager::GetKeyTimeDown(float& TimeDown) const
{
	const APlayerController* PlayerController = Cast<APlayerController>(RefInteractor);
	TimeDown = PlayerController->GetInputKeyTimeDown(PressedInteractionKey);
	return  TimeDown > 0.0f;
}

void UInteractableManager::MultiplePress()
{
	float ReturnValue = 0.0f; // Initialize ReturnValue
	if (MashingInput(ReturnValue))
	{
		// Execute OnUpdateMashingValue regardless of whether OnUpdateHoldingValue is bound
		OnUpdateMashingValue.Broadcast(ReturnValue);

		// If OnUpdateHoldingValue is NOT bound, delay the interaction.
		if (!OnUpdateHoldingValue.IsBound())
		{
			DelayedStartInteraction();
		}
	}
}

void UInteractableManager::DelayedStartInteraction()
{
	// Make sure to check if RefInteractor is still valid
	if(RefInteractor )
	{
		
		IInteractionProcessInterface::Execute_StartInteractionWithObject(RefInteractor, this, false);
	}
}

bool UInteractableManager::MashingInput(float& Value)
{
		MashingProgress += 1.0f / FMath::Max(1, MashingAmount); // Ensure no division by zero
		MashingProgress = FMath::Clamp(MashingProgress, 0.0f, 1.0f);
		if (MashingProgress >= 1.0f)
		{
			Value = MashingProgress; // Assign the progress before resetting.
			OnUpdateMashingValue.Broadcast(MashingProgress);
			MashingProgress = 0.0f; // Reset progress for next time.
			GetWorld()->GetTimerManager().ClearTimer(MashingDecreaseTimerHandle); // Stop the decrease timer as the mashing was successful.
			return true; // Completed.
		}
		else if (MashingProgress == 0.0f)
		{
			OnUpdateMashingValue.Broadcast(MashingProgress);
			return false;
		}
		else 
		{
			Value = MashingProgress; // Update the value with current progress.
			OnUpdateMashingValue.Broadcast(MashingProgress);
			// Restart or trigger the timer to decrease mashing progress over time.
			if (!GetWorld()->GetTimerManager().IsTimerActive(MashingDecreaseTimerHandle))
			{
				GetWorld()->GetTimerManager().SetTimer(MashingDecreaseTimerHandle, this, &UInteractableManager::DecreaseMashingProgress, MashingKeyRetriggerableTime, true);
			}

			return false;
		}
}

void UInteractableManager:: Interaction(AActor* Interactor, bool WasHeld)
{
	RefInteractor = Interactor;
	AlreadyInteracted = true;
	if (GetOwner()->GetClass()->ImplementsInterface(UInteractableObjectInterface::StaticClass()))
	{
		IInteractableObjectInterface::Execute_BPIInteraction(GetOwner(), Interactor,WasHeld);
		//RemoveInteractionByResponse();
	}
}

void UInteractableManager::ClientInteraction(AActor* Interactor, bool WasHeld) const
{
	if (GetOwner()->Implements<UInteractableObjectInterface>())
	{
		IInteractableObjectInterface::Execute_BPIClientStartInteraction(GetOwner(), Interactor, WasHeld);
	}
}

void UInteractableManager::AssociatedActorInteraction(AActor* Interactor)
{
	RefInteractor = Interactor;
	TArray<AActor*> AIAArray;
	AssociatedInteractableActors.GetKeys(AIAArray);
	for(const AActor* CurrentActor : AIAArray)
	{
		if(IsValid(CurrentActor))
		{
			if(UInteractableManager* ReturnManager =  CurrentActor->GetComponentByClass<UInteractableManager>())
			{
				ReturnManager->CheckForInteractionWithAssociate(Interactor);
			}
		}
	}
}

void UInteractableManager::CheckForInteractionWithAssociate(AActor* Interactor)
{
	RefInteractor = Interactor;
	if(CheckForAssociatedActors)
	{
		if(IsTargetInteractableValue())
		{
			Interaction(RefInteractor, false);
			if(RemoveAssociatedInteractablesOnComplete)
			{
	
			}
			switch (InteractableResponse)
			{
			case EInteractableResponseType::Temporary:
			case EInteractableResponseType::OnlyOnce:
			case EInteractableResponseType::Persistent:
			default:
				break;
			}
		}
	}
}

bool UInteractableManager::IsTargetInteractableValue()
{
	TArray<AActor*> AIAArray;
	AssociatedInteractableActors.GetKeys(AIAArray); // Populate the array with actor keys from the map.

	for (const AActor* CurrentActor : AIAArray)
	{
		// Ensure the current actor is valid before proceeding.
		if (CurrentActor)
		{
			// Check if a value was found for the actor and the actor has the Interactable component.
			if (const int* CurrentActorsValue = AssociatedInteractableActors.Find(CurrentActor);
				CurrentActorsValue && CurrentActor->GetComponentByClass(UInteractableManager::StaticClass()))
			{
				// Ensure the cast was successful before accessing the component.
				if (const UInteractableManager* InteractableComponent = Cast<UInteractableManager>(
					CurrentActor->GetComponentByClass(UInteractableManager::StaticClass())); InteractableComponent && *CurrentActorsValue == InteractableComponent->InteractableValue)
				{
					return true; // Found a matching actor, no need to check the rest.
				}
			}
		}
	}

	return false; // No matching actors found.
}

void UInteractableManager::AssociateActorRemoveHandle(bool IsTemp) const
{
	TArray<AActor*> AIAArray;
	AssociatedInteractableActors.GetKeys(AIAArray);
	for (const AActor* CurrentActor : AIAArray)
	{
		UInteractableManager* ReturnInteractableManager = Cast<UInteractableManager>(CurrentActor->GetComponentByClass(UInteractableManager::StaticClass()));
		ReturnInteractableManager->RemoveInteraction();

		if(IsTemp)
		{
			ReturnInteractableManager->ToggleCanBeReInitialized(false);
		}
		else
		{
			ReturnInteractableManager->InteractableResponse = EInteractableResponseType::OnlyOnce;
		}
	}

}

void UInteractableManager::EndInteraction(AActor* Interactor) const
{
	if(GetOwner()->Implements<UInteractableObjectInterface>())
	{
		IInteractableObjectInterface::Execute_BPIEndInteraction( GetOwner(), Interactor);
	}
}

void UInteractableManager::ClientEndInteraction(AActor* Interactor) const
{
	if(GetOwner()->Implements<UInteractableObjectInterface>())
	{
		IInteractableObjectInterface::Execute_BPIClientEndInteraction( GetOwner(), Interactor);
	}
}

void UInteractableManager::AssociatedActorEndInteraction() const
{
	TArray<AActor*> AIAArray;
	AssociatedInteractableActors.GetKeys(AIAArray);
	for (const AActor* CurrentActor : AIAArray)
	{
		if(const UInteractableManager* ReturnInteractableManager =
			Cast<UInteractableManager>(CurrentActor->GetComponentByClass(UInteractableManager::StaticClass()));
			IsValid(CurrentActor) && IsValid(ReturnInteractableManager))
		{
			ReturnInteractableManager->EndInteraction(RefInteractor);
		}
	}
}

void UInteractableManager::RemoveInteraction_Implementation()
{
	ClientRemoveInteraction();
}

void UInteractableManager::ClientRemoveInteraction()
{
	IInteractableObjectInterface::Execute_BPIRemoveInteraction(GetOwner());
	if(DestroyAfterInteract)
	{
		
		GetOwner()->Destroy();
	}
	else if (!CanBeReInitialized )
	{
		IsInteractable = false;
	}
	
}

void UInteractableManager::Re_Initialize()
{
	if(InteractableResponse == EInteractableResponseType::Temporary && CanBeReInitialized)
	{
		IInteractableObjectInterface::Execute_BPIInitialize(GetOwner());
		ToggleIsInteractable(true);
	}
}

void UInteractableManager::Re_InitializeAssociatedActors()
{
	TArray<AActor*> AIAArray;
	AssociatedInteractableActors.GetKeys(AIAArray);
	for (const AActor* CurrentActor : AIAArray)
	{
		if(UInteractableManager* ReturnInteractableManager = Cast<UInteractableManager>(
			CurrentActor->GetComponentByClass(UInteractableManager::StaticClass())); IsValid(CurrentActor) &&
			IsValid(ReturnInteractableManager))
		{
			ReturnInteractableManager->ToggleCanBeReInitialized(true);
			Re_Initialize();
		}
	}
	
}

void UInteractableManager::ToggleCanBeReInitialized(const bool bCond)
{
	if(bCond != CanBeReInitialized) {CanBeReInitialized = bCond;}
}

void UInteractableManager::ToggleIsInteractable(const bool bCond)
{
	if(bCond != IsInteractable) { IsInteractable = bCond;}
}

void UInteractableManager::ToggleInteractionWidget(const bool Condition) const
{                              
	if(IsInteractable && IsValid(InteractionWidgetRef))
	{
		InteractionWidgetRef->SetVisibility(Condition ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);

	}
}

void UInteractableManager::ChangeInteractableValue(const bool Increment)
{
	if(Increment)
	{
		InteractableValue++;
		if( InteractableValue > InteractableLimitValue)
		{
			InteractableValue = 0;
		}
	}
	else
	{
		InteractableValue--;
		if(InteractableValue < 0 )
		{
			InteractableValue = InteractableLimitValue;
		}
	}
}

void UInteractableManager::RemoveInteractionByResponse()
{
	switch (InteractableResponse)
	{
	case EInteractableResponseType::Persistent:
		break;
	case EInteractableResponseType::OnlyOnce:
	case EInteractableResponseType::Temporary:
		RemoveInteraction();
		break;
	default:
		break;
	}
}

void UInteractableManager::DecreaseMashingProgress()
{
	// This is the function called by the timer to decrease mashing progress.
	// Adjust the decrement value as needed for your game's design.
	MashingProgress -= 1.0f / MashingAmount;
	OnUpdateMashingValue.Broadcast(MashingProgress);
	if (MashingProgress < 0.0f)
	{
		MashingProgress = 0.0f; // Ensure progress doesn't go below 0.
		GetWorld()->GetTimerManager().ClearTimer(MashingDecreaseTimerHandle); // Optionally stop the timer if progress reaches 0.
	}
}




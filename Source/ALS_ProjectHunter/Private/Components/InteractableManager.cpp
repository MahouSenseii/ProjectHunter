// Copyright@2024 Quentin Davis 


#include "Components/InteractableManager.h"
#include "EnhancedInputSubsystems.h"
#include "Components/TextBlock.h"
#include "Interfaces/InteractionProcessInterface.h"
#include "Library/InteractionEnumLibrary.h"
#include "InputCoreTypes.h"
#include "Character/Player/PHPlayerController.h"
#include "Interfaces/InteractableObjectInterface.h"
#include "Net/UnrealNetwork.h"
#include "UI/InteractableWidget.h"

class UEnhancedInputLocalPlayerSubsystem;

UInteractableManager::UInteractableManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// Attempt to load the default widget class from assets
	static ConstructorHelpers::FClassFinder<UInteractableWidget> WidgetFinder(TEXT("/Game/PeojectHunter/UI/PopUps/WBP_InteractableWidget"));

	if (WidgetFinder.Succeeded())
	{
		WidgetClass = WidgetFinder.Class;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find widget class at the specified path."));
	}

	ResetInteractable();
}




void UInteractableManager::BeginPlay()
{
	Super::BeginPlay();
	SetIsReplicated(true);
	Initialize();

	if (AActor* Owner = GetOwner())
	{
		Owner->Tags.AddUnique(InteractableTag);
		if (DestroyAfterInteract)
		{
			Owner->Tags.AddUnique(DestroyableTag);
		}
	}

	if (InteractionWidget)
	{
		InteractionWidget->SetWidget(nullptr); // clear editor-copied widget
	}

	InteractionWidgetRef = nullptr; // clear any stale reference

	ResetInteractable();
}

void UInteractableManager::ResetInteractable()
{
	IsInteractable = true;
	AlreadyInteracted = false;
	InteractableValue = FMath::RandRange(0, InteractableLimitValue);
	MashingProgress = 0.0f;
}

void UInteractableManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UInteractableManager, IsInteractable);
	DOREPLIFETIME(UInteractableManager, InteractableValue);
}


void UInteractableManager::Initialize()
{
	if (GetOwner()->Implements<UInteractableObjectInterface>())
	{
		IInteractableObjectInterface::Execute_BPIInitialize(GetOwner());
	}
}

void UInteractableManager::SetupInteractableReferences(USphereComponent* InInteractableArea, UWidgetComponent* InInteractionWidget, const TSet<UPrimitiveComponent*> InHighlightableObject)
{
	if (!IsValid(InInteractableArea) || !IsValid(InInteractionWidget) || InHighlightableObject.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("One or more input parameters are null or empty."));
		return;
	}

	InteractableArea = InInteractableArea;
	InteractionWidget = InInteractionWidget;
	SetupHighlightableObjects(InHighlightableObject);
}

void UInteractableManager::SetupHighlightableObjects(TSet<UPrimitiveComponent*> InHighlightableObject)
{
	ObjectsToHighlight.Empty();
	for (UPrimitiveComponent* Component : InHighlightableObject)
	{
		if (IsValid(Component))
		{
			Component->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
			ObjectsToHighlight.Add(Component);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid Component in Highlightable Objects."));
		}
	}
}

void UInteractableManager::ToggleHighlight(bool Highlight, AActor* Interactor)
{
	// Toggle visibility of the interaction widget
	InteractionWidget->SetVisibility(Highlight);

	// Iterate through highlightable components and toggle their custom depth rendering
	for (UPrimitiveComponent* Component : ObjectsToHighlight)
	{
		Component->SetRenderCustomDepth(Highlight);
	}

	// If the interactor is a valid player controller, assign it as the widget owner
	if (APHPlayerController* AlsPlayerController = Cast<APHPlayerController>(Interactor))
	{
		SetWidgetLocalOwner(AlsPlayerController);
	}
}


void UInteractableManager::SetWidgetLocalOwner(APlayerController* OwnerPlayerController)
{
	if (!OwnerPlayerController || !OwnerPlayerController->IsLocalPlayerController())
	{
		return;
	}
	
	if (InteractionWidgetRef)
	{
		InteractionWidgetRef->RemoveFromParent();
		InteractionWidgetRef = nullptr;
	}


	// Remove the previous widget if it exists
	if (InteractionWidget && InteractionWidget->GetWidget())
	{
		InteractionWidget->SetWidget(nullptr);
	}

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


void UInteractableManager::PreInteraction(AActor* Interactor)
{
	RefInteractor = Interactor;

	if (GetOwner()->Implements<UInteractableObjectInterface>())
	{
		IInteractableObjectInterface::Execute_BPIClientPreInteraction(GetOwner(), Interactor);
	}

	switch (InputType)
	{
	case EInteractType::Single:
		if (Interactor->Implements<UInteractionProcessInterface>())
		{
			IInteractionProcessInterface::Execute_StartInteractionWithObject(Interactor, this, false);
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
	const UWorld* World = GetWorld();
	check(World);

	if (!World->GetTimerManager().IsTimerActive(KeyDownTimer))
	{
		World->GetTimerManager().SetTimer(KeyDownTimer, this, &UInteractableManager::IsKeyDown, 0.05f, true);
	}

	PressedInteractionKey = EKeys::E;

	FKey TempKey;
	if (GetPressedByAction(ActionToCheckForInteract, Interactor, TempKey))
	{
		PressedInteractionKey = TempKey;
	}
}


void UInteractableManager::IsKeyDown()
{
	// Only proceed if we have a valid interactor that supports interaction
	if (!RefInteractor || !RefInteractor->Implements<UInteractionProcessInterface>())
	{
		ClearInteractionTimer(); // Stop checking key input
		return;
	}

	// Cast interactor to PlayerController and check if the interaction key is still held
	const APlayerController* PlayerController = Cast<APlayerController>(RefInteractor);
	if (!PlayerController->IsInputKeyDown(PressedInteractionKey))
	{
		ClearInteractionTimer(); // Player released key early
		return;
	}

	// Confirm the interactor is still targeting this interactable
	if (const AActor* FoundInteractableObject = IInteractionProcessInterface::Execute_GetCurrentInteractableObject(RefInteractor);
		IsValid(FoundInteractableObject) && GetOwner() == FoundInteractableObject)
	{
		HoldingInputHandle(); // Continue holding progress
	}
	else
	{
		ClearInteractionTimer(); // Interaction target changed
	}
}


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
	if (!PlayerController || !Action) return false;

	const UEnhancedInputLocalPlayerSubsystem* EnhancedInputSubsystem = PlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!EnhancedInputSubsystem) return false;

	TArray<FKey> KeysMappedToAction = EnhancedInputSubsystem->QueryKeysMappedToAction(Action);
	for (const FKey& Key : KeysMappedToAction)
	{
		if (PlayerController->WasInputKeyJustPressed(Key))
		{
			OutKey = Key;
			return true;
		}
	}

	return false;
}

void UInteractableManager::HoldingInputHandle()
{
	if (const APlayerController* PlayerController = Cast<APlayerController>(RefInteractor); !PlayerController)
	{
		return;  // Cannot process input without a valid player controller
	}

	float HeldDownTime = 0.0f;

	// Check how long the input key has been held down
	if (GetKeyTimeDown(HeldDownTime))
	{
		// Update visual feedback for holding interaction
		OnUpdateHoldingValue.Broadcast(FMath::Clamp(HeldDownTime / MaxKeyTimeDown, 0.f, 1.f));

		// Once time is exceeded, start interaction
		if (HeldDownTime >= MaxKeyTimeDown)
		{
			IInteractionProcessInterface::Execute_StartInteractionWithObject(RefInteractor, this, true);
		}
	}
	else
	{
		// Reset holding UI if no valid key duration
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




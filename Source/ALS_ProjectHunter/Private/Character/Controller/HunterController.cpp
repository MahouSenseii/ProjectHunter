#include "Character/Controller/HunterController.h"

#include "EnhancedActionKeyMapping.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Character/Component/Interaction/InteractionManager.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogHunterController, Log, All);

AHunterController::AHunterController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AHunterController::OnPossess(APawn* NewPawn)
{
	Super::OnPossess(NewPawn);

	CacheComponents();
}

void AHunterController::Interact(const FInputActionValue& Value)
{
	if (!InteractionManager)
	{
		return;
	}

	if (Value.Get<bool>())
	{
		InteractionManager->OnInteractPressed();
	}
	else
	{
		InteractionManager->OnInteractReleased();
	}
}

void AHunterController::Menu(const FInputActionValue& Value) const
{
	if (Value.Get<bool>())
	{
		UE_LOG(LogHunterController, Log, TEXT("Menu button pressed"));
	}
}

const UInputAction* AHunterController::GetInputActionByName(const FString& InString) const
{
	const UInputMappingContext* Context = DefaultInputMappingContext;
	TObjectPtr<const UInputAction> FoundAction  = nullptr;

	if (Context)
	{
		const TArray<FEnhancedActionKeyMapping>& Mappings = Context->GetMappings();
		for (const FEnhancedActionKeyMapping& Keymapping : Mappings)
		{
			if (Keymapping.Action && Keymapping.Action->GetFName() == InString)
			{
				FoundAction = Keymapping.Action;
				break;
			}
		}
		return FoundAction;
	}
	return nullptr;
}



float AHunterController::GetElapsedSeconds(const UInputAction* Action) const
{
	
	if (const auto EnhancedInput = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(this->GetLocalPlayer()))
	{
		if (const auto LocalPlayerInput = EnhancedInput->GetPlayerInput())
		{
			if (const auto ActionData = LocalPlayerInput->FindActionInstanceData(Action))
			{
				return ActionData->GetElapsedTime();
			}
		}
	}
	return 0;
}

bool AHunterController::DoOnce(FDoOnceState& State, bool bReset, bool bStartClosed)
{
	if (bReset)
	{
		State.bHasBeenInitialized = true;
		State.bIsClosed = false;
		return false;
	}

	if (!State.bHasBeenInitialized)
	{
		State.bHasBeenInitialized = true;
		if (bStartClosed)
		{
			State.bIsClosed = true;
			return false;
		}
	}

	if (!State.bIsClosed)
	{
		State.bIsClosed = true;
		return true;
	}

	return false; 
}
void AHunterController::CacheComponents()
{
	if (APawn* PossessedPawn = GetPawn())
	{
		InteractionManager = PossessedPawn->FindComponentByClass<UInteractionManager>();
		
		if (!InteractionManager)
		{
			UE_LOG(LogHunterController, Warning, TEXT("HunterController: No InteractionManager found on %s"), 
				*PossessedPawn->GetName());
		}
		else
		{
			UE_LOG(LogHunterController, Log, TEXT("HunterController: Cached InteractionManager"));
		}
	}
}

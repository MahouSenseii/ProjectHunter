// Character/Controller/HunterController.cpp
// SIMPLIFIED - NO TICK DEPENDENCY!

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
	
	// Cache component references
	CacheComponents();
}

// ═══════════════════════════════════════════════════════════════════════
// INPUT HANDLERS - Pure Delegation!
// ═══════════════════════════════════════════════════════════════════════

void AHunterController::Interact(const FInputActionValue& Value)
{
	if (!InteractionManager)
	{
		return;
	}

	// Just route to InteractionManager - that's it!
	if (Value.Get<bool>()) // Pressed
	{
		InteractionManager->OnInteractPressed();
	}
	else // Released
	{
		InteractionManager->OnInteractReleased();
	}
}

void AHunterController::Menu(const FInputActionValue& Value) const
{
	// TODO: Implement menu logic
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
				break; // Found the Interact action, no need to search further.
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
	// If any step fails (e.g., the action is not found), return 0 to indicate no elapsed time.
	return 0;
}

bool AHunterController::DoOnce(FDoOnceState& State, bool bReset, bool bStartClosed)
{
	if (bReset)
	{
		State.bHasBeenInitialized = true;
		State.bIsClosed = false;
		return false; // Nothing should execute on reset
	}

	if (!State.bHasBeenInitialized)
	{
		State.bHasBeenInitialized = true;
		if (bStartClosed)
		{
			State.bIsClosed = true;
			return false; // Starts closed, skip execution
		}
	}

	if (!State.bIsClosed)
	{
		State.bIsClosed = true;
		return true;
	}

	return false; 
}
// ═══════════════════════════════════════════════════════════════════════
// INTERNAL METHODS
// ═══════════════════════════════════════════════════════════════════════

void AHunterController::CacheComponents()
{
	if (APawn* PossessedPawn = GetPawn())
	{
		// Cache InteractionManager
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

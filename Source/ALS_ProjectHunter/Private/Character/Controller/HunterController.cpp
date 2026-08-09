#include "Character/Controller/HunterController.h"
#include "Framework/System/Cheats/HunterCheatManager.h"
#include "Framework/System/Cheats/HunterCheatComponent.h"
#include "EnhancedActionKeyMapping.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Character/Components/Interaction/InteractionManager.h"
#include "UI/HUD/HunterHUD.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogHunterController, Log, All);

AHunterController::AHunterController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FallbackMenuKeys = { EKeys::Tab, EKeys::Gamepad_Special_Right };

#if !UE_BUILD_SHIPPING
	CheatClass = UHunterCheatManager::StaticClass();
	CheatComponent = CreateDefaultSubobject<UHunterCheatComponent>(TEXT("CheatComponent"));
#endif
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
}

void AHunterController::Interact_Started(const FInputActionValue& Value)
{
	if (!InteractionManager || !Value.Get<bool>())
	{
		return;
	}

	InteractionManager->OnInteractPressed();
}

void AHunterController::Interact_Completed(const FInputActionValue& Value)
{
	if (!InteractionManager || Value.Get<bool>())
	{
		return;
	}

	InteractionManager->OnInteractReleased();
}

void AHunterController::Interact_Canceled(const FInputActionValue& Value)
{
	if (!InteractionManager || Value.Get<bool>())
	{
		return;
	}

	InteractionManager->OnInteractReleased();
}

void AHunterController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent || !bBindFallbackMenuKeys || HasNamedMenuInputAction())
	{
		return;
	}

	for (const FKey& MenuKey : FallbackMenuKeys)
	{
		if (MenuKey.IsValid())
		{
			InputComponent->BindKey(MenuKey, IE_Pressed, this, &AHunterController::ToggleMenuFromFallbackInput);
		}
	}

	UE_LOG(LogHunterController, Log,
		TEXT("SetupInputComponent: no InputAction named 'Menu' was found; bound %d fallback menu key(s)."),
		FallbackMenuKeys.Num());
}

void AHunterController::CycleGroundItem(const FInputActionValue& Value)
{
	const float Direction = Value.Get<float>();
	if (!FMath::IsNearlyZero(Direction))
	{
		CycleGroundItemDirection(Direction > 0.0f ? 1 : -1);
	}
}

void AHunterController::CycleGroundItemDirection(int32 Direction)
{
	if (InteractionManager && Direction != 0)
	{
		InteractionManager->CycleGroundItemFocus(Direction);
	}
}

void AHunterController::Menu(const FInputActionValue& Value)
{
	if (!Value.Get<bool>())
	{
		return;
	}

	if (AHunterHUD* HunterHUD = Cast<AHunterHUD>(GetHUD()))
	{
		HunterHUD->ToggleMenu();
	}
	else
	{
		UE_LOG(LogHunterController, Warning,
			TEXT("Menu: HUD is not an AHunterHUD (current: %s) - set HUD Class in your GameMode."),
			*GetNameSafe(GetHUD()));
	}
}

bool AHunterController::HasNamedMenuInputAction() const
{
	if (!DefaultInputMappingContext)
	{
		return false;
	}

	for (const FEnhancedActionKeyMapping& Mapping : DefaultInputMappingContext->GetMappings())
	{
		if (Mapping.Action && Mapping.Action->GetFName() == GET_FUNCTION_NAME_CHECKED(AHunterController, Menu))
		{
			return true;
		}
	}

	return false;
}

void AHunterController::ToggleMenuFromFallbackInput()
{
	if (AHunterHUD* HunterHUD = Cast<AHunterHUD>(GetHUD()))
	{
		HunterHUD->ToggleMenu();
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

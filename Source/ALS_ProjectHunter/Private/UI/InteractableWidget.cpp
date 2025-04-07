// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/InteractableWidget.h"

#include "EnhancedInputSubsystems.h"
#include "Character/Player/PHPlayerController.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Library/FL_InteractUtility.h"
#include "Library/PHItemEnumLibrary.h"


class UEnhancedInputLocalPlayerSubsystem;
struct FInputActionKeyMapping;

void UInteractableWidget::NativeConstruct()
{
	// Call the base class NativeConstruct
	Super::NativeConstruct();
	
	// Initialize the materials for the widget
	InitializeMaterials();

	// Initialize the UI states for the widget
	InitializeUIStates();

	// Initialize the player controller and event bindings
	InitializePlayerControllerAndBindings();

	BindEventDispatchers();
	// Create utility instance
	MyUtilityInstance = NewObject<UFL_InteractUtility>();

	Img_Key->SetBrush(GetInteractionIcon());
}

void UInteractableWidget::NativeDestruct()
{
	UnbindEventDispatchers();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	}
    
	// Clean up utility instance
	if (MyUtilityInstance)
	{
		MyUtilityInstance->ConditionalBeginDestroy();
		MyUtilityInstance = nullptr;
	}
    
	// Clean up dynamic materials
	if (DynamicSquareMaterial)
	{
		DynamicSquareMaterial->ConditionalBeginDestroy();
		DynamicSquareMaterial = nullptr;
	}
	if (DynamicCircularMaterial)
	{
		DynamicCircularMaterial->ConditionalBeginDestroy();
		DynamicCircularMaterial = nullptr;
	}
    
	Super::NativeDestruct();
}

void UInteractableWidget::BindEventDispatchers()
{
	if (InteractableManager) // Check if InteractableManager is valid
	{
		if (InputType == EInteractType::Holding)
		{
			// Ensure that EventBorderFill is not already bound to OnUpdateHoldingValue before binding
			if (!InteractableManager->OnUpdateHoldingValue.IsAlreadyBound(this, &UInteractableWidget::EventBorderFill))
			{
				InteractableManager->OnUpdateHoldingValue.AddDynamic(this, &UInteractableWidget::EventBorderFill);
			}
		}
		else if (InputType == EInteractType::Mashing) // Changed to 'else if' for mutual exclusivity
		{
			// Ensure that EventBorderFill is not already bound to OnUpdateMashingValue before binding
			if (!InteractableManager->OnUpdateMashingValue.IsAlreadyBound(this, &UInteractableWidget::EventBorderFill))
			{
				InteractableManager->OnUpdateMashingValue.AddDynamic(this, &UInteractableWidget::EventBorderFill);
			}
		}
	}
}

void UInteractableWidget::UnbindEventDispatchers()
{
	if(InteractableManager) // Check if InteractableManager is valid
	{
		// Check and unbind for holding input type
		if (InputType == EInteractType::Holding)
		{
			if (InteractableManager->OnUpdateHoldingValue.IsAlreadyBound(this, &UInteractableWidget::EventBorderFill))
			{
				InteractableManager->OnUpdateHoldingValue.RemoveDynamic(this, &UInteractableWidget::EventBorderFill);
			}
		}

		// Check and unbind for mashing input type
		if (InputType == EInteractType::Mashing)
		{
			if (InteractableManager->OnUpdateMashingValue.IsAlreadyBound(this, &UInteractableWidget::EventBorderFill))
			{
				InteractableManager->OnUpdateMashingValue.RemoveDynamic(this, &UInteractableWidget::EventBorderFill);
			}
		}
	}
}


bool UInteractableWidget::Initialize()
{
	
	return Super::Initialize();
}

void UInteractableWidget::InitializePlayerControllerAndBindings()
{
	// Attempt to get the ALSPlayerController

	// Check if the player controller is valid
	if (AALSPlayerController* ALSPlayerController = GetALSPlayerController())
	{
		UE_LOG(LogPlayerController, Warning, TEXT("ALSPlayerController is valid"));

		// Bind to the OnGamepadStateChanged event if not already bound
		if (!ALSPlayerController->OnGamepadStateChanged.IsAlreadyBound(this, &UInteractableWidget::HandleGamepadStateChange))
		{
			ALSPlayerController->OnGamepadStateChanged.AddDynamic(this, &UInteractableWidget::HandleGamepadStateChange);
		}
	}
	else
	{
		// Log a warning if the player controller is null. Consider making this an error if it's a critical issue.
		UE_LOG(LogPlayerController, Warning, TEXT("ALSPlayerController is NULL"));
		// Consider adding more error-handling code here.
	}
}

// Function to get the ALSPlayerController that owns this widget
AALSPlayerController* UInteractableWidget::GetALSPlayerController() const
{
	// Get the APlayerController that owns this widget
	APlayerController* PlayerController = GetOwningPlayer();

	// Early return if PlayerController is nullptr
	if (!PlayerController)
	{
		UE_LOG(LogPlayerController, Warning, TEXT("PlayerController is NULL"));
		return nullptr;
	}

	// Attempt to cast it to AALSPlayerController
	AALSPlayerController* ALSPlayerController = Cast<AALSPlayerController>(PlayerController);

	// Check if the cast was successful
	if (!ALSPlayerController)
	{
		UE_LOG(LogPlayerController, Warning, TEXT("Failed to cast PlayerController to AALSPlayerController"));
		return nullptr;
	}

	return ALSPlayerController;
}


// Function called before the object is destroyed
void UInteractableWidget::BeginDestroy()
{
	// Call parent implementation of BeginDestroy
	Super::BeginDestroy();

	// Attempt to get the ALSPlayerController

	// Check if the cast was successful
	if (AALSPlayerController* ALSPlayerController = GetALSPlayerController())
	{
		// Remove the dynamic event binding for HandleGamepadStateChange
		ALSPlayerController->OnGamepadStateChanged.RemoveDynamic(this, &UInteractableWidget::HandleGamepadStateChange);

		// Optionally, log for debugging purposes
		UE_LOG(LogTemp, Warning, TEXT("Event HandleGamepadStateChange has been unbound."));
	}
	else
	{
		// Log a warning if unable to unbind event. Consider making this an error if it's a critical issue.
		UE_LOG(LogTemp, Warning, TEXT("Unable to unbind event HandleGamepadStateChange as ALSPlayerController is NULL."));
	}
}



// Function to initialize dynamic material instances
void UInteractableWidget::InitializeMaterials()
{
	// Check if M_SquareFill is set and initialize
	InitializeDynamicMaterial(M_SquareFill, DynamicSquareMaterial);

	// Check if M_CircularFill is set and initialize
	InitializeDynamicMaterial(M_CircularFill, DynamicCircularMaterial);
}

// Helper function to initialize a given dynamic material
void UInteractableWidget::InitializeDynamicMaterial(UMaterialInterface* MaterialToSet, UMaterialInstanceDynamic*& DynamicMaterialRef)
{
	if (MaterialToSet)
	{
		// Create a new dynamic material instance
		DynamicMaterialRef = UMaterialInstanceDynamic::Create(MaterialToSet, this);
		if (DynamicMaterialRef == nullptr)
		{
			// Log error if Dynamic Material failed to be created
			UE_LOG(LogTemp, Error, TEXT("Dynamic Material failed to be created."));
		}
	}
}

// Function to initialize various UI states
void UInteractableWidget::InitializeUIStates()
{
	// Sets the appropriate filling background based on certain conditions (not shown)
	SetAppropriateFillingBackground();

	// Control the visibility of the fill border based on the InputType
	if (InputType == EInteractType::Single)
	{
		Img_FillBorder->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		Img_FillBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// Set the initial value for the fill decimal
	SetFillDecimalValue(0.05f);

	// Play the FillAnimOpacity animation if it exists
	if (FillAnimOpacity) // Null Check
	{
		PlayAnimation(FillAnimOpacity, 0.0f, 0, EUMGSequencePlayMode::PingPong, 1.0f, false);
	}
}


// Function to handle the change in gamepad state
void UInteractableWidget::HandleGamepadStateChange()
{
	// Check if a gamepad is being used
	if (IsUsingGamepad())
	{
		// If so, update the appropriate background for the fill
		SetAppropriateFillingBackground();
	}
}



// Add default functionality here for any IInteractableWidget functions that are not pure virtual.
FText UInteractableWidget::GetInteractionText()
{
	return InteractionDescription->GetText();
}

// Function to set the fill value for the material
void UInteractableWidget::SetFillDecimalValue(float Value)
{
	// Clamp the incoming value to be between 0.005f and 1.0f
	const float ClampedValue = FMath::Clamp(Value, 0.005f, 1.0f);

	// Check if either of the dynamic materials (square or circular) is valid
	if (DynamicSquareMaterial || DynamicCircularMaterial)
	{
		// If a gamepad is not being used
		if (!IsUsingGamepad())
		{
			SetMaterialDecimal(DynamicSquareMaterial, ClampedValue);
		}
		else // If a gamepad is being used
		{
			SetMaterialDecimal(DynamicCircularMaterial, ClampedValue);
		}
	}
}

// Helper function to set decimal value of a dynamic material
void UInteractableWidget::SetMaterialDecimal(UMaterialInstanceDynamic* DynamicMaterial, float DecimalValue)
{
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue("Decimal", DecimalValue);
		if (GEngine)
		{
			// Log debug statement for online debugging
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Decimal Value set to: %f"), DecimalValue));
		}
	}
	else
	{
		// Log error if Dynamic Material is null
		UE_LOG(LogTemp, Error, TEXT("Dynamic Material is null, failed to set decimal value."));
	}
}

// Function to set the appropriate filling background based on input device
void UInteractableWidget::SetAppropriateFillingBackground()
{
	// Null checks
	if (!Img_FillBorder)
	{
		UE_LOG(LogTemp, Warning, TEXT("Img_FillBorder is null."));
		return;
	}

	// Choose appropriate material based on input device
	if (!IsUsingGamepad())
	{
		if (!DynamicSquareMaterial)
		{
			UE_LOG(LogTemp, Warning, TEXT("DynamicSquareMaterial is null."));
			return;
		}
		Img_FillBorder->SetBrushFromMaterial(DynamicSquareMaterial);
	}
	else
	{
		if (!DynamicCircularMaterial)
		{
			UE_LOG(LogTemp, Warning, TEXT("DynamicCircularMaterial is null."));
			return;
		}
		Img_FillBorder->SetBrushFromMaterial(DynamicCircularMaterial);
	}
}



// Function to check if the player is using a gamepad
bool UInteractableWidget::IsUsingGamepad()
{
	// Get the owning PlayerController

	// Check if the PlayerController is valid
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		// Cast to custom ALSPlayerController class

		// Check if the cast was successful
		if (const AALSPlayerController* ALSPlayerController = Cast<AALSPlayerController>(PlayerController))
		{
			// Return the result of the IsUsingGamepad function from ALSPlayerController
			return ALSPlayerController->IsUsingGamepad();
		}
		else
		{
			// Log a warning if the cast fails
			UE_LOG(LogTemp, Warning, TEXT("Failed to cast PlayerController to AALSPlayerController."));
		}
	}
	else
	{
		// Log a warning if PlayerController is null
		UE_LOG(LogTemp, Warning, TEXT("PlayerController is null."));
	}

	// Return false by default
	return false;
}


// Function to get the background color and opacity based on item rarity (Grade)
FLinearColor UInteractableWidget::GetImageBackgroundColorandOpacity() const
{
	// Variable to hold the chosen color
	FLinearColor Color;

	// Switch based on the Grade (rarity) of the item
	switch (Grade)
	{
	case EItemRarity::IR_GradeF:
		Color = Color_GradeF;
		break;
	case EItemRarity::IR_GradeD:
		Color = Color_GradeD;
		break;
	case EItemRarity::IR_GradeC:
		Color = Color_GradeC;
		break;
	case EItemRarity::IR_GradeB:
		Color = Color_GradeB;
		break;
	case EItemRarity::IR_GradeA:
		Color = Color_GradeA;
		break;
	case EItemRarity::IR_GradeS:
		Color = Color_GradeS;
		break;
	case EItemRarity::IR_Unkown:  // Note: "Unknown" is the correct spelling
		Color = Color_GradeUnkown; // Consider renaming to Color_GradeUnknown
		break;
	case EItemRarity::IR_Corrupted:
		Color = Color_GradeCorrupted;
		break;
	default:
		Color = FLinearColor::White; // Default to white if the grade doesn't match any known values
		break;
	}

	// Return the chosen color
	return Color;
}

void UInteractableWidget::EventBorderFill(float Value)
{
	StopAnimation(FillAnimOpacity);
	if (Img_FillBorder)
	{
		Img_FillBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		SetFillDecimalValue(Value);
	}
}


FSlateBrush UInteractableWidget::GetInteractionIcon()
{
	FSlateBrush MyBrush;
	const APHPlayerController* PC = Cast<APHPlayerController>(GetOwningPlayer());
	
	/*if (const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr)
	{
		if (const UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>())
		{
			if (TArray<FKey> MappedKeys = InputSubsystem->GetAllPlayerMappedKeys("Interact"); !MappedKeys.IsEmpty())
			{
				for (const FKey& Key : MappedKeys)
				{
					if (IsUsingGamepad()) // This should be your method to check if the last input device is a gamepad.
					{
						if (Key.IsGamepadKey())
						{
							const EGamepadIcon GamepadIcon = MyUtilityInstance->TranslateToGamepadIcon(Key);
							MyBrush.SetResourceObject(MyUtilityInstance->GetGamepadIcon(GamepadIcon));
							return MyBrush; // Return the brush for the gamepad key
						}
					}
					else // If not using gamepad, assume it's a keyboard key
					{
						if (!Key.IsGamepadKey()) // This is the change: using !Key.IsGamepadKey() instead of Key.IsKeyboardKey()
						{
							const EKeyboardIcon KeyboardIcon = MyUtilityInstance->TranslateToKeyboardIcon(Key);
							MyBrush.SetResourceObject(MyUtilityInstance->GetKeyboardIcon(KeyboardIcon));
							return MyBrush; // Return the brush for the keyboard key
						}
					}
				}
			}
		}
	}*/ // Add back after implementing save inputs
	
	if(IsUsingGamepad())
	{
		MyBrush.SetResourceObject(MyUtilityInstance->GetGamepadIcon(EGamepadIcon::GI_FaceButtonLeft));
		return MyBrush; // Return the brush for the gamepad key
	}
	else
	{
		MyBrush.SetResourceObject(MyUtilityInstance->GetKeyboardIcon(EKeyboardIcon::Key_E));
		return MyBrush; // Return the brush for the gamepad key
	}
}



// Separate function to set the size for Img_Key
void UInteractableWidget::SetKeyImageSize() const
{
	const FVector2D NewSize = FVector2D(25.0f, 25.0f);
	Img_Key->SetDesiredSizeOverride(NewSize);
}

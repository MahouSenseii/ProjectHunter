// Interactable/Widget/InteractableWidget.cpp

#include "Interactable/Widget/InteractableWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "PlayerMappableInputConfig.h"

DEFINE_LOG_CATEGORY(LogInteractableWidget);

// ═══════════════════════════════════════════════════════════════════════
// LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════

void UInteractableWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Detect initial input mode
	bIsUsingGamepad = DetectGamepadMode();
	bLastInputModeGamepad = bIsUsingGamepad;

	// Initialize border material
	UpdateBorderMaterial();

	// Set initial state
	SetWidgetState(EInteractionWidgetState::IWS_Idle);

	UE_LOG(LogInteractableWidget, Log, TEXT("InteractableWidget constructed (Gamepad: %s)"),
		bIsUsingGamepad ? TEXT("Yes") : TEXT("No"));
}

void UInteractableWidget::NativeDestruct()
{
	BorderMID = nullptr;
	Super::NativeDestruct();
}

void UInteractableWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// ═══════════════════════════════════════════════
	// INPUT MODE DETECTION (Only check periodically, not every frame)
	// ═══════════════════════════════════════════════
	static float InputCheckAccumulator = 0.0f;
	InputCheckAccumulator += InDeltaTime;
	
	if (InputCheckAccumulator >= 0.25f)  // Check every 250ms
	{
		InputCheckAccumulator = 0.0f;
		
		bool bCurrentlyGamepad = DetectGamepadMode();
		if (bCurrentlyGamepad != bIsUsingGamepad)
		{
			bIsUsingGamepad = bCurrentlyGamepad;
			RefreshInputMode();
			
			UE_LOG(LogInteractableWidget, Verbose, TEXT("Input mode changed to: %s"),
				bIsUsingGamepad ? TEXT("Gamepad") : TEXT("Keyboard"));
		}
	}

	// ═══════════════════════════════════════════════
	// STATE-SPECIFIC TICK
	// ═══════════════════════════════════════════════
	TickState(InDeltaTime);
}

// ═══════════════════════════════════════════════════════════════════════
// PUBLIC API
// ═══════════════════════════════════════════════════════════════════════

void UInteractableWidget::SetInteractionData(UInputAction* InputAction, const FText& Description)
{
	if (!InputAction)
	{
		UE_LOG(LogInteractableWidget, Warning, TEXT("SetInteractionData called with null InputAction!"));
		CurrentInputKey = EKeys::Invalid;
		
		// Still update description
		if (InteractionDescription)
		{
			InteractionDescription->SetText(Description);
		}
		return;
	}

	// ═══════════════════════════════════════════════
	// QUERY ENHANCED INPUT FOR BOUND KEY
	// ═══════════════════════════════════════════════
	FKey BoundKey = GetBoundKeyForInputAction(InputAction);
	
	// Use the direct key setter
	SetInteractionDataWithKey(BoundKey, Description);

	UE_LOG(LogInteractableWidget, Verbose, TEXT("SetInteractionData: InputAction='%s', BoundKey='%s', Description='%s'"),
		*InputAction->GetName(), *BoundKey.ToString(), *Description.ToString());
}

void UInteractableWidget::SetInteractionDataWithKey(const FKey& Key, const FText& Description)
{
	CurrentInputKey = Key;

	// Update description text
	if (InteractionDescription)
	{
		InteractionDescription->SetText(Description);
	}

	// Update icon for new key
	UpdateKeyIcon();

	UE_LOG(LogInteractableWidget, Verbose, TEXT("SetInteractionDataWithKey: Key='%s', Description='%s'"),
		*Key.ToString(), *Description.ToString());
}

void UInteractableWidget::SetWidgetState(EInteractionWidgetState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	EInteractionWidgetState OldState = CurrentState;
	CurrentState = NewState;
	StateTimer = 0.0f;

	// State entry logic
	switch (NewState)
	{
		case EInteractionWidgetState::IWS_Idle:
			// Reset progress for idle
			CurrentProgress = 0.0f;
			LastSetProgress = -1.0f;  // Force update
			break;

		case EInteractionWidgetState::IWS_Holding:
		case EInteractionWidgetState::IWS_Mashing:
			// Progress will be set externally
			break;

		case EInteractionWidgetState::IWS_Completed:
			// Flash at full progress
			CurrentProgress = 1.0f;
			break;

		case EInteractionWidgetState::IWS_Cancelled:
			// Keep current progress, will deplete
			break;
	}

	// Immediate material update
	UpdateMaterialParameters();

	UE_LOG(LogInteractableWidget, Verbose, TEXT("State changed: %s -> %s"),
		*UEnum::GetValueAsString(OldState),
		*UEnum::GetValueAsString(NewState));
}

void UInteractableWidget::SetProgressBarVisible(bool bVisible)
{
	if (Img_FillBorder)
	{
		Img_FillBorder->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UInteractableWidget::SetProgress(float Progress)
{
	// Only update if in appropriate state
	if (CurrentState != EInteractionWidgetState::IWS_Holding && 
		CurrentState != EInteractionWidgetState::IWS_Mashing)
	{
		return;
	}

	// Clamp and check for change
	float NewProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
	
	// Only update material if progress actually changed (optimization)
	if (FMath::Abs(NewProgress - LastSetProgress) > 0.001f)
	{
		CurrentProgress = NewProgress;
		LastSetProgress = NewProgress;
		UpdateMaterialParameters();
	}
}

void UInteractableWidget::RefreshInputMode()
{
	// Rebuild material for new input mode (square vs circle)
	UpdateBorderMaterial();
	
	// Update icon (might need to switch from keyboard to gamepad key)
	UpdateKeyIcon();
}

void UInteractableWidget::Show()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	
	// Reset to idle when shown
	SetWidgetState(EInteractionWidgetState::IWS_Idle);
}

void UInteractableWidget::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL - STATE TICK
// ═══════════════════════════════════════════════════════════════════════

void UInteractableWidget::TickState(float DeltaTime)
{
	StateTimer += DeltaTime;

	switch (CurrentState)
	{
		case EInteractionWidgetState::IWS_Idle:
		{
			// Idle animation (shimmer/pulse effect)
			if (bEnableIdleAnimation && BorderMID)
			{
				AnimationTime += DeltaTime * IdleAnimationSpeed;
				
				// Set animation phase parameter for material
				BorderMID->SetScalarParameterValue(FName("AnimationPhase"), AnimationTime);
				
				// Optional: Subtle progress pulse (0.0 to ~0.1 range)
				float IdlePulse = (FMath::Sin(AnimationTime * 2.0f) * 0.5f + 0.5f) * 0.05f;
				BorderMID->SetScalarParameterValue(FName("Progress"), IdlePulse);
			}
			break;
		}

		case EInteractionWidgetState::IWS_Holding:
		case EInteractionWidgetState::IWS_Mashing:
		{
			// Progress is set externally via SetProgress()
			// Nothing to tick here - material updates happen in SetProgress()
			break;
		}

		case EInteractionWidgetState::IWS_Completed:
		{
			// Flash effect then return to idle
			if (StateTimer >= CompletionFlashDuration)
			{
				SetWidgetState(EInteractionWidgetState::IWS_Idle);
			}
			else
			{
				// Pulse effect during flash
				float FlashAlpha = 1.0f - (StateTimer / CompletionFlashDuration);
				if (BorderMID)
				{
					// Bright flash fading out
					FLinearColor FlashColor = FillColorCompleted;
					FlashColor.A = FlashAlpha;
					BorderMID->SetVectorParameterValue(FName("FillColor"), FlashColor);
				}
			}
			break;
		}

		case EInteractionWidgetState::IWS_Cancelled:
		{
			// Deplete progress over time
			float DepleteSpeed = 2.0f;  // 0.5 seconds to fully deplete
			CurrentProgress = FMath::Max(0.0f, CurrentProgress - (DeltaTime * DepleteSpeed));
			
			UpdateMaterialParameters();
			
			// Return to idle when depleted
			if (CurrentProgress <= 0.0f)
			{
				SetWidgetState(EInteractionWidgetState::IWS_Idle);
			}
			break;
		}
	}
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL - MATERIAL MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════

void UInteractableWidget::UpdateBorderMaterial()
{
	if (!Img_FillBorder)
	{
		UE_LOG(LogInteractableWidget, Warning, TEXT("UpdateBorderMaterial: Img_FillBorder not bound!"));
		return;
	}

	// Select material based on input mode
	UMaterialInterface* SourceMaterial = bIsUsingGamepad ? CircleBorderMaterial : SquareBorderMaterial;

	if (!SourceMaterial)
	{
		UE_LOG(LogInteractableWidget, Warning, TEXT("UpdateBorderMaterial: No %s material assigned!"),
			bIsUsingGamepad ? TEXT("Circle") : TEXT("Square"));
		return;
	}

	// Create dynamic material instance
	BorderMID = UMaterialInstanceDynamic::Create(SourceMaterial, this);
	Img_FillBorder->SetBrushFromMaterial(BorderMID);

	// Initialize material parameters
	UpdateMaterialParameters();

	UE_LOG(LogInteractableWidget, Verbose, TEXT("Created border material: %s"),
		bIsUsingGamepad ? TEXT("Circle (Gamepad)") : TEXT("Square (Keyboard)"));
}

void UInteractableWidget::UpdateMaterialParameters()
{
	if (!BorderMID)
	{
		return;
	}

	// Set progress (0.0 to 1.0 - controls border fill amount)
	BorderMID->SetScalarParameterValue(FName("Progress"), CurrentProgress);

	// Set fill color based on current state
	FLinearColor FillColor = GetCurrentFillColor();
	BorderMID->SetVectorParameterValue(FName("FillColor"), FillColor);

	// Set background color (unfilled portion)
	BorderMID->SetVectorParameterValue(FName("BackgroundColor"), BorderBackgroundColor);
}

FLinearColor UInteractableWidget::GetCurrentFillColor() const
{
	switch (CurrentState)
	{
		case EInteractionWidgetState::IWS_Idle:
		case EInteractionWidgetState::IWS_Holding:
		case EInteractionWidgetState::IWS_Mashing:
			return FillColorNormal;

		case EInteractionWidgetState::IWS_Completed:
			return FillColorCompleted;

		case EInteractionWidgetState::IWS_Cancelled:
			return FillColorCancelled;

		default:
			return FillColorNormal;
	}
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL - ICON MANAGEMENT (REFACTORED TO USE FKey)
// ═══════════════════════════════════════════════════════════════════════

void UInteractableWidget::UpdateKeyIcon()
{
	if (!Img_Key)
	{
		return;
	}

	// Early out if key is invalid
	if (!CurrentInputKey.IsValid())
	{
		// Show fallback or hide
		if (FallbackIcon)
		{
			Img_Key->SetBrushFromTexture(FallbackIcon);
			Img_Key->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Img_Key->SetVisibility(ESlateVisibility::Collapsed);
		}
		
		UE_LOG(LogInteractableWidget, Warning, TEXT("CurrentInputKey is invalid, using fallback or hiding"));
		return;
	}

	// ═══════════════════════════════════════════════
	// SELECT ICON MAP BASED ON INPUT MODE
	// ═══════════════════════════════════════════════
	const TMap<FKey, TObjectPtr<UTexture2D>>& IconMap = bIsUsingGamepad ? GamepadIcons : KeyboardIcons;

	// ═══════════════════════════════════════════════
	// DIRECT KEY LOOKUP (FKey → Texture)
	// ═══════════════════════════════════════════════
	if (const TObjectPtr<UTexture2D>* FoundIcon = IconMap.Find(CurrentInputKey))
	{
		if (*FoundIcon)
		{
			Img_Key->SetBrushFromTexture(*FoundIcon);
			Img_Key->SetVisibility(ESlateVisibility::HitTestInvisible);
			
			UE_LOG(LogInteractableWidget, Verbose, TEXT("Showing icon for key: %s (%s mode)"),
				*CurrentInputKey.ToString(),
				bIsUsingGamepad ? TEXT("Gamepad") : TEXT("Keyboard"));
			return;
		}
	}

	// ═══════════════════════════════════════════════
	// FALLBACK: Use generic icon if specific key not found
	// ═══════════════════════════════════════════════
	if (FallbackIcon)
	{
		Img_Key->SetBrushFromTexture(FallbackIcon);
		Img_Key->SetVisibility(ESlateVisibility::HitTestInvisible);
		
		UE_LOG(LogInteractableWidget, Verbose, TEXT("No specific icon for key '%s', using fallback"),
			*CurrentInputKey.ToString());
		return;
	}

	// ═══════════════════════════════════════════════
	// NO ICON FOUND - HIDE
	// ═══════════════════════════════════════════════
	Img_Key->SetVisibility(ESlateVisibility::Collapsed);
	
	UE_LOG(LogInteractableWidget, Warning, TEXT("No icon found for key '%s' in %s mode (no fallback configured)"),
		*CurrentInputKey.ToString(),
		bIsUsingGamepad ? TEXT("Gamepad") : TEXT("Keyboard"));
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL - INPUT DETECTION
// ═══════════════════════════════════════════════════════════════════════

bool UInteractableWidget::DetectGamepadMode() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return false;
	}

	// Check for any recent gamepad input
	// This is a simple heuristic - for production, use InputDeviceMapper or similar
	
	// Check analog sticks
	if (FMath::Abs(PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX)) > 0.1f ||
		FMath::Abs(PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY)) > 0.1f ||
		FMath::Abs(PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX)) > 0.1f ||
		FMath::Abs(PC->GetInputAnalogKeyState(EKeys::Gamepad_RightY)) > 0.1f)
	{
		return true;
	}

	// Check common gamepad buttons (face buttons, shoulders)
	static const TArray<FKey> GamepadKeys = {
		EKeys::Gamepad_FaceButton_Bottom,
		EKeys::Gamepad_FaceButton_Right,
		EKeys::Gamepad_FaceButton_Left,
		EKeys::Gamepad_FaceButton_Top,
		EKeys::Gamepad_LeftShoulder,
		EKeys::Gamepad_RightShoulder,
		EKeys::Gamepad_LeftTrigger,
		EKeys::Gamepad_RightTrigger
	};

	for (const FKey& Key : GamepadKeys)
	{
		if (PC->IsInputKeyDown(Key))
		{
			return true;
		}
	}

	return false;
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL - ENHANCED INPUT KEY QUERY
// ═══════════════════════════════════════════════════════════════════════

FKey UInteractableWidget::GetBoundKeyForInputAction(UInputAction* InputAction) const
{
	if (!InputAction)
	{
		UE_LOG(LogInteractableWidget, Warning, TEXT("GetBoundKeyForInputAction: InputAction is null!"));
		return EKeys::Invalid;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem();
	if (!Subsystem)
	{
		UE_LOG(LogInteractableWidget, Warning, TEXT("GetBoundKeyForInputAction: Cannot get Enhanced Input Subsystem!"));
		return EKeys::Invalid;
	}

	// ═══════════════════════════════════════════════
	// QUERY ALL ACTIVE MAPPING CONTEXTS
	// ═══════════════════════════════════════════════
	TArray<FKey> BoundKeys;
	
	// Get all active mapping contexts
	TArray<FEnhancedActionKeyMapping> Mappings = Subsystem->GetAllPlayerMappableActionKeyMappings();
	
	for (const FEnhancedActionKeyMapping& Mapping : Mappings)
	{
		if (Mapping.Action == InputAction)
		{
			// Found a binding for this action
			FKey MappedKey = Mapping.Key;
			
			// ═══════════════════════════════════════════════
			// FILTER BY INPUT MODE
			// ═══════════════════════════════════════════════
			bool bIsGamepadKey = MappedKey.IsGamepadKey();
			
			// If current mode matches this key type, use it
			if (bIsUsingGamepad == bIsGamepadKey)
			{
				UE_LOG(LogInteractableWidget, Verbose, TEXT("Found bound key '%s' for InputAction '%s' (Mode: %s)"),
					*MappedKey.ToString(),
					*InputAction->GetName(),
					bIsUsingGamepad ? TEXT("Gamepad") : TEXT("Keyboard"));
				
				return MappedKey;
			}
			
			// Store for fallback
			BoundKeys.Add(MappedKey);
		}
	}

	// ═══════════════════════════════════════════════
	// FALLBACK: Return first found key (wrong mode is better than nothing)
	// ═══════════════════════════════════════════════
	if (BoundKeys.Num() > 0)
	{
		UE_LOG(LogInteractableWidget, Verbose, TEXT("Using fallback key '%s' for InputAction '%s' (mode mismatch)"),
			*BoundKeys[0].ToString(),
			*InputAction->GetName());
		return BoundKeys[0];
	}

	// ═══════════════════════════════════════════════
	// NO BINDING FOUND
	// ═══════════════════════════════════════════════
	UE_LOG(LogInteractableWidget, Warning, TEXT("No key binding found for InputAction '%s'!"),
		*InputAction->GetName());
	
	return EKeys::Invalid;
}

UEnhancedInputLocalPlayerSubsystem* UInteractableWidget::GetEnhancedInputSubsystem() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return nullptr;
	}

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return nullptr;
	}

	return LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
}
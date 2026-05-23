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

namespace InteractableWidgetMaterialParams
{
	const FName Progress(TEXT("Progress"));
	const FName ProgressLower(TEXT("progress"));
	const FName AnimationPhase(TEXT("AnimationPhase"));
	const FName FillColor(TEXT("FillColor"));
	const FName BackgroundColor(TEXT("BackgroundColor"));
}

// Seeds KeyboardIcons and GamepadIcons with nullptr slots so every key appears
// in the Blueprint details panel ready to assign.
UInteractableWidget::UInteractableWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	KeyboardIcons.Add(EKeys::A, nullptr);
	KeyboardIcons.Add(EKeys::B, nullptr);
	KeyboardIcons.Add(EKeys::C, nullptr);
	KeyboardIcons.Add(EKeys::D, nullptr);
	KeyboardIcons.Add(EKeys::E, nullptr);
	KeyboardIcons.Add(EKeys::F, nullptr);
	KeyboardIcons.Add(EKeys::G, nullptr);
	KeyboardIcons.Add(EKeys::H, nullptr);
	KeyboardIcons.Add(EKeys::I, nullptr);
	KeyboardIcons.Add(EKeys::J, nullptr);
	KeyboardIcons.Add(EKeys::K, nullptr);
	KeyboardIcons.Add(EKeys::L, nullptr);
	KeyboardIcons.Add(EKeys::M, nullptr);
	KeyboardIcons.Add(EKeys::N, nullptr);
	KeyboardIcons.Add(EKeys::O, nullptr);
	KeyboardIcons.Add(EKeys::P, nullptr);
	KeyboardIcons.Add(EKeys::Q, nullptr);
	KeyboardIcons.Add(EKeys::R, nullptr);
	KeyboardIcons.Add(EKeys::S, nullptr);
	KeyboardIcons.Add(EKeys::T, nullptr);
	KeyboardIcons.Add(EKeys::U, nullptr);
	KeyboardIcons.Add(EKeys::V, nullptr);
	KeyboardIcons.Add(EKeys::W, nullptr);
	KeyboardIcons.Add(EKeys::X, nullptr);
	KeyboardIcons.Add(EKeys::Y, nullptr);
	KeyboardIcons.Add(EKeys::Z, nullptr);

	KeyboardIcons.Add(EKeys::Zero,  nullptr);
	KeyboardIcons.Add(EKeys::One,   nullptr);
	KeyboardIcons.Add(EKeys::Two,   nullptr);
	KeyboardIcons.Add(EKeys::Three, nullptr);
	KeyboardIcons.Add(EKeys::Four,  nullptr);
	KeyboardIcons.Add(EKeys::Five,  nullptr);
	KeyboardIcons.Add(EKeys::Six,   nullptr);
	KeyboardIcons.Add(EKeys::Seven, nullptr);
	KeyboardIcons.Add(EKeys::Eight, nullptr);
	KeyboardIcons.Add(EKeys::Nine,  nullptr);

	KeyboardIcons.Add(EKeys::NumPadZero,     nullptr);
	KeyboardIcons.Add(EKeys::NumPadOne,      nullptr);
	KeyboardIcons.Add(EKeys::NumPadTwo,      nullptr);
	KeyboardIcons.Add(EKeys::NumPadThree,    nullptr);
	KeyboardIcons.Add(EKeys::NumPadFour,     nullptr);
	KeyboardIcons.Add(EKeys::NumPadFive,     nullptr);
	KeyboardIcons.Add(EKeys::NumPadSix,      nullptr);
	KeyboardIcons.Add(EKeys::NumPadSeven,    nullptr);
	KeyboardIcons.Add(EKeys::NumPadEight,    nullptr);
	KeyboardIcons.Add(EKeys::NumPadNine,     nullptr);
	KeyboardIcons.Add(EKeys::Multiply,       nullptr);
	KeyboardIcons.Add(EKeys::Add,            nullptr);
	KeyboardIcons.Add(EKeys::Subtract,       nullptr);
	KeyboardIcons.Add(EKeys::Decimal,        nullptr);
	KeyboardIcons.Add(EKeys::Divide,         nullptr);

	KeyboardIcons.Add(EKeys::F1,  nullptr);
	KeyboardIcons.Add(EKeys::F2,  nullptr);
	KeyboardIcons.Add(EKeys::F3,  nullptr);
	KeyboardIcons.Add(EKeys::F4,  nullptr);
	KeyboardIcons.Add(EKeys::F5,  nullptr);
	KeyboardIcons.Add(EKeys::F6,  nullptr);
	KeyboardIcons.Add(EKeys::F7,  nullptr);
	KeyboardIcons.Add(EKeys::F8,  nullptr);
	KeyboardIcons.Add(EKeys::F9,  nullptr);
	KeyboardIcons.Add(EKeys::F10, nullptr);
	KeyboardIcons.Add(EKeys::F11, nullptr);
	KeyboardIcons.Add(EKeys::F12, nullptr);

	KeyboardIcons.Add(EKeys::SpaceBar,  nullptr);
	KeyboardIcons.Add(EKeys::Enter,     nullptr);
	KeyboardIcons.Add(EKeys::Escape,    nullptr);
	KeyboardIcons.Add(EKeys::Tab,       nullptr);
	KeyboardIcons.Add(EKeys::BackSpace, nullptr);
	KeyboardIcons.Add(EKeys::Delete,    nullptr);
	KeyboardIcons.Add(EKeys::Insert,    nullptr);
	KeyboardIcons.Add(EKeys::Home,      nullptr);
	KeyboardIcons.Add(EKeys::End,       nullptr);
	KeyboardIcons.Add(EKeys::PageUp,    nullptr);
	KeyboardIcons.Add(EKeys::PageDown,  nullptr);

	KeyboardIcons.Add(EKeys::Up,    nullptr);
	KeyboardIcons.Add(EKeys::Down,  nullptr);
	KeyboardIcons.Add(EKeys::Left,  nullptr);
	KeyboardIcons.Add(EKeys::Right, nullptr);

	KeyboardIcons.Add(EKeys::LeftShift,   nullptr);
	KeyboardIcons.Add(EKeys::RightShift,  nullptr);
	KeyboardIcons.Add(EKeys::LeftControl, nullptr);
	KeyboardIcons.Add(EKeys::RightControl,nullptr);
	KeyboardIcons.Add(EKeys::LeftAlt,     nullptr);
	KeyboardIcons.Add(EKeys::RightAlt,    nullptr);
	KeyboardIcons.Add(EKeys::CapsLock,    nullptr);
	KeyboardIcons.Add(EKeys::NumLock,     nullptr);
	KeyboardIcons.Add(EKeys::ScrollLock,  nullptr);

	KeyboardIcons.Add(EKeys::Tilde,        nullptr); // ` ~
	KeyboardIcons.Add(EKeys::Hyphen,       nullptr); // - _
	KeyboardIcons.Add(EKeys::Equals,       nullptr); // = +
	KeyboardIcons.Add(EKeys::LeftBracket,  nullptr); // [ {
	KeyboardIcons.Add(EKeys::RightBracket, nullptr); // ] }
	KeyboardIcons.Add(EKeys::Backslash,    nullptr); // \ |
	KeyboardIcons.Add(EKeys::Semicolon,    nullptr); // ; :
	KeyboardIcons.Add(EKeys::Apostrophe,   nullptr); // ' "
	KeyboardIcons.Add(EKeys::Comma,        nullptr); // , <
	KeyboardIcons.Add(EKeys::Period,       nullptr); // . >
	KeyboardIcons.Add(EKeys::Slash,        nullptr); // / ?

	GamepadIcons.Add(EKeys::Gamepad_FaceButton_Bottom, nullptr);
	GamepadIcons.Add(EKeys::Gamepad_FaceButton_Right,  nullptr);
	GamepadIcons.Add(EKeys::Gamepad_FaceButton_Left,   nullptr);
	GamepadIcons.Add(EKeys::Gamepad_FaceButton_Top,    nullptr);

	GamepadIcons.Add(EKeys::Gamepad_LeftShoulder,  nullptr);
	GamepadIcons.Add(EKeys::Gamepad_RightShoulder, nullptr);
	GamepadIcons.Add(EKeys::Gamepad_LeftTrigger,   nullptr);
	GamepadIcons.Add(EKeys::Gamepad_RightTrigger,  nullptr);

	GamepadIcons.Add(EKeys::Gamepad_LeftThumbstick,  nullptr);
	GamepadIcons.Add(EKeys::Gamepad_RightThumbstick, nullptr);

	GamepadIcons.Add(EKeys::Gamepad_DPad_Up,    nullptr);
	GamepadIcons.Add(EKeys::Gamepad_DPad_Down,  nullptr);
	GamepadIcons.Add(EKeys::Gamepad_DPad_Left,  nullptr);
	GamepadIcons.Add(EKeys::Gamepad_DPad_Right, nullptr);

	GamepadIcons.Add(EKeys::Gamepad_Special_Left,  nullptr);
	GamepadIcons.Add(EKeys::Gamepad_Special_Right, nullptr);
}

void UInteractableWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bIsUsingGamepad = DetectGamepadMode();
	bLastInputModeGamepad = bIsUsingGamepad;

	UpdateBorderMaterial();
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

	InputCheckAccumulator += InDeltaTime;

	if (InputCheckAccumulator >= 0.15f)
	{
		InputCheckAccumulator = 0.0f;

		const bool bCurrentlyGamepad = DetectGamepadMode();
		if (bCurrentlyGamepad != bIsUsingGamepad)
		{
			bIsUsingGamepad = bCurrentlyGamepad;
			RefreshInputMode();

			UE_LOG(LogInteractableWidget, Verbose, TEXT("Input mode changed to: %s"),
				bIsUsingGamepad ? TEXT("Gamepad") : TEXT("Keyboard"));
		}
	}
	
	TickState(InDeltaTime);
}

void UInteractableWidget::SetInteractionData(UInputAction* InputAction, const FText& Description)
{
	if (!InputAction)
	{
		UE_LOG(LogInteractableWidget, Warning, TEXT("SetInteractionData called with null InputAction!"));
		CurrentInputKey = EKeys::Invalid;

		if (InteractionDescription)
		{
			InteractionDescription->SetText(Description);
		}
		return;
	}

	FKey BoundKey = GetBoundKeyForInputAction(InputAction);
	SetInteractionDataWithKey(BoundKey, Description);

	UE_LOG(LogInteractableWidget, Verbose, TEXT("SetInteractionData: InputAction='%s', BoundKey='%s', Description='%s'"),
		*InputAction->GetName(), *BoundKey.ToString(), *Description.ToString());
}

void UInteractableWidget::SetInteractionDataWithKey(const FKey& Key, const FText& Description)
{
	CurrentInputKey = Key;

	if (InteractionDescription)
	{
		InteractionDescription->SetText(Description);
	}

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

	switch (NewState)
	{
		case EInteractionWidgetState::IWS_Idle:
			CurrentProgress = 0.0f;
			LastSetProgress = -1.0f;
			break;

		case EInteractionWidgetState::IWS_Holding:
		case EInteractionWidgetState::IWS_Mashing:
			break;

		case EInteractionWidgetState::IWS_Completed:
			CurrentProgress = 1.0f;
			break;

		case EInteractionWidgetState::IWS_Cancelled:
			break;
	}

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
	if (CurrentState != EInteractionWidgetState::IWS_Holding &&
		CurrentState != EInteractionWidgetState::IWS_Mashing)
	{
		return;
	}

	float NewProgress = FMath::Clamp(Progress, 0.0f, 1.0f);

	if (FMath::Abs(NewProgress - LastSetProgress) > 0.001f)
	{
		CurrentProgress = NewProgress;
		LastSetProgress = NewProgress;
		UpdateMaterialParameters();
	}
}

void UInteractableWidget::RefreshInputMode()
{
	UpdateBorderMaterial();
	UpdateKeyIcon();
}

void UInteractableWidget::Show()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetWidgetState(EInteractionWidgetState::IWS_Idle);
}

void UInteractableWidget::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UInteractableWidget::TickState(float DeltaTime)
{
	StateTimer += DeltaTime;

	switch (CurrentState)
	{
		case EInteractionWidgetState::IWS_Idle:
		{
			if (bEnableIdleAnimation && BorderMID)
			{
				AnimationTime += DeltaTime * IdleAnimationSpeed;

			BorderMID->SetScalarParameterValue(InteractableWidgetMaterialParams::AnimationPhase, AnimationTime);

			const float IdlePulse = (FMath::Sin(AnimationTime * 2.0f) * 0.5f + 0.5f) * 0.05f;
				ApplyBorderProgress(IdlePulse);
			}
			break;
		}

		case EInteractionWidgetState::IWS_Holding:
		case EInteractionWidgetState::IWS_Mashing:
		{
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
				float FlashAlpha = 1.0f - (StateTimer / CompletionFlashDuration);
				if (BorderMID)
				{
					FLinearColor FlashColor = FillColorCompleted;
					FlashColor.A = FlashAlpha;
					BorderMID->SetVectorParameterValue(FName("FillColor"), FlashColor);
				}
			}
			break;
		}

		case EInteractionWidgetState::IWS_Cancelled:
		{
			float DepleteSpeed = 2.0f;
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

void UInteractableWidget::UpdateBorderMaterial()
{
	if (!Img_FillBorder)
	{
		UE_LOG(LogInteractableWidget, Warning, TEXT("UpdateBorderMaterial: Img_FillBorder not bound!"));
		return;
	}

	UMaterialInterface* SourceMaterial = bIsUsingGamepad ? CircleBorderMaterial : SquareBorderMaterial;

	if (!SourceMaterial)
	{
		UE_LOG(LogInteractableWidget, Warning, TEXT("UpdateBorderMaterial: No %s material assigned!"),
			bIsUsingGamepad ? TEXT("Circle") : TEXT("Square"));
		return;
	}

	BorderMID = UMaterialInstanceDynamic::Create(SourceMaterial, this);
	Img_FillBorder->SetBrushFromMaterial(BorderMID);

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

	ApplyBorderProgress(CurrentProgress);

	const FLinearColor FillColor = GetCurrentFillColor();
	BorderMID->SetVectorParameterValue(InteractableWidgetMaterialParams::FillColor, FillColor);
	BorderMID->SetVectorParameterValue(InteractableWidgetMaterialParams::BackgroundColor, BorderBackgroundColor);
}

void UInteractableWidget::ApplyBorderProgress(float ProgressValue)
{
	if (!BorderMID)
	{
		return;
	}

	const float ClampedProgress = FMath::Clamp(ProgressValue, 0.0f, 1.0f);
	BorderMID->SetScalarParameterValue(InteractableWidgetMaterialParams::Progress, ClampedProgress);
	BorderMID->SetScalarParameterValue(InteractableWidgetMaterialParams::ProgressLower, ClampedProgress);
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

void UInteractableWidget::UpdateKeyIcon()
{
	if (!Img_Key)
	{
		return;
	}

	if (!CurrentInputKey.IsValid())
	{
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

	const TMap<FKey, TObjectPtr<UTexture2D>>& IconMap = bIsUsingGamepad ? GamepadIcons : KeyboardIcons;

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

	if (FallbackIcon)
	{
		Img_Key->SetBrushFromTexture(FallbackIcon);
		Img_Key->SetVisibility(ESlateVisibility::HitTestInvisible);
		
		UE_LOG(LogInteractableWidget, Verbose, TEXT("No specific icon for key '%s', using fallback"),
			*CurrentInputKey.ToString());
		return;
	}

	Img_Key->SetVisibility(ESlateVisibility::Collapsed);
	
	UE_LOG(LogInteractableWidget, Warning, TEXT("No icon found for key '%s' in %s mode (no fallback configured)"),
		*CurrentInputKey.ToString(),
		bIsUsingGamepad ? TEXT("Gamepad") : TEXT("Keyboard"));
}

bool UInteractableWidget::DetectGamepadMode() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return false;
	}

	if (FMath::Abs(PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX)) > 0.1f ||
		FMath::Abs(PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY)) > 0.1f ||
		FMath::Abs(PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX)) > 0.1f ||
		FMath::Abs(PC->GetInputAnalogKeyState(EKeys::Gamepad_RightY)) > 0.1f)
	{
		return true;
	}

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

	TArray<FKey> BoundKeys;
	TArray<FEnhancedActionKeyMapping> Mappings = Subsystem->GetAllPlayerMappableActionKeyMappings();

	for (const FEnhancedActionKeyMapping& Mapping : Mappings)
	{
		if (Mapping.Action == InputAction)
		{
			FKey MappedKey = Mapping.Key;
			bool bIsGamepadKey = MappedKey.IsGamepadKey();

			if (bIsUsingGamepad == bIsGamepadKey)
			{
				UE_LOG(LogInteractableWidget, Verbose, TEXT("Found bound key '%s' for InputAction '%s' (Mode: %s)"),
					*MappedKey.ToString(),
					*InputAction->GetName(),
					bIsUsingGamepad ? TEXT("Gamepad") : TEXT("Keyboard"));
				
				return MappedKey;
			}

			BoundKeys.Add(MappedKey);
		}
	}

	if (BoundKeys.Num() > 0)
	{
		UE_LOG(LogInteractableWidget, Verbose, TEXT("Using fallback key '%s' for InputAction '%s' (mode mismatch)"),
			*BoundKeys[0].ToString(),
			*InputAction->GetName());
		return BoundKeys[0];
	}

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

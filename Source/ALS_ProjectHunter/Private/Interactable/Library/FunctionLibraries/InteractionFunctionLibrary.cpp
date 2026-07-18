#include "Interactable/Library/FunctionLibraries/InteractionFunctionLibrary.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

FText UInteractionFunctionLibrary::GetDisplayTextForConfig(const FInteractionConfig& Config)
{
	switch (Config.InteractionType)
	{
	case EInteractionType::IT_Tap:
		return Config.InteractionText;
	case EInteractionType::IT_Hold:
		return Config.HoldText;
	case EInteractionType::IT_Mash:
		return Config.MashText;
	case EInteractionType::IT_TapOrHold:
		return FText::Format(
			FText::FromString("{0}\n{1}"),
			Config.TapText,
			Config.HoldActionText);
	case EInteractionType::IT_Toggle:
	case EInteractionType::IT_Continuous:
	default:
		return Config.InteractionText;
	}
}

bool UInteractionFunctionLibrary::SupportsProgressBar(EInteractionType InteractionType)
{
	return InteractionType == EInteractionType::IT_Hold
		|| InteractionType == EInteractionType::IT_TapOrHold
		|| InteractionType == EInteractionType::IT_Mash;
}

EInteractionWidgetState UInteractionFunctionLibrary::GetProgressWidgetState(EInteractionType InteractionType)
{
	return InteractionType == EInteractionType::IT_Mash
		? EInteractionWidgetState::IWS_Mashing
		: EInteractionWidgetState::IWS_Holding;
}

FInteractableHighlightStyle UInteractionFunctionLibrary::MakeHighlightStyle(
	bool bEnableHighlight,
	int32 StencilValue,
	FLinearColor Color,
	float OutlineWidth,
	float Threshold)
{
	FInteractableHighlightStyle Style;
	Style.bEnableHighlight = bEnableHighlight;
	Style.StencilValue = StencilValue;
	Style.Color = Color;
	Style.OutlineWidth = OutlineWidth;
	Style.Threshold = Threshold;
	return Style;
}

bool UInteractionFunctionLibrary::GetInteractorView(AActor* Interactor, FVector& OutViewLocation, FRotator& OutViewRotation)
{
	if (!Interactor)
	{
		return false;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Interactor))
	{
		PlayerController->GetPlayerViewPoint(OutViewLocation, OutViewRotation);
		return true;
	}

	if (APawn* Pawn = Cast<APawn>(Interactor))
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController()))
		{
			PlayerController->GetPlayerViewPoint(OutViewLocation, OutViewRotation);
			return true;
		}
	}

	if (UCameraComponent* CameraComponent = Interactor->FindComponentByClass<UCameraComponent>())
	{
		OutViewLocation = CameraComponent->GetComponentLocation();
		OutViewRotation = CameraComponent->GetComponentRotation();
		return true;
	}

	OutViewLocation = Interactor->GetActorLocation();
	OutViewRotation = Interactor->GetActorRotation();
	return true;
}

FRotator UInteractionFunctionLibrary::CalculateCameraFacingRotation(
	FVector WidgetLocation,
	FVector CameraLocation,
	FRotator CurrentRotation,
	float DeltaTime,
	float RotationSmoothSpeed)
{
	const FVector DirectionToCamera = (CameraLocation - WidgetLocation).GetSafeNormal();
	const FRotator TargetRotation = DirectionToCamera.Rotation();

	if (RotationSmoothSpeed > 0.0f && DeltaTime > 0.0f)
	{
		return FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationSmoothSpeed);
	}

	return TargetRotation;
}

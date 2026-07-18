#pragma once

#include "CoreMinimal.h"
#include "Interactable/Library/Structs/InteractionStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InteractionFunctionLibrary.generated.h"

class AActor;

UCLASS()
class ALS_PROJECTHUNTER_API UInteractionFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Interaction|Config")
	static FText GetDisplayTextForConfig(const FInteractionConfig& Config);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Interaction|Config")
	static bool SupportsProgressBar(EInteractionType InteractionType);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Interaction|Widget")
	static EInteractionWidgetState GetProgressWidgetState(EInteractionType InteractionType);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Interaction|Highlight")
	static FInteractableHighlightStyle MakeHighlightStyle(
		bool bEnableHighlight,
		int32 StencilValue,
		FLinearColor Color,
		float OutlineWidth,
		float Threshold);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Interaction|Camera")
	static bool GetInteractorView(AActor* Interactor, FVector& OutViewLocation, FRotator& OutViewRotation);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Interaction|Camera")
	static FRotator CalculateCameraFacingRotation(
		FVector WidgetLocation,
		FVector CameraLocation,
		FRotator CurrentRotation,
		float DeltaTime,
		float RotationSmoothSpeed);
};

#pragma once

#include "CoreMinimal.h"

class UInteractableWidget;
enum class EInteractionWidgetState : uint8;

class ALS_PROJECTHUNTER_API FInteractableWidgetPresentationHelper
{
public:
	static void SetIdle(UInteractableWidget& Widget, bool bResetProgress);
	static void StartProgress(UInteractableWidget& Widget, EInteractionWidgetState State);
	static void UpdateProgress(UInteractableWidget& Widget, EInteractionWidgetState State, float Progress);
	static void SetCompleted(UInteractableWidget& Widget);
	static void SetCancelled(UInteractableWidget& Widget, bool bResetProgress);
};

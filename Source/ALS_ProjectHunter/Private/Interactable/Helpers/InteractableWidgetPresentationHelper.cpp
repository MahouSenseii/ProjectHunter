#include "Interactable/Helpers/InteractableWidgetPresentationHelper.h"

#include "Interactable/Widget/InteractableWidget.h"

void FInteractableWidgetPresentationHelper::SetIdle(UInteractableWidget& Widget, bool bResetProgress)
{
	Widget.SetWidgetState(EInteractionWidgetState::IWS_Idle);
	Widget.SetProgressBarVisible(false);

	if (bResetProgress)
	{
		Widget.SetProgress(0.0f);
	}
}

void FInteractableWidgetPresentationHelper::StartProgress(UInteractableWidget& Widget, EInteractionWidgetState State)
{
	Widget.SetWidgetState(State);
	Widget.SetProgress(0.0f);
}

void FInteractableWidgetPresentationHelper::UpdateProgress(UInteractableWidget& Widget, EInteractionWidgetState State, float Progress)
{
	if (Widget.GetWidgetState() != State)
	{
		Widget.SetWidgetState(State);
	}

	Widget.SetProgress(FMath::Clamp(Progress, 0.0f, 1.0f));
}

void FInteractableWidgetPresentationHelper::SetCompleted(UInteractableWidget& Widget)
{
	Widget.SetWidgetState(EInteractionWidgetState::IWS_Completed);
	Widget.SetProgressBarVisible(true);
	Widget.SetProgress(1.0f);
}

void FInteractableWidgetPresentationHelper::SetCancelled(UInteractableWidget& Widget, bool bResetProgress)
{
	Widget.SetWidgetState(EInteractionWidgetState::IWS_Cancelled);
	Widget.SetProgressBarVisible(true);

	if (bResetProgress)
	{
		Widget.SetProgress(0.0f);
	}
}

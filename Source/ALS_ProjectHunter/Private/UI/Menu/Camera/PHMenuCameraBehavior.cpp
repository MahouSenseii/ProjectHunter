// Author: Quentin Davis

#include "UI/Menu/Camera/PHMenuCameraBehavior.h"

void UPHMenuCameraBehavior::UpdateMenuState(
	const EMenuType InMenuType,
	const EEquipmentSlot InFocusedSlot,
	const float InZoomMultiplier,
	const float InTurntableYaw,
	const bool bInDragging,
	const bool bInMenuCameraActive)
{
	MenuType = InMenuType;
	FocusedEquipmentSlot = InFocusedSlot;
	bHasEquipmentFocus = InFocusedSlot != EEquipmentSlot::ES_None;
	ZoomMultiplier = InZoomMultiplier;
	TurntableYaw = InTurntableYaw;
	bDragging = bInDragging;
	bMenuCameraActive = bInMenuCameraActive;
}

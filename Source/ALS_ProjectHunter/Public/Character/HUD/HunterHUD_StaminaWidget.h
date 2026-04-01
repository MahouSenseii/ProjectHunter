// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Character/HUD/HunterHUDResourceWidget.h"
#include "HunterHUD_StaminaWidget.generated.h"

/**
 * Concrete resource widget pre-wired to the Stamina pool.
 *
 * Attribute mapping (set in constructor, override in BP defaults if needed):
 *   Current   → Stamina
 *   Max       → MaxEffectiveStamina   (accounts for reserved portion)
 *   Reserved  → ReservedStamina       (greyed segment on the right of the bar)
 *
 * Blueprint usage:
 *   Create a Blueprint child, implement OnResourceUpdated to drive the bar.
 *   OnCurrentValueChanged with a negative Delta indicates sprint drain — useful
 *   for triggering a stamina-low pulse animation at a threshold (e.g. < 20 %).
 */
UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UHunterHUD_StaminaWidget : public UHunterHUDResourceWidget
{
	GENERATED_BODY()

public:
	UHunterHUD_StaminaWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Character/HUD/HunterHUDResourceWidget.h"
#include "HunterHUD_HealthWidget.generated.h"

/**
 * Concrete resource widget pre-wired to the Health pool.
 *
 * Attribute mapping (set in constructor, override in BP defaults if needed):
 *   Current   → Health
 *   Max       → MaxEffectiveHealth   (accounts for reserved portion)
 *   Reserved  → ReservedHealth       (greyed segment on the right of the bar)
 *
 * Blueprint usage:
 *   Create a Blueprint child of this class, implement OnResourceUpdated to
 *   drive your progress bar UMG widgets, and add it to the HUD layout.
 *   Use OnCurrentValueChanged(NewValue, Delta) to trigger damage-flash or
 *   heal-pulse animations (Delta < 0 → damage, Delta > 0 → heal).
 */
UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UHunterHUD_HealthWidget : public UHunterHUDResourceWidget
{
	GENERATED_BODY()

public:
	UHunterHUD_HealthWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Character/HUD/HunterHUDResourceWidget.h"
#include "HunterHUD_ManaWidget.generated.h"

/**
 * Concrete resource widget pre-wired to the Mana pool.
 *
 * Attribute mapping (set in constructor, override in BP defaults if needed):
 *   Current   → Mana
 *   Max       → MaxEffectiveMana   (accounts for reserved portion)
 *   Reserved  → ReservedMana       (greyed segment on the right of the bar)
 *
 * Blueprint usage:
 *   Create a Blueprint child, implement OnResourceUpdated to drive the bar.
 *   OnCurrentValueChanged with a negative Delta indicates spell cost — useful
 *   for triggering a mana-depleted flash when the pool runs dry.
 */
UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UHunterHUD_ManaWidget : public UHunterHUDResourceWidget
{
	GENERATED_BODY()

public:
	UHunterHUD_ManaWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "HunterGE_ArcaneShieldRegen.generated.h"

/**
 * Instant GE used by the passive-regen timer to restore ArcaneShield.
 * Modifier: Add on HunterAttributeSet.ArcaneShield, magnitude via Data.Recovery.ArcaneShield SetByCaller.
 * The timer sets the magnitude each tick via SetSetByCallerMagnitude before applying.
 * ArcaneShield regen is gated by the Effect.ArcaneShield.RegenActive tag (handled by the timer).
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterGE_ArcaneShieldRegen : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UHunterGE_ArcaneShieldRegen();
};

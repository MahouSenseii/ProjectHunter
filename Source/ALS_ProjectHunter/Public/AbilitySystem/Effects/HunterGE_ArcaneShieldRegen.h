#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "HunterGE_ArcaneShieldRegen.generated.h"

/**
 * Instant GE used by the passive-regen timer to restore ArcaneShield.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterGE_ArcaneShieldRegen : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UHunterGE_ArcaneShieldRegen();
};

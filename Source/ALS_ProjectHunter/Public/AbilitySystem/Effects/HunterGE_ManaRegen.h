#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "HunterGE_ManaRegen.generated.h"

/**
 * Instant GE used by the passive-regen timer to restore Mana.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterGE_ManaRegen : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UHunterGE_ManaRegen();
};

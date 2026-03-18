#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "HunterGE_DerivedPrimaryVitals.generated.h"

/**
 * Infinite derived-vitals GE powered by MMCs.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterGE_DerivedPrimaryVitals : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UHunterGE_DerivedPrimaryVitals();
};

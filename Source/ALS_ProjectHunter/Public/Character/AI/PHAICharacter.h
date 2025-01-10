// Copyright@2024 Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Character/PHBaseCharacter.h"
#include "PHAICharacter.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API APHAICharacter : public APHBaseCharacter
{
	GENERATED_BODY()
public:
	APHAICharacter(const FObjectInitializer& ObjectInitializer);

	
protected:

	virtual void BeginPlay() override;

};

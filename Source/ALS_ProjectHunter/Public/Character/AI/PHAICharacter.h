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

	/** Combat Interface */

	virtual int32 GetPlayerLevel() override;
	/** End Combat Interface */
	
protected:

	virtual void BeginPlay() override;

	virtual void InitAbilityActorInfo() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
	int32 Level = 1;
	
};

// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "AI/Mob/MobManagerActor.h"
#include "Character/PHBaseCharacter.h"
#include "PHGenerationReviewTestActors.generated.h"

/** Records the configuration the real encounter lifecycle receives, without spawning test enemies. */
UCLASS(NotPlaceable, Transient)
class APHGenerationReviewTestManager : public AMobManagerActor
{
	GENERATED_BODY()

public:
	APHGenerationReviewTestManager();
	virtual void BeginPlay() override;

	int32 BudgetAtBeginPlay = INDEX_NONE;
	int32 MobTypesAtBeginPlay = INDEX_NONE;
	int32 SeedAtBeginPlay = 0;
	FVector ExtentAtBeginPlay = FVector::ZeroVector;
};

/** Concrete, unpossessed native pawn for encounter cleanup checks. */
UCLASS(NotPlaceable, Transient)
class APHGenerationReviewTestCharacter : public APHBaseCharacter
{
	GENERATED_BODY()

public:
	explicit APHGenerationReviewTestCharacter(const FObjectInitializer& ObjectInitializer);
};

// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "Tests/PHGenerationReviewTestActors.h"
#include "Components/BoxComponent.h"

APHGenerationReviewTestManager::APHGenerationReviewTestManager()
{
	bUseActorPooling = false;
	bInitialBurst = false;
	SpawnInterval = 600.0f;
}

void APHGenerationReviewTestManager::BeginPlay()
{
	BudgetAtBeginPlay = MaxNumOfMobs;
	MobTypesAtBeginPlay = MobTypes.Num();
	SeedAtBeginPlay = EncounterSeedOverride;
	ExtentAtBeginPlay = SpawnArea->GetUnscaledBoxExtent();
	Super::BeginPlay();
}

APHGenerationReviewTestCharacter::APHGenerationReviewTestCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::Disabled;
}

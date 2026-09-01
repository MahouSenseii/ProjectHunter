// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "Tests/PHWallTraversalTestProbe.h"

UPHWallTraversalTestMovementComponent::UPHWallTraversalTestMovementComponent(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPHWallTraversalTestMovementComponent::ConfigurePhysicsForTest()
{
	bRunPhysicsWithNoController = true;
	bAlignCapsuleToWall = false;
	WallMovementFriction = 0.0f;
	WallBrakingDeceleration = 0.0f;
	WallAcceleration = 1000.0f;
	MaxSimulationTimeStep = 0.03125f;
	MaxSimulationIterations = 8;
}

void UPHWallTraversalTestMovementComponent::SampleWallSurface(
	const FHitResult& Hit, const bool bInitial, const float DeltaTime)
{
	UpdateWallSurface(Hit, bInitial, DeltaTime);
}

APHWallTraversalTestCharacter::APHWallTraversalTestCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPHWallTraversalTestMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::Disabled;
}

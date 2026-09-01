// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Character/Components/PHCharacterMovementComponent.h"
#include "Character/PHBaseCharacter.h"
#include "PHWallTraversalTestProbe.generated.h"

/** Exposes protected movement operations only to editor automation. */
UCLASS(Transient)
class UPHWallTraversalTestMovementComponent : public UPHCharacterMovementComponent
{
	GENERATED_BODY()

public:
	explicit UPHWallTraversalTestMovementComponent(const FObjectInitializer& ObjectInitializer);

	void ConfigurePhysicsForTest();
	void EnableCapsuleAlignment() { bAlignCapsuleToWall = true; }
	void SetInputAcceleration(const FVector& Input)
	{
		Acceleration = Input;
		AnalogInputModifier = ComputeAnalogInputModifier();
	}
	void SimulateWallPhysics(float DeltaTime) { PhysCustom(DeltaTime, 0); }
	void SampleWallSurface(const FHitResult& Hit, bool bInitial, float DeltaTime);
	void StartGroundTransition(const FHitResult& Hit) { BeginWallToGroundTransition(Hit); }
	bool ProbeStandingFloor(FHitResult& Hit) const { return IsStandingOnWalkableFloor(Hit); }
	bool HasSavedRootMotionMode() const { return bHasSavedWallTraversalRootMotionMode; }
	bool WantsWallInput() const { return bWantsWallTraversalInput; }
};

/** Concrete native character with no Blueprint or authored content dependency. */
UCLASS(NotPlaceable, Transient)
class APHWallTraversalTestCharacter : public APHBaseCharacter
{
	GENERATED_BODY()

public:
	explicit APHWallTraversalTestCharacter(const FObjectInitializer& ObjectInitializer);
};

// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Effects/HunterGE_StaminaDegen.h"
#include "AbilitySystem/HunterAbilitySystemComponent.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "Animation/AnimInstance.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tags/Components/TagManager.h"
#include "Tests/AutomationCommon.h"
#include "Tests/PHWallTraversalTestProbe.h"

namespace PHWallTraversalTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	constexpr double Tolerance = 0.0001;

	struct FTraversalFixture
	{
		FTestWorldWrapper TestWorld;
		APHWallTraversalTestCharacter* Character = nullptr;
		UPHWallTraversalTestMovementComponent* Movement = nullptr;
		UHunterAbilitySystemComponent* ASC = nullptr;
		UAnimInstance* Animation = nullptr;

		~FTraversalFixture()
		{
			if (ASC)
			{
				ASC->RemoveActiveEffects(FGameplayEffectQuery());
			}
			if (Character && Character->GetMesh())
			{
				Character->GetMesh()->AnimScriptInstance = nullptr;
			}
		}

		bool Initialize(FAutomationTestBase& Test, const FVector& Location = FVector(44.0, 0.0, 300.0))
		{
			if (!TestWorld.CreateTestWorld(EWorldType::GamePreview))
			{
				TestWorld.ForwardErrorMessages(&Test);
				return false;
			}
			TestWorld.GetTestWorld()->DeltaTimeSeconds = 0.1f;
			Test.TestNull(TEXT("Traversal fixture does not create a game instance"), TestWorld.GetTestWorld()->GetGameInstance());
			FActorSpawnParameters Parameters;
			Parameters.bDeferConstruction = true;
			Parameters.ObjectFlags = RF_Transient;
			Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Character = TestWorld.GetTestWorld()->SpawnActor<APHWallTraversalTestCharacter>(
				APHWallTraversalTestCharacter::StaticClass(), FTransform(Location), Parameters);
			if (!Test.TestNotNull(TEXT("The native test character spawned"), Character))
			{
				return false;
			}
			Character->FinishSpawning(FTransform(Location));
			if (!Character->IsActorInitialized())
			{
				Character->PreInitializeComponents();
				Character->InitializeComponents();
				Character->PostInitializeComponents();
			}
			Character->GetCapsuleComponent()->SetCapsuleSize(42.0f, 96.0f);
			Movement = Cast<UPHWallTraversalTestMovementComponent>(Character->GetCharacterMovement());
			ASC = Cast<UHunterAbilitySystemComponent>(Character->GetAbilitySystemComponent());
			if (!Test.TestNotNull(TEXT("The native movement probe is the character's real movement component"), Movement) ||
				!Test.TestNotNull(TEXT("The character has its real ASC"), ASC))
			{
				return false;
			}
			ASC->InitAbilityActorInfo(Character, Character);
			UHunterAttributeSet* Attributes = const_cast<UHunterAttributeSet*>(ASC->GetSet<UHunterAttributeSet>());
			if (!Test.TestNotNull(TEXT("Native component initialization registered the live attributes"), Attributes))
			{
				return false;
			}
			Attributes->InitMaxStamina(100.0f);
			Attributes->InitMaxEffectiveStamina(100.0f);
			Attributes->InitStamina(100.0f);
			Attributes->InitStaminaDegenRate(1.0f);
			Attributes->InitStaminaDegenAmount(10.0f);
			Character->FindComponentByClass<UTagManager>()->Initialize(ASC);
			Movement->ConfigurePhysicsForTest();
			Movement->SetMovementMode(MOVE_Falling);
			// Only root-motion mode is observed; no skeleton, animation update, or viewport is needed.
			Animation = NewObject<UAnimInstance>(Character->GetMesh());
			Character->GetMesh()->AnimScriptInstance = Animation;
			Animation->SetRootMotionMode(ERootMotionMode::RootMotionFromEverything);
			return Test.TestFalse(TEXT("No character BeginPlay ran"), Character->HasActorBegunPlay()) &&
				Test.TestNull(TEXT("The test character remains unpossessed"), Character->GetController()) &&
				Test.TestTrue(TEXT("The fixture has usable stamina"), Character->CanUseStaminaMovement());
		}

		UBoxComponent* AddBox(const FVector& Center, const FVector& Extent)
		{
			AActor* Surface = TestWorld.GetTestWorld()->SpawnActor<AActor>();
			UBoxComponent* Box = NewObject<UBoxComponent>(Surface);
			Surface->SetRootComponent(Box);
			Surface->AddInstanceComponent(Box);
			Box->SetMobility(EComponentMobility::Static);
			Box->SetBoxExtent(Extent);
			Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Box->SetCollisionObjectType(ECC_WorldStatic);
			Box->SetCollisionResponseToAllChannels(ECR_Block);
			Box->SetCanEverAffectNavigation(false);
			Box->SetWorldLocation(Center);
			Box->RegisterComponent();
			return Box;
		}

		bool Trace(const FVector& End, FHitResult& Hit) const
		{
			return TestWorld.GetTestWorld()->LineTraceSingleByChannel(Hit, Character->GetActorLocation(), End,
				ECC_Visibility, FCollisionQueryParams(SCENE_QUERY_STAT(PHWallTraversalTest), false, Character));
		}

		bool AttachToFlatWall(FAutomationTestBase& Test, const bool bHoldSprint = false)
		{
			AddBox(FVector(-20.0, 0.0, 300.0), FVector(20.0, 1000.0, 1000.0));
			Movement->Velocity = FVector(-100.0, 0.0, 0.0);
			FHitResult Hit;
			if (!Test.TestTrue(TEXT("The attachment uses a real flat wall hit"), Trace(FVector(-40.0, 0.0, 300.0), Hit)))
			{
				return false;
			}
			if (bHoldSprint)
			{
				Character->SprintAction_Implementation(true);
			}
			const bool bAttached = Movement->IsWallRunning() ||
				Movement->TryStartWallTraversal(EALSMovementState::WallRunning, &Hit);
			return Test.TestTrue(TEXT("The public wall-start path attaches"), bAttached) &&
				Test.TestTrue(TEXT("Traversal extracts but ignores montage root motion"),
					Animation->RootMotionMode == ERootMotionMode::IgnoreRootMotion);
		}

		int32 DrainEffectCount() const
		{
			return ASC->GetGameplayEffectCount(UHunterGE_StaminaDegen::StaticClass(), ASC, false);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHWallTraversalExternalExitTest,
	"ProjectHunter.Movement.WallTraversal.ExternalModeChangesCleanUp", PHWallTraversalTests::TestFlags)

bool FPHWallTraversalExternalExitTest::RunTest(const FString&)
{
	using namespace PHWallTraversalTests;
	for (const EMovementMode TargetMode : {MOVE_None, MOVE_Falling, MOVE_Walking})
	{
		FTraversalFixture Fixture;
		if (!Fixture.Initialize(*this) || !Fixture.AttachToFlatWall(*this))
		{
			return false;
		}
		Fixture.Movement->SetWallTraversalCombatMovementLocked(true);
		TestTrue(TEXT("Traversal saved the pre-wall animation mode"), Fixture.Movement->HasSavedRootMotionMode());
		Fixture.Movement->SetMovementMode(TargetMode);
		TestFalse(TEXT("External movement mode change exits traversal"), Fixture.Movement->IsWallTraversing());
		TestTrue(TEXT("External exit clears the wall normal consumed by animation"), Fixture.Movement->GetWallNormal().IsNearlyZero());
		TestFalse(TEXT("External exit clears the wall combat lock"), Fixture.Movement->IsWallTraversalCombatMovementLocked());
		TestFalse(TEXT("External exit releases the saved root-motion override"), Fixture.Movement->HasSavedRootMotionMode());
		TestTrue(TEXT("External exit restores the original animation root-motion mode"),
			Fixture.Animation->RootMotionMode == ERootMotionMode::RootMotionFromEverything);
		TestEqual(TEXT("External exit removes the wall stamina drain"), Fixture.DrainEffectCount(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHWallTraversalInternalTransitionTest,
	"ProjectHunter.Movement.WallTraversal.InternalTransitionsPreserveState", PHWallTraversalTests::TestFlags)

bool FPHWallTraversalInternalTransitionTest::RunTest(const FString&)
{
	using namespace PHWallTraversalTests;
	FTraversalFixture Fixture;
	if (!Fixture.Initialize(*this) || !Fixture.AttachToFlatWall(*this, true))
	{
		return false;
	}
	const FVector WallNormal = Fixture.Movement->GetWallNormal();
	Fixture.Movement->SetWallTraversalCombatMovementLocked(true);
	Fixture.Movement->SetMovementMode(MOVE_Custom, static_cast<uint8>(EPHCustomMovementMode::WallClimbing));
	TestTrue(TEXT("Run to climb keeps the wall surface"), Fixture.Movement->GetWallNormal().Equals(WallNormal));
	TestTrue(TEXT("Run to climb keeps the wall combat lock"), Fixture.Movement->IsWallTraversalCombatMovementLocked());
	TestTrue(TEXT("Run to climb retains IgnoreRootMotion"), Fixture.Animation->RootMotionMode == ERootMotionMode::IgnoreRootMotion);
	Fixture.AddBox(FVector(500.0, 0.0, -10.0), FVector(500.0, 500.0, 10.0));
	FHitResult Ground;
	if (!TestTrue(TEXT("Ground transfer uses a real floor hit"), Fixture.Trace(FVector(44.0, 0.0, -20.0), Ground)))
	{
		return false;
	}
	Fixture.Movement->StartGroundTransition(Ground);
	TestTrue(TEXT("A valid floor starts the ground transition"), Fixture.Movement->IsTransitioningFromWallToGround());
	TestTrue(TEXT("Ground transfer retains its presentation data"), Fixture.Movement->GetWallTransitionData().bActive);
	TestTrue(TEXT("Ground transfer remembers that it started climbing"), Fixture.Movement->GetWallTransitionData().bStartedFromWallClimbing);
	TestTrue(TEXT("Ground transfer retains the outgoing wall"), Fixture.Movement->GetWallNormal().Equals(WallNormal));
	Fixture.Character->SprintAction_Implementation(false);
	TestTrue(TEXT("Releasing traversal input does not cancel an in-progress ground transfer"),
		Fixture.Movement->IsTransitioningFromWallToGround());
	Fixture.Movement->CompleteWallToGroundTransition();
	TestFalse(TEXT("Completed ground transfer leaves traversal"), Fixture.Movement->IsWallTraversing());
	TestFalse(TEXT("Completed ground transfer clears presentation state"), Fixture.Movement->GetWallTransitionData().bActive);
	TestTrue(TEXT("Completed ground transfer clears the outgoing normal"), Fixture.Movement->GetWallNormal().IsNearlyZero());
	TestTrue(TEXT("Completed ground transfer restores animation root motion"),
		Fixture.Animation->RootMotionMode == ERootMotionMode::RootMotionFromEverything);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHWallTraversalSubstepAccelerationTest,
	"ProjectHunter.Movement.WallTraversal.CombatScaleDoesNotCompoundAcrossSubsteps", PHWallTraversalTests::TestFlags)

bool FPHWallTraversalSubstepAccelerationTest::RunTest(const FString&)
{
	using namespace PHWallTraversalTests;
	FTraversalFixture LongFrame;
	FTraversalFixture ShortFrames;
	if (!LongFrame.Initialize(*this) || !LongFrame.AttachToFlatWall(*this) ||
		!ShortFrames.Initialize(*this) || !ShortFrames.AttachToFlatWall(*this))
	{
		return false;
	}
	const FVector Input(0.0, 100.0, 0.0);
	LongFrame.Movement->SetWallTraversalCombatMovementScale(0.5f);
	ShortFrames.Movement->SetWallTraversalCombatMovementScale(0.5f);
	LongFrame.Movement->SetInputAcceleration(Input);
	LongFrame.Movement->SimulateWallPhysics(0.125f);
	for (int32 Frame = 0; Frame < 4; ++Frame)
	{
		ShortFrames.Movement->SetInputAcceleration(Input);
		ShortFrames.Movement->SimulateWallPhysics(0.03125f);
	}
	TestTrue(TEXT("Both frame partitions remain on the real wall"),
		LongFrame.Movement->IsWallRunning() && ShortFrames.Movement->IsWallRunning());
	TestTrue(TEXT("A long frame preserves the original steering acceleration"),
		LongFrame.Movement->GetCurrentAcceleration().Equals(Input, Tolerance));
	TestEqual(TEXT("Half of 100 acceleration over 0.125 seconds gives 6.25 tangent speed"),
		LongFrame.Movement->Velocity.Y, 6.25, Tolerance);
	TestTrue(TEXT("One substepped frame matches four frames with refreshed input"),
		LongFrame.Movement->Velocity.Equals(ShortFrames.Movement->Velocity, Tolerance));
	TestTrue(TEXT("Frame partition does not change the travelled distance"),
		LongFrame.Character->GetActorLocation().Equals(ShortFrames.Character->GetActorLocation(), Tolerance));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHWallTraversalEligibleCandidateTest,
	"ProjectHunter.Movement.WallTraversal.ChoosesAnApproachedWallBeforeDistance", PHWallTraversalTests::TestFlags)

bool FPHWallTraversalEligibleCandidateTest::RunTest(const FString&)
{
	using namespace PHWallTraversalTests;
	FTraversalFixture Fixture;
	if (!Fixture.Initialize(*this, FVector(0.0, 0.0, 300.0)))
	{
		return false;
	}
	Fixture.AddBox(FVector(-65.0, 0.0, 300.0), FVector(10.0, 500.0, 500.0));
	Fixture.AddBox(FVector(90.0, 0.0, 300.0), FVector(10.0, 500.0, 500.0));
	Fixture.Movement->Velocity = FVector(100.0, 0.0, 0.0);
	TestTrue(TEXT("The nearby receding wall does not veto the farther approached wall"),
		Fixture.Movement->TryStartWallTraversal(EALSMovementState::WallRunning));
	TestTrue(TEXT("Traversal attaches to the wall in the movement direction"),
		Fixture.Movement->GetWallNormal().Equals(-FVector::ForwardVector, Tolerance));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHWallTraversalNormalTimeTest,
	"ProjectHunter.Movement.WallTraversal.NormalSmoothingUsesSimulationTime", PHWallTraversalTests::TestFlags)

bool FPHWallTraversalNormalTimeTest::RunTest(const FString&)
{
	using namespace PHWallTraversalTests;
	FTraversalFixture WholeStep;
	FTraversalFixture SplitSteps;
	if (!WholeStep.Initialize(*this) || !SplitSteps.Initialize(*this))
	{
		return false;
	}
	FHitResult Sample;
	Sample.bBlockingHit = true;
	Sample.ImpactPoint = FVector(0.0, 0.0, 300.0);
	Sample.Normal = Sample.ImpactNormal = FVector::ForwardVector;
	WholeStep.Movement->SampleWallSurface(Sample, true, 0.0f);
	SplitSteps.Movement->SampleWallSurface(Sample, true, 0.0f);
	Sample.Normal = Sample.ImpactNormal = FVector(1.0, 1.0, 0.0).GetSafeNormal();
	WholeStep.Movement->SampleWallSurface(Sample, false, 0.1f);
	for (int32 Step = 0; Step < 4; ++Step)
	{
		SplitSteps.Movement->SampleWallSurface(Sample, false, 0.025f);
	}
	TestFalse(TEXT("A surface change actually advances the normal"),
		WholeStep.Movement->GetWallNormal().Equals(FVector::ForwardVector, Tolerance));
	TestTrue(TEXT("Equal simulated time gives the same normal despite fixed world-frame delta"),
		WholeStep.Movement->GetWallNormal().Equals(SplitSteps.Movement->GetWallNormal(), Tolerance));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHWallTraversalFloorMarginTest,
	"ProjectHunter.Movement.WallTraversal.FloorContactMarginExcludesSweepRadius", PHWallTraversalTests::TestFlags)

bool FPHWallTraversalFloorMarginTest::RunTest(const FString&)
{
	using namespace PHWallTraversalTests;
	FTraversalFixture Fixture;
	if (!Fixture.Initialize(*this, FVector(0.0, 0.0, 65.0)))
	{
		return false;
	}
	Fixture.AddBox(FVector(0.0, 0.0, -10.0), FVector(500.0, 500.0, 10.0));
	Fixture.Character->SetActorRotation(FRotator(90.0, 0.0, 0.0));
	FHitResult Ground;
	TestFalse(TEXT("A floor 23 units below the rotated capsule is outside the five-unit contact margin"),
		Fixture.Movement->ProbeStandingFloor(Ground));
	Fixture.Character->SetActorLocation(FVector(0.0, 0.0, 45.0));
	TestTrue(TEXT("A floor three units below the capsule is within the contact margin"),
		Fixture.Movement->ProbeStandingFloor(Ground));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHWallTraversalReleasedInputTest,
	"ProjectHunter.Movement.WallTraversal.ReleasedInputDetachesButExplicitStartsRemain", PHWallTraversalTests::TestFlags)

bool FPHWallTraversalReleasedInputTest::RunTest(const FString&)
{
	using namespace PHWallTraversalTests;
	FTraversalFixture HeldInput;
	if (!HeldInput.Initialize(*this) || !HeldInput.AttachToFlatWall(*this, true))
	{
		return false;
	}
	TestTrue(TEXT("Accepted held input reaches predicted movement immediately"), HeldInput.Movement->WantsWallInput());
	TestEqual(TEXT("The attached character pays wall stamina drain"), HeldInput.DrainEffectCount(), 1);
	HeldInput.Character->SprintAction_Implementation(false);
	TestFalse(TEXT("Releasing the accepted input detaches from the wall"), HeldInput.Movement->IsWallTraversing());
	TestFalse(TEXT("Released input is also published to predicted movement"), HeldInput.Movement->WantsWallInput());
	TestEqual(TEXT("Releasing traversal removes its stamina drain"), HeldInput.DrainEffectCount(), 0);
	TestTrue(TEXT("Releasing traversal restores animation root motion"),
		HeldInput.Animation->RootMotionMode == ERootMotionMode::RootMotionFromEverything);

	FTraversalFixture ExplicitStart;
	if (!ExplicitStart.Initialize(*this) || !ExplicitStart.AttachToFlatWall(*this))
	{
		return false;
	}
	ExplicitStart.Character->RefreshStaminaMovementInput();
	TestTrue(TEXT("Refreshing an unheld input does not cancel a direct wall-start request"),
		ExplicitStart.Movement->IsWallRunning());
	ExplicitStart.Character->SprintAction_Implementation(false);
	TestTrue(TEXT("An already released button does not cancel explicit traversal"), ExplicitStart.Movement->IsWallRunning());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHWallTraversalAlignedJumpTest,
	"ProjectHunter.Movement.WallTraversal.AlignedCapsuleCanRunAndJumpAway", PHWallTraversalTests::TestFlags)

bool FPHWallTraversalAlignedJumpTest::RunTest(const FString&)
{
	using namespace PHWallTraversalTests;
	FTraversalFixture Fixture;
	if (!Fixture.Initialize(*this))
	{
		return false;
	}
	Fixture.Movement->EnableCapsuleAlignment();
	if (!Fixture.AttachToFlatWall(*this, true))
	{
		return false;
	}
	for (int32 Frame = 0; Frame < 16; ++Frame)
	{
		Fixture.Movement->SetInputAcceleration(FVector(0.0, 100.0, 0.0));
		Fixture.Movement->SimulateWallPhysics(0.03125f);
	}
	TestTrue(TEXT("Capsule alignment retains traversal on the real wall"), Fixture.Movement->IsWallRunning());
	TestTrue(TEXT("The capsule's feet rotate toward the wall"),
		FVector::DotProduct(Fixture.Character->GetActorUpVector(), Fixture.Movement->GetWallNormal()) > 0.95);
	TestTrue(TEXT("The aligned capsule makes progress along the wall"), Fixture.Character->GetActorLocation().Y > 1.0);
	const UCapsuleComponent* Capsule = Fixture.Character->GetCapsuleComponent();
	TestFalse(TEXT("Running with a rotated capsule does not penetrate the wall"),
		Fixture.TestWorld.GetTestWorld()->OverlapBlockingTestByChannel(
			Capsule->GetComponentLocation(), Capsule->GetComponentQuat(), ECC_Pawn,
			FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()),
			FCollisionQueryParams(SCENE_QUERY_STAT(PHWallTraversalAlignedTest), false, Fixture.Character)));
	Fixture.Character->JumpAction_Implementation(true);
	TestTrue(TEXT("Jumping away enters falling"), Fixture.Movement->IsFalling());
	TestTrue(TEXT("Wall jump keeps its configured away and upward speeds"),
		Fixture.Movement->Velocity.Equals(FVector(450.0, 0.0, 600.0), Tolerance));
	TestTrue(TEXT("Wall jump restores the capsule to world up"),
		Fixture.Character->GetActorUpVector().Equals(FVector::UpVector, Tolerance));
	TestTrue(TEXT("Wall jump clears the animation surface"), Fixture.Movement->GetWallNormal().IsNearlyZero());
	TestEqual(TEXT("Wall jump stops wall stamina drain"), Fixture.DrainEffectCount(), 0);
	TestTrue(TEXT("Wall jump restores the prior root-motion mode"),
		Fixture.Animation->RootMotionMode == ERootMotionMode::RootMotionFromEverything);
	return true;
}

#endif

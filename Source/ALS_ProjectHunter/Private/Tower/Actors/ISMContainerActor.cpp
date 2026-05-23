#include "Tower/Actors/ISMContainerActor.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

AISMContainerActor::AISMContainerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootSceneComponent);
}

void AISMContainerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (AnimationStates.Num() == 0)
	{
		return;
	}

	AnimationTime += DeltaTime;

	TSet<UInstancedStaticMeshComponent*> DirtyISMs;

	for (auto& Pair : AnimationStates)
	{
		FGroundItemAnimState& State = Pair.Value;
		if (!State.IsValid())
		{
			continue;
		}

		const float T = AnimationTime;
		const float Phase = State.PhaseOffset;

		const float BobZ = BobAmplitudeCm * FMath::Sin(T * BobFrequencyHz * TWO_PI + Phase);
		FVector AnimLocation = State.BaseLocation;
		AnimLocation.Z += BobZ;

		const float YawDeg = FMath::Fmod(T * SpinDegreesPerSecond + FMath::RadiansToDegrees(Phase), 360.0f);

		FRotator AnimRotation(State.BasePitch, YawDeg, State.BaseRoll);
		FTransform AnimTransform(AnimRotation, AnimLocation, FVector::OneVector);

		State.ISMComponent->UpdateInstanceTransform(
			State.InstanceIndex,
			AnimTransform,
			/*bWorldSpace=*/true,
			/*bMarkRenderStateDirty=*/false,
			/*bTeleport=*/true);

		DirtyISMs.Add(State.ISMComponent);
	}

	for (UInstancedStaticMeshComponent* ISM : DirtyISMs)
	{
		ISM->MarkRenderStateDirty();
	}
}

void AISMContainerActor::RegisterItemForAnimation(
	int32 ItemID,
	UInstancedStaticMeshComponent* ISM,
	int32 InstanceIndex,
	FVector BaseLocation,
	FRotator BaseRotation)
{
	if (!ISM || InstanceIndex == INDEX_NONE)
	{
		return;
	}

	FGroundItemAnimState State;
	State.ISMComponent  = ISM;
	State.InstanceIndex = InstanceIndex;
	State.BaseLocation  = BaseLocation;
	State.BasePitch     = BaseRotation.Pitch;
	State.BaseRoll      = BaseRotation.Roll;

	// Distribute phases using the golden angle (≈137.5°) so N items are
	// evenly spread without clustering even at small N.
	const float GoldenAngleRad = 2.399963f; // 137.508° in radians
	State.PhaseOffset = FMath::Fmod(static_cast<float>(ItemID) * GoldenAngleRad, TWO_PI);

	AnimationStates.Add(ItemID, State);
}

void AISMContainerActor::UnregisterItemFromAnimation(int32 ItemID)
{
	AnimationStates.Remove(ItemID);
}

void AISMContainerActor::UpdateItemAnimationIndex(int32 ItemID, int32 NewInstanceIndex)
{
	if (FGroundItemAnimState* State = AnimationStates.Find(ItemID))
	{
		State->InstanceIndex = NewInstanceIndex;
	}
}

void AISMContainerActor::ClearAllAnimationState()
{
	AnimationStates.Empty();
	AnimationTime = 0.0f;
}

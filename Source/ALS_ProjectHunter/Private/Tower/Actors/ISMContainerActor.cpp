#include "Tower/Actors/ISMContainerActor.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

AISMContainerActor::AISMContainerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 1.0f / AnimationUpdatesPerSecond;
	bReplicates = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootSceneComponent);
}

void AISMContainerActor::BeginPlay()
{
	Super::BeginPlay();

	SetActorTickInterval(1.0f / FMath::Max(AnimationUpdatesPerSecond, 1.0f));
	RefreshAnimationTickState();
}

void AISMContainerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (AnimationStates.IsEmpty() || GetNetMode() == NM_DedicatedServer)
	{
		RefreshAnimationTickState();
		return;
	}

	AnimationTime += DeltaTime;

	TArray<FVector, TInlineAllocator<4>> LocalViewLocations;
	if (RenderCullEndDistance > 0.0f)
	{
		if (const UWorld* World = GetWorld())
		{
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				const APlayerController* PlayerController = It->Get();
				if (!PlayerController || !PlayerController->IsLocalController())
				{
					continue;
				}

				FVector ViewLocation;
				FRotator ViewRotation;
				PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
				LocalViewLocations.Add(ViewLocation);
			}
		}
	}

	const float RenderCullEndDistanceSq = FMath::Square(RenderCullEndDistance);
	TSet<UInstancedStaticMeshComponent*> DirtyISMs;

	for (auto& Pair : AnimationStates)
	{
		FGroundItemAnimState& State = Pair.Value;
		if (!State.IsValid())
		{
			continue;
		}

		if (bSkipRecentlyUnrenderedMeshes &&
			!State.ISMComponent->WasRecentlyRendered(RecentlyRenderedTolerance))
		{
			continue;
		}

		if (RenderCullEndDistance > 0.0f && !LocalViewLocations.IsEmpty())
		{
			bool bWithinAnimationDistance = false;
			for (const FVector& ViewLocation : LocalViewLocations)
			{
				if (FVector::DistSquared(ViewLocation, State.BaseLocation) <= RenderCullEndDistanceSq)
				{
					bWithinAnimationDistance = true;
					break;
				}
			}

			if (!bWithinAnimationDistance)
			{
				continue;
			}
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

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	FGroundItemAnimState State;
	State.ISMComponent  = ISM;
	State.InstanceIndex = InstanceIndex;
	State.BaseLocation  = BaseLocation;
	State.BasePitch     = BaseRotation.Pitch;
	State.BaseRoll      = BaseRotation.Roll;

	// Distribute phases using the golden angle (about 137.5 deg) so N items are
	// evenly spread without clustering even at small N.
	const float GoldenAngleRad = 2.399963f; // 137.508 deg in radians
	State.PhaseOffset = FMath::Fmod(static_cast<float>(ItemID) * GoldenAngleRad, TWO_PI);

	AnimationStates.Add(ItemID, State);
	RefreshAnimationTickState();
}

void AISMContainerActor::UnregisterItemFromAnimation(int32 ItemID)
{
	AnimationStates.Remove(ItemID);
	if (AnimationStates.IsEmpty())
	{
		AnimationTime = 0.0f;
	}
	RefreshAnimationTickState();
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
	RefreshAnimationTickState();
}

void AISMContainerActor::ConfigureISMComponent(UInstancedStaticMeshComponent* ISM) const
{
	if (!ISM)
	{
		return;
	}

	const int32 EndDistance = FMath::Max(FMath::RoundToInt(RenderCullEndDistance), 0);
	const int32 StartDistance = EndDistance > 0
		? FMath::Clamp(FMath::RoundToInt(RenderCullStartDistance), 0, EndDistance)
		: 0;
	ISM->SetCullDistances(StartDistance, EndDistance);
}

void AISMContainerActor::RefreshAnimationTickState()
{
	const bool bShouldTick =
		GetNetMode() != NM_DedicatedServer &&
		!AnimationStates.IsEmpty();
	SetActorTickEnabled(bShouldTick);
}

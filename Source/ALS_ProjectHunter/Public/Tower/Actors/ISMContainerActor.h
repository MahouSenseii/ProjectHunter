#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tower/Library/Structs/GroundItemAnimationStructs.h"
#include "ISMContainerActor.generated.h"

class UInstancedStaticMeshComponent;

UCLASS(NotBlueprintable, NotPlaceable)
class ALS_PROJECTHUNTER_API AISMContainerActor : public AActor
{
	GENERATED_BODY()

public:
	AISMContainerActor();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Items|Animation")
	float SpinDegreesPerSecond = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Items|Animation")
	float BobAmplitudeCm = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Items|Animation")
	float BobFrequencyHz = 1.0f;

	void RegisterItemForAnimation(
		int32 ItemID,
		UInstancedStaticMeshComponent* ISM,
		int32 InstanceIndex,
		FVector BaseLocation,
		FRotator BaseRotation);

	void UnregisterItemFromAnimation(int32 ItemID);
	void UpdateItemAnimationIndex(int32 ItemID, int32 NewInstanceIndex);
	void ClearAllAnimationState();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootSceneComponent;

private:
	UPROPERTY()
	TMap<int32, FGroundItemAnimState> AnimationStates;

	float AnimationTime = 0.0f;
};

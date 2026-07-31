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
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Items|Animation")
	float SpinDegreesPerSecond = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Items|Animation")
	float BobAmplitudeCm = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Items|Animation")
	float BobFrequencyHz = 1.0f;

	/** Visual update rate. Rotation speed remains time-based and unchanged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Items|Animation|Optimization",
		meta = (ClampMin = "1.0", ClampMax = "120.0", Units = "Hz"))
	float AnimationUpdatesPerSecond = 30.0f;

	/**
	 * Distance where the renderer may begin fading an instance. Materials can
	 * consume PerInstanceFadeAmount for a smooth fade.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Items|Animation|Optimization",
		meta = (ClampMin = "0.0", Units = "cm"))
	float RenderCullStartDistance = 4000.0f;

	/**
	 * Distance where the renderer fully hides an instance. Zero disables
	 * distance culling. Item data and gameplay locations are never removed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Items|Animation|Optimization",
		meta = (ClampMin = "0.0", Units = "cm"))
	float RenderCullEndDistance = 5000.0f;

	/** Skip transform uploads for mesh groups that have not recently rendered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Items|Animation|Optimization")
	bool bSkipRecentlyUnrenderedMeshes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Items|Animation|Optimization",
		meta = (ClampMin = "0.0", Units = "s", EditCondition = "bSkipRecentlyUnrenderedMeshes"))
	float RecentlyRenderedTolerance = 0.25f;

	void RegisterItemForAnimation(
		int32 ItemID,
		UInstancedStaticMeshComponent* ISM,
		int32 InstanceIndex,
		FVector BaseLocation,
		FRotator BaseRotation);

	void UnregisterItemFromAnimation(int32 ItemID);
	void UpdateItemAnimationIndex(int32 ItemID, int32 NewInstanceIndex);
	void ClearAllAnimationState();
	void ConfigureISMComponent(UInstancedStaticMeshComponent* ISM) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootSceneComponent;

private:
	UPROPERTY()
	TMap<int32, FGroundItemAnimState> AnimationStates;

	float AnimationTime = 0.0f;

	void RefreshAnimationTickState();
};

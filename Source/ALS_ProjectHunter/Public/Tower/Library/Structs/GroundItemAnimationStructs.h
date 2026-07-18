#pragma once

#include "CoreMinimal.h"
#include "GroundItemAnimationStructs.generated.h"

class UInstancedStaticMeshComponent;

USTRUCT()
struct ALS_PROJECTHUNTER_API FGroundItemAnimState
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> ISMComponent = nullptr;

	int32 InstanceIndex = INDEX_NONE;
	FVector BaseLocation = FVector::ZeroVector;
	float BasePitch = 0.0f;
	float BaseRoll = 0.0f;
	float PhaseOffset = 0.0f;

	bool IsValid() const { return ISMComponent != nullptr && InstanceIndex != INDEX_NONE; }
};

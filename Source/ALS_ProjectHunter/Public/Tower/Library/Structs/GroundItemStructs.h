#pragma once

#include "CoreMinimal.h"
#include "GroundItemStructs.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;

USTRUCT()
struct ALS_PROJECTHUNTER_API FGroundItemISMData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> ISMComponent = nullptr;

	int32 InstanceIndex = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	FGroundItemISMData() = default;

	FGroundItemISMData(UInstancedStaticMeshComponent* InISM, const int32 InIndex, UStaticMesh* InMesh)
		: ISMComponent(InISM)
		, InstanceIndex(InIndex)
		, Mesh(InMesh)
	{
	}

	bool IsValid() const { return ISMComponent != nullptr && InstanceIndex != INDEX_NONE; }
};

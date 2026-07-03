#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PHWallTraversalFunctionLibrary.generated.h"

class UPrimitiveComponent;

/**
 * Stateless wall-traversal queries shared by movement, AI, and editor tooling.
 * Holds no state and changes no actor; all functions are pure.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHWallTraversalFunctionLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * True when Component is static world geometry the character may attach to.
	 * Rejects pawns and their capsules/skeletal meshes, and physics-simulating
	 * bodies. Does not test surface angle or distance; the movement component
	 * owns that decision.
	 */
	UFUNCTION(BlueprintPure, Category = "Wall Traversal")
	static bool IsValidWallTraversalSurface(const UPrimitiveComponent* Component);
};

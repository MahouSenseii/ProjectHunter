#include "Character/Library/FunctionLibraries/PHWallTraversalFunctionLibrary.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"

bool UPHWallTraversalFunctionLibrary::IsValidWallTraversalSurface(
	const UPrimitiveComponent* Component)
{
	if (!Component)
	{
		return false;
	}

	// Never treat characters/pawns or their capsules/skeletal meshes as walls.
	if (const AActor* SurfaceOwner = Component->GetOwner())
	{
		if (SurfaceOwner->IsA<APawn>())
		{
			return false;
		}
	}
	if (Component->IsA<USkeletalMeshComponent>() ||
		Component->IsA<UCapsuleComponent>())
	{
		return false;
	}

	// Physics-simulating bodies are not stable surfaces to run on.
	if (Component->IsSimulatingPhysics())
	{
		return false;
	}

	// Everything else is static world geometry: static meshes, BSP/brushes,
	// landscape, and procedural meshes are all valid traversal surfaces.
	return true;
}

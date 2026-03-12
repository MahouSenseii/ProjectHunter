// Character/Component/InteractionTraceManager.cpp

#include "Character/Component/Interaction/InteractionTraceManager.h"
#include "Interactable/Interface/Interactable.h"
#include "Interactable/Component/InteractableManager.h"
#include "Tower/Subsystem/GroundItemSubsystem.h"
#include "Item/ItemInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/ALSPlayerCameraManager.h"
#include "Character/Component/Interaction/InteractionDebugManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
DEFINE_LOG_CATEGORY(LogInteractionTraceManager);
FInteractionTraceManager::FInteractionTraceManager()
	: InteractionDistance(300.0f)
	, CheckFrequency(0.1f)
	, InteractionTraceChannel(ECC_Visibility)
	, bUseALSCameraOrigin(true)
	, OffsetForward(0.0f)
	, OffsetRight(0.0f)
	, OffsetUp(60.0f)
	, OwnerActor(nullptr)
	, WorldContext(nullptr)
	, CachedPlayerController(nullptr)
	, CachedALSCameraManager(nullptr)
	, CachedGroundItemSubsystem(nullptr)
{
}

void FInteractionTraceManager::Initialize(AActor* Owner, UWorld* World)
{
	OwnerActor = Owner;
	WorldContext = World;
	
	if (!OwnerActor || !WorldContext)
	{
		UE_LOG(LogInteractionTraceManager, Error, TEXT("InteractionTraceManager: Invalid initialization parameters"));
		return;
	}

	CacheComponents();
	
	UE_LOG(LogInteractionTraceManager, Log, TEXT("InteractionTraceManager: Initialized for %s"), *OwnerActor->GetName());
}

void FInteractionTraceManager::SetDebugManager(FInteractionDebugManager* InDebugManager)
{
	DebugManager = InDebugManager;
}

// ═══════════════════════════════════════════════════════════════════════
// PRIMARY FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════

// Character/Component/InteractionTraceManager.cpp

TScriptInterface<IInteractable> FInteractionTraceManager::TraceForActorInteractable()
{
	TScriptInterface<IInteractable> Result;

	// Get camera view
	FVector CameraLocation;
	FRotator CameraRotation;
	if (!GetCameraViewPoint(CameraLocation, CameraRotation))
	{
		return Result;
	}

	// Calculate trace points
	FVector TraceStart = GetTraceStartLocation(CameraLocation, CameraRotation);
	FVector TraceEnd = GetTraceEndLocation(CameraLocation, CameraRotation);

	// Perform line trace
	FHitResult HitResult;
	bool bHit = PerformLineTrace(TraceStart, TraceEnd, HitResult);
    
	// ═══════════════════════════════════════════════════════════════
	// DEBUG VISUALIZATION - Draw immediately after trace
	// ═══════════════════════════════════════════════════════════════
	if (DebugManager)
	{
		DebugManager->DrawTraceLine(TraceStart, TraceEnd, bHit);
        
		if (bHit)
		{
			DebugManager->DrawHitPoint(HitResult.Location, HitResult.Normal);
		}
	}

	// Early exit if no hit
	if (!bHit)
	{
		return Result;
	}

	// Check if hit actor is interactable
	AActor* HitActor = HitResult.GetActor();
	if (!IsActorInteractable(HitActor))
	{
		return Result;
	}

	// Try component first
	if (UInteractableManager* InteractableComp = HitActor->FindComponentByClass<UInteractableManager>())
	{
		Result.SetObject(InteractableComp);
		Result.SetInterface(Cast<IInteractable>(InteractableComp));
	}
	// Try actor interface
	else if (HitActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		Result.SetObject(HitActor);
		Result.SetInterface(Cast<IInteractable>(HitActor));
	}

	// Store last trace result
	LastTraceResult = HitResult;

	return Result;
}

UItemInstance* FInteractionTraceManager::FindNearestGroundItem(int32& OutItemID)
{
	OutItemID = -1;

	if (!CachedGroundItemSubsystem)
	{
		return nullptr;
	}

	// Get camera location for distance check
	FVector CameraLocation;
	FRotator CameraRotation;
	if (!GetCameraViewPoint(CameraLocation, CameraRotation))
	{
		return nullptr;
	}

	// Query subsystem for nearest item
	return CachedGroundItemSubsystem->GetNearestItem(
		CameraLocation,
		InteractionDistance,
		OutItemID
	);
}

bool FInteractionTraceManager::GetCameraViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	if (!CachedPlayerController)
	{
		// Fallback to owner location/rotation
		if (OwnerActor)
		{
			OutLocation = OwnerActor->GetActorLocation();
			OutRotation = OwnerActor->GetActorRotation();
			return true;
		}
		return false;
	}

	// Get camera rotation
	FVector RawCameraLoc;
	CachedPlayerController->GetPlayerViewPoint(RawCameraLoc, OutRotation);

	// Calculate location with ALS-style offsets
	if (OwnerActor)
	{
		FVector PivotLocation = OwnerActor->GetActorLocation();

		// Apply offset vectors
		OutLocation = PivotLocation
			+ UKismetMathLibrary::GetForwardVector(OutRotation) * OffsetForward
			+ UKismetMathLibrary::GetRightVector(OutRotation) * OffsetRight
			+ UKismetMathLibrary::GetUpVector(OutRotation) * OffsetUp;

		return true;
	}

	OutLocation = RawCameraLoc;
	return true;
}

void FInteractionTraceManager::GetTraceOrigin(FVector& OutCameraLocation, FVector& OutCameraDirection) const
{
	FRotator CameraRotation;
	GetCameraViewPoint(OutCameraLocation, CameraRotation);

	
	OutCameraDirection = CameraRotation.Vector();
}

FVector FInteractionTraceManager::GetTraceStartLocation(const FVector& CameraLocation, const FRotator& CameraRotation) const
{
	// If using ALS camera and camera manager available, use enhanced origin
	if (bUseALSCameraOrigin && CachedALSCameraManager)
	{
		return CameraLocation; // Already calculated with offsets in GetCameraViewPoint
	}

	// Standard camera location
	return CameraLocation;
}

FVector FInteractionTraceManager::GetTraceEndLocation(const FVector& CameraLocation, const FRotator& CameraRotation) const
{
	// Project forward from camera
	FVector ForwardVector = UKismetMathLibrary::GetForwardVector(CameraRotation);
	return CameraLocation + (ForwardVector * InteractionDistance);
}

bool FInteractionTraceManager::IsLocallyControlled() const
{
	if (!OwnerActor)
	{
		return false;
	}

	if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		return OwnerPawn->IsLocallyControlled();
	}

	return false;
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL HELPERS
// ═══════════════════════════════════════════════════════════════════════

void FInteractionTraceManager::CacheComponents()
{
	if (!OwnerActor)
	{
		return;
	}

	// Cache player controller
	if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		CachedPlayerController = Cast<APlayerController>(OwnerPawn->GetController());

		// Cache ALS camera manager if available
		if (CachedPlayerController && CachedPlayerController->PlayerCameraManager)
		{
			CachedALSCameraManager = Cast<AALSPlayerCameraManager>(CachedPlayerController->PlayerCameraManager);
			if (CachedALSCameraManager)
			{
				UE_LOG(LogInteractionTraceManager, Log, TEXT("InteractionTraceManager: Found ALS Camera Manager"));
			}
		}
	}

	// Cache ground item subsystem
	if (WorldContext)
	{
		CachedGroundItemSubsystem = WorldContext->GetSubsystem<UGroundItemSubsystem>();
		if (!CachedGroundItemSubsystem)
		{
			UE_LOG(LogInteractionTraceManager, Warning, TEXT("InteractionTraceManager: No GroundItemSubsystem found"));
		}
	}
}

bool FInteractionTraceManager::PerformLineTrace(const FVector& Start, const FVector& End, FHitResult& OutHit)
{
	if (!WorldContext)
	{
		return false;
	}

	// Setup trace params
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);
	QueryParams.bTraceComplex = false;

	// Perform line trace
	return WorldContext->LineTraceSingleByChannel(
		OutHit,
		Start,
		End,
		InteractionTraceChannel,
		QueryParams
	);
}

bool FInteractionTraceManager::IsActorInteractable(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	// Check for InteractableManager component
	if (Actor->FindComponentByClass<UInteractableManager>())
	{
		return true;
	}

	// Check if actor implements interface
	return Actor->GetClass()->ImplementsInterface(UInteractable::StaticClass());
}
#include "Character/Component/Interaction/InteractionTraceManager.h"
#include "Interactable/Interface/Interactable.h"
#include "Interactable/Component/InteractableManager.h"
#include "Tower/Subsystem/GroundItemSubsystem.h"
#include "Item/ItemInstance.h"
#include "Components/InstancedStaticMeshComponent.h"
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
	  , CachedGroundItemSubsystem(nullptr), DebugManager(nullptr)
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

TScriptInterface<IInteractable> FInteractionTraceManager::TraceForActorInteractable()
{
	TScriptInterface<IInteractable> Result;

	FVector CameraLocation;
	FRotator CameraRotation;
	if (!GetCameraViewPoint(CameraLocation, CameraRotation))
	{
		return Result;
	}

	FVector TraceStart = GetTraceStartLocation(CameraLocation, CameraRotation);
	FVector TraceEnd = GetTraceEndLocation(CameraLocation, CameraRotation);

	FHitResult HitResult;
	bool bHit = PerformLineTrace(TraceStart, TraceEnd, HitResult);

	if (DebugManager)
	{
		DebugManager->DrawTraceLine(TraceStart, TraceEnd, bHit);
        
		if (bHit)
		{
			DebugManager->DrawHitPoint(HitResult.Location, HitResult.Normal);
		}
	}

	if (!bHit)
	{
		return Result;
	}

	AActor* HitActor = HitResult.GetActor();
	if (!IsActorInteractable(HitActor))
	{
		return Result;
	}

	if (UInteractableManager* InteractableComp = HitActor->FindComponentByClass<UInteractableManager>())
	{
		Result.SetObject(InteractableComp);
		Result.SetInterface(Cast<IInteractable>(InteractableComp));
	}
	else if (HitActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		Result.SetObject(HitActor);
		Result.SetInterface(Cast<IInteractable>(HitActor));
	}

	LastTraceResult = HitResult;

	return Result;
}

int32 FInteractionTraceManager::FindGroundItemByTrace()
{
	if (!CachedGroundItemSubsystem)
	{
		return INDEX_NONE;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	if (!GetCameraViewPoint(CameraLocation, CameraRotation))
	{
		return INDEX_NONE;
	}

	const FVector TraceStart = GetTraceStartLocation(CameraLocation, CameraRotation);
	const FVector TraceEnd   = GetTraceEndLocation(CameraLocation, CameraRotation);

	FHitResult HitResult;
	if (!PerformLineTrace(TraceStart, TraceEnd, HitResult))
	{
		return INDEX_NONE;
	}

	UInstancedStaticMeshComponent* HitISM = Cast<UInstancedStaticMeshComponent>(HitResult.Component.Get());
	if (!HitISM)
	{
		return INDEX_NONE;
	}

	const int32 InstanceIndex = HitResult.Item;
	if (InstanceIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	return CachedGroundItemSubsystem->FindItemByISMInstance(HitISM, InstanceIndex);
}

UItemInstance* FInteractionTraceManager::FindNearestGroundItem(int32& OutItemID)
{
	OutItemID = -1;

	if (!CachedGroundItemSubsystem)
	{
		return nullptr;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	if (!GetCameraViewPoint(CameraLocation, CameraRotation))
	{
		return nullptr;
	}

	return CachedGroundItemSubsystem->GetNearestItem(
		CameraLocation,
		InteractionDistance,
		OutItemID
	);
}

bool FInteractionTraceManager::GetCameraViewPoint(FVector& OutLocation, FRotator& OutRotation)
{

	if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		CachedPlayerController = Cast<APlayerController>(OwnerPawn->GetController());

		if (CachedPlayerController && CachedPlayerController->PlayerCameraManager)
		{
			CachedALSCameraManager = Cast<AALSPlayerCameraManager>(
				CachedPlayerController->PlayerCameraManager);
		}
		else
		{
			CachedALSCameraManager = nullptr;
		}
	}

	if (!CachedPlayerController)
	{
		if (OwnerActor)
		{
			OutLocation = OwnerActor->GetActorLocation();
			OutRotation = OwnerActor->GetActorRotation();
			return true;
		}
		return false;
	}

	FVector RawCameraLoc;
	CachedPlayerController->GetPlayerViewPoint(RawCameraLoc, OutRotation);

	if (OwnerActor)
	{
		FVector PivotLocation = OwnerActor->GetActorLocation();

		const FRotationMatrix RotMat(OutRotation);
		OutLocation = PivotLocation
			+ RotMat.GetUnitAxis(EAxis::X) * OffsetForward
			+ RotMat.GetUnitAxis(EAxis::Y) * OffsetRight
			+ RotMat.GetUnitAxis(EAxis::Z) * OffsetUp;

		return true;
	}

	OutLocation = RawCameraLoc;
	return true;
}

void FInteractionTraceManager::GetTraceOrigin(FVector& OutCameraLocation, FVector& OutCameraDirection)
{
	FRotator CameraRotation;
	GetCameraViewPoint(OutCameraLocation, CameraRotation);

	
	OutCameraDirection = CameraRotation.Vector();
}

FVector FInteractionTraceManager::GetTraceStartLocation(const FVector& CameraLocation, const FRotator& CameraRotation) const
{
	
	if (bUseALSCameraOrigin && CachedALSCameraManager)
	{
		return CachedALSCameraManager->GetCameraLocation();
	}

	return CameraLocation;
}

FVector FInteractionTraceManager::GetTraceEndLocation(const FVector& CameraLocation, const FRotator& CameraRotation) const
{
	return CameraLocation + CameraRotation.Vector() * InteractionDistance;
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

void FInteractionTraceManager::CacheComponents()
{
	if (!OwnerActor)
	{
		return;
	}

	if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		CachedPlayerController = Cast<APlayerController>(OwnerPawn->GetController());

		if (CachedPlayerController && CachedPlayerController->PlayerCameraManager)
		{
			CachedALSCameraManager = Cast<AALSPlayerCameraManager>(CachedPlayerController->PlayerCameraManager);
			if (CachedALSCameraManager)
			{
				UE_LOG(LogInteractionTraceManager, Log, TEXT("InteractionTraceManager: Found ALS Camera Manager"));
			}
		}
	}

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

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);
	QueryParams.bTraceComplex = false;

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

	if (Actor->FindComponentByClass<UInteractableManager>())
	{
		return true;
	}

	return Actor->GetClass()->ImplementsInterface(UInteractable::StaticClass());
}
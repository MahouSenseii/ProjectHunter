// Character/Component/InteractionValidatorManager.cpp

#include "Character/Component/Interaction/InteractionValidatorManager.h"
#include "Interactable/Interface/Interactable.h"
#include "Interactable/Component/InteractableManager.h"
#include "Tower/Subsystem/GroundItemSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogInteractionValidatorManager);

FInteractionValidatorManager::FInteractionValidatorManager()
	: LatencyBuffer(50.0f)
	, bUseDynamicLatencyBuffer(true)
	, MinLatencyBuffer(50.0f)
	, MaxLatencyBuffer(200.0f)
	, bRequireLineOfSight(false)
	, bLogValidationFailures(true)
	, OwnerActor(nullptr)
	, WorldContext(nullptr)
	, CachedGroundItemSubsystem(nullptr)
	, ValidationFailureCount(0)
	, LastValidationFailureTime(0.0f)
{
}

void FInteractionValidatorManager::Initialize(AActor* Owner, UWorld* World)
{
	OwnerActor = Owner;
	WorldContext = World;
	
	if (!OwnerActor || !WorldContext)
	{
		UE_LOG(LogInteractionValidatorManager, Error, TEXT("InteractionValidatorManager: Invalid initialization parameters"));
		return;
	}

	CacheComponents();
	
	UE_LOG(LogInteractionValidatorManager, Log, TEXT("InteractionValidatorManager: Initialized for %s"), *OwnerActor->GetName());
}

// ═══════════════════════════════════════════════════════════════════════
// VALIDATION FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════

bool FInteractionValidatorManager::ValidateActorInteraction(AActor* TargetActor, FVector ClientLocation, float MaxDistance)
{
	if (!TargetActor)
	{
		if (bLogValidationFailures)
		{
			UE_LOG(LogInteractionValidatorManager, Warning, TEXT("Validation Failed: Null target actor"));
		}
		return false;
	}

	// Validate distance
	FVector TargetLocation = TargetActor->GetActorLocation();
	if (!ValidateDistance(ClientLocation, TargetLocation, MaxDistance, true))
	{
		LogValidationFailure("Distance check failed", ClientLocation, TargetLocation);
		return false;
	}

	// Validate line of sight (optional)
	if (bRequireLineOfSight)
	{
		// Pass both source and target actors for proper line of sight validation
		if (!HasLineOfSight(ClientLocation, TargetLocation, OwnerActor, TargetActor))
		{
			LogValidationFailure("Line of sight check failed", ClientLocation, TargetLocation);
			return false;
		}
	}

	// Validate interactable state
	if (!IsValidInteractable(TargetActor, OwnerActor))
	{
		if (bLogValidationFailures)
		{
			UE_LOG(LogInteractionValidatorManager, Warning, TEXT("Validation Failed: Target not interactable - %s"), *TargetActor->GetName());
		}
		return false;
	}

	return true;
}

bool FInteractionValidatorManager::ValidateGroundItemPickup(int32 ItemID, FVector ClientLocation, float MaxDistance)
{
	if (!CachedGroundItemSubsystem)
	{
		return false;
	}

	// Get item location from subsystem
	const FVector* ItemLocation = CachedGroundItemSubsystem->GetInstanceLocations().Find(ItemID);
	if (!ItemLocation)
	{
		if (bLogValidationFailures)
		{
			UE_LOG(LogInteractionValidatorManager, Warning, TEXT("Validation Failed: Ground item %d not found"), ItemID);
		}
		return false;
	}

	// Validate distance
	if (!ValidateDistance(ClientLocation, *ItemLocation, MaxDistance, true))
	{
		LogValidationFailure("Ground item distance check failed", ClientLocation, *ItemLocation);
		return false;
	}

	return true;
}

bool FInteractionValidatorManager::IsValidInteractable(AActor* Actor, AActor* Interactor) const
{
	if (!Actor)
	{
		return false;
	}

	// Check component
	UInteractableManager* InteractableComp = Actor->FindComponentByClass<UInteractableManager>();
	if (InteractableComp)
	{
		return IInteractable::Execute_CanInteract(InteractableComp, Interactor);
	}

	// Check actor interface
	if (Actor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		return IInteractable::Execute_CanInteract(Actor, Interactor);
	}

	return false;
}

bool FInteractionValidatorManager::ValidateDistance(FVector LocationA, FVector LocationB, float MaxDistance, bool bUseLatencyBuffer)
{
	float ActualDistance = FVector::Distance(LocationA, LocationB);
	float AllowedDistance = MaxDistance;

	// Apply latency buffer
	if (bUseLatencyBuffer)
	{
		if (bUseDynamicLatencyBuffer)
		{
			AllowedDistance += GetDynamicLatencyBuffer();
		}
		else
		{
			AllowedDistance += LatencyBuffer;
		}
	}

	return ActualDistance <= AllowedDistance;
}

bool FInteractionValidatorManager::HasLineOfSight(FVector Start, FVector End, AActor* SourceActor, AActor* TargetActor)
{
	if (!WorldContext)
	{
		return false;
	}

	// Setup trace params
	FCollisionQueryParams QueryParams;
	
	// Ignore source actor (typically the player)
	if (SourceActor)
	{
		QueryParams.AddIgnoredActor(SourceActor);
	}
	
	QueryParams.bTraceComplex = false;

	// Perform line trace
	FHitResult HitResult;
	bool bHit = WorldContext->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	// No hit means clear line of sight
	if (!bHit)
	{
		return true;
	}

	// If we hit something, check if it's the target actor we're trying to interact with
	// Hitting the target actor itself (or its components) counts as valid line of sight
	if (TargetActor && HitResult.GetActor() == TargetActor)
	{
		return true;
	}

	// Hit something else that's blocking line of sight
	return false;
}

// ═══════════════════════════════════════════════════════════════════════
// HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════

float FInteractionValidatorManager::GetDynamicLatencyBuffer() const
{
	float Ping = GetPlayerPing();
	if (Ping <= 0.0f)
	{
		return LatencyBuffer; // Fallback to static buffer
	}

	// Convert ping (ms) to buffer distance
	float DynamicBuffer = Ping * 0.1f;
	return FMath::Clamp(DynamicBuffer, MinLatencyBuffer, MaxLatencyBuffer);
}

float FInteractionValidatorManager::GetPlayerPing() const
{
	if (!OwnerActor)
	{
		return 0.0f;
	}

	// Get player state
	APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	if (!OwnerPawn)
	{
		return 0.0f;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC || !PC->PlayerState)
	{
		return 0.0f;
	}

	// Get ping in milliseconds
	return PC->PlayerState->GetPingInMilliseconds();
}

bool FInteractionValidatorManager::HasAuthority() const
{
	return OwnerActor && OwnerActor->HasAuthority();
}

void FInteractionValidatorManager::LogValidationFailure(const FString& Reason, FVector ClientLocation, FVector TargetLocation)
{
	if (!bLogValidationFailures)
	{
		return;
	}

	float Distance = FVector::Distance(ClientLocation, TargetLocation);
	float CurrentTime = WorldContext ? WorldContext->GetTimeSeconds() : 0.0f;

	ValidationFailureCount++;
	LastValidationFailureTime = CurrentTime;

	UE_LOG(LogInteractionValidatorManager, Warning, TEXT("VALIDATION FAILURE [%s] on %s: Distance=%.1f | ClientLoc=%s | TargetLoc=%s | Failures=%d"),
		*Reason,
		OwnerActor ? *OwnerActor->GetName() : TEXT("NULL"),
		Distance,
		*ClientLocation.ToString(),
		*TargetLocation.ToString(),
		ValidationFailureCount
	);
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL HELPERS
// ═══════════════════════════════════════════════════════════════════════

void FInteractionValidatorManager::CacheComponents()
{
	// Cache ground item subsystem
	if (WorldContext)
	{
		CachedGroundItemSubsystem = WorldContext->GetSubsystem<UGroundItemSubsystem>();
		if (!CachedGroundItemSubsystem)
		{
			UE_LOG(LogInteractionValidatorManager, Warning, TEXT("InteractionValidatorManager: No GroundItemSubsystem found"));
		}
	}
}
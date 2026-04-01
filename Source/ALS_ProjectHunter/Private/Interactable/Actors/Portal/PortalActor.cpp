// Interactable/Actors/Portal/PortalActor.cpp
#include "Interactable/Actors/Portal/PortalActor.h"
#include "Tower/Subsystem/PortalSubsystem.h"
#include "Interactable/Component/InteractableManager.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogPortalActor);

APortalActor::APortalActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	SetRootComponent(RootSceneComponent);

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(RootSceneComponent);
	PortalMesh->SetCollisionProfileName(TEXT("NoCollision"));

	PortalVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalVFX"));
	PortalVFX->SetupAttachment(RootSceneComponent);
	PortalVFX->SetAutoActivate(false);

	InteractableManager = CreateDefaultSubobject<UInteractableManager>(TEXT("InteractableManager"));
}

void APortalActor::BeginPlay()
{
	Super::BeginPlay();

	// Register with subsystem (authority + clients both register so the subsystem
	// can answer "where do I arrive?" queries on the client for map-pins etc.)
	if (UWorld* World = GetWorld())
	{
		if (UPortalSubsystem* PortalSub = World->GetSubsystem<UPortalSubsystem>())
		{
			PortalSub->RegisterPortal(PortalID, this);
		}
	}

	if (HasAuthority())
	{
		SetStateInternal(bActiveOnBeginPlay
			? EPortalState::PS_Active
			: EPortalState::PS_Inactive);
	}
}

void APortalActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UPortalSubsystem* PortalSub = GetWorld() ? GetWorld()->GetSubsystem<UPortalSubsystem>() : nullptr)
	{
		PortalSub->UnregisterPortal(PortalID);
	}

	Super::EndPlay(EndPlayReason);
}

void APortalActor::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APortalActor, PortalState);
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void APortalActor::SetPortalActive(bool bActive)
{
	if (!HasAuthority())
	{
		return;
	}
	SetStateInternal(bActive ? EPortalState::PS_Active : EPortalState::PS_Inactive);
}

FTransform APortalActor::GetArrivalTransform() const
{
	// Return a transform offset forward from this portal's root
	const FVector ForwardOffset = GetActorForwardVector() * ArrivalOffset.X
		+ GetActorRightVector()  * ArrivalOffset.Y
		+ GetActorUpVector()     * ArrivalOffset.Z;

	return FTransform(GetActorRotation(), GetActorLocation() + ForwardOffset);
}

// ─────────────────────────────────────────────────────────────────────────────
// IInteractable
// ─────────────────────────────────────────────────────────────────────────────

void APortalActor::OnInteract_Implementation(AActor* Interactor)
{
	APawn* Traveller = Cast<APawn>(Interactor);
	if (!Traveller)
	{
		return;
	}

	if (GetLocalRole() == ROLE_Authority)
	{
		Server_ActivatePortal_Implementation(Traveller);
	}
	else
	{
		Server_ActivatePortal(Traveller);
	}
}

bool APortalActor::CanInteract_Implementation(AActor* Interactor) const
{
	return IsUsable();
}

EInteractionType APortalActor::GetInteractionType_Implementation() const
{
	return EInteractionType::IT_Tap;
}

void APortalActor::OnBeginFocus_Implementation(AActor* Interactor)
{
	// Blueprint can override to highlight the portal mesh
}

void APortalActor::OnEndFocus_Implementation(AActor* Interactor)
{
}

FInteractableHighlightStyle APortalActor::GetHighlightStyle_Implementation() const
{
	FInteractableHighlightStyle Style;
	Style.bEnableHighlight = true;
	Style.Color = FLinearColor(0.0f, 0.7f, 1.0f); // Cyan-blue for portals
	Style.StencilValue = 252;
	Style.OutlineWidth = 3.0f;
	return Style;
}

FText APortalActor::GetInteractionText_Implementation() const
{
	if (!IsUsable())
	{
		return FText::FromString(TEXT("Portal Inactive"));
	}
	return FText::Format(
		FText::FromString(TEXT("Travel to {0}")),
		PortalDisplayName);
}

FText APortalActor::GetHoldInteractionText_Implementation() const         { return FText::GetEmpty(); }
float APortalActor::GetTapHoldThreshold_Implementation() const            { return 0.3f; }
float APortalActor::GetHoldDuration_Implementation() const                { return 1.0f; }
void APortalActor::OnHoldInteractionStart_Implementation(AActor*)         {}
void APortalActor::OnHoldInteractionUpdate_Implementation(AActor*, float) {}
void APortalActor::OnHoldInteractionComplete_Implementation(AActor*)      {}
void APortalActor::OnHoldInteractionCancelled_Implementation(AActor*)     {}
int32 APortalActor::GetRequiredMashCount_Implementation() const           { return 1; }
float APortalActor::GetMashDecayRate_Implementation() const               { return 0.0f; }
void APortalActor::OnMashInteractionStart_Implementation(AActor*)         {}
void APortalActor::OnMashInteractionUpdate_Implementation(AActor*, int32, int32, float) {}
void APortalActor::OnMashInteractionComplete_Implementation(AActor*)      {}
void APortalActor::OnMashInteractionFailed_Implementation(AActor*)        {}
FText APortalActor::GetMashInteractionText_Implementation() const         { return FText::GetEmpty(); }
void APortalActor::OnContinuousInteractionStart_Implementation(AActor*)   {}
void APortalActor::OnContinuousInteractionUpdate_Implementation(AActor*, float) {}
void APortalActor::OnContinuousInteractionEnd_Implementation(AActor*)     {}
bool APortalActor::HasTooltip_Implementation() const                      { return false; }
UObject* APortalActor::GetTooltipData_Implementation() const              { return nullptr; }
FVector APortalActor::GetTooltipWorldLocation_Implementation() const      { return GetActorLocation(); }
UInputAction* APortalActor::GetInputAction_Implementation() const         { return InteractInputAction; }
FVector APortalActor::GetWidgetOffset_Implementation() const              { return FVector(0.0f, 0.0f, 120.0f); }

// ─────────────────────────────────────────────────────────────────────────────
// Server RPC
// ─────────────────────────────────────────────────────────────────────────────

void APortalActor::Server_ActivatePortal_Implementation(APawn* Traveller)
{
	if (!Traveller || !IsUsable())
	{
		return;
	}

	// Validate ownership — only the controlling player can use the portal
	AController* Controller = Traveller->GetController();
	if (!Controller)
	{
		return;
	}

	ExecuteTravel(Traveller);
}

void APortalActor::ExecuteTravel(APawn* Traveller)
{
	UPortalSubsystem* PortalSub = GetWorld()->GetSubsystem<UPortalSubsystem>();
	if (!PortalSub)
	{
		UE_LOG(LogPortalActor, Warning,
			TEXT("ExecuteTravel: PortalSubsystem not found for portal '%s'"),
			*PortalID.ToString());
		return;
	}

	APortalActor* Destination = PortalSub->FindPortal(DestinationPortalID);
	if (!Destination)
	{
		UE_LOG(LogPortalActor, Warning,
			TEXT("ExecuteTravel: Destination portal '%s' not registered (same-map travel)"),
			*DestinationPortalID.ToString());
		return;
	}

	const FTransform ArrivalTF = Destination->GetArrivalTransform();

	// Teleport the pawn
	Traveller->SetActorTransform(ArrivalTF, false, nullptr, ETeleportType::TeleportPhysics);

	// Notify destination portal (e.g., start its cooldown too if desired)
	PlayTravelFeedback();
	OnPortalActivated.Broadcast(this, Traveller);

	UE_LOG(LogPortalActor, Log,
		TEXT("ExecuteTravel: '%s' teleported pawn '%s' to portal '%s'"),
		*PortalID.ToString(),
		*Traveller->GetName(),
		*DestinationPortalID.ToString());

	if (bSingleUse)
	{
		SetStateInternal(EPortalState::PS_Disabled);
	}
	else if (UseCooldown > 0.0f)
	{
		StartCooldown();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// State helpers
// ─────────────────────────────────────────────────────────────────────────────

void APortalActor::SetStateInternal(EPortalState NewState)
{
	PortalState = NewState;
	UpdateVFXForState();
}

void APortalActor::StartCooldown()
{
	SetStateInternal(EPortalState::PS_Cooldown);
	GetWorldTimerManager().SetTimer(
		CooldownTimer, this, &APortalActor::EndCooldown, UseCooldown, false);
}

void APortalActor::EndCooldown()
{
	SetStateInternal(EPortalState::PS_Active);
}

// ─────────────────────────────────────────────────────────────────────────────
// Replication
// ─────────────────────────────────────────────────────────────────────────────

void APortalActor::OnRep_PortalState()
{
	UpdateVFXForState();
}

// ─────────────────────────────────────────────────────────────────────────────
// Visuals
// ─────────────────────────────────────────────────────────────────────────────

void APortalActor::UpdateVFXForState()
{
	if (!PortalVFX)
	{
		return;
	}

	switch (PortalState)
	{
	case EPortalState::PS_Active:
		if (IdleVFXSystem)
		{
			PortalVFX->SetAsset(IdleVFXSystem);
		}
		PortalVFX->Activate(true);
		break;

	case EPortalState::PS_Inactive:
	case EPortalState::PS_Disabled:
		PortalVFX->Deactivate();
		break;

	case EPortalState::PS_Cooldown:
		// Keep VFX playing but Blueprint can tint it differently via the state
		break;

	default:
		break;
	}
}

void APortalActor::PlayTravelFeedback()
{
	// VFX burst — spawned at portal location, not attached
	if (TravelBurstVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), TravelBurstVFX, GetActorLocation());
	}

	if (TravelSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, TravelSound, GetActorLocation());
	}
}

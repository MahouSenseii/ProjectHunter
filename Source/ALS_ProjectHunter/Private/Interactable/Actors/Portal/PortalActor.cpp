#include "Interactable/Actors/Portal/PortalActor.h"
#include "Tower/Subsystems/PortalSubsystem.h"
#include "Tower/Subsystems/PortalTravelSubsystem.h"
#include "Interactable/Components/InteractableManager.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
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

	// If a direct actor reference is set, derive the destination ID from it so
	// you only need to drag the other portal into the Details slot - no manual
	// name matching required.
	if (LinkedPortal)
	{
		DestinationPortalID = LinkedPortal->PortalID;

		// Wire the return link automatically if the other portal hasn't been
		// linked yet, so connecting A->B also connects B->A.
		if (LinkedPortal->LinkedPortal == nullptr && LinkedPortal->DestinationPortalID == NAME_None)
		{
			LinkedPortal->LinkedPortal        = this;
			LinkedPortal->DestinationPortalID = PortalID;
		}
	}

	// Register with subsystem on both authority and clients so the subsystem
	// can answer "where do I arrive?" queries on the client (e.g. map pins).
	if (UWorld* World = GetWorld())
	{
		if (UPortalSubsystem* PortalSub = World->GetSubsystem<UPortalSubsystem>())
		{
			PortalSub->RegisterPortal(PortalID, this);
		}
	}

	// The player's InteractionManager calls OnInteract on the UInteractableManager
	// component (not the actor) when one is present. Bind here so tap events
	// route back to this actor's server logic.
	if (InteractableManager)
	{
		InteractableManager->OnTapInteracted.AddUniqueDynamic(this, &APortalActor::OnInteractableManagerTap);

		InteractableManager->Config.InteractionType = EInteractionType::IT_Tap;
		InteractableManager->Config.InteractionText  = GetInteractionText_Implementation();
		InteractableManager->Config.bCanInteract     = bActiveOnBeginPlay;

		if (!InteractableManager->Config.InputAction)
		{
			UE_LOG(LogPortalActor, Warning,
				TEXT("PortalActor '%s': InteractableManager.Config.InputAction is not set. "
				     "Select the InteractableManager component in the Blueprint and set "
				     "Config -> Input Action in the Details panel."),
				*GetName());
		}
	}

	if (HasAuthority())
	{
		SetStateInternal(bActiveOnBeginPlay
			? EPortalState::PS_Active
			: EPortalState::PS_Inactive);

		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UPortalTravelSubsystem* TravelSubsystem = GameInstance->GetSubsystem<UPortalTravelSubsystem>();
				TravelSubsystem && TravelSubsystem->MatchesArrivalPortal(PortalID))
			{
				GetWorldTimerManager().SetTimerForNextTick(this, &APortalActor::TryResolvePendingArrival);
			}
		}
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
	const FVector ForwardOffset = GetActorForwardVector() * ArrivalOffset.X
		+ GetActorRightVector()  * ArrivalOffset.Y
		+ GetActorUpVector()     * ArrivalOffset.Z;

	return FTransform(GetActorRotation(), GetActorLocation() + ForwardOffset);
}

void APortalActor::OnInteract_Implementation(AActor* Interactor)
{
	APawn* Traveller = Cast<APawn>(Interactor);
	if (!Traveller)
	{
		return;
	}

	// InteractionManager already owns the player RPC and invokes this again on
	// the server. Actor-owned RPCs from an unowned world portal would be dropped.
	if (HasAuthority())
	{
		TryActivatePortal(Traveller);
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
}

void APortalActor::OnEndFocus_Implementation(AActor* Interactor)
{
}

FInteractableHighlightStyle APortalActor::GetHighlightStyle_Implementation() const
{
	FInteractableHighlightStyle Style;
	Style.bEnableHighlight = true;
	Style.Color = FLinearColor(0.0f, 0.7f, 1.0f);
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
UInputAction* APortalActor::GetInputAction_Implementation() const         { return InteractableManager ? InteractableManager->Config.InputAction : nullptr; }
FVector APortalActor::GetWidgetOffset_Implementation() const              { return FVector(0.0f, 0.0f, 120.0f); }

void APortalActor::OnInteractableManagerTap(AActor* Interactor)
{
	APawn* Traveller = Cast<APawn>(Interactor);
	if (!Traveller)
	{
		return;
	}

	if (HasAuthority())
	{
		TryActivatePortal(Traveller);
	}
}

void APortalActor::TryActivatePortal(APawn* Traveller)
{
	if (!HasAuthority() || !Traveller || !IsUsable())
	{
		return;
	}

	AController* Controller = Traveller->GetController();
	if (!Controller)
	{
		return;
	}

	if (FVector::DistSquared(Traveller->GetActorLocation(), GetActorLocation())
		> FMath::Square(FMath::Max(100.f, MaxUseDistance)))
	{
		UE_LOG(LogPortalActor, Warning, TEXT("Portal '%s' rejected distant traveller '%s'."),
			*PortalID.ToString(), *Traveller->GetName());
		return;
	}

	ExecuteTravel(Traveller);
}

void APortalActor::ExecuteTravel(APawn* Traveller)
{
	if (!HasAuthority())
	{
		return;
	}

	Multicast_PlayTravelFeedback();
	OnPortalActivated.Broadcast(this, Traveller);

	// A listener owning the destination has already been told; resolving a destination portal
	// here would only log a failure for a link that was never meant to exist.
	if (bDestinationHandledByListener)
	{
		if (bSingleUse)
		{
			SetStateInternal(EPortalState::PS_Disabled);
		}
		else if (UseCooldown > 0.0f)
		{
			StartCooldown();
		}
		return;
	}

	if (DestinationLevelName != NAME_None)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UPortalTravelSubsystem* TravelSubsystem = GameInstance->GetSubsystem<UPortalTravelSubsystem>())
			{
				int32 ExpectedTravellers = 0;
				for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
				{
					ExpectedTravellers += It->IsValid() ? 1 : 0;
				}
				TravelSubsystem->SetPendingArrival(
					DestinationLevelName, DestinationPortalID, ExpectedTravellers);
			}
		}

		UE_LOG(LogPortalActor, Log,
			TEXT("ExecuteTravel: '%s' loading level '%s' (arrival portal: '%s')"),
			*PortalID.ToString(),
			*DestinationLevelName.ToString(),
			*DestinationPortalID.ToString());

		if (GetNetMode() == NM_Standalone)
		{
			UGameplayStatics::OpenLevel(this, DestinationLevelName);
		}
		else
		{
			GetWorld()->ServerTravel(DestinationLevelName.ToString(), false);
		}
		return;
	}

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
			TEXT("ExecuteTravel: Destination portal '%s' not registered"),
			*DestinationPortalID.ToString());
		return;
	}

	const FTransform ArrivalTF = Destination->GetArrivalTransform();
	if (!TeleportTraveller(Traveller, ArrivalTF))
	{
		UE_LOG(LogPortalActor, Warning, TEXT("ExecuteTravel: failed to place pawn '%s' at portal '%s'."),
			*Traveller->GetName(), *DestinationPortalID.ToString());
		return;
	}

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

void APortalActor::SetStateInternal(EPortalState NewState)
{
	PortalState = NewState;
	UpdateVFXForState();

	// Keep the component's bCanInteract in sync so CanInteract checks on the
	// component (which the interaction system calls instead of the actor) match
	// the portal's actual state.
	if (InteractableManager)
	{
		const bool bUsable = (NewState == EPortalState::PS_Active);
		InteractableManager->SetCanInteract(bUsable);
		InteractableManager->Config.InteractionText = GetInteractionText_Implementation();
	}
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

void APortalActor::OnRep_PortalState()
{
	UpdateVFXForState();
	if (InteractableManager)
	{
		InteractableManager->SetCanInteract(IsUsable());
		InteractableManager->SetInteractionText(GetInteractionText_Implementation());
	}
}

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

void APortalActor::Multicast_PlayTravelFeedback_Implementation()
{
	PlayTravelFeedback();
}

bool APortalActor::TeleportTraveller(APawn* Traveller, const FTransform& ArrivalTransform) const
{
	if (!Traveller)
	{
		return false;
	}

	const bool bTeleported = Traveller->TeleportTo(
		ArrivalTransform.GetLocation(), ArrivalTransform.Rotator(), false, true);
	if (bTeleported)
	{
		if (ACharacter* Character = Cast<ACharacter>(Traveller))
		{
			if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
			{
				Movement->StopMovementImmediately();
			}
		}
	}
	return bTeleported;
}

void APortalActor::TryResolvePendingArrival()
{
	if (!HasAuthority())
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UPortalTravelSubsystem* TravelSubsystem = GameInstance
		? GameInstance->GetSubsystem<UPortalTravelSubsystem>()
		: nullptr;
	if (!TravelSubsystem || !TravelSubsystem->MatchesArrivalPortal(PortalID))
	{
		return;
	}

	int32 AvailableTravellers = 0;
	int32 PlacedTravellers = 0;
	const FTransform ArrivalTransform = GetArrivalTransform();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}

		++AvailableTravellers;
		PlacedTravellers += TeleportTraveller(Pawn, ArrivalTransform) ? 1 : 0;
	}

	const int32 ExpectedTravellers = TravelSubsystem->GetExpectedTravellerCount();
	const int32 MaxAttempts = FMath::Max(1, FMath::CeilToInt(ArrivalRetryWindow / 0.25f));
	++ArrivalRetryAttempts;
	if (AvailableTravellers >= ExpectedTravellers && PlacedTravellers >= ExpectedTravellers)
	{
		UE_LOG(LogPortalActor, Log, TEXT("Portal '%s' placed %d traveller(s) after map travel."),
			*PortalID.ToString(), PlacedTravellers);
		TravelSubsystem->ClearPendingArrival();
		return;
	}

	if (ArrivalRetryAttempts >= MaxAttempts)
	{
		UE_LOG(LogPortalActor, Warning,
			TEXT("Portal '%s' timed out waiting for travellers (placed %d of %d)."),
			*PortalID.ToString(), PlacedTravellers, ExpectedTravellers);
		TravelSubsystem->ClearPendingArrival();
		return;
	}

	GetWorldTimerManager().SetTimer(
		ArrivalRetryTimer, this, &APortalActor::TryResolvePendingArrival, 0.25f, false);
}

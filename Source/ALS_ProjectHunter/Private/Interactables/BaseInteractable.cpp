// Copyright@2024 Quentin Davis 


#include "Interactables/BaseInteractable.h"
#include "Character/PHBaseCharacter.h"
#include "Character/Player/PHPlayerController.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InteractableManager.h"
#include "UObject/ConstructorHelpers.h"



// Sets default values
ABaseInteractable::ABaseInteractable()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	InitializeComponent();
	SetupMesh();

	if (InteractableManager)
	{
		InteractableManager->InteractableValue = FMath::RandRange(0, InteractableManager->InteractableLimitValue);
	}
		
	ValidateComponents();
	SetupMeshVisibility();
	SetupUI();
	SetupTrigger();
	SetupOverlapEvents();

	
}

void ABaseInteractable::Destroyed()
{
	Super::Destroyed();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_DelayedStart);
	}
}

void ABaseInteractable::InitializeComponent()
{
	// Base component setup
	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	RootComponent = Scene;
	InteractableManager = CreateDefaultSubobject<UInteractableManager>(TEXT("AC_InteractableManager"));

	// Mesh components
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(RootComponent);
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(RootComponent);

	// Interactable area setup
	InteractableArea = CreateDefaultSubobject<USphereComponent>(TEXT("InteractableArea"));
	InteractableArea->SetupAttachment(RootComponent);

	InteractableArea->SetSphereRadius(TriggerRadius);

	// Interaction widget setup
	InteractionWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionWidget"));
	InteractionWidget->SetupAttachment(RootComponent);
	InteractionWidget->SetWidgetSpace(EWidgetSpace::Screen);
	InteractionWidget->SetDrawSize(FVector2D(125.0f,125.0f));
}




void ABaseInteractable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void ABaseInteractable::CheckForActors()
{
	check(InteractableArea);

	for(TArray<APawn*> LocalOverlappingActors; APawn* Actor : LocalOverlappingActors)
	{
		if(Actor->IsPlayerControlled())
		{
			OnOverlapBegin(InteractableArea, Actor, nullptr, -1, false, FHitResult());
		}
	}
}

bool ABaseInteractable::	InteractionHandle(AActor* Actor, bool WasHeld) const
{
	if (WasHeld)
	{
		// override and do something if the interaction key was held
		return true;

	}
	else
	{
		// override and do  something else if the interaction key was just pressed
		return false;
	}
}


// Called when the game starts or when spawned
void ABaseInteractable::BeginPlay()
{
	Super::BeginPlay();
	// Delay duration in seconds
	check(InteractionWidget)
	InteractionWidget->SetVisibility(false, true);
	
	if (InteractableManager && InteractableManager->InteractableLimitValue > 0)
	{
		InteractableManager->InteractableValue = FMath::RandRange(0, InteractableManager->InteractableLimitValue);
	}
	

	
}

void ABaseInteractable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Remove from all interaction lists on destroy
	if (InteractableManager)
	{
		// Find all player controllers and remove this from their lists
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (const APHPlayerController* PHController = Cast<APHPlayerController>(It->Get()))
			{
				if (PHController->InteractionManager)
				{
					PHController->InteractionManager->RemoveFromInteractionList(InteractableManager);
					if (PHController->InteractionManager->GetCurrentInteractable() == InteractableManager)
					{
						PHController->InteractionManager->SetCurrentInteraction(nullptr);
					}
				}
			}
		}
	}
}

void ABaseInteractable::SetupMeshVisibility() const
{
	// Toggles the visibility based on the validity of the StaticMesh.
	// No Item should have both StaticMesh and SkeletalMesh, this will make sure that its always one or the other.
	if (StaticMesh)
	{
		SkeletalMesh->SetVisibility(false);
	}
	else
	{
		SkeletalMesh->SetVisibility(true);
	}
}

void ABaseInteractable::SetupOverlapEvents()
{
		// Attach dynamic overlap events for InteractableArea.
		if (!InteractableArea->OnComponentBeginOverlap.IsBound())
		{
			InteractableArea->OnComponentBeginOverlap.AddDynamic(this, &ABaseInteractable::OnOverlapBegin);
		}
		if (!InteractableArea->OnComponentEndOverlap.IsBound())
		{
			InteractableArea->OnComponentEndOverlap.AddDynamic(this, &ABaseInteractable::OnOverlapEnd);
		}
}

void ABaseInteractable::SetupTrigger() const
{
		InteractableArea->SetSphereRadius(TriggerRadius);
		InteractableArea->SetRelativeLocation(TriggerLocation);
}

void ABaseInteractable::SetupUI() const
{
	InteractionWidget->SetRelativeLocation(UILocation);
}

void ABaseInteractable::SetupMesh() const
{
	if(StaticMesh != nullptr)
	{
		StaticMesh->SetStaticMesh(NewMesh);
		StaticMesh->SetRelativeTransform(MeshTransform);
	}

	if(SkeletalMesh != nullptr)
	{
		SkeletalMesh->SetRelativeTransform(MeshTransform);
	}
}


void ABaseInteractable::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ValidateComponents();
	
	if(!InteractableManager->IsInteractable) { return;}
	// Attempt to cast the OtherActor to your player character class.
	if(const APHBaseCharacter* CastCharacter = Cast<APHBaseCharacter>(OtherActor))
	{
		// If the cast is successful, get the player character's controller and cast it to APHPlayerController.
		if(const APHPlayerController* CastController = Cast<APHPlayerController>(CastCharacter->GetController()))
		{
			// Now access the InteractionManager from the controller, assuming it's a public member or accessible through a getter method.
			if(UInteractionManager* ReturnInteractionManager = CastController->InteractionManager)
			{
				ReturnInteractionManager->AddToInteractionList(InteractableManager);
			}
		}
	}
}

void ABaseInteractable::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	//ValidateComponents();
	if(!InteractableManager->IsInteractable) { return;}
	// Attempt to cast the OtherActor to your player character class.
	if(const APHBaseCharacter* CastCharacter = Cast<APHBaseCharacter>(OtherActor))
	{
		// If the cast is successful, get the player character's controller and cast it to APHPlayerController.
		if(const APHPlayerController* CastController = Cast<APHPlayerController>(CastCharacter->GetController()))
		{
			// Now access the InteractionManager from the controller, assuming it's a public member or accessible through a getter method.
			if(CastController->InteractionManager)
			{
				CastController->InteractionManager->RemoveFromInteractionList(InteractableManager);
				CastController->InteractionManager->RemoveInteractionFromCurrent(InteractableManager);
			}
		}
	}
}

void ABaseInteractable::BPIInitialize_Implementation()
{
	IInteractableObjectInterface::BPIInitialize_Implementation();

	MeshSet.Empty(); 
	if (SkeletalMesh) MeshSet.Add(SkeletalMesh);
	if (StaticMesh) MeshSet.Add(StaticMesh);

	check(InteractableManager);

	// This sets up area + mesh highlighting
	InteractableManager->SetupInteractableReferences(InteractableArea, InteractionWidget, MeshSet);

	//forcefully wipe any old widget assignments from copy/paste
	if (InteractionWidget)
	{
		InteractionWidget->SetWidget(nullptr);
	}
}


void ABaseInteractable::BPIClientStartInteraction_Implementation(AActor* Interactor, const bool bIsHeld)
{
	IInteractableObjectInterface::BPIClientStartInteraction_Implementation(Interactor, bIsHeld);
	BPStartOverride(Interactor, bIsHeld);
}

void ABaseInteractable::BPIClientEndInteraction_Implementation(AActor* Interactor)
{
	IInteractableObjectInterface::BPIClientEndInteraction_Implementation(Interactor);
	BPEndInteractionOverride(Interactor);	
}

void ABaseInteractable::ValidateComponents() const
{
	check(IsValid(InteractableManager));
	check(IsValid(InteractableArea));
	check(IsValid(InteractionWidget))
}


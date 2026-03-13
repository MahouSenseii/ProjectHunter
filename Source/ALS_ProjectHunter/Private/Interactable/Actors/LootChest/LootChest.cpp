// Interactable/Actors/LootChest/LootChest.cpp

#include "Interactable/Actors/LootChest/LootChest.h"
#include "Interactable/Component/InteractableManager.h"
#include "Loot/Component/LootComponent.h"
#include "Loot/Subsystem/LootSubsystem.h"
#include "Character/Component/StatsManager.h"
#include "Item/ItemInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogLootChest);

// ═══════════════════════════════════════════════════════════════════════
// CONSTRUCTOR
// ═══════════════════════════════════════════════════════════════════════

ALootChest::ALootChest()
{
	// OPTIMIZATION: No tick needed - we use timers for animation
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Create root component
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootSceneComponent;

	// Create static chest mesh (will be configured in OnConstruction)
	Static_ChestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticChestMesh"));
	Static_ChestMesh->SetupAttachment(RootComponent);
	Static_ChestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Create skeletal chest mesh (will be configured in OnConstruction)
	Skeletal_ChestMesh =CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalChestMesh"));
	Skeletal_ChestMesh->SetupAttachment(RootComponent);
	Skeletal_ChestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// Use single node instance for direct animation control
	Skeletal_ChestMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	// Create interaction component
	InteractableManager = CreateDefaultSubobject<UInteractableManager>(TEXT("InteractableManager"));

	// Create loot component
	LootComponent = CreateDefaultSubobject<ULootComponent>(TEXT("LootComponent"));

	// Initialize state
	ChestState = EChestState::CS_Closed;
	LastInteractor = nullptr;
	
	void ConfigureMeshVisibilityAndCollision();
}

// ═══════════════════════════════════════════════════════════════════════
// LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════

void ALootChest::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Configure visuals and collision for editor preview and runtime
	ConfigureMeshVisibilityAndCollision();

	UE_LOG(LogLootChest, Verbose, TEXT("%s: OnConstruction - Configured mesh (Type: %s)"),
		*GetName(), VisualConfig.bUseStaticMesh ? TEXT("Static") : TEXT("Skeletal"));
}

void ALootChest::BeginPlay()
{
	Super::BeginPlay();

	// Setup components
	SetupInteraction();
	SetupVisuals();
	SetupLootComponent();

	// Validate source
	if (!IsSourceValid())
	{
		UE_LOG(LogLootChest, Warning, TEXT("%s: LootComponent.SourceID '%s' not found in registry"),
			*GetName(), *LootComponent->SourceID.ToString());
	}

	UE_LOG(LogLootChest, Log, TEXT("%s: Initialized with source '%s' (MeshType: %s)"),
		*GetName(), 
		*LootComponent->SourceID.ToString(),
		VisualConfig.bUseStaticMesh ? TEXT("Static") : TEXT("Skeletal"));
}

#if WITH_EDITOR
void ALootChest::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property == nullptr)
	{
		return;
	}

	const FName PropertyName = PropertyChangedEvent.Property->GetFName();
	const FName MemberPropertyName = PropertyChangedEvent.MemberProperty ? 
		PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	// Handle changes to VisualConfig
	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(ALootChest, VisualConfig))
	{
		// Mesh type changed or mesh assets changed
		if (PropertyName == GET_MEMBER_NAME_CHECKED(FChestVisualConfig, bUseStaticMesh) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(FChestVisualConfig, ClosedMesh) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(FChestVisualConfig, OpenMesh) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(FChestVisualConfig, SkeletalMesh) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(FChestVisualConfig, OpenAnimation))
		{
			ConfigureMeshVisibilityAndCollision();
			
			UE_LOG(LogLootChest, Verbose, TEXT("%s: Editor - Visual config changed, reconfigured mesh"),
				*GetName());
		}
	}

	// Handle changes to CollisionConfig
	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(ALootChest, CollisionConfig))
	{
		ApplyCollisionSettings();
		
		UE_LOG(LogLootChest, Verbose, TEXT("%s: Editor - Collision config changed, reapplied settings"),
			*GetName());
	}
}
#endif

// ═══════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════

void ALootChest::ConfigureMeshVisibilityAndCollision()
{
	if (VisualConfig.bUseStaticMesh)
	{
		// ─────────────────────────────────────────────
		// STATIC MESH MODE
		// ─────────────────────────────────────────────
		
		if (Static_ChestMesh)
		{
			// Set mesh asset
			if (VisualConfig.ClosedMesh)
			{
				Static_ChestMesh->SetStaticMesh(VisualConfig.ClosedMesh);
			}

			// Enable and show
			Static_ChestMesh->SetVisibility(true);
			Static_ChestMesh->SetHiddenInGame(false);
			Static_ChestMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}

		if (Skeletal_ChestMesh)
		{
			// Disable and hide
			Skeletal_ChestMesh->SetVisibility(false);
			Skeletal_ChestMesh->SetHiddenInGame(true);
			Skeletal_ChestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	else
	{
		// ─────────────────────────────────────────────
		// SKELETAL MESH MODE
		// ─────────────────────────────────────────────
		
		if (Static_ChestMesh)
		{
			// Disable and hide
			Static_ChestMesh->SetVisibility(false);
			Static_ChestMesh->SetHiddenInGame(true);
			Static_ChestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		if (Skeletal_ChestMesh)
		{
			// Set skeletal mesh asset
			if (VisualConfig.SkeletalMesh)
			{
				Skeletal_ChestMesh->SetSkeletalMesh(VisualConfig.SkeletalMesh);
			}

			// Enable and show
			Skeletal_ChestMesh->SetVisibility(true);
			Skeletal_ChestMesh->SetHiddenInGame(false);
			Skeletal_ChestMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

			// Set to closed position (frame 0)
			SetSkeletalAnimationPosition(0.0f);
		}
	}

	// Apply collision settings to active mesh
	ApplyCollisionSettings();

	UE_LOG(LogLootChest, Verbose, TEXT("%s: Configured mesh visibility and collision (Static: %s, Skeletal: %s)"),
		*GetName(),
		Static_ChestMesh && Static_ChestMesh->IsVisible() ? TEXT("Visible") : TEXT("Hidden"),
		Skeletal_ChestMesh && Skeletal_ChestMesh->IsVisible() ? TEXT("Visible") : TEXT("Hidden"));
}

void ALootChest::ApplyCollisionSettings()
{
	// Get active mesh component
	UPrimitiveComponent* ActiveMesh = nullptr;
	
	if (VisualConfig.bUseStaticMesh && Static_ChestMesh)
	{
		ActiveMesh = Static_ChestMesh;
	}
	else if (!VisualConfig.bUseStaticMesh && Skeletal_ChestMesh)
	{
		ActiveMesh = Skeletal_ChestMesh;
	}

	if (!ActiveMesh)
	{
		UE_LOG(LogLootChest, Warning, TEXT("%s: No active mesh to apply collision settings"),
			*GetName());
		return;
	}

	// ─────────────────────────────────────────────
	// APPLY COLLISION SETTINGS
	// ─────────────────────────────────────────────

	// Set object type to WorldStatic (best for static objects that block)
	ActiveMesh->SetCollisionObjectType(ECC_WorldStatic);

	// Configure individual channel responses
	ActiveMesh->SetCollisionResponseToAllChannels(ECR_Ignore); // Start with all ignored

	// Visibility (always block for proper rendering/occlusion)
	ActiveMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Camera (configurable)
	ActiveMesh->SetCollisionResponseToChannel(ECC_Camera, 
		CollisionConfig.bBlockCamera ? ECR_Block : ECR_Ignore);

	// Player movement (Pawn channel - configurable)
	ActiveMesh->SetCollisionResponseToChannel(ECC_Pawn, 
		CollisionConfig.bBlockPlayer ? ECR_Block : ECR_Ignore);

	// World static/dynamic (block other physics objects)
	ActiveMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	ActiveMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	// Physics body (block)
	ActiveMesh->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);

	// Vehicle (block)
	ActiveMesh->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);

	// Destructible (block)
	ActiveMesh->SetCollisionResponseToChannel(ECC_Destructible, ECR_Block);

	// Custom Interactable trace channel (CRITICAL - must block for interaction)
	// ECC_GameTraceChannel1 is typically the first custom trace channel
	// Adjust this if your Interactable channel is on a different index
	ActiveMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, 
		CollisionConfig.bBlockInteractable ? ECR_Block : ECR_Overlap);

	// Configure overlap events
	ActiveMesh->SetGenerateOverlapEvents(CollisionConfig.bGenerateOverlapEvents);

	UE_LOG(LogLootChest, Log, TEXT("%s: Applied collision settings (BlockPlayer: %s, BlockInteractable: %s, BlockCamera: %s)"),
		*GetName(),
		CollisionConfig.bBlockPlayer ? TEXT("Yes") : TEXT("No"),
		CollisionConfig.bBlockInteractable ? TEXT("Yes") : TEXT("No"),
		CollisionConfig.bBlockCamera ? TEXT("Yes") : TEXT("No"));
}

void ALootChest::SetupInteraction()
{
	if (!InteractableManager)
	{
		return;
	}

	// Configure interaction
	InteractableManager->Config.bCanInteract = true;
	InteractableManager->Config.InteractionType = EInteractionType::IT_Tap;
	InteractableManager->Config.InteractionText = FText::FromString(TEXT("Open Chest"));

	// Setup highlight meshes (only add the visible one)
	InteractableManager->MeshesToHighlight.Empty();
	
	if (VisualConfig.bUseStaticMesh && Static_ChestMesh)
	{
		InteractableManager->MeshesToHighlight.Add(Static_ChestMesh);
	}
	else if (!VisualConfig.bUseStaticMesh && Skeletal_ChestMesh)
	{
		InteractableManager->MeshesToHighlight.Add(Skeletal_ChestMesh);
	}

	// Bind interaction event
	if (!InteractableManager->OnHoldCompleted.IsAlreadyBound(this, &ALootChest::OnInteracted))
	{
		InteractableManager->OnHoldCompleted.AddDynamic(this, &ALootChest::OnInteracted);
	}

	UE_LOG(LogLootChest, Log, TEXT("%s: Interaction setup complete"), *GetName());
}

void ALootChest::SetupVisuals()
{
	UpdateMeshForState();
}

void ALootChest::SetupLootComponent()
{
	if (!LootComponent)
	{
		UE_LOG(LogLootChest, Error, TEXT("%s: LootComponent is null!"), *GetName());
		return;
	}

	// Configure spawn settings from our config
	LootComponent->DefaultSpawnSettings = SpawnConfig.ToSpawnSettings(GetActorLocation());
}

// ═══════════════════════════════════════════════════════════════════════
// INTERACTION CALLBACKS
// ═══════════════════════════════════════════════════════════════════════

void ALootChest::OnInteracted(AActor* Interactor)
{
	// Only allow interaction when closed
	if (ChestState != EChestState::CS_Closed)
	{
		UE_LOG(LogLootChest, Verbose, TEXT("%s: Cannot interact - state is %s"),
			*GetName(), *UEnum::GetValueAsString(ChestState));
		return;
	}

	OpenChest(Interactor);
}

// ═══════════════════════════════════════════════════════════════════════
// PUBLIC INTERFACE
// ═══════════════════════════════════════════════════════════════════════

void ALootChest::OpenChest(AActor* Opener)
{
	if (ChestState != EChestState::CS_Closed)
	{
		return;
	}

	// Server authority check
	if (!HasAuthority())
	{
		// TODO: RPC to server
		return;
	}

	LastInteractor = Opener;

	// Change state to opening
	SetChestState(EChestState::CS_Opening);

	// Play feedback
	PlayOpenSound();
	PlayOpenVFX();

	// Start animation (timer-based, not tick-based!)
	if (AnimationConfig.bPlayOpenAnimation)
	{
		StartOpenAnimation();
	}
	else
	{
		// Skip animation, go directly to open
		OnOpenAnimationComplete();
	}

	// Broadcast event
	OnChestOpened(Opener);

	UE_LOG(LogLootChest, Log, TEXT("%s: Opened by %s"),
		*GetName(), Opener ? *Opener->GetName() : TEXT("Unknown"));
}

void ALootChest::ResetChest()
{
	if (!HasAuthority())
	{
		return;
	}

	// Clear any existing timers
	GetWorldTimerManager().ClearTimer(OpenAnimationTimer);
	GetWorldTimerManager().ClearTimer(CloseAnimationTimer);
	GetWorldTimerManager().ClearTimer(RespawnTimer);

	// If using skeletal mesh with animation, play reverse animation
	if (!VisualConfig.bUseStaticMesh && AnimationConfig.bPlayOpenAnimation && VisualConfig.OpenAnimation)
	{
		SetChestState(EChestState::CS_Closing);
		PlayCloseSound();
		StartCloseAnimation();
	}
	else
	{
		// Static mesh or no animation - immediate reset
		LastInteractor = nullptr;
		LastLootBatch = FLootResultBatch();
		SetChestState(EChestState::CS_Closed);
		
		UE_LOG(LogLootChest, Log, TEXT("%s: Reset to closed state (immediate)"), *GetName());
	}
}

void ALootChest::ForceRespawn()
{
	if (!HasAuthority())
	{
		return;
	}

	// Clear respawn timer
	GetWorldTimerManager().ClearTimer(RespawnTimer);

	// Trigger respawn immediately
	HandleRespawn();

	UE_LOG(LogLootChest, Log, TEXT("%s: Forced respawn"), *GetName());
}

// ═══════════════════════════════════════════════════════════════════════
// STATE MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════

void ALootChest::SetChestState(EChestState NewState)
{
	if (ChestState == NewState)
	{
		return;
	}

	const EChestState OldState = ChestState;
	ChestState = NewState;

	// Update visuals and interaction based on new state
	UpdateMeshForState();
	UpdateInteractionForState();

	// Trigger replication
	if (HasAuthority())
	{
		OnRep_ChestState();
	}

	UE_LOG(LogLootChest, Verbose, TEXT("%s: State changed from %s to %s"),
		*GetName(), *UEnum::GetValueAsString(OldState), *UEnum::GetValueAsString(NewState));
}

void ALootChest::UpdateMeshForState()
{
	// For static mesh, swap between closed and open mesh
	if (VisualConfig.bUseStaticMesh && Static_ChestMesh)
	{
		switch (ChestState)
		{
		case EChestState::CS_Closed:
		case EChestState::CS_Closing:
		case EChestState::CS_Respawning:
			if (VisualConfig.ClosedMesh)
			{
				Static_ChestMesh->SetStaticMesh(VisualConfig.ClosedMesh);
			}
			break;

		case EChestState::CS_Opening:
		case EChestState::CS_Open:
		case EChestState::CS_Looted:
			if (VisualConfig.OpenMesh)
			{
				Static_ChestMesh->SetStaticMesh(VisualConfig.OpenMesh);
			}
			break;
		}
	}

	// For skeletal mesh, animation handles the visual state
	// (No mesh swap needed)
}

void ALootChest::UpdateInteractionForState()
{
	if (!InteractableManager)
	{
		return;
	}

	// Only allow interaction when closed
	const bool bCanInteractNow = (ChestState == EChestState::CS_Closed);
	InteractableManager->Config.bCanInteract = bCanInteractNow;
}

// ═══════════════════════════════════════════════════════════════════════
// GETTERS
// ═══════════════════════════════════════════════════════════════════════

bool ALootChest::IsSourceValid() const
{
	if (!LootComponent)
	{
		return false;
	}

	// Check if source exists in loot subsystem registry
	if (UWorld* World = GetWorld())
	{
		if (ULootSubsystem* LootSubsystem = World->GetSubsystem<ULootSubsystem>())
		{
			return LootSubsystem->IsSourceRegistered(LootComponent->SourceID);
		}
	}

	return false;
}

// ═══════════════════════════════════════════════════════════════════════
// NETWORKING
// ═══════════════════════════════════════════════════════════════════════

void ALootChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALootChest, ChestState);
}

void ALootChest::OnRep_ChestState()
{
	// Update client visuals and interaction when state replicates
	UpdateMeshForState();
	UpdateInteractionForState();

	UE_LOG(LogLootChest, Verbose, TEXT("%s: Client replicated state: %s"),
		*GetName(), *UEnum::GetValueAsString(ChestState));
}

// ═══════════════════════════════════════════════════════════════════════
// LOOT (Delegates to LootComponent)
// ═══════════════════════════════════════════════════════════════════════

void ALootChest::GetPlayerLootStats(AActor* Player, float& OutLuck, float& OutMagicFind) const
{
	OutLuck = 0.0f;
	OutMagicFind = 0.0f;

	if (!Player)
	{
		return;
	}

	// Try to get stats from player's StatsManager component
	if (UStatsManager* StatsManager = Player->FindComponentByClass<UStatsManager>())
	{
		if (bApplyPlayerLuck)
		{
			OutLuck = StatsManager->GetLuck();
		}

		if (bApplyPlayerMagicFind)
		{
			//OutMagicFind = StatsManager->GetStatValue(EStatType::MagicFind);
		}

		UE_LOG(LogLootChest, Verbose, TEXT("%s: Player %s stats - Luck: %.2f, MagicFind: %.2f"),
			*GetName(), *Player->GetName(), OutLuck, OutMagicFind);
	}
	else
	{
		UE_LOG(LogLootChest, Warning, TEXT("%s: Player %s has no StatsManager component"),
			*GetName(), *Player->GetName());
	}
}

void ALootChest::GenerateAndSpawnLoot(AActor* Opener)
{
	if (!LootComponent)
	{
		UE_LOG(LogLootChest, Error, TEXT("%s: Cannot generate loot - LootComponent unavailable"),
			*GetName());
		return;
	}

	// Get player stats
	float Luck = 0.0f;
	float MagicFind = 0.0f;
	GetPlayerLootStats(Opener, Luck, MagicFind);

	// Update spawn location (in case chest moved)
	LootComponent->DefaultSpawnSettings = SpawnConfig.ToSpawnSettings(GetActorLocation());

	// Generate and spawn loot via component
	LastLootBatch = LootComponent->DropLoot(Luck, MagicFind);

	// Broadcast event
	OnLootGenerated(LastLootBatch);

	UE_LOG(LogLootChest, Log, TEXT("%s: Generated %d items, %d currency"),
		*GetName(), LastLootBatch.TotalItemCount, LastLootBatch.CurrencyDropped);

	// Transition to looted state
	SetChestState(EChestState::CS_Looted);

	// Broadcast looted event
	OnChestLooted();

	// Start respawn timer if enabled
	if (RespawnConfig.bCanRespawn)
	{
		StartRespawnTimer();
	}
}

// ═══════════════════════════════════════════════════════════════════════
// ANIMATION 
// ═══════════════════════════════════════════════════════════════════════

float ALootChest::GetAnimationDuration() const
{
	// For skeletal mesh, use actual animation length
	if (!VisualConfig.bUseStaticMesh && VisualConfig.OpenAnimation)
	{
		const float AnimLength = VisualConfig.OpenAnimation->GetPlayLength();
		const float PlayRate = FMath::Max(AnimationConfig.AnimationPlayRate, 0.1f);
		return AnimLength / PlayRate;
	}

	// For static mesh, use configured duration
	return AnimationConfig.OpenAnimationDuration;
}

void ALootChest::StartOpenAnimation()
{
	const float Duration = GetAnimationDuration();

	// Start animation based on mesh type
	if (!VisualConfig.bUseStaticMesh)
	{
		PlaySkeletalAnimation(false);
	}

	// Set timer for completion callback
	GetWorldTimerManager().SetTimer(
		OpenAnimationTimer,
		this,
		&ALootChest::OnOpenAnimationComplete,
		Duration,
		false
	);

	UE_LOG(LogLootChest, Verbose, TEXT("%s: Started open animation (%.2fs, %s)"),
		*GetName(), Duration, VisualConfig.bUseStaticMesh ? TEXT("Static") : TEXT("Skeletal"));
}

void ALootChest::OnOpenAnimationComplete()
{
	// Animation complete - transition to open state
	SetChestState(EChestState::CS_Open);

	// Now generate and spawn the loot
	GenerateAndSpawnLoot(LastInteractor);

	UE_LOG(LogLootChest, Verbose, TEXT("%s: Open animation complete"), *GetName());
}

void ALootChest::StartCloseAnimation()
{
	const float Duration = GetAnimationDuration();

	// Start reverse animation
	if (!VisualConfig.bUseStaticMesh)
	{
		PlaySkeletalAnimation(true); // Reverse
	}

	// Set timer for completion callback
	GetWorldTimerManager().SetTimer(
		CloseAnimationTimer,
		this,
		&ALootChest::OnCloseAnimationComplete,
		Duration,
		false
	);

	UE_LOG(LogLootChest, Verbose, TEXT("%s: Started close animation (%.2fs, reverse)"),
		*GetName(), Duration);
}

void ALootChest::OnCloseAnimationComplete()
{
	// Reset state data
	LastInteractor = nullptr;
	LastLootBatch = FLootResultBatch();

	// Transition to closed state
	SetChestState(EChestState::CS_Closed);

	UE_LOG(LogLootChest, Log, TEXT("%s: Reset to closed state (animation complete)"), *GetName());
}

// ═══════════════════════════════════════════════════════════════════════
// SKELETAL MESH ANIMATION HELPERS
// ═══════════════════════════════════════════════════════════════════════

void ALootChest::PlaySkeletalAnimation(bool bReverse)
{
	if (!Skeletal_ChestMesh || !VisualConfig.OpenAnimation)
	{
		UE_LOG(LogLootChest, Warning, TEXT("%s: Cannot play skeletal animation - missing component or animation"),
			*GetName());
		return;
	}

	const float PlayRate = AnimationConfig.AnimationPlayRate * (bReverse ? -1.0f : 1.0f);
	const float StartPosition = bReverse ? VisualConfig.OpenAnimation->GetPlayLength() : 0.0f;

	// Play animation with specified rate and start position
	Skeletal_ChestMesh->PlayAnimation(VisualConfig.OpenAnimation, false);
	Skeletal_ChestMesh->SetPlayRate(PlayRate);
	Skeletal_ChestMesh->SetPosition(StartPosition);
	
	UE_LOG(LogLootChest, Verbose, TEXT("%s: Playing skeletal animation (Rate: %.2f, Start: %.2f, Reverse: %s)"),
		*GetName(), PlayRate, StartPosition, bReverse ? TEXT("Yes") : TEXT("No"));
}

void ALootChest::StopSkeletalAnimation()
{
	if (!Skeletal_ChestMesh)
	{
		return;
	}

	Skeletal_ChestMesh->Stop();
}

void ALootChest::SetSkeletalAnimationPosition(float NormalizedPosition)
{
	if (!Skeletal_ChestMesh || !VisualConfig.OpenAnimation)
	{
		return;
	}

	// Clamp to valid range
	NormalizedPosition = FMath::Clamp(NormalizedPosition, 0.0f, 1.0f);

	// Calculate actual position in animation
	const float AnimLength = VisualConfig.OpenAnimation->GetPlayLength();
	const float TargetPosition = NormalizedPosition * AnimLength;

	// Set animation to specific frame (stopped)
	Skeletal_ChestMesh->SetAnimation(VisualConfig.OpenAnimation);
	Skeletal_ChestMesh->SetPosition(TargetPosition);
	Skeletal_ChestMesh->SetPlayRate(0.0f); // Stopped

	UE_LOG(LogLootChest, Verbose, TEXT("%s: Set skeletal animation position to %.2f (%.2fs)"),
		*GetName(), NormalizedPosition, TargetPosition);
}

// ═══════════════════════════════════════════════════════════════════════
// VISUAL/AUDIO FEEDBACK
// ═══════════════════════════════════════════════════════════════════════

void ALootChest::PlayOpenSound()
{
	if (FeedbackConfig.OpenSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			FeedbackConfig.OpenSound,
			GetActorLocation()
		);
	}
}

void ALootChest::PlayCloseSound()
{
	if (FeedbackConfig.CloseSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			FeedbackConfig.CloseSound,
			GetActorLocation()
		);
	}
}

void ALootChest::PlayOpenVFX()
{
	if (FeedbackConfig.OpenNiagaraEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			FeedbackConfig.OpenNiagaraEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}
}

// ═══════════════════════════════════════════════════════════════════════
// RESPAWN
// ═══════════════════════════════════════════════════════════════════════

void ALootChest::StartRespawnTimer()
{
	if (!RespawnConfig.bCanRespawn || RespawnConfig.RespawnTime <= 0.0f)
	{
		return;
	}

	SetChestState(EChestState::CS_Respawning);

	GetWorldTimerManager().SetTimer(
		RespawnTimer,
		this,
		&ALootChest::HandleRespawn,
		RespawnConfig.RespawnTime,
		false
	);

	UE_LOG(LogLootChest, Log, TEXT("%s: Respawn timer started (%.1fs)"),
		*GetName(), RespawnConfig.RespawnTime);
}

void ALootChest::HandleRespawn()
{
	// Play close animation if configured
	if (RespawnConfig.bPlayCloseAnimationOnRespawn && 
		!VisualConfig.bUseStaticMesh && 
		VisualConfig.OpenAnimation &&
		AnimationConfig.bPlayOpenAnimation)
	{
		PlayCloseSound();
		StartCloseAnimation();
		// Close animation will set state to CS_Closed when complete
	}
	else
	{
		// Immediate respawn
		LastInteractor = nullptr;
		LastLootBatch = FLootResultBatch();
		SetChestState(EChestState::CS_Closed);
	}

	// Broadcast event
	OnChestRespawned();

	UE_LOG(LogLootChest, Log, TEXT("%s: Respawned"), *GetName());
}
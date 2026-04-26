// Interactable/Actors/LootChest/LootChest.cpp

#include "Interactable/Actors/LootChest/LootChest.h"
#include "Interactable/Component/InteractableManager.h"
#include "Components/BoxComponent.h"
#include "Systems/Loot/Components/LootComponent.h"
#include "Systems/Loot/Subsystems/LootSubsystem.h"
#include "Systems/Stats/Components/StatsManager.h"
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

	// Optional spawn area box — resize in the editor viewport per chest.
	// Collision is disabled; it's purely a visual/data volume.
	SpawnAreaBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnAreaBox"));
	SpawnAreaBox->SetupAttachment(RootComponent);
	SpawnAreaBox->SetBoxExtent(FVector(150.f, 150.f, 0.f));
	SpawnAreaBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnAreaBox->SetHiddenInGame(true);
	SpawnAreaBox->ShapeColor = FColor::Cyan;

	// Initialize state
	ChestState = EChestState::CS_Closed;
	LastInteractor = nullptr;
	bLootSpawnedForCurrentOpen = false;
	bOpenSequenceFinalizedForCurrentOpen = false;
	
	ConfigureMeshVisibilityAndCollision();
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
	PreloadLootSourceIfPossible();

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

	// Base settings from SpawnConfig
	FLootSpawnSettings Settings = SpawnConfig.ToSpawnSettings(GetActorLocation());

	// If a SpawnAreaBox is present with non-zero extent, use box mode
	if (SpawnAreaBox)
	{
		const FVector Extent = SpawnAreaBox->GetScaledBoxExtent();
		if (!Extent.IsNearlyZero())
		{
			Settings.bUseSpawnBox = true;
			Settings.SpawnBoxExtent = Extent;
			// Use the box's world centre as the spawn origin
			Settings.SpawnLocation = SpawnAreaBox->GetComponentLocation();
		}
	}

	LootComponent->DefaultSpawnSettings = Settings;
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

	// C-7 FIX: Forward to the server via RPC so clients can actually open chests.
	if (!HasAuthority())
	{
		ServerOpenChest(Opener);
		return;
	}

	LastInteractor = Opener;
	LastLootBatch = FLootResultBatch();
	ResetOpenSequenceTracking();

	// Change state to opening
	SetChestState(EChestState::CS_Opening);

	// N-09 FIX: Sound and VFX are client-only presentation — skip on dedicated server
	// to avoid wasted cycles and potential audio-system null-dereferences.
	if (!IsNetMode(NM_DedicatedServer))
	{
		PlayOpenSound();
		PlayOpenVFX();
	}

	GenerateAndSpawnLoot(Opener);

	// Broadcast event before any immediate finalization path runs.
	OnChestOpened(Opener);

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

	UE_LOG(LogLootChest, Log, TEXT("%s: Opened by %s"),
		*GetName(), Opener ? *Opener->GetName() : TEXT("Unknown"));
}

void ALootChest::ServerOpenChest_Implementation(AActor* Opener)
{
	// C-7 FIX: Server-side body for the Server RPC.  Delegates straight to
	// OpenChest(); HasAuthority() will be true here so the guard passes.
	OpenChest(Opener);
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
	ResetOpenSequenceTracking();

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

	// N-11 FIX: Do NOT call OnRep_ChestState() on the server. OnRep callbacks are
	// client-only notification paths; calling them server-side conflates two different
	// code paths and can cause double-execution on listen servers.
	// The server runs UpdateMeshForState() and UpdateInteractionForState() directly above.
	// Clients receive the state change via normal property replication and their
	// OnRep_ChestState() fires automatically.

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
		return;
	}

	// ─────────────────────────────────────────────
	// SKELETAL MESH: drive animation from state so that BOTH the server
	// (via SetChestState) and remote clients (via OnRep_ChestState) play
	// the OpenAnimation. Previously only the server called
	// PlaySkeletalAnimation() from StartOpenAnimation(), which meant
	// clients never saw the chest open. This also pins the mesh to the
	// correct pose for terminal states so a non-looping single-node
	// animation doesn't snap back to frame 0 when playback ends.
	// ─────────────────────────────────────────────
	if (!Skeletal_ChestMesh)
	{
		return;
	}

	const bool bCanAnimate = AnimationConfig.bPlayOpenAnimation && VisualConfig.OpenAnimation != nullptr;

	switch (ChestState)
	{
	case EChestState::CS_Opening:
		if (bCanAnimate)
		{
			PlaySkeletalAnimation(false);
		}
		else if (VisualConfig.OpenAnimation)
		{
			SetSkeletalAnimationPosition(1.0f);
		}
		break;

	case EChestState::CS_Open:
	case EChestState::CS_Looted:
		// Pin fully-open pose so the non-looping animation doesn't
		// rest back on frame 0 once PlayLength is reached.
		if (VisualConfig.OpenAnimation)
		{
			SetSkeletalAnimationPosition(1.0f);
		}
		break;

	case EChestState::CS_Closing:
	case EChestState::CS_Respawning:
		if (bCanAnimate)
		{
			PlaySkeletalAnimation(true);
		}
		break;

	case EChestState::CS_Closed:
		// Ensure closed pose (frame 0) after reset / initial spawn.
		if (VisualConfig.OpenAnimation)
		{
			SetSkeletalAnimationPosition(0.0f);
		}
		break;
	}
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
			// C-6 FIX: Call GetMagicFind() – previously commented out, leaving
			// OutMagicFind stuck at 0.0f and magic find never influencing loot.
			OutMagicFind = StatsManager->GetMagicFind();
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
	if (bLootSpawnedForCurrentOpen)
	{
		return;
	}

	bLootSpawnedForCurrentOpen = true;

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

	// Refresh spawn settings (updates location and box in case chest moved)
	SetupLootComponent();

	// Generate and spawn loot via component
	LastLootBatch = LootComponent->DropLoot(Luck, MagicFind);

	// Broadcast event
	OnLootGenerated(LastLootBatch);

	UE_LOG(LogLootChest, Log, TEXT("%s: Generated %d items, %d currency"),
		*GetName(), LastLootBatch.TotalItemCount, LastLootBatch.CurrencyDropped);
}

void ALootChest::FinalizeOpenSequence()
{
	if (bOpenSequenceFinalizedForCurrentOpen)
	{
		return;
	}

	bOpenSequenceFinalizedForCurrentOpen = true;

	// Transition to looted state
	SetChestState(EChestState::CS_Open);
	SetChestState(EChestState::CS_Looted);

	// Broadcast looted event
	OnChestLooted();

	// Start respawn timer if enabled
	if (RespawnConfig.bCanRespawn)
	{
		StartRespawnTimer();
	}
}

void ALootChest::PreloadLootSourceIfPossible()
{
	if (!LootComponent || LootComponent->SourceID.IsNone())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (ULootSubsystem* LootSubsystem = World->GetSubsystem<ULootSubsystem>())
		{
			TArray<FName> SourceIDs;
			SourceIDs.Add(LootComponent->SourceID);
			LootSubsystem->PreloadLootTables(SourceIDs);
		}
	}
}

void ALootChest::ResetOpenSequenceTracking()
{
	bLootSpawnedForCurrentOpen = false;
	bOpenSequenceFinalizedForCurrentOpen = false;
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

	// NOTE: The actual skeletal animation is now kicked off by
	// UpdateMeshForState() in response to the state transition
	// (CS_Closed -> CS_Opening). This guarantees remote clients
	// play it too via OnRep_ChestState. The server only needs the
	// completion timer below so FinalizeOpenSequence() runs.

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
	FinalizeOpenSequence();

	UE_LOG(LogLootChest, Verbose, TEXT("%s: Open animation complete"), *GetName());
}

void ALootChest::StartCloseAnimation()
{
	const float Duration = GetAnimationDuration();

	// NOTE: Reverse playback is driven by UpdateMeshForState() when the
	// state transitions to CS_Closing / CS_Respawning, so clients play it
	// too. The server only needs the completion timer below.

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
	ResetOpenSequenceTracking();

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

	// Ensure we're in single-node mode (ConfigureMeshVisibilityAndCollision
	// may have set rate to 0 via SetSkeletalAnimationPosition; we must
	// fully re-drive the instance or it will appear "stuck" / "reset").
	Skeletal_ChestMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	// SetAnimation() guarantees a valid UAnimSingleNodeInstance is attached
	// to the mesh's current asset before we poke at it.
	Skeletal_ChestMesh->SetAnimation(VisualConfig.OpenAnimation);

	UAnimSingleNodeInstance* SingleNode = Skeletal_ChestMesh->GetSingleNodeInstance();
	if (!SingleNode)
	{
		UE_LOG(LogLootChest, Warning, TEXT("%s: Skeletal mesh has no single-node instance; cannot play OpenAnimation"),
			*GetName());
		return;
	}

	const float AbsRate     = FMath::Max(AnimationConfig.AnimationPlayRate, 0.01f);
	const float PlayRate    = bReverse ? -AbsRate : AbsRate;
	const float AnimLength  = VisualConfig.OpenAnimation->GetPlayLength();
	const float StartPos    = bReverse ? AnimLength : 0.0f;

	// Order matters: set asset + position BEFORE toggling Playing, and set
	// the signed PlayRate last so Play() (which resets rate to +1) can't
	// clobber us.
	SingleNode->SetLooping(false);
	SingleNode->SetPosition(StartPos, /*bFireNotifies*/ false);
	SingleNode->SetPlaying(true);
	SingleNode->SetPlayRate(PlayRate);

	UE_LOG(LogLootChest, Verbose, TEXT("%s: Playing skeletal animation (Rate: %.2f, Start: %.2f, Reverse: %s)"),
		*GetName(), PlayRate, StartPos, bReverse ? TEXT("Yes") : TEXT("No"));
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
		ResetOpenSequenceTracking();
		SetChestState(EChestState::CS_Closed);
	}

	// Broadcast event
	OnChestRespawned();

	UE_LOG(LogLootChest, Log, TEXT("%s: Respawned"), *GetName());
}

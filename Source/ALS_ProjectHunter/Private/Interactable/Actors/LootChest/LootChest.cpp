#include "Interactable/Actors/LootChest/LootChest.h"
#include "Interactable/Component/InteractableManager.h"
#include "Components/BoxComponent.h"
#include "Loot/Components/LootComponent.h"
#include "Loot/Subsystems/LootSubsystem.h"
#include "Stats/Components/StatsManager.h"
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

ALootChest::ALootChest()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootSceneComponent;

	Static_ChestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticChestMesh"));
	Static_ChestMesh->SetupAttachment(RootComponent);
	Static_ChestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Skeletal_ChestMesh =CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalChestMesh"));
	Skeletal_ChestMesh->SetupAttachment(RootComponent);
	Skeletal_ChestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Skeletal_ChestMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	InteractableManager = CreateDefaultSubobject<UInteractableManager>(TEXT("InteractableManager"));

	LootComponent = CreateDefaultSubobject<ULootComponent>(TEXT("LootComponent"));

	// Optional spawn area box — resize in the editor viewport per chest.
	// Collision is disabled; it's purely a visual/data volume.
	SpawnAreaBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnAreaBox"));
	SpawnAreaBox->SetupAttachment(RootComponent);
	SpawnAreaBox->SetBoxExtent(FVector(150.f, 150.f, 0.f));
	SpawnAreaBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnAreaBox->SetHiddenInGame(true);
	SpawnAreaBox->ShapeColor = FColor::Cyan;

	ChestState = EChestState::CS_Closed;
	LastInteractor = nullptr;
	bLootSpawnedForCurrentOpen = false;
	bOpenSequenceFinalizedForCurrentOpen = false;

	ConfigureMeshVisibilityAndCollision();
}

void ALootChest::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ConfigureMeshVisibilityAndCollision();

	UE_LOG(LogLootChest, Verbose, TEXT("%s: OnConstruction - Configured mesh (Type: %s)"),
		*GetName(), VisualConfig.bUseStaticMesh ? TEXT("Static") : TEXT("Skeletal"));
}

void ALootChest::BeginPlay()
{
	Super::BeginPlay();

	SetupInteraction();
	SetupVisuals();
	SetupLootComponent();
	PreloadLootSourceIfPossible();

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

	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(ALootChest, VisualConfig))
	{
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

	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(ALootChest, CollisionConfig))
	{
		ApplyCollisionSettings();

		UE_LOG(LogLootChest, Verbose, TEXT("%s: Editor - Collision config changed, reapplied settings"),
			*GetName());
	}
}
#endif

void ALootChest::ConfigureMeshVisibilityAndCollision()
{
	if (VisualConfig.bUseStaticMesh)
	{
		if (Static_ChestMesh)
		{
			if (VisualConfig.ClosedMesh)
			{
				Static_ChestMesh->SetStaticMesh(VisualConfig.ClosedMesh);
			}

			Static_ChestMesh->SetVisibility(true);
			Static_ChestMesh->SetHiddenInGame(false);
			Static_ChestMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}

		if (Skeletal_ChestMesh)
		{
			Skeletal_ChestMesh->SetVisibility(false);
			Skeletal_ChestMesh->SetHiddenInGame(true);
			Skeletal_ChestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	else
	{
		if (Static_ChestMesh)
		{
			Static_ChestMesh->SetVisibility(false);
			Static_ChestMesh->SetHiddenInGame(true);
			Static_ChestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		if (Skeletal_ChestMesh)
		{
			if (VisualConfig.SkeletalMesh)
			{
				Skeletal_ChestMesh->SetSkeletalMesh(VisualConfig.SkeletalMesh);
			}

			Skeletal_ChestMesh->SetVisibility(true);
			Skeletal_ChestMesh->SetHiddenInGame(false);
			Skeletal_ChestMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

			SetSkeletalAnimationPosition(0.0f);
		}
	}

	ApplyCollisionSettings();

	UE_LOG(LogLootChest, Verbose, TEXT("%s: Configured mesh visibility and collision (Static: %s, Skeletal: %s)"),
		*GetName(),
		Static_ChestMesh && Static_ChestMesh->IsVisible() ? TEXT("Visible") : TEXT("Hidden"),
		Skeletal_ChestMesh && Skeletal_ChestMesh->IsVisible() ? TEXT("Visible") : TEXT("Hidden"));
}

void ALootChest::ApplyCollisionSettings()
{
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

	ActiveMesh->SetCollisionObjectType(ECC_WorldStatic);

	ActiveMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

	ActiveMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	ActiveMesh->SetCollisionResponseToChannel(ECC_Camera,
		CollisionConfig.bBlockCamera ? ECR_Block : ECR_Ignore);

	ActiveMesh->SetCollisionResponseToChannel(ECC_Pawn,
		CollisionConfig.bBlockPlayer ? ECR_Block : ECR_Ignore);

	ActiveMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	ActiveMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	ActiveMesh->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);

	ActiveMesh->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);

	ActiveMesh->SetCollisionResponseToChannel(ECC_Destructible, ECR_Block);

	// ECC_GameTraceChannel1 is the Interactable trace channel; must block for interaction to register.
	// Adjust this if your Interactable channel is on a different index.
	ActiveMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1,
		CollisionConfig.bBlockInteractable ? ECR_Block : ECR_Overlap);

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

	InteractableManager->MeshesToHighlight.Empty();

	if (VisualConfig.bUseStaticMesh && Static_ChestMesh)
	{
		InteractableManager->MeshesToHighlight.Add(Static_ChestMesh);
	}
	else if (!VisualConfig.bUseStaticMesh && Skeletal_ChestMesh)
	{
		InteractableManager->MeshesToHighlight.Add(Skeletal_ChestMesh);
	}

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

	FLootSpawnSettings Settings = SpawnConfig.ToSpawnSettings(GetActorLocation());

	if (SpawnAreaBox)
	{
		const FVector Extent = SpawnAreaBox->GetScaledBoxExtent();
		if (!Extent.IsNearlyZero())
		{
			Settings.bUseSpawnBox = true;
			Settings.SpawnBoxExtent = Extent;
			Settings.SpawnLocation = SpawnAreaBox->GetComponentLocation();
		}
	}

	LootComponent->DefaultSpawnSettings = Settings;
}

void ALootChest::OnInteracted(AActor* Interactor)
{
	if (ChestState != EChestState::CS_Closed)
	{
		UE_LOG(LogLootChest, Verbose, TEXT("%s: Cannot interact - state is %s"),
			*GetName(), *UEnum::GetValueAsString(ChestState));
		return;
	}

	OpenChest(Interactor);
}

void ALootChest::OpenChest(AActor* Opener)
{
	if (ChestState != EChestState::CS_Closed)
	{
		return;
	}

	if (!HasAuthority())
	{
		ServerOpenChest(Opener);
		return;
	}

	LastInteractor = Opener;
	LastLootBatch = FLootResultBatch();
	ResetOpenSequenceTracking();

	SetChestState(EChestState::CS_Opening);

	// Sound and VFX are client-only presentation — skip on dedicated server
	// to avoid wasted cycles and potential audio-system null-dereferences.
	if (!IsNetMode(NM_DedicatedServer))
	{
		PlayOpenSound();
		PlayOpenVFX();
	}

	GenerateAndSpawnLoot(Opener);

	OnChestOpened(Opener);

	if (AnimationConfig.bPlayOpenAnimation)
	{
		StartOpenAnimation();
	}
	else
	{
		OnOpenAnimationComplete();
	}

	UE_LOG(LogLootChest, Log, TEXT("%s: Opened by %s"),
		*GetName(), Opener ? *Opener->GetName() : TEXT("Unknown"));
}

void ALootChest::ServerOpenChest_Implementation(AActor* Opener)
{
	OpenChest(Opener);
}

void ALootChest::ResetChest()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(OpenAnimationTimer);
	GetWorldTimerManager().ClearTimer(CloseAnimationTimer);
	GetWorldTimerManager().ClearTimer(RespawnTimer);
	ResetOpenSequenceTracking();

	if (!VisualConfig.bUseStaticMesh && AnimationConfig.bPlayOpenAnimation && VisualConfig.OpenAnimation)
	{
		SetChestState(EChestState::CS_Closing);
		PlayCloseSound();
		StartCloseAnimation();
	}
	else
	{
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

	GetWorldTimerManager().ClearTimer(RespawnTimer);

	HandleRespawn();

	UE_LOG(LogLootChest, Log, TEXT("%s: Forced respawn"), *GetName());
}

void ALootChest::SetChestState(EChestState NewState)
{
	if (ChestState == NewState)
	{
		return;
	}

	const EChestState OldState = ChestState;
	ChestState = NewState;

	UpdateMeshForState();
	UpdateInteractionForState();

	// Do NOT call OnRep_ChestState() on the server. OnRep callbacks are
	// client-only notification paths; calling them server-side conflates two
	// code paths and can cause double-execution on listen servers.
	// Clients receive the state change via normal property replication and their
	// OnRep_ChestState() fires automatically.

	UE_LOG(LogLootChest, Verbose, TEXT("%s: State changed from %s to %s"),
		*GetName(), *UEnum::GetValueAsString(OldState), *UEnum::GetValueAsString(NewState));
}

void ALootChest::UpdateMeshForState()
{
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

	// Drive the skeletal animation from state so that BOTH the server (via
	// SetChestState) and remote clients (via OnRep_ChestState) play the
	// OpenAnimation. Previously only the server called PlaySkeletalAnimation()
	// from StartOpenAnimation(), which meant clients never saw the chest open.
	// Terminal states pin the mesh to the correct pose so a non-looping
	// single-node animation doesn't snap back to frame 0 when playback ends.
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

	const bool bCanInteractNow = (ChestState == EChestState::CS_Closed);
	InteractableManager->Config.bCanInteract = bCanInteractNow;
}

bool ALootChest::IsSourceValid() const
{
	if (!LootComponent)
	{
		return false;
	}

	if (UWorld* World = GetWorld())
	{
		if (ULootSubsystem* LootSubsystem = World->GetSubsystem<ULootSubsystem>())
		{
			return LootSubsystem->IsSourceRegistered(LootComponent->SourceID);
		}
	}

	return false;
}

void ALootChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALootChest, ChestState);
}

void ALootChest::OnRep_ChestState()
{
	UpdateMeshForState();
	UpdateInteractionForState();

	UE_LOG(LogLootChest, Verbose, TEXT("%s: Client replicated state: %s"),
		*GetName(), *UEnum::GetValueAsString(ChestState));
}

void ALootChest::GetPlayerLootStats(AActor* Player, float& OutLuck, float& OutMagicFind) const
{
	OutLuck = 0.0f;
	OutMagicFind = 0.0f;

	if (!Player)
	{
		return;
	}

	if (UStatsManager* StatsManager = Player->FindComponentByClass<UStatsManager>())
	{
		if (bApplyPlayerLuck)
		{
			OutLuck = StatsManager->GetLuck();
		}

		if (bApplyPlayerMagicFind)
		{
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

	float Luck = 0.0f;
	float MagicFind = 0.0f;
	GetPlayerLootStats(Opener, Luck, MagicFind);

	SetupLootComponent();

	LastLootBatch = LootComponent->DropLoot(Luck, MagicFind);

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

	SetChestState(EChestState::CS_Open);
	SetChestState(EChestState::CS_Looted);

	OnChestLooted();

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

float ALootChest::GetAnimationDuration() const
{
	if (!VisualConfig.bUseStaticMesh && VisualConfig.OpenAnimation)
	{
		const float AnimLength = VisualConfig.OpenAnimation->GetPlayLength();
		const float PlayRate = FMath::Max(AnimationConfig.AnimationPlayRate, 0.1f);
		return AnimLength / PlayRate;
	}

	return AnimationConfig.OpenAnimationDuration;
}

void ALootChest::StartOpenAnimation()
{
	const float Duration = GetAnimationDuration();

	// The actual skeletal animation is kicked off by UpdateMeshForState() in
	// response to the CS_Closed -> CS_Opening state transition, guaranteeing
	// remote clients play it too via OnRep_ChestState. The server only needs
	// the completion timer so FinalizeOpenSequence() runs.
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

	// Reverse playback is driven by UpdateMeshForState() when the state
	// transitions to CS_Closing / CS_Respawning, so clients play it too.
	// The server only needs the completion timer below.
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
	LastInteractor = nullptr;
	LastLootBatch = FLootResultBatch();
	ResetOpenSequenceTracking();

	SetChestState(EChestState::CS_Closed);

	UE_LOG(LogLootChest, Log, TEXT("%s: Reset to closed state (animation complete)"), *GetName());
}

void ALootChest::PlaySkeletalAnimation(bool bReverse)
{
	if (!Skeletal_ChestMesh || !VisualConfig.OpenAnimation)
	{
		UE_LOG(LogLootChest, Warning, TEXT("%s: Cannot play skeletal animation - missing component or animation"),
			*GetName());
		return;
	}

	// Ensure single-node mode. ConfigureMeshVisibilityAndCollision may have
	// set rate to 0 via SetSkeletalAnimationPosition; we must fully re-drive
	// the instance or it will appear stuck.
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

	NormalizedPosition = FMath::Clamp(NormalizedPosition, 0.0f, 1.0f);

	const float AnimLength = VisualConfig.OpenAnimation->GetPlayLength();
	const float TargetPosition = NormalizedPosition * AnimLength;

	Skeletal_ChestMesh->SetAnimation(VisualConfig.OpenAnimation);
	Skeletal_ChestMesh->SetPosition(TargetPosition);
	Skeletal_ChestMesh->SetPlayRate(0.0f);

	UE_LOG(LogLootChest, Verbose, TEXT("%s: Set skeletal animation position to %.2f (%.2fs)"),
		*GetName(), NormalizedPosition, TargetPosition);
}

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
	if (RespawnConfig.bPlayCloseAnimationOnRespawn &&
		!VisualConfig.bUseStaticMesh &&
		VisualConfig.OpenAnimation &&
		AnimationConfig.bPlayOpenAnimation)
	{
		PlayCloseSound();
		StartCloseAnimation();
	}
	else
	{
		LastInteractor = nullptr;
		LastLootBatch = FLootResultBatch();
		ResetOpenSequenceTracking();
		SetChestState(EChestState::CS_Closed);
	}

	OnChestRespawned();

	UE_LOG(LogLootChest, Log, TEXT("%s: Respawned"), *GetName());
}

// Interactable/Actors/LootChest/LootChest.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Systems/Loot/Library/LootStructs.h"
#include "LootChest.generated.h"

// Forward declarations
class UInteractableManager;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UBoxComponent;
class UAnimSequence;
class UItemInstance;
class ULootComponent;
class ULootSubsystem;
class UStatsManager;
class USoundBase;
class UNiagaraSystem;

DECLARE_LOG_CATEGORY_EXTERN(LogLootChest, Log, All);

// ═══════════════════════════════════════════════════════════════════════
// CHEST STATE
// ═══════════════════════════════════════════════════════════════════════

/**
 * Chest state machine
 */
UENUM(BlueprintType)
enum class EChestState : uint8
{
	CS_Closed     UMETA(DisplayName = "Closed"),
	CS_Opening    UMETA(DisplayName = "Opening"),
	CS_Open       UMETA(DisplayName = "Open"),
	CS_Looted     UMETA(DisplayName = "Looted"),
	CS_Closing    UMETA(DisplayName = "Closing"),
	CS_Respawning UMETA(DisplayName = "Respawning")
};

// ═══════════════════════════════════════════════════════════════════════
// SPLIT CONFIG STRUCTS (Single Responsibility)
// ═══════════════════════════════════════════════════════════════════════

/**
 * FChestVisualConfig - Mesh and appearance settings
 * SINGLE RESPONSIBILITY: Visual representation only
 */
USTRUCT(BlueprintType)
struct FChestVisualConfig
{
	GENERATED_BODY()

	// ─────────────────────────────────────────────
	// MESH TYPE SELECTION
	// ─────────────────────────────────────────────

	/** Use static mesh (true) or skeletal mesh (false) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
	bool bUseStaticMesh = true;

	// ─────────────────────────────────────────────
	// STATIC MESH OPTIONS (shown when bUseStaticMesh = true)
	// ─────────────────────────────────────────────

	/** Static mesh when closed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals|Static Mesh", 
		meta = (EditCondition = "bUseStaticMesh", EditConditionHides))
	UStaticMesh* ClosedMesh = nullptr;

	/** Static mesh when open */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals|Static Mesh", 
		meta = (EditCondition = "bUseStaticMesh", EditConditionHides))
	UStaticMesh* OpenMesh = nullptr;

	// ─────────────────────────────────────────────
	// SKELETAL MESH OPTIONS (shown when bUseStaticMesh = false)
	// ─────────────────────────────────────────────

	/** Skeletal mesh (animated) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals|Skeletal Mesh", 
		meta = (EditCondition = "!bUseStaticMesh", EditConditionHides))
	USkeletalMesh* SkeletalMesh = nullptr;

	/** Open animation sequence (plays forward to open, reverse to close) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals|Skeletal Mesh", 
		meta = (EditCondition = "!bUseStaticMesh", EditConditionHides))
	UAnimSequence* OpenAnimation = nullptr;
};

/**
 * FChestCollisionConfig - Collision settings
 * SINGLE RESPONSIBILITY: Collision behavior only
 */
USTRUCT(BlueprintType)
struct FChestCollisionConfig
{
	GENERATED_BODY()

	/** Block player movement? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	bool bBlockPlayer = true;

	/** Block Interactable trace channel? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	bool bBlockInteractable = true;

	/** Block camera? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	bool bBlockCamera = true;

	/** Generate overlap events? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	bool bGenerateOverlapEvents = false;
};

/**
 * FChestAnimationConfig - Animation settings
 * SINGLE RESPONSIBILITY: Animation behavior only
 */
USTRUCT(BlueprintType)
struct FChestAnimationConfig
{
	GENERATED_BODY()

	/** Play open animation? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bPlayOpenAnimation = true;

	/** 
	 * Open animation duration (for static mesh timing)
	 * NOTE: For skeletal mesh, this is overridden by the actual animation length
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", 
		meta = (EditCondition = "bPlayOpenAnimation", ClampMin = "0.1"))
	float OpenAnimationDuration = 0.5f;

	/** Playback rate multiplier for skeletal mesh animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", 
		meta = (EditCondition = "bPlayOpenAnimation", ClampMin = "0.1", ClampMax = "5.0"))
	float AnimationPlayRate = 1.0f;
};

/**
 * FChestFeedbackConfig - Audio and VFX settings
 * SINGLE RESPONSIBILITY: Sensory feedback only
 */
USTRUCT(BlueprintType)
struct FChestFeedbackConfig
{
	GENERATED_BODY()

	/** Sound when opened */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* OpenSound = nullptr;

	/** Sound when closed/reset (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* CloseSound = nullptr;

	/** Niagara effect when opened */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* OpenNiagaraEffect = nullptr;
};

/**
 * FChestRespawnConfig - Respawn behavior settings
 * SINGLE RESPONSIBILITY: Respawn logic only
 */
USTRUCT(BlueprintType)
struct FChestRespawnConfig
{
	GENERATED_BODY()

	/** Can this chest respawn? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn")
	bool bCanRespawn = false;

	/** Time until respawn (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn", 
		meta = (EditCondition = "bCanRespawn", ClampMin = "0.0"))
	float RespawnTime = 300.0f;

	/** Re-roll loot on respawn? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn", 
		meta = (EditCondition = "bCanRespawn"))
	bool bRerollLootOnRespawn = true;

	/** Play close animation before respawn completes? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn", 
		meta = (EditCondition = "bCanRespawn"))
	bool bPlayCloseAnimationOnRespawn = true;
};

/**
 * FChestSpawnConfig - Loot spawn positioning
 * SINGLE RESPONSIBILITY: Item spawn placement only
 */
USTRUCT(BlueprintType)
struct FChestSpawnConfig
{
	GENERATED_BODY()

	/** Radius to scatter loot around chest */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0.0"))
	float ScatterRadius = 150.0f;

	/** Height offset for spawned items */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	float SpawnHeightOffset = 50.0f;

	/** Use random scatter (vs organized circle) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	bool bRandomScatter = true;

	/** Convert to FLootSpawnSettings for subsystem */
	FLootSpawnSettings ToSpawnSettings(FVector BaseLocation) const
	{
		FLootSpawnSettings Settings;
		Settings.SpawnLocation = BaseLocation;
		Settings.ScatterRadius = ScatterRadius;
		Settings.HeightOffset = SpawnHeightOffset;
		Settings.bRandomScatter = bRandomScatter;
		return Settings;
	}
};

// ═══════════════════════════════════════════════════════════════════════
// LOOT CHEST ACTOR
// ═══════════════════════════════════════════════════════════════════════

/**
 * ALootChest - Interactable loot chest actor
 */
UCLASS()
class ALS_PROJECTHUNTER_API ALootChest : public AActor
{
	GENERATED_BODY()
	
public:	
	ALootChest();

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// NOTE: No Tick() - uses timer-based animation instead

	// ═══════════════════════════════════════════════
	// COMPONENTS
	// ═══════════════════════════════════════════════

	/** Root scene component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;

	/** Static chest mesh (visible when bUseStaticMesh = true) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Static_ChestMesh;

	/** Skeletal chest mesh (visible when bUseStaticMesh = false) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* Skeletal_ChestMesh;

	/** Interaction component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInteractableManager* InteractableManager;

	/**
	 * Loot component - handles loot generation and spawning
	 * Configure SourceID here (e.g., "Chest_Common", "Chest_Rare")
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	ULootComponent* LootComponent;

	/**
	 * Optional box that defines the area items are scattered into when looted.
	 * If placed and has non-zero extent, overrides SpawnConfig scatter radius.
	 * Visible in editor so designers can resize it per-chest in the viewport.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* SpawnAreaBox;

	// ═══════════════════════════════════════════════
	// CONFIGURATION (Split by responsibility)
	// ═══════════════════════════════════════════════

	/** Visual appearance settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Chest|Visuals")
	FChestVisualConfig VisualConfig;

	/** Collision settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Chest|Collision")
	FChestCollisionConfig CollisionConfig;

	/** Animation settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Chest|Animation")
	FChestAnimationConfig AnimationConfig;

	/** Audio/VFX feedback settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Chest|Feedback")
	FChestFeedbackConfig FeedbackConfig;

	/** Respawn behavior settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Chest|Respawn")
	FChestRespawnConfig RespawnConfig;

	/** Loot spawn positioning settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Chest|Spawn")
	FChestSpawnConfig SpawnConfig;

	/** Apply player luck bonus to loot? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Chest|Loot")
	bool bApplyPlayerLuck = true;

	/** Apply player magic find bonus to loot? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Chest|Loot")
	bool bApplyPlayerMagicFind = true;

	// ═══════════════════════════════════════════════
	// STATE
	// ═══════════════════════════════════════════════

	/** Current chest state */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ChestState, Category = "Loot Chest|State")
	EChestState ChestState;

	/** Last generated loot batch (for reference) */
	UPROPERTY(BlueprintReadOnly, Category = "Loot Chest|State")
	FLootResultBatch LastLootBatch;

	/** Who opened this chest? */
	UPROPERTY(BlueprintReadOnly, Category = "Loot Chest|State")
	TObjectPtr<AActor> LastInteractor;

	// ═══════════════════════════════════════════════
	// EVENTS (Blueprint implementable)
	// ═══════════════════════════════════════════════

	/** Called when chest is opened */
	UFUNCTION(BlueprintImplementableEvent, Category = "Loot Chest|Events")
	void OnChestOpened(AActor* Opener);

	/** Called when loot is generated */
	UFUNCTION(BlueprintImplementableEvent, Category = "Loot Chest|Events")
	void OnLootGenerated(const FLootResultBatch& LootBatch);

	/** Called when chest is fully looted */
	UFUNCTION(BlueprintImplementableEvent, Category = "Loot Chest|Events")
	void OnChestLooted();

	/** Called when chest respawns */
	UFUNCTION(BlueprintImplementableEvent, Category = "Loot Chest|Events")
	void OnChestRespawned();

	// ═══════════════════════════════════════════════
	// PUBLIC INTERFACE
	// ═══════════════════════════════════════════════

	/**
	 * Open the chest
	 * @param Opener - Who opened it
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot Chest")
	void OpenChest(AActor* Opener);

	/**
	 * Reset chest to closed state (plays reverse animation if skeletal)
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot Chest")
	void ResetChest();

	/**
	 * Force respawn (ignores timer)
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot Chest")
	void ForceRespawn();

	// ═══════════════════════════════════════════════
	// GETTERS
	// ═══════════════════════════════════════════════

	UFUNCTION(BlueprintPure, Category = "Loot Chest")
	EChestState GetChestState() const { return ChestState; }

	UFUNCTION(BlueprintPure, Category = "Loot Chest")
	bool IsOpen() const { return ChestState == EChestState::CS_Open; }

	UFUNCTION(BlueprintPure, Category = "Loot Chest")
	bool IsLooted() const { return ChestState == EChestState::CS_Looted; }

	UFUNCTION(BlueprintPure, Category = "Loot Chest")
	bool IsSourceValid() const;

	UFUNCTION(BlueprintPure, Category = "Loot Chest")
	bool IsUsingSkeletalMesh() const { return !VisualConfig.bUseStaticMesh; }

	// ═══════════════════════════════════════════════
	// NETWORKING
	// ═══════════════════════════════════════════════

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_ChestState();

	/**
	 * C-7 FIX: Server RPC so clients can request chest opening.
	 * OpenChest() on a client now calls this instead of returning early.
	 */
	UFUNCTION(Server, Reliable)
	void ServerOpenChest(AActor* Opener);

protected:
	// ═══════════════════════════════════════════════
	// INITIALIZATION
	// ═══════════════════════════════════════════════

	void SetupInteraction();
	void SetupVisuals();
	void SetupLootComponent();

	/** Configure mesh component visibility and collision based on settings */
	void ConfigureMeshVisibilityAndCollision();

	/** Apply collision settings to active mesh component */
	void ApplyCollisionSettings();

	// ═══════════════════════════════════════════════
	// INTERACTION CALLBACKS
	// ═══════════════════════════════════════════════

	UFUNCTION()
	void OnInteracted(AActor* Interactor);

	// ═══════════════════════════════════════════════
	// STATE MANAGEMENT
	// ═══════════════════════════════════════════════

	void SetChestState(EChestState NewState);
	void UpdateMeshForState();
	void UpdateInteractionForState();

	// ═══════════════════════════════════════════════
	// LOOT (Delegates to LootComponent)
	// ═══════════════════════════════════════════════

	void GetPlayerLootStats(AActor* Player, float& OutLuck, float& OutMagicFind) const;
	void GenerateAndSpawnLoot(AActor* Opener);
	void FinalizeOpenSequence();
	void PreloadLootSourceIfPossible();
	void ResetOpenSequenceTracking();

	// ═══════════════════════════════════════════════
	// ANIMATION (Timer-based, not Tick-based)
	// ═══════════════════════════════════════════════

	/** Start open animation (forward for skeletal, timer for static) */
	void StartOpenAnimation();

	/** Called when open animation completes */
	void OnOpenAnimationComplete();

	/** Start close animation (reverse for skeletal, timer for static) */
	void StartCloseAnimation();

	/** Called when close animation completes */
	void OnCloseAnimationComplete();

	/** Get the effective animation duration */
	float GetAnimationDuration() const;

	// ─────────────────────────────────────────────
	// SKELETAL MESH ANIMATION HELPERS
	// ─────────────────────────────────────────────

	/** Play skeletal mesh animation in specified direction */
	void PlaySkeletalAnimation(bool bReverse);

	/** Stop any playing skeletal animation */
	void StopSkeletalAnimation();

	/** Set skeletal mesh to specific animation position (0.0 = closed, 1.0 = open) */
	void SetSkeletalAnimationPosition(float NormalizedPosition);

	// ═══════════════════════════════════════════════
	// VISUAL/AUDIO FEEDBACK
	// ═══════════════════════════════════════════════

	void PlayOpenSound();
	void PlayCloseSound();
	void PlayOpenVFX();

	// ═══════════════════════════════════════════════
	// RESPAWN
	// ═══════════════════════════════════════════════

	void StartRespawnTimer();
	void HandleRespawn();

	// ═══════════════════════════════════════════════
	// TIMERS
	// ═══════════════════════════════════════════════

	FTimerHandle OpenAnimationTimer;
	FTimerHandle CloseAnimationTimer;
	FTimerHandle RespawnTimer;

	/** Guards against duplicate world spawns while an open animation is still running. */
	bool bLootSpawnedForCurrentOpen = false;

	/** Guards against duplicate post-animation finalization. */
	bool bOpenSequenceFinalizedForCurrentOpen = false;
};

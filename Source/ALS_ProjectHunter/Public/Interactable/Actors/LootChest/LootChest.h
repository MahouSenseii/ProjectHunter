// Interactable/Actors/LootChest/LootChest.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable/Library/Enums/ChestEnums.h"
#include "Interactable/Library/Structs/ChestStructs.h"
#include "Loot/Library/LootStructs.h"
#include "LootChest.generated.h"

class UInteractableManager;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UBoxComponent;
class ULootComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogLootChest, Log, All);

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Static_ChestMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* Skeletal_ChestMesh;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Chest|Visuals")
	FChestVisualConfig VisualConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Chest|Collision")
	FChestCollisionConfig CollisionConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Chest|Animation")
	FChestAnimationConfig AnimationConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Chest|Feedback")
	FChestFeedbackConfig FeedbackConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Chest|Respawn")
	FChestRespawnConfig RespawnConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Chest|Spawn")
	FChestSpawnConfig SpawnConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Chest|Loot")
	bool bApplyPlayerLuck = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Chest|Loot")
	bool bApplyPlayerMagicFind = true;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ChestState, Category = "Loot Chest|State")
	EChestState ChestState;

	UPROPERTY(BlueprintReadOnly, Category = "Loot Chest|State")
	FLootResultBatch LastLootBatch;

	UPROPERTY(BlueprintReadOnly, Category = "Loot Chest|State")
	TObjectPtr<AActor> LastInteractor;

	UFUNCTION(BlueprintImplementableEvent, Category = "Loot Chest|Events")
	void OnChestOpened(AActor* Opener);

	UFUNCTION(BlueprintImplementableEvent, Category = "Loot Chest|Events")
	void OnLootGenerated(const FLootResultBatch& LootBatch);

	UFUNCTION(BlueprintImplementableEvent, Category = "Loot Chest|Events")
	void OnChestLooted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Loot Chest|Events")
	void OnChestRespawned();

	/**
	 * Opens the chest authoritatively. Client input should use the normal
	 * InteractionManager path, whose RPC is owned by the interacting player.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot Chest")
	void OpenChest(AActor* Opener);

	UFUNCTION(BlueprintCallable, Category = "Loot Chest")
	void ResetChest();

	UFUNCTION(BlueprintCallable, Category = "Loot Chest")
	void ForceRespawn();

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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_ChestState();

protected:
	void SetupInteraction();
	void SetupVisuals();
	void SetupLootComponent();

	void ConfigureMeshVisibilityAndCollision();

	void ApplyCollisionSettings();

	UFUNCTION()
	void OnInteracted(AActor* Interactor);

	void SetChestState(EChestState NewState);
	void UpdateMeshForState();
	void UpdateInteractionForState();

	void GetPlayerLootStats(AActor* Player, float& OutLuck, float& OutMagicFind) const;
	void GenerateAndSpawnLoot(AActor* Opener);
	void FinalizeOpenSequence();
	void PreloadLootSourceIfPossible();
	void ResetOpenSequenceTracking();

	void StartOpenAnimation();

	void OnOpenAnimationComplete();

	void StartCloseAnimation();

	void OnCloseAnimationComplete();

	float GetAnimationDuration() const;

	void PlaySkeletalAnimation(bool bReverse);

	void StopSkeletalAnimation();

	void SetSkeletalAnimationPosition(float NormalizedPosition);

	void PlayOpenSound();
	void PlayCloseSound();
	void PlayOpenVFX();

	void StartRespawnTimer();
	void HandleRespawn();

	FTimerHandle OpenAnimationTimer;
	FTimerHandle CloseAnimationTimer;
	FTimerHandle RespawnTimer;

	/** Guards against duplicate world spawns while an open animation is still running. */
	bool bLootSpawnedForCurrentOpen = false;

	/** Guards against duplicate post-animation finalization. */
	bool bOpenSequenceFinalizedForCurrentOpen = false;
};

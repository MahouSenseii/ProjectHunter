// Interactable/Actors/Portal/PortalActor.h
// Interactable portal that fast-travels the activating player to a registered destination.
//
// FLOW:
//   1. Place APortalActor in a level.  Set PortalID and DestinationPortalID in the Details panel.
//   2. At BeginPlay the actor registers itself with UPortalSubsystem::RegisterPortal().
//   3. Player approaches and interacts (tap) → ServerActivate RPC fires on authority.
//   4. Authority asks UPortalSubsystem for the destination portal's spawn transform.
//   5. APlayerController::ClientTravel() (same-map) or SeamlessTravel (cross-map) moves the player.
//
// For now only same-map travel is implemented.  Cross-map travel requires a
// session/level-streaming layer that can be added later.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable/Interface/Interactable.h"
#include "PortalActor.generated.h"

class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class UInteractableManager;
class UPortalSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogPortalActor, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// Portal state
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EPortalState : uint8
{
	PS_Inactive     UMETA(DisplayName = "Inactive"),
	PS_Active       UMETA(DisplayName = "Active"),
	PS_Cooldown     UMETA(DisplayName = "Cooldown"),    // Brief lockout after use
	PS_Disabled     UMETA(DisplayName = "Disabled"),    // Cannot be used (e.g., quest-locked)
};

// ─────────────────────────────────────────────────────────────────────────────
// Delegate — broadcast when a player uses this portal
// ─────────────────────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPortalActivated,
	APortalActor*, Portal, APawn*, Traveller);

// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class ALS_PROJECTHUNTER_API APortalActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	APortalActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ── Components ────────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;

	/** Optional decorative mesh (arch, ring, etc.) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PortalMesh;

	/** Niagara visual effect for the portal aperture */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* PortalVFX;

	/** Interaction manager that links this actor to the interaction system */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInteractableManager* InteractableManager;

	// ── Identity ──────────────────────────────────────────────────────────────

	/**
	 * Unique ID for this portal within a level.
	 * Used by UPortalSubsystem to look up the portal by name.
	 * Must be unique per level.  Convention: "Town_A_Gate", "Dungeon_B_Exit".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Identity")
	FName PortalID = NAME_None;

	/**
	 * Display name shown on the interaction widget and map.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Identity")
	FText PortalDisplayName = FText::FromString(TEXT("Portal"));

	// ── Destination (same-map travel) ─────────────────────────────────────────

	/**
	 * PortalID of the destination portal on the SAME map.
	 * Leave empty if this portal triggers cross-map travel (see DestinationLevel).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Destination")
	FName DestinationPortalID = NAME_None;

	/**
	 * Destination level name for cross-map fast travel.
	 * Used when DestinationPortalID refers to a portal in a different level.
	 * Leave NAME_None for same-map travel only.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Destination")
	FName DestinationLevelName = NAME_None;

	/**
	 * Offset applied on top of the destination portal's transform so the
	 * player appears in front of the portal rather than inside it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Destination")
	FVector ArrivalOffset = FVector(150.0f, 0.0f, 0.0f);

	// ── Behaviour ─────────────────────────────────────────────────────────────

	/** Per-use cooldown in seconds — prevents immediate return travel spam. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Behaviour",
		meta = (ClampMin = 0.0f))
	float UseCooldown = 2.0f;

	/** Whether this portal is active on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Behaviour")
	bool bActiveOnBeginPlay = true;

	/** True if this portal can only be used once (e.g., one-way dungeon entrance). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Behaviour")
	bool bSingleUse = false;

	// ── Visuals / Feedback ────────────────────────────────────────────────────

	/** Niagara system to play when portal is idle/active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Visuals")
	UNiagaraSystem* IdleVFXSystem = nullptr;

	/** Niagara burst when a player teleports through. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Visuals")
	UNiagaraSystem* TravelBurstVFX = nullptr;

	/** Sound played when portal is activated by a player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Visuals")
	USoundBase* TravelSound = nullptr;

	// ── State ─────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PortalState,
		Category = "Portal|State")
	EPortalState PortalState = EPortalState::PS_Inactive;

	// ── Events ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Portal|Events")
	FOnPortalActivated OnPortalActivated;

	// ── Public API ────────────────────────────────────────────────────────────

	/** Activate or deactivate this portal at runtime (e.g., from quest logic). */
	UFUNCTION(BlueprintCallable, Category = "Portal")
	void SetPortalActive(bool bActive);

	/** Returns the world transform a player should arrive at when exiting. */
	UFUNCTION(BlueprintPure, Category = "Portal")
	FTransform GetArrivalTransform() const;

	UFUNCTION(BlueprintPure, Category = "Portal")
	EPortalState GetPortalState() const { return PortalState; }

	UFUNCTION(BlueprintPure, Category = "Portal")
	bool IsUsable() const { return PortalState == EPortalState::PS_Active; }

	// ── IInteractable ─────────────────────────────────────────────────────────

	virtual void OnInteract_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual EInteractionType GetInteractionType_Implementation() const override;
	virtual void OnBeginFocus_Implementation(AActor* Interactor) override;
	virtual void OnEndFocus_Implementation(AActor* Interactor) override;
	virtual FInteractableHighlightStyle GetHighlightStyle_Implementation() const override;
	virtual FText GetInteractionText_Implementation() const override;
	virtual FText GetHoldInteractionText_Implementation() const override;
	virtual float GetTapHoldThreshold_Implementation() const override;
	virtual float GetHoldDuration_Implementation() const override;
	virtual void OnHoldInteractionStart_Implementation(AActor* Interactor) override;
	virtual void OnHoldInteractionUpdate_Implementation(AActor* Interactor, float Progress) override;
	virtual void OnHoldInteractionComplete_Implementation(AActor* Interactor) override;
	virtual void OnHoldInteractionCancelled_Implementation(AActor* Interactor) override;
	virtual int32 GetRequiredMashCount_Implementation() const override;
	virtual float GetMashDecayRate_Implementation() const override;
	virtual void OnMashInteractionStart_Implementation(AActor* Interactor) override;
	virtual void OnMashInteractionUpdate_Implementation(AActor* Interactor, int32 CurrentCount, int32 RequiredCount, float Progress) override;
	virtual void OnMashInteractionComplete_Implementation(AActor* Interactor) override;
	virtual void OnMashInteractionFailed_Implementation(AActor* Interactor) override;
	virtual FText GetMashInteractionText_Implementation() const override;
	virtual void OnContinuousInteractionStart_Implementation(AActor* Interactor) override;
	virtual void OnContinuousInteractionUpdate_Implementation(AActor* Interactor, float HeldSeconds) override;
	virtual void OnContinuousInteractionEnd_Implementation(AActor* Interactor) override;
	virtual bool HasTooltip_Implementation() const override;
	virtual UObject* GetTooltipData_Implementation() const override;
	virtual FVector GetTooltipWorldLocation_Implementation() const override;
	virtual UInputAction* GetInputAction_Implementation() const override;
	virtual FVector GetWidgetOffset_Implementation() const override;

protected:
	// ── InteractableManager bridge ────────────────────────────────────────────

	/**
	 * Bound to InteractableManager->OnTapInteracted in BeginPlay.
	 * The player's InteractionManager routes interaction to the UInteractableManager
	 * component first (not the actor), so we bridge the tap event back here.
	 */
	UFUNCTION()
	void OnInteractableManagerTap(AActor* Interactor);

	// ── Server logic ──────────────────────────────────────────────────────────

	UFUNCTION(Server, Reliable)
	void Server_ActivatePortal(APawn* Traveller);

	/** Performs the actual teleport on the authority — called after validation. */
	void ExecuteTravel(APawn* Traveller);

	// ── State helpers ─────────────────────────────────────────────────────────

	void SetStateInternal(EPortalState NewState);
	void StartCooldown();
	void EndCooldown();

	// ── Replication ───────────────────────────────────────────────────────────

	UFUNCTION()
	void OnRep_PortalState();

	// ── Visuals ───────────────────────────────────────────────────────────────

	void UpdateVFXForState();
	void PlayTravelFeedback();

	// ── Data ──────────────────────────────────────────────────────────────────

	FTimerHandle CooldownTimer;

	/** Optional input action reference (assigned in Blueprint defaults). */
	UPROPERTY(EditAnywhere, Category = "Portal|Input")
	TObjectPtr<UInputAction> InteractInputAction;
};

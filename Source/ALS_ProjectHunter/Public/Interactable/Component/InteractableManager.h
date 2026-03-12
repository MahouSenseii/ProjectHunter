// Interactable/Component/InteractableManager.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interactable/Interface/Interactable.h"
#include "Interactable/Library/InteractionEnumLibrary.h"
#include "InteractableManager.generated.h"

class UWidgetComponent;
class UInteractableWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogInteractable, Log, All);
// Delegate for Blueprint events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractionEvent, AActor*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMashProgressEvent, AActor*, Interactor, int32, CurrentCount, int32, RequiredCount);

/**
 * Add this component to ANY actor to make it interactable
 * Handles all IInteractable interface implementation
 * Configure in Blueprint per-actor
 * 
 * SUPPORTS:
 * - Tap interactions (instant)
 * - Hold interactions (with progress bar)
 * - Mash interactions (button mashing)
 * - Tap OR Hold (ground items: tap=inventory, hold=equip)
 * - Camera-facing widgets (always visible from any angle)
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UInteractableManager : public UActorComponent, public IInteractable
{
	GENERATED_BODY()

public:
	UInteractableManager();

	// ═══════════════════════════════════════════════════════════════════════
	// CONFIGURATION (Set in Blueprint)
	// ═══════════════════════════════════════════════════════════════════════
	
	/** Interaction configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FInteractionConfig Config;

	/** Widget offset from actor location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Widget")
	FVector WidgetOffset = FVector(0, 0, 100);

	/** Widget class to spawn (must inherit from UInteractableWidget) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Widget")
	TSubclassOf<UUserWidget> InteractionWidgetClass;

	/** Show widget on focus? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Widget")
	bool bShowWidget = true;

	/** Widget draw size */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Widget")
	FVector2D WidgetDrawSize = FVector2D(300, 80);

	/** Use desired size for better quality? (Recommended: true) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Widget")
	bool bUseDesiredSize = true;

	/** Manual resolution scale (if not using desired size). Higher = sharper but more expensive */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Widget", meta = (EditCondition = "!bUseDesiredSize", ClampMin = "0.5", ClampMax = "4.0"))
	float ResolutionScale = 2.0f;

	// ─────────────────────────────────────────────────────────────────────
	// CAMERA-FACING WIDGET SETTINGS
	// ─────────────────────────────────────────────────────────────────────

	/** Make widget always face the camera? (Prevents disappearing at angles) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Widget")
	bool bAlwaysFaceCamera = true;

	/** How often to update widget rotation (seconds). Lower = smoother but more expensive. 0 = only update at key moments */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Widget", 
		meta = (EditCondition = "bAlwaysFaceCamera", ClampMin = "0.0", ClampMax = "1.0"))
	float CameraFacingUpdateRate = 0.05f;

	/** Smooth rotation speed (degrees/second). 0 = instant snap */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Widget", 
		meta = (EditCondition = "bAlwaysFaceCamera", ClampMin = "0.0", ClampMax = "1000.0"))
	float RotationSmoothSpeed = 0.0f;

	// ─────────────────────────────────────────────────────────────────────
	// HIGHLIGHT SETTINGS
	// ─────────────────────────────────────────────────────────────────────

	/** Meshes to highlight on focus */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight")
	TArray<UPrimitiveComponent*> MeshesToHighlight;

	/** Enable custom depth on focus? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight")
	bool bEnableHighlight = true;

	/** Custom depth stencil value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight")
	int32 HighlightStencilValue = 250;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float HighlightWidth = 3.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight", meta = (ClampMin = "0.0", ClampMax = "50.0"))
	float HighlightThreshold = 50.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Highlight")
	APostProcessVolume* TargetPostProcessVolume = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Highlight")
	UMaterialInterface* OutlineMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Highlight")
	FLinearColor HighlightColor = FLinearColor::Yellow;

	// ═══════════════════════════════════════════════════════════════════════
	// EVENTS (Blueprint implementable)
	// ═══════════════════════════════════════════════════════════════════════
	
	/** Called when tap interacted with */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FInteractionEvent OnTapInteracted;

	/** Called when hold interaction completes */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FInteractionEvent OnHoldCompleted;

	/** Called when hold cancelled */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FInteractionEvent OnHoldCancelled;

	/** Called when mash completes */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FInteractionEvent OnMashCompleted;

	/** Called when mash fails */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FInteractionEvent OnMashFailed;

	/** Called on mash progress update */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FMashProgressEvent OnMashProgress;

	/** Called on begin focus */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FInteractionEvent OnFocusBegin;

	/** Called on end focus */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FInteractionEvent OnFocusEnd;

	// ═══════════════════════════════════════════════════════════════════════
	// INTERACTABLE INTERFACE IMPLEMENTATION
	// ═══════════════════════════════════════════════════════════════════════
	
	virtual void OnInteract_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void OnBeginFocus_Implementation(AActor* Interactor) override;
	virtual void OnEndFocus_Implementation(AActor* Interactor) override;
	virtual EInteractionType GetInteractionType_Implementation() const override;
	virtual UInputAction* GetInputAction_Implementation() const override;
	virtual FText GetInteractionText_Implementation() const override;
	virtual FVector GetWidgetOffset_Implementation() const override;
	virtual FInteractableHighlightStyle GetHighlightStyle_Implementation() const override; 
	
	// Hold interaction interface
	virtual float GetTapHoldThreshold_Implementation() const override;
	virtual float GetHoldDuration_Implementation() const override;
	virtual void OnHoldInteractionStart_Implementation(AActor* Interactor) override;
	virtual void OnHoldInteractionUpdate_Implementation(AActor* Interactor, float Progress) override;
	virtual void OnHoldInteractionComplete_Implementation(AActor* Interactor) override;
	virtual void OnHoldInteractionCancelled_Implementation(AActor* Interactor) override;
	virtual FText GetHoldInteractionText_Implementation() const override;

	// Mash interaction interface
	virtual int32 GetRequiredMashCount_Implementation() const override;
	virtual float GetMashDecayRate_Implementation() const override;
	virtual void OnMashInteractionStart_Implementation(AActor* Interactor) override;
	virtual void OnMashInteractionUpdate_Implementation(AActor* Interactor, int32 CurrentCount, int32 RequiredCount, float Progress) override;
	virtual void OnMashInteractionComplete_Implementation(AActor* Interactor) override;
	virtual void OnMashInteractionFailed_Implementation(AActor* Interactor) override;
	virtual FText GetMashInteractionText_Implementation() const override;

	// Tooltip interface
	virtual bool HasTooltip_Implementation() const override;
	virtual UObject* GetTooltipData_Implementation() const override;
	virtual FVector GetTooltipWorldLocation_Implementation() const override;

	// ═══════════════════════════════════════════════════════════════════════
	// PROGRESS BAR SUPPORT (for hold/mash interactions)
	// ═══════════════════════════════════════════════════════════════════════
	
	/** Update progress bar on widget [0.0 - 1.0] */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void UpdateProgress(float Progress, bool bIsDepleting = false);
	void SetProgressBarVisible(bool bVisible);

	// ═══════════════════════════════════════════════════════════════════════
	// BLUEPRINT CALLABLE
	// ═══════════════════════════════════════════════════════════════════════
	
	/** Enable/disable interaction */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetCanInteract(bool bNewCanInteract) { Config.bCanInteract = bNewCanInteract; }

	/** Update interaction text */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionText(FText NewText) { Config.InteractionText = NewText; }

	/** Change interaction type at runtime */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionType(EInteractionType NewType) { Config.InteractionType = NewType; }

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetHighlightColor(FLinearColor NewColor) { HighlightColor = NewColor; }

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetHighlightWidth(float NewWidth) { HighlightWidth = FMath::Clamp(NewWidth, 0.0f, 10.0f); }

	/** Get current interaction type */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	EInteractionType GetCurrentInteractionType() const { return Config.InteractionType; }

	/** Enable/disable camera facing at runtime */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Widget")
	void SetCameraFacingEnabled(bool bEnabled);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// ═══════════════════════════════════════════════════════════════════════
	// WIDGET MANAGEMENT
	// ═══════════════════════════════════════════════════════════════════════

	/** Widget component (created in BeginPlay) */
	UPROPERTY()
	UWidgetComponent* WidgetComponent = nullptr;

	/** Current interactor (for camera-facing calculations) */
	UPROPERTY()
	AActor* CurrentInteractor = nullptr;

	/** Timer handle for camera-facing updates */
	FTimerHandle CameraFacingTimerHandle;

	/** Create and setup widget component */
	void CreateWidgetComponent();
	
	/** Update widget text based on the interaction type */
	void UpdateWidgetText();

	/** Get Display Text For Current Interaction Type*/ 
	FText GetDisplayTextForCurrentType() const;

	// ═══════════════════════════════════════════════════════════════════════
	// CAMERA-FACING LOGIC (SINGLE RESPONSIBILITY: Widget rotation)
	// ═══════════════════════════════════════════════════════════════════════

	/** 
	 * Update widget rotation to face interactor's camera
	 * @param Interactor - Actor to face towards
	 * @param DeltaTime - Time delta for smooth rotation (0 for instant)
	 */
	void UpdateWidgetRotationToFaceCamera(AActor* Interactor, float DeltaTime = 0.0f);

	/** 
	 * Get camera location and rotation for the given actor
	 * @param Interactor - Actor to get camera from
	 * @param OutCameraLocation - Camera world location
	 * @param OutCameraRotation - Camera world rotation
	 * @return True if camera found
	 */
	bool GetInteractorCamera(AActor* Interactor, FVector& OutCameraLocation, FRotator& OutCameraRotation) const;

	/** Start camera-facing update timer */
	void StartCameraFacingUpdates();

	/** Stop camera-facing update timer */
	void StopCameraFacingUpdates();

	/** Timer callback for continuous camera-facing updates */
	void UpdateCameraFacingTimer();

	// ═══════════════════════════════════════════════════════════════════════
	// MESH MANAGEMENT
	// ═══════════════════════════════════════════════════════════════════════

	/** Auto-find meshes to highlight */
	void AutoFindMeshes();

	/** Apply highlight */
	void ApplyHighlight(bool bHighlight);
	
};
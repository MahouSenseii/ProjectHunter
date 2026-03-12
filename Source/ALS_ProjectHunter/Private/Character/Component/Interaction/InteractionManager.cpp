// Character/Component/Interaction/InteractionManager.cpp

#include "Character/Component/Interaction/InteractionManager.h"

#include "EngineUtils.h"
#include "Interactable/Interface/Interactable.h"
#include "Interactable/Component/InteractableManager.h"
#include "Interactable/Widget/InteractableWidget.h"
#include "Tower/Subsystem/GroundItemSubsystem.h"
#include "Item/ItemInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Interactable/Library/InteractionStructLibrary.h"

DEFINE_LOG_CATEGORY(LogInteractionManager);

// ═══════════════════════════════════════════════════════════════════════
// LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════

UInteractionManager::UInteractionManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(false);
	
	CurrentGroundItemID = -1;
	bSystemInitialized = false;
	bIsHolding = false;
	LastInteractionProgress = -1.0f;
}

void UInteractionManager::BeginPlay()
{
	Super::BeginPlay();

	// ═══════════════════════════════════════════════
	// EARLY VALIDATION
	// ═══════════════════════════════════════════════

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		UE_LOG(LogInteractionManager, Warning, TEXT("InteractionManager: Owner is not a Pawn"));
		return;
	}

	// ═══════════════════════════════════════════════
	// CHECK LOCAL CONTROL (Direct check, not delegated)
	// ═══════════════════════════════════════════════
	
	// Check locally controlled directly here instead of delegating
	// to uninitialized TraceManager. We can't call IsLocallyControlled()
	// because TraceManager.OwnerActor is still nullptr at this point.
	if (!OwnerPawn->IsLocallyControlled())
	{
		UE_LOG(LogInteractionManager, Log, TEXT("InteractionManager: Not locally controlled - skipping initialization"));
		return;
	}

	// ═══════════════════════════════════════════════
	// INITIALIZE SYSTEM
	// ═══════════════════════════════════════════════
	
	InitializeInteractionSystem();
	
	// ═══════════════════════════════════════════════
	// POSSESSION CHECK (Fallback for MP)
	// ═══════════════════════════════════════════════
	
	// This handles edge cases in multiplayer where possession
	// might not be fully established yet
	UE_LOG(LogInteractionManager, Log, TEXT("InteractionManager: Starting possession verification timer..."));
	
	GetWorld()->GetTimerManager().SetTimer(
		PossessionCheckTimer,
		this,
		&UInteractionManager::CheckPossessionAndInitialize,
		0.1f,  // Check every 0.1 seconds
		true   // Loop
	);
}

void UInteractionManager::Initialize()
{
	// Public initialization function (for manual setup if needed)
	if (bInteractionEnabled)
	{
		GetWorld()->GetTimerManager().SetTimer(
			InteractionCheckTimer,
			this,
			&UInteractionManager::CheckForInteractables,
			TraceManager.CheckFrequency,
			true
		);

		UE_LOG(LogInteractionManager, Log, TEXT("InteractionManager: Manually initialized on %s (Frequency: %.2fs)"), 
			*GetOwner()->GetName(), TraceManager.CheckFrequency);
	}
}

void UInteractionManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bInteractionEnabled && bIsHolding)
	{
		UpdateHoldProgress(DeltaTime);
	}
}

void UInteractionManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetActiveInteractionState(true);
	bInteractInputHeld = false;

	// Clear all timers
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(InteractionCheckTimer);
		GetWorld()->GetTimerManager().ClearTimer(HoldProgressTimer);
		GetWorld()->GetTimerManager().ClearTimer(PossessionCheckTimer);
	}

	// End focus on current interactable
	if (CurrentInteractable.GetInterface())
	{
		IInteractable::Execute_OnEndFocus(CurrentInteractable.GetObject(), GetOwner());
	}

	// Remove widget
	if (InteractionWidget)
	{
		InteractionWidget->RemoveFromParent();
		InteractionWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

// ═══════════════════════════════════════════════════════════════════════
// PRIMARY INTERFACE
// ═══════════════════════════════════════════════════════════════════════

void UInteractionManager::OnInteractPressed()
{
	if (!bInteractionEnabled)
	{
		return;
	}

	// Ignore repeated press signals while key/button is still held down.
	if (bInteractInputHeld)
	{
		return;
	}
	bInteractInputHeld = true;

	// If we're already in a hold-like mode, ignore new press events.
	if (bIsHolding && ActiveInteractionMode != EManagedInteractionMode::ActorMash)
	{
		return;
	}

	// 1) Actor-based interactables have priority.
	if (CurrentInteractable.GetInterface())
	{
		const EInteractionType InteractionType = IInteractable::Execute_GetInteractionType(CurrentInteractable.GetObject());
		switch (InteractionType)
		{
			case EInteractionType::IT_Hold:
				BeginActorHoldInteraction(CurrentInteractable, false);
				return;

			case EInteractionType::IT_TapOrHold:
				BeginActorHoldInteraction(CurrentInteractable, true);
				return;

			case EInteractionType::IT_Mash:
				BeginOrAdvanceActorMashInteraction(CurrentInteractable);
				return;

			case EInteractionType::IT_Tap:
			case EInteractionType::IT_Toggle:
			case EInteractionType::IT_Continuous:
			case EInteractionType::IT_None:
			default:
			{
				AActor* TargetActor = nullptr;
				if (UInteractableManager* InteractableComp = Cast<UInteractableManager>(CurrentInteractable.GetObject()))
				{
					TargetActor = InteractableComp->GetOwner();
				}
				else
				{
					TargetActor = Cast<AActor>(CurrentInteractable.GetObject());
				}

				InteractWithActor(TargetActor);
				return;
			}
		}
	}

	// 2) Ground items: tap or hold with threshold buffer.
	if (CurrentGroundItemID != -1)
	{
		BeginGroundTapOrHoldInteraction(CurrentGroundItemID);
		return;
	}
}

void UInteractionManager::OnInteractReleased()
{
	bInteractInputHeld = false;

	if (!bInteractionEnabled)
	{
		return;
	}

	if (!bIsHolding)
	{
		return;
	}

	// Mash mode is press/release based; releasing should not cancel the whole sequence.
	if (ActiveInteractionMode == EManagedInteractionMode::ActorMash)
	{
		return;
	}

	const bool bCompleted = ActiveProgress >= 1.0f;
	const bool bWasHoldPhase = bHoldPhaseStarted;

	// Ground item tap-or-hold.
	if (ActiveInteractionMode == EManagedInteractionMode::GroundTapOrHold)
	{
		const int32 ItemID = ActiveGroundItemID;
		if (!bWasHoldPhase && ItemID != -1)
		{
			PickupGroundItemToInventory(ItemID);
			SetWidgetCompletedState();
		}
		else if (!bCompleted)
		{
			SetWidgetCancelledState();
		}

		ResetActiveInteractionState(true);
		return;
	}

	// Actor hold/tap-or-hold.
	if (!ActiveInteractable.GetInterface())
	{
		ResetActiveInteractionState(true);
		return;
	}

	AActor* TargetActor = nullptr;
	if (UInteractableManager* InteractableComp = Cast<UInteractableManager>(ActiveInteractable.GetObject()))
	{
		TargetActor = InteractableComp->GetOwner();
	}
	else
	{
		TargetActor = Cast<AActor>(ActiveInteractable.GetObject());
	}

	if (ActiveInteractionMode == EManagedInteractionMode::ActorTapOrHold && !bWasHoldPhase)
	{
		InteractWithActor(TargetActor);
		ResetActiveInteractionState(true);
		return;
	}

	if (!bCompleted && ActiveInteractable.GetInterface())
	{
		IInteractable::Execute_OnHoldInteractionCancelled(ActiveInteractable.GetObject(), GetOwner());
		SetWidgetCancelledState();
	}

	ResetActiveInteractionState(true);
}

void UInteractionManager::PickupAllNearbyItems()
{
	// Get camera location
	FVector CameraLocation;
	FRotator CameraRotation;
	if (!TraceManager.GetCameraViewPoint(CameraLocation, CameraRotation))
	{
		return;
	}

	// Pickup all in radius
	int32 PickedUpCount = PickupManager.PickupAllNearby(CameraLocation);

	UE_LOG(LogInteractionManager, Log, TEXT("InteractionManager: Picked up %d items from area"), PickedUpCount);
}

void UInteractionManager::CheckForInteractables()
{
	// Now TraceManager is properly initialized, so we can safely
	// delegate to TraceManager.IsLocallyControlled()
	if (!bInteractionEnabled || !IsLocallyControlled())
	{
		return;
	}

	// Don't update focus while holding (would be confusing)
	if (bIsHolding)
	{
		return;
	}

	// Get camera location for debug visualization
	FVector CameraLocation;
	FRotator CameraRotation;
	TraceManager.GetCameraViewPoint(CameraLocation, CameraRotation);

	// ═══════════════════════════════════════════════
	// DEBUG: Draw interaction range sphere
	// ═══════════════════════════════════════════════
	if (bDebugEnabled)
	{
		DebugManager.DrawInteractionRange(CameraLocation, TraceManager.InteractionDistance);
	}

	// PRIORITY 1: Check for actor-based interactables
	TScriptInterface<IInteractable> NewInteractable = TraceManager.TraceForActorInteractable();

	// PRIORITY 2: Check for ground items (if no actor found)
	int32 NewGroundItemID = -1;
	if (!NewInteractable.GetInterface())
	{
		TraceManager.FindNearestGroundItem(NewGroundItemID);
	}

	// Update actor interactable state
	if (NewInteractable != CurrentInteractable)
	{
		UpdateFocusState(NewInteractable);
	}

	// Update ground item state
	if (NewGroundItemID != CurrentGroundItemID)
	{
		UpdateGroundItemFocus(NewGroundItemID);
	}

	// ═══════════════════════════════════════════════
	// WIDGET UPDATE (Based on current focus)
	// ═══════════════════════════════════════════════
	if (CurrentInteractable.GetInterface())
	{
		UpdateWidgetForActorInteractable(GetCurrentInteractable());
	}
	else if (CurrentGroundItemID != -1)
	{
		UpdateWidgetForGroundItem(CurrentGroundItemID);
	}
	else
	{
		HideWidget();
	}

	// ═══════════════════════════════════════════════
	// DEBUG VISUALIZATION
	// ═══════════════════════════════════════════════
	if (bDebugEnabled)
	{
		UInteractableManager* InteractableComp = GetCurrentInteractable();
		float Distance = 0.0f;
		
		// Calculate distance if we have an interactable
		if (InteractableComp)
		{
			Distance = FVector::Distance(CameraLocation, InteractableComp->GetOwner()->GetActorLocation());
			
			// Draw detailed interactable info
			DebugManager.DrawInteractableInfo(InteractableComp, Distance);
		}
		
		// Draw ground item visualization
		if (CurrentGroundItemID != -1)
		{
			// Get ground item location from subsystem
			if (UGroundItemSubsystem* Subsystem = GetWorld()->GetSubsystem<UGroundItemSubsystem>())
			{
				const TMap<int32, FVector>& Locations = Subsystem->GetInstanceLocations();
				if (const FVector* LocationPtr = Locations.Find(CurrentGroundItemID))
				{
					DebugManager.DrawGroundItem(*LocationPtr, CurrentGroundItemID);
				}
			}
		}

		// Display on-screen debug text
		DebugManager.DisplayInteractionState(InteractableComp, Distance, CurrentGroundItemID);
	}
}

// ═══════════════════════════════════════════════════════════════════════
// WIDGET ACCESS
// ═══════════════════════════════════════════════════════════════════════

void UInteractionManager::SetWidgetVisible(bool bVisible)
{
	if (!InteractionWidget)
	{
		return;
	}

	if (bVisible)
	{
		InteractionWidget->Show();
	}
	else
	{
		InteractionWidget->Hide();
	}
}

// ═══════════════════════════════════════════════════════════════════════
// GETTERS
// ═══════════════════════════════════════════════════════════════════════

UInteractableManager* UInteractionManager::GetCurrentInteractable() const
{
	if (CurrentInteractable.GetInterface())
	{
		return Cast<UInteractableManager>(CurrentInteractable.GetObject());
	}
	return nullptr;
}

float UInteractionManager::GetCurrentHoldProgress() const
{
	if (bIsHolding)
	{
		return ActiveProgress;
	}
	return 0.0f;
}

bool UInteractionManager::InitializeOutlineMID()
{
	if (OutlineMID)
	{
		return true;
	}

	if (!TargetPostProcessVolume)
	{
		for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
		{
			TargetPostProcessVolume = *It;
			if (TargetPostProcessVolume)
			{
				break;
			}
		}
	}

	if (!TargetPostProcessVolume)
	{
		UE_LOG(LogTemp, Warning, TEXT("InteractionManager: No PostProcessVolume found."));
		return false;
	}

	if (!OutlineMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("InteractionManager: OutlineMaterial is not assigned."));
		return false;
	}

	OutlineMID = UMaterialInstanceDynamic::Create(OutlineMaterial, this);
	if (!OutlineMID)
	{
		UE_LOG(LogTemp, Warning, TEXT("InteractionManager: Failed to create OutlineMID."));
		return false;
	}

	FWeightedBlendable Blendable;
	Blendable.Object = OutlineMID;
	Blendable.Weight = 1.0f;
	TargetPostProcessVolume->Settings.WeightedBlendables.Array.Add(Blendable);

	return true;
}

void UInteractionManager::ApplyHighlightStyle(const FInteractableHighlightStyle& Style)
{
	if (!InitializeOutlineMID())
	{
		return;
	}

	OutlineMID->SetVectorParameterValue(TEXT("Color"), Style.Color);
	OutlineMID->SetScalarParameterValue(TEXT("OutlineWidth"), Style.OutlineWidth);
	OutlineMID->SetScalarParameterValue(TEXT("Threshold"), Style.Threshold);
}

void UInteractionManager::ResetHighlightStyle()
{
	if (!InitializeOutlineMID())
	{
		return;
	}

	OutlineMID->SetVectorParameterValue(TEXT("Color"), PlayerHighlightColor);
	OutlineMID->SetScalarParameterValue(TEXT("OutlineWidth"), PlayerHighlightWidth);
	OutlineMID->SetScalarParameterValue(TEXT("Threshold"), PlayerHighlightThreshold);
}

// ═══════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════

void UInteractionManager::InitializeInteractionSystem()
{
	// ═══════════════════════════════════════════════
	// FIX: Guard against double initialization
	// ═══════════════════════════════════════════════
	if (bSystemInitialized)
	{
		UE_LOG(LogInteractionManager, Verbose, TEXT("InteractionManager: Already initialized, skipping"));
		return;
	}

	UE_LOG(LogInteractionManager, Log, TEXT("═══════════════════════════════════════════"));
	UE_LOG(LogInteractionManager, Log, TEXT("  INTERACTION MANAGER - Initializing"));
	UE_LOG(LogInteractionManager, Log, TEXT("═══════════════════════════════════════════"));

	// Initialize all sub-managers (lightweight!)
	InitializeSubManagers();

	// Initialize widget
	InitializeWidget();

	// Apply quick settings (debug mode)
	ApplyQuickSettings();

	// Start interaction check timer
	if (bInteractionEnabled)
	{
		GetWorld()->GetTimerManager().SetTimer(
			InteractionCheckTimer,
			this,
			&UInteractionManager::CheckForInteractables,
			TraceManager.CheckFrequency,
			true
		);

		UE_LOG(LogInteractionManager, Log, TEXT("InteractionManager: ✓ Initialized on %s (Frequency: %.2fs)"), 
			*GetOwner()->GetName(), TraceManager.CheckFrequency);
	}
	
	bSystemInitialized = true;
	
	UE_LOG(LogInteractionManager, Log, TEXT("═══════════════════════════════════════════"));
}

void UInteractionManager::CheckPossessionAndInitialize()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		GetWorld()->GetTimerManager().ClearTimer(PossessionCheckTimer);
		return;
	}

	// Check if possessed now (direct check again, not delegated)
	if (OwnerPawn->IsLocallyControlled())
	{
		UE_LOG(LogInteractionManager, Log, TEXT("InteractionManager: Possession confirmed!"));
		
		// Stop checking
		GetWorld()->GetTimerManager().ClearTimer(PossessionCheckTimer);
		
		// Already initialized in BeginPlay, so we just confirm here
		UE_LOG(LogInteractionManager, Log, TEXT("InteractionManager: System already active"));
	}
}

void UInteractionManager::InitializeSubManagers()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();

	if (!Owner || !World)
	{
		UE_LOG(LogInteractionManager, Error, TEXT("InteractionManager: Invalid owner or world"));
		return;
	}

	// Initialize all managers (lightweight, just passing references!)
	TraceManager.Initialize(Owner, World);
	ValidatorManager.Initialize(Owner, World);
	PickupManager.Initialize(Owner, World);
	DebugManager.Initialize(Owner, World);

	// Connect debug manager to trace manager for trace visualization
	TraceManager.SetDebugManager(&DebugManager);

	UE_LOG(LogInteractionManager, Log, TEXT("InteractionManager: All sub-managers initialized"));
}

void UInteractionManager::InitializeWidget()
{
	// Need a valid widget class
	if (!InteractionWidgetClass)
	{
		UE_LOG(LogInteractionManager, Warning, TEXT("InteractionManager: No InteractionWidgetClass set - widget disabled"));
		return;
	}

	// Get owning player controller
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC)
	{
		UE_LOG(LogInteractionManager, Warning, TEXT("InteractionManager: No PlayerController - widget deferred"));
		return;
	}

	// Create widget
	InteractionWidget = CreateWidget<UInteractableWidget>(PC, InteractionWidgetClass);
	if (!InteractionWidget)
	{
		UE_LOG(LogInteractionManager, Error, TEXT("InteractionManager: Failed to create interaction widget"));
		return;
	}

	// Add to viewport (hidden initially)
	InteractionWidget->AddToViewport(WidgetZOrder);
	InteractionWidget->Hide();

	UE_LOG(LogInteractionManager, Log, TEXT("InteractionManager: Widget initialized (Class: %s)"), 
		*InteractionWidgetClass->GetName());
}

void UInteractionManager::ApplyQuickSettings()
{
	// Apply quick-access debug setting to DebugManager
	if (bDebugEnabled)
	{
		DebugManager.DebugMode = EInteractionDebugMode::Full;
	}
	else
	{
		DebugManager.DebugMode = EInteractionDebugMode::None;
	}
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL LOGIC
// ═══════════════════════════════════════════════════════════════════════

void UInteractionManager::InteractWithActor(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}

	// Get client location for validation
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	FVector ClientLocation = Owner->GetActorLocation();

	// Validate interaction (server-side)
	if (ValidatorManager.HasAuthority())
	{
		if (!ValidatorManager.ValidateActorInteraction(TargetActor, ClientLocation, TraceManager.InteractionDistance))
		{
			UE_LOG(LogInteractionManager, Warning, TEXT("InteractionManager: Actor interaction failed validation"));
			
			if (bDebugEnabled)
			{
				DebugManager.LogInteraction(GetCurrentInteractable(), false, "Validation failed");
			}
			return;
		}
	}

	// Execute interaction
	UInteractableManager* InteractableComp = TargetActor->FindComponentByClass<UInteractableManager>();
	if (InteractableComp)
	{
		IInteractable::Execute_OnInteract(InteractableComp, Owner);
		
		if (bDebugEnabled)
		{
			DebugManager.LogInteraction(InteractableComp, true);
		}
	}
	else if (TargetActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		IInteractable::Execute_OnInteract(TargetActor, Owner);
	}
}

void UInteractionManager::PickupGroundItemToInventory(int32 ItemID)
{
	bool bSuccess = PickupManager.PickupToInventory(ItemID);
	
	if (bDebugEnabled)
	{
		DebugManager.LogGroundItemPickup(ItemID, true, bSuccess);
	}
	
	// Item was picked up, clear current focus
	if (bSuccess && CurrentGroundItemID == ItemID)
	{
		CurrentGroundItemID = -1;
	}
}

void UInteractionManager::PickupGroundItemAndEquip(int32 ItemID)
{
	bool bSuccess = PickupManager.PickupAndEquip(ItemID);
	
	if (bDebugEnabled)
	{
		DebugManager.LogGroundItemPickup(ItemID, false, bSuccess);
	}
	
	// Item was picked up, clear current focus
	if (bSuccess && CurrentGroundItemID == ItemID)
	{
		CurrentGroundItemID = -1;
	}
}

void UInteractionManager::UpdateFocusState(TScriptInterface<IInteractable> NewInteractable)
{
	if (CurrentInteractable.GetObject() == NewInteractable.GetObject())
	{
		return;
	}

	// End focus on old interactable
	if (CurrentInteractable.GetInterface())
	{
		IInteractable::Execute_OnEndFocus(CurrentInteractable.GetObject(), GetOwner());
		ResetHighlightStyle();
	}

	// Update current interactable
	CurrentInteractable = NewInteractable;

	// Start focus on new interactable
	if (CurrentInteractable.GetInterface())
	{
		FInteractableHighlightStyle Style =
			IInteractable::Execute_GetHighlightStyle(CurrentInteractable.GetObject());

		// Player owns final visual presentation
		Style.Color = PlayerHighlightColor;
		Style.OutlineWidth = PlayerHighlightWidth;
		Style.Threshold = PlayerHighlightThreshold;

		if (Style.bEnableHighlight)
		{
			ApplyHighlightStyle(Style);
		}

		IInteractable::Execute_OnBeginFocus(CurrentInteractable.GetObject(), GetOwner());
	}

	OnCurrentInteractableChanged.Broadcast(GetCurrentInteractable());
}

void UInteractionManager::UpdateGroundItemFocus(int32 NewGroundItemID)
{
	int32 OldGroundItemID = CurrentGroundItemID;
	CurrentGroundItemID = NewGroundItemID;
	
	// Broadcast change event
	if (OldGroundItemID != NewGroundItemID)
	{
		OnGroundItemFocusChanged.Broadcast(NewGroundItemID);
	}
}

void UInteractionManager::BeginGroundTapOrHoldInteraction(int32 GroundItemID)
{
	if (GroundItemID == -1)
	{
		return;
	}

	ResetActiveInteractionState(true);

	bIsHolding = true;
	ActiveInteractionMode = EManagedInteractionMode::GroundTapOrHold;
	ActiveGroundItemID = GroundItemID;
	PressElapsedSeconds = 0.0f;
	HoldElapsedSeconds = 0.0f;
	ActiveHoldThresholdSeconds = FMath::Max(0.0f, PickupManager.TapHoldThreshold);
	ActiveHoldDurationSeconds = FMath::Max(0.05f, PickupManager.HoldToEquipDuration);
	ActiveProgress = 0.0f;
	bHoldPhaseStarted = false;
	LastInteractionProgress = -1.0f;

	UE_LOG(LogInteractionManager, Verbose, TEXT("InteractionManager: Tap/Hold candidate started for ground item %d (Threshold: %.2fs, Hold: %.2fs)"),
		GroundItemID, ActiveHoldThresholdSeconds, ActiveHoldDurationSeconds);
}

void UInteractionManager::BeginActorHoldInteraction(const TScriptInterface<IInteractable>& Interactable, bool bAllowTapOnRelease)
{
	if (!Interactable.GetInterface())
	{
		return;
	}

	ResetActiveInteractionState(true);

	bIsHolding = true;
	ActiveInteractionMode = bAllowTapOnRelease
		? EManagedInteractionMode::ActorTapOrHold
		: EManagedInteractionMode::ActorHold;
	ActiveInteractable = Interactable;
	PressElapsedSeconds = 0.0f;
	HoldElapsedSeconds = 0.0f;
	ActiveHoldThresholdSeconds = FMath::Max(0.0f, IInteractable::Execute_GetTapHoldThreshold(Interactable.GetObject()));
	ActiveHoldDurationSeconds = FMath::Max(0.05f, IInteractable::Execute_GetHoldDuration(Interactable.GetObject()));
	ActiveProgress = 0.0f;
	bHoldPhaseStarted = false;
	LastInteractionProgress = -1.0f;

	UE_LOG(LogInteractionManager, Verbose, TEXT("InteractionManager: Actor hold candidate started (AllowTap: %s, Threshold: %.2fs, Hold: %.2fs)"),
		bAllowTapOnRelease ? TEXT("Yes") : TEXT("No"),
		ActiveHoldThresholdSeconds,
		ActiveHoldDurationSeconds);
}

void UInteractionManager::BeginOrAdvanceActorMashInteraction(const TScriptInterface<IInteractable>& Interactable)
{
	if (!Interactable.GetInterface())
	{
		return;
	}

	const bool bIsSameMashInteraction =
		bIsHolding &&
		ActiveInteractionMode == EManagedInteractionMode::ActorMash &&
		ActiveInteractable.GetObject() == Interactable.GetObject();

	if (!bIsSameMashInteraction)
	{
		ResetActiveInteractionState(true);

		bIsHolding = true;
		ActiveInteractionMode = EManagedInteractionMode::ActorMash;
		ActiveInteractable = Interactable;
		ActiveMashRequiredCount = FMath::Max(1, IInteractable::Execute_GetRequiredMashCount(Interactable.GetObject()));
		ActiveMashDecayRate = FMath::Max(0.0f, IInteractable::Execute_GetMashDecayRate(Interactable.GetObject()));
		ActiveMashCountProgress = 0.0f;
		ActiveMashCount = 0;
		ActiveProgress = 0.0f;
		LastInteractionProgress = -1.0f;

		IInteractable::Execute_OnMashInteractionStart(ActiveInteractable.GetObject(), GetOwner());
	}

	ActiveMashCountProgress = FMath::Clamp(ActiveMashCountProgress + 1.0f, 0.0f, static_cast<float>(ActiveMashRequiredCount));
	ActiveMashCount = FMath::Clamp(FMath::FloorToInt(ActiveMashCountProgress + KINDA_SMALL_NUMBER), 0, ActiveMashRequiredCount);
	ActiveProgress = (ActiveMashRequiredCount > 0)
		? FMath::Clamp(ActiveMashCountProgress / static_cast<float>(ActiveMashRequiredCount), 0.0f, 1.0f)
		: 0.0f;

	SetWidgetHoldingState(ActiveProgress);
	IInteractable::Execute_OnMashInteractionUpdate(ActiveInteractable.GetObject(), GetOwner(), ActiveMashCount, ActiveMashRequiredCount, ActiveProgress);
	OnHoldProgressChanged.Broadcast(ActiveProgress);

	if (ActiveProgress >= 1.0f)
	{
		IInteractable::Execute_OnMashInteractionComplete(ActiveInteractable.GetObject(), GetOwner());
		SetWidgetCompletedState();
		ResetActiveInteractionState(true);
	}
}

void UInteractionManager::ResetActiveInteractionState(bool bForceCancelHoldOnPickupManager)
{
	if (bForceCancelHoldOnPickupManager && PickupManager.IsHoldingForGroundItem())
	{
		PickupManager.CancelHoldInteraction();
	}

	bIsHolding = false;
	ActiveInteractionMode = EManagedInteractionMode::None;
	ActiveInteractable = nullptr;
	ActiveGroundItemID = -1;
	bHoldPhaseStarted = false;
	PressElapsedSeconds = 0.0f;
	HoldElapsedSeconds = 0.0f;
	ActiveHoldThresholdSeconds = 0.0f;
	ActiveHoldDurationSeconds = 0.0f;
	ActiveProgress = 0.0f;
	LastInteractionProgress = -1.0f;
	ActiveMashCountProgress = 0.0f;
	ActiveMashRequiredCount = 0;
	ActiveMashCount = 0;
	ActiveMashDecayRate = 0.0f;
}

void UInteractionManager::UpdateHoldProgress(float DeltaTime)
{
	if (!bIsHolding || DeltaTime <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	switch (ActiveInteractionMode)
	{
		case EManagedInteractionMode::GroundTapOrHold:
		case EManagedInteractionMode::ActorHold:
		case EManagedInteractionMode::ActorTapOrHold:
		{
			PressElapsedSeconds += DeltaTime;

			if (!bHoldPhaseStarted)
			{
				if (PressElapsedSeconds < ActiveHoldThresholdSeconds)
				{
					return;
				}

				bHoldPhaseStarted = true;
				HoldElapsedSeconds = FMath::Max(0.0f, PressElapsedSeconds - ActiveHoldThresholdSeconds);
				LastInteractionProgress = -1.0f;

				if (ActiveInteractionMode == EManagedInteractionMode::GroundTapOrHold)
				{
					PickupManager.StartHoldInteraction(ActiveGroundItemID);
				}
				else if (ActiveInteractable.GetInterface())
				{
					IInteractable::Execute_OnHoldInteractionStart(ActiveInteractable.GetObject(), GetOwner());
				}
			}
			else
			{
				HoldElapsedSeconds += DeltaTime;
			}

			const float Duration = FMath::Max(ActiveHoldDurationSeconds, 0.01f);
			const float NewProgress = FMath::Clamp(HoldElapsedSeconds / Duration, 0.0f, 1.0f);
			ActiveProgress = NewProgress;

			if (FMath::Abs(NewProgress - LastInteractionProgress) > 0.005f)
			{
				LastInteractionProgress = NewProgress;
				SetWidgetHoldingState(NewProgress);
				OnHoldProgressChanged.Broadcast(NewProgress);

				if (ActiveInteractionMode != EManagedInteractionMode::GroundTapOrHold && ActiveInteractable.GetInterface())
				{
					IInteractable::Execute_OnHoldInteractionUpdate(ActiveInteractable.GetObject(), GetOwner(), NewProgress);
				}
			}

			if (NewProgress >= 1.0f)
			{
				if (ActiveInteractionMode == EManagedInteractionMode::GroundTapOrHold)
				{
					const int32 ItemID = ActiveGroundItemID;
					PickupGroundItemAndEquip(ItemID);
				}
				else if (ActiveInteractable.GetInterface())
				{
					IInteractable::Execute_OnHoldInteractionComplete(ActiveInteractable.GetObject(), GetOwner());
				}

				SetWidgetCompletedState();
				ResetActiveInteractionState(true);
			}

			break;
		}

		case EManagedInteractionMode::ActorMash:
		{
			if (!ActiveInteractable.GetInterface())
			{
				ResetActiveInteractionState(true);
				return;
			}

			const bool bHadMashProgress = ActiveMashCountProgress > 0.0f;

			if (ActiveMashDecayRate > 0.0f && ActiveMashCountProgress > 0.0f)
			{
				ActiveMashCountProgress = FMath::Max(0.0f, ActiveMashCountProgress - (ActiveMashDecayRate * DeltaTime));
			}

			ActiveMashCount = FMath::Clamp(FMath::FloorToInt(ActiveMashCountProgress + KINDA_SMALL_NUMBER), 0, ActiveMashRequiredCount);
			ActiveProgress = (ActiveMashRequiredCount > 0)
				? FMath::Clamp(ActiveMashCountProgress / static_cast<float>(ActiveMashRequiredCount), 0.0f, 1.0f)
				: 0.0f;

			if (FMath::Abs(ActiveProgress - LastInteractionProgress) > 0.005f)
			{
				LastInteractionProgress = ActiveProgress;
				SetWidgetHoldingState(ActiveProgress);
				IInteractable::Execute_OnMashInteractionUpdate(ActiveInteractable.GetObject(), GetOwner(), ActiveMashCount, ActiveMashRequiredCount, ActiveProgress);
				OnHoldProgressChanged.Broadcast(ActiveProgress);
			}

			if (bHadMashProgress && ActiveMashCountProgress <= KINDA_SMALL_NUMBER)
			{
				IInteractable::Execute_OnMashInteractionFailed(ActiveInteractable.GetObject(), GetOwner());
				SetWidgetCancelledState();
				ResetActiveInteractionState(true);
				return;
			}

			if (ActiveProgress >= 1.0f)
			{
				IInteractable::Execute_OnMashInteractionComplete(ActiveInteractable.GetObject(), GetOwner());
				SetWidgetCompletedState();
				ResetActiveInteractionState(true);
			}

			break;
		}

		case EManagedInteractionMode::None:
		default:
			break;
	}
}

// ═══════════════════════════════════════════════════════════════════════
// WIDGET MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════

void UInteractionManager::UpdateWidgetForActorInteractable(UInteractableManager* Interactable)
{
	if (!InteractionWidget || !Interactable)
	{
		return;
	}

	// Get interaction text from interface so tap/hold/mash prompts are accurate.
	FText Description = IInteractable::Execute_GetInteractionText(Interactable);

	// Update widget
	InteractionWidget->SetInteractionData(Interactable->Config.InputAction, Description);
	InteractionWidget->SetWidgetState(EInteractionWidgetState::IWS_Idle);
	InteractionWidget->Show();
}

void UInteractionManager::UpdateWidgetForGroundItem(int32 GroundItemID)
{
	if (!InteractionWidget || GroundItemID == -1)
	{
		return;
	}

	// Try to get item name
	FText Description = GroundItemDefaultText;
	
	if (UItemInstance* Item = GetGroundItemInstance(GroundItemID))
	{
		FText ItemName = Item->GetDisplayName();
		if (!ItemName.IsEmpty())
		{
			// Format: "Pick Up {ItemName}"
			Description = FText::Format(GroundItemNameFormat, ItemName);
		}
	}

	// Update widget
	InteractionWidget->SetInteractionData(GroundItemActionInput, Description);
	InteractionWidget->SetWidgetState(EInteractionWidgetState::IWS_Idle);
	InteractionWidget->Show();
}

void UInteractionManager::HideWidget()
{
	if (!InteractionWidget)
	{
		return;
	}

	InteractionWidget->Hide();
}

void UInteractionManager::SetWidgetHoldingState(float Progress)
{
	if (!InteractionWidget)
	{
		return;
	}

	const EInteractionWidgetState WidgetState =
		(ActiveInteractionMode == EManagedInteractionMode::ActorMash)
			? EInteractionWidgetState::IWS_Mashing
			: EInteractionWidgetState::IWS_Holding;
	InteractionWidget->SetWidgetState(WidgetState);
	InteractionWidget->SetProgress(Progress);
}

void UInteractionManager::SetWidgetCompletedState()
{
	if (!InteractionWidget)
	{
		return;
	}

	InteractionWidget->SetWidgetState(EInteractionWidgetState::IWS_Completed);
}

void UInteractionManager::SetWidgetCancelledState()
{
	if (!InteractionWidget)
	{
		return;
	}

	InteractionWidget->SetWidgetState(EInteractionWidgetState::IWS_Cancelled);
}

UItemInstance* UInteractionManager::GetGroundItemInstance(int32 GroundItemID) const
{
	if (GroundItemID == -1)
	{
		return nullptr;
	}

	UGroundItemSubsystem* Subsystem = GetWorld()->GetSubsystem<UGroundItemSubsystem>();
	if (!Subsystem)
	{
		return nullptr;
	}

	return Subsystem->GetItemByID(GroundItemID);
}

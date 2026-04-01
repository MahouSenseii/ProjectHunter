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
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "Interactable/Library/InteractionStructLibrary.h"

DEFINE_LOG_CATEGORY(LogInteractionManager);

// ═══════════════════════════════════════════════════════════════════════
// LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════

UInteractionManager::UInteractionManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);  // OPT-RPC: Required for Server RPCs on this component

	CurrentGroundItemID = INDEX_NONE;
	bSystemInitialized = false;
	// ActiveInteraction is zero-initialised by its own default constructor.
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

}

void UInteractionManager::Initialize()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(InteractionCheckTimer);

	if (!bSystemInitialized)
	{
		InitializeInteractionSystem();
		return;
	}

	if (bInteractionEnabled)
	{
		World->GetTimerManager().SetTimer(
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

	if (bInteractionEnabled && HasActiveInteraction())
	{
		UpdateActiveInteraction(DeltaTime);
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
	}

	// End focus on current interactable
	if (UObject* CurrentInteractableTarget = GetCurrentInteractableObject())
	{
		IInteractable::Execute_OnEndFocus(CurrentInteractableTarget, GetOwner());
	}

	CurrentInteractable = nullptr;
	CurrentInteractableObject.Reset();
	CurrentGroundItemID = INDEX_NONE;

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
	if (!bInteractionEnabled || bInteractInputHeld)
	{
		return;
	}

	bInteractInputHeld = true;

	// Resolve focus right before deciding what interaction path to enter.
	const UWorld* World = GetWorld();
	const float CurrentTimeSeconds = World ? World->GetTimeSeconds() : 0.0f;
	const bool bHasRecentTrace = LastInteractionCheckTimeSeconds >= 0.0f
		&& TraceManager.CheckFrequency > KINDA_SMALL_NUMBER
		&& (CurrentTimeSeconds - LastInteractionCheckTimeSeconds) < TraceManager.CheckFrequency;

	if (!bHasRecentTrace)
	{
		CheckForInteractables();
	}

	const TScriptInterface<IInteractable> FocusedInteractable = GetCurrentInteractableInterface();
	if (UObject* FocusedInteractableObject = GetCurrentInteractableObject())
	{
		// ── Issue #5: Gate every interaction path on CanInteract ──────────
		// InteractableManager.Config.bCanInteract can be toggled at runtime
		// (e.g. a door that locks after first use). Without this check the
		// interaction would fire regardless of that flag.
		if (!IInteractable::Execute_CanInteract(FocusedInteractableObject, GetOwner()))
		{
			return;
		}

		const EInteractionType InteractionType = IInteractable::Execute_GetInteractionType(FocusedInteractableObject);

		switch (InteractionType)
		{
			case EInteractionType::IT_Tap:
			case EInteractionType::IT_Toggle:
			{
				InteractWithActor(ResolveInteractionActor(FocusedInteractableObject));
				return;
			}

			case EInteractionType::IT_Hold:
			{
				// ── Issue #6: IT_Hold calls StartHoldPhaseIfNeeded() immediately ──
				// Because TapThresholdSeconds is 0 for pure holds, the hold phase
				// fires synchronously on press — before the first tick runs.
				// IT_TapOrHold intentionally does NOT do this: it lets the tick
				// decide whether the player tapped or held, creating a one-frame
				// difference that is by design.  Keep both paths consistent with
				// their own semantics rather than unifying them.
				BeginActorHoldInteraction(FocusedInteractable, false);
				StartHoldPhaseIfNeeded();
				return;
			}

			case EInteractionType::IT_TapOrHold:
				BeginActorHoldInteraction(FocusedInteractable, true);
				return;

			case EInteractionType::IT_Mash:
				BeginOrAdvanceActorMashInteraction(FocusedInteractable);
				return;

			case EInteractionType::IT_Continuous:
				BeginActorContinuousInteraction(FocusedInteractable);
				return;

			case EInteractionType::IT_None:
			default:
				return;
		}
	}

	if (CurrentGroundItemID != INDEX_NONE)
	{
		BeginGroundTapOrHoldInteraction(CurrentGroundItemID);
	}
}

void UInteractionManager::OnInteractReleased()
{
	bInteractInputHeld = false;

	if (!bInteractionEnabled)
	{
		return;
	}

	if (!HasActiveInteraction())
	{
		return;
	}

	switch (ActiveInteraction.Mode)
	{
		case EManagedInteractionMode::ActorMash:
			// Mash progress is press-based. Releasing should not end the sequence.
			return;

		case EManagedInteractionMode::ActorContinuous:
			EndActorContinuousInteraction();
			return;

		case EManagedInteractionMode::GroundTapOrHold:
		{
			const int32 ItemID = ActiveInteraction.GroundItemID;

			if (ActiveInteraction.ElapsedTime < ActiveInteraction.TapThresholdSeconds)
			{
				const bool bSuccess = PickupGroundItemToInventory(ItemID);
				bSuccess ? SetWidgetCompletedState() : SetWidgetCancelledState();
				ResetActiveInteractionState(true);
				return;
			}

			if (!ActiveInteraction.bHoldCompleted)
			{
				SetWidgetCancelledState();
			}

			ResetActiveInteractionState(true);
			return;
		}

		case EManagedInteractionMode::ActorTapOrHold:
		{
			if (ActiveInteraction.ElapsedTime < ActiveInteraction.TapThresholdSeconds)
			{
				InteractWithActor(ResolveInteractionActor(GetActiveInteractableObject()));
				ResetActiveInteractionState(true);
				return;
			}

			if (!ActiveInteraction.bHoldCompleted)
			{
				CancelActiveHoldInteraction(true);
				return;
			}

			ResetActiveInteractionState(true);
			return;
		}

		case EManagedInteractionMode::ActorHold:
		{
			if (!ActiveInteraction.bHoldCompleted)
			{
				CancelActiveHoldInteraction(true);
				return;
			}

			ResetActiveInteractionState(true);
			return;
		}

		case EManagedInteractionMode::None:
		default:
			return;
	}
}


void UInteractionManager::CheckForInteractables()
{
	// Now TraceManager is properly initialized, so we can safely
	// delegate to TraceManager.IsLocallyControlled()
	if (!bInteractionEnabled || !IsLocallyControlled())
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		LastInteractionCheckTimeSeconds = World->GetTimeSeconds();
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
	int32 NewGroundItemID = INDEX_NONE;
	if (!NewInteractable.GetInterface())
	{
		TraceManager.FindNearestGroundItem(NewGroundItemID);
	}

	// Update actor interactable state
	if (GetCurrentInteractableObject() != NewInteractable.GetObject())
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
	if (ShouldUpdatePromptWidgetFromFocus())
	{
		RefreshFocusedWidget();
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
		if (CurrentGroundItemID != INDEX_NONE)
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
	if (UObject* CurrentInteractableTarget = GetCurrentInteractableObject())
	{
		return Cast<UInteractableManager>(CurrentInteractableTarget);
	}
	return nullptr;
}

float UInteractionManager::GetCurrentHoldProgress() const
{
	if (HasActiveInteraction())
	{
		return ActiveInteraction.Progress;
	}
	return 0.0f;
}

bool UInteractionManager::IsHoldingInteraction() const
{
	return ActiveInteraction.Mode == EManagedInteractionMode::GroundTapOrHold
		|| ActiveInteraction.Mode == EManagedInteractionMode::ActorHold
		|| ActiveInteraction.Mode == EManagedInteractionMode::ActorTapOrHold
		|| ActiveInteraction.Mode == EManagedInteractionMode::ActorContinuous;
}

bool UInteractionManager::InitializeOutlineMID()
{
	if (OutlineMID)
	{
		return true;
	}

	// ── Issue #8: Cache scan failure to avoid a repeated TActorIterator walk ──
	// If the level has no PostProcessVolume, iterating every tick is wasteful.
	// bPostProcessSearchFailed stays false as long as we haven't tried yet or
	// a volume was found previously.
	if (!TargetPostProcessVolume && !bPostProcessSearchFailed)
	{
		for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
		{
			TargetPostProcessVolume = *It;
			if (TargetPostProcessVolume)
			{
				break;
			}
		}

		if (!TargetPostProcessVolume)
		{
			bPostProcessSearchFailed = true;
			UE_LOG(LogInteractionManager, Warning, TEXT("InteractionManager: No PostProcessVolume found in level — outline disabled."));
			return false;
		}
	}

	if (!TargetPostProcessVolume)
	{
		return false;
	}

	if (!OutlineMaterial)
	{
		UE_LOG(LogInteractionManager, Warning, TEXT("InteractionManager: OutlineMaterial is not assigned."));
		return false;
	}

	OutlineMID = UMaterialInstanceDynamic::Create(OutlineMaterial, this);
	if (!OutlineMID)
	{
		UE_LOG(LogInteractionManager, Warning, TEXT("InteractionManager: Failed to create OutlineMID."));
		return false;
	}

	TargetPostProcessVolume->AddOrUpdateBlendable(OutlineMID, 1.0f);

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
		GetWorld()->GetTimerManager().ClearTimer(InteractionCheckTimer);
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

	// ── Issue #9: Dual widget system note ────────────────────────────────
	// InteractableManager (on the interactable actor) owns a world-space
	// UWidgetComponent that shows a prompt attached to the object itself.
	// This widget (screen-space, added to viewport) is the HUD overlay that
	// shows progress bars, hold states, and cancellation feedback.
	//
	// Both call SetWidgetState/SetProgress on the same interaction events.
	// InteractionManager (this class) is authoritative for state transitions;
	// the world-space widget on InteractableManager is a passive visual echo.
	// If they diverge, audit the OnBeginFocus / OnHoldInteractionUpdate paths.
	// ─────────────────────────────────────────────────────────────────────
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

bool UInteractionManager::InteractWithActor(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return false;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	const FVector ClientLocation = Owner->GetActorLocation();

	if (ValidatorManager.HasAuthority()
		&& !ValidatorManager.ValidateActorInteraction(TargetActor, ClientLocation, TraceManager.InteractionDistance))
	{
		UE_LOG(LogInteractionManager, Warning, TEXT("InteractionManager: Actor interaction failed validation"));

		if (bDebugEnabled)
		{
			DebugManager.LogInteraction(GetCurrentInteractable(), false, "Validation failed");
		}

		return false;
	}

	if (UInteractableManager* InteractableComp = TargetActor->FindComponentByClass<UInteractableManager>())
	{
		IInteractable::Execute_OnInteract(InteractableComp, Owner);

		if (bDebugEnabled)
		{
			DebugManager.LogInteraction(InteractableComp, true);
		}

		return true;
	}

	if (TargetActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		IInteractable::Execute_OnInteract(TargetActor, Owner);
		return true;
	}

	return false;
}

bool UInteractionManager::PickupGroundItemToInventory(int32 ItemID)
{
	if (ItemID == INDEX_NONE)
	{
		return false;
	}

	// OPT-RPC: Route through Server RPC when not on authority
	AActor* Owner = GetOwner();
	if (Owner && !Owner->HasAuthority())
	{
		Server_PickupToInventory(ItemID, Owner->GetActorLocation());
		// Optimistically clear focus on the client so the UI feels responsive.
		if (CurrentGroundItemID == ItemID)
		{
			UpdateGroundItemFocus(INDEX_NONE);
		}
		return true; // Assume success — server will validate
	}

	const bool bSuccess = PickupManager.PickupToInventory(ItemID);

	if (bDebugEnabled)
	{
		DebugManager.LogGroundItemPickup(ItemID, true, bSuccess);
	}

	if (bSuccess && CurrentGroundItemID == ItemID)
	{
		UpdateGroundItemFocus(INDEX_NONE);
	}

	return bSuccess;
}

bool UInteractionManager::PickupGroundItemAndEquip(int32 ItemID)
{
	if (ItemID == INDEX_NONE)
	{
		return false;
	}

	// OPT-RPC: Route through Server RPC when not on authority
	AActor* Owner = GetOwner();
	if (Owner && !Owner->HasAuthority())
	{
		Server_PickupAndEquip(ItemID, Owner->GetActorLocation());
		if (CurrentGroundItemID == ItemID)
		{
			UpdateGroundItemFocus(INDEX_NONE);
		}
		return true;
	}

	const bool bSuccess = PickupManager.PickupAndEquip(ItemID);

	if (bDebugEnabled)
	{
		DebugManager.LogGroundItemPickup(ItemID, false, bSuccess);
	}

	if (bSuccess && CurrentGroundItemID == ItemID)
	{
		UpdateGroundItemFocus(INDEX_NONE);
	}

	return bSuccess;
}

void UInteractionManager::UpdateFocusState(TScriptInterface<IInteractable> NewInteractable)
{
	if (GetCurrentInteractableObject() == NewInteractable.GetObject())
	{
		return;
	}

	if (UObject* PreviousInteractableObject = GetCurrentInteractableObject())
	{
		IInteractable::Execute_OnEndFocus(PreviousInteractableObject, GetOwner());
		ResetHighlightStyle();
	}

	CurrentInteractable = NewInteractable;
	CurrentInteractableObject = NewInteractable.GetObject();

	if (UObject* NewInteractableObject = GetCurrentInteractableObject())
	{
		FInteractableHighlightStyle Style = IInteractable::Execute_GetHighlightStyle(NewInteractableObject);
		Style.Color = PlayerHighlightColor;
		Style.OutlineWidth = PlayerHighlightWidth;
		Style.Threshold = PlayerHighlightThreshold;

		if (Style.bEnableHighlight)
		{
			ApplyHighlightStyle(Style);
		}

		IInteractable::Execute_OnBeginFocus(NewInteractableObject, GetOwner());
	}

	OnCurrentInteractableChanged.Broadcast(GetCurrentInteractable());
}

void UInteractionManager::UpdateGroundItemFocus(int32 NewGroundItemID)
{
	const int32 OldGroundItemID = CurrentGroundItemID;
	CurrentGroundItemID = NewGroundItemID;

	if (OldGroundItemID != NewGroundItemID)
	{
		OnGroundItemFocusChanged.Broadcast(NewGroundItemID);
	}
}

void UInteractionManager::BeginGroundTapOrHoldInteraction(int32 GroundItemID)
{
	if (GroundItemID == INDEX_NONE)
	{
		return;
	}

	ResetActiveInteractionState(true);

	ActiveInteraction.Mode                = EManagedInteractionMode::GroundTapOrHold;
	ActiveInteraction.Type                = EInteractionType::IT_TapOrHold;
	ActiveInteraction.State               = EInteractionState::IS_Started;
	ActiveInteraction.Interactor          = GetOwner();
	ActiveInteraction.GroundItemID        = GroundItemID;
	ActiveInteraction.TapThresholdSeconds = FMath::Max(0.0f, PickupManager.TapHoldThreshold);
	ActiveInteraction.HoldDurationSeconds = FMath::Max(0.01f, PickupManager.HoldToEquipDuration);

	UpdateWidgetForGroundItem(GroundItemID);

	UE_LOG(LogInteractionManager, Verbose, TEXT("InteractionManager: Ground tap-or-hold started for item %d (Threshold: %.2fs, Hold: %.2fs)"),
		GroundItemID, ActiveInteraction.TapThresholdSeconds, ActiveInteraction.HoldDurationSeconds);
}

void UInteractionManager::BeginActorHoldInteraction(const TScriptInterface<IInteractable>& Interactable, bool bAllowTapOnRelease)
{
	if (!Interactable.GetInterface())
	{
		return;
	}

	ResetActiveInteractionState(true);

	ActiveInteraction.Mode                = bAllowTapOnRelease
		? EManagedInteractionMode::ActorTapOrHold
		: EManagedInteractionMode::ActorHold;
	ActiveInteraction.Type                = bAllowTapOnRelease
		? EInteractionType::IT_TapOrHold
		: EInteractionType::IT_Hold;
	ActiveInteraction.State               = EInteractionState::IS_Started;
	ActiveInteraction.Interactor          = GetOwner();
	ActiveInteraction.Target              = Interactable;
	ActiveInteraction.TargetObject        = Interactable.GetObject();
	ActiveInteraction.TapThresholdSeconds = bAllowTapOnRelease
		? FMath::Max(0.0f, IInteractable::Execute_GetTapHoldThreshold(Interactable.GetObject()))
		: 0.0f;
	ActiveInteraction.HoldDurationSeconds = FMath::Max(0.01f, IInteractable::Execute_GetHoldDuration(Interactable.GetObject()));

	UpdateWidgetForActorInteractable(Interactable);

	UE_LOG(LogInteractionManager, Verbose, TEXT("InteractionManager: Actor hold started (AllowTap: %s, Threshold: %.2fs, Hold: %.2fs)"),
		bAllowTapOnRelease ? TEXT("Yes") : TEXT("No"),
		ActiveInteraction.TapThresholdSeconds,
		ActiveInteraction.HoldDurationSeconds);
}

void UInteractionManager::BeginActorContinuousInteraction(const TScriptInterface<IInteractable>& Interactable)
{
	if (!Interactable.GetInterface())
	{
		return;
	}

	ResetActiveInteractionState(true);

	ActiveInteraction.Mode         = EManagedInteractionMode::ActorContinuous;
	ActiveInteraction.Type         = EInteractionType::IT_Continuous;
	ActiveInteraction.State        = EInteractionState::IS_Started;
	ActiveInteraction.Interactor   = GetOwner();
	ActiveInteraction.Target       = Interactable;
	ActiveInteraction.TargetObject = Interactable.GetObject();

	UpdateWidgetForActorInteractable(Interactable);
	if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
	{
		IInteractable::Execute_OnContinuousInteractionStart(ActiveInteractableObjectPtr, GetOwner());
	}
}

void UInteractionManager::BeginOrAdvanceActorMashInteraction(const TScriptInterface<IInteractable>& Interactable)
{
	if (!Interactable.GetInterface())
	{
		return;
	}

	const bool bIsSameMashInteraction =
		ActiveInteraction.Mode == EManagedInteractionMode::ActorMash
		&& GetActiveInteractableObject() == Interactable.GetObject();

	// ── Issue #4: Cache GetWorld()->GetTimeSeconds() once per call ────────
	const float CurrentWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	if (!bIsSameMashInteraction)
	{
		ResetActiveInteractionState(true);

		ActiveInteraction.Mode            = EManagedInteractionMode::ActorMash;
		ActiveInteraction.Type            = EInteractionType::IT_Mash;
		ActiveInteraction.State           = EInteractionState::IS_Started;
		ActiveInteraction.Interactor      = GetOwner();
		ActiveInteraction.Target          = Interactable;
		ActiveInteraction.TargetObject    = Interactable.GetObject();
		ActiveInteraction.MashRequiredCount = FMath::Max(1, IInteractable::Execute_GetRequiredMashCount(Interactable.GetObject()));
		ActiveInteraction.MashDecayRate   = FMath::Max(0.0f, IInteractable::Execute_GetMashDecayRate(Interactable.GetObject()));
		ActiveInteraction.LastMashTime    = CurrentWorldTime;

		UpdateWidgetForActorInteractable(Interactable);
		if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
		{
			IInteractable::Execute_OnMashInteractionStart(ActiveInteractableObjectPtr, GetOwner());
		}
	}

	ActiveInteraction.State        = EInteractionState::IS_InProgress;
	ActiveInteraction.LastMashTime = CurrentWorldTime;

	ActiveInteraction.MashProgressUnits = FMath::Clamp(ActiveInteraction.MashProgressUnits + 1.0f, 0.0f, static_cast<float>(ActiveInteraction.MashRequiredCount));
	ActiveInteraction.MashCount         = FMath::Clamp(FMath::FloorToInt(ActiveInteraction.MashProgressUnits + KINDA_SMALL_NUMBER), 0, ActiveInteraction.MashRequiredCount);
	ActiveInteraction.Progress          = ActiveInteraction.MashRequiredCount > 0
		? FMath::Clamp(ActiveInteraction.MashProgressUnits / static_cast<float>(ActiveInteraction.MashRequiredCount), 0.0f, 1.0f)
		: 0.0f;
	ActiveInteraction.LastProgress = ActiveInteraction.Progress;

	SetWidgetHoldingState(ActiveInteraction.Progress);
	if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
	{
		IInteractable::Execute_OnMashInteractionUpdate(ActiveInteractableObjectPtr, GetOwner(), ActiveInteraction.MashCount, ActiveInteraction.MashRequiredCount, ActiveInteraction.Progress);
	}
	OnHoldProgressChanged.Broadcast(ActiveInteraction.Progress);

	if (ActiveInteraction.Progress >= 1.0f)
	{
		if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
		{
			IInteractable::Execute_OnMashInteractionComplete(ActiveInteractableObjectPtr, GetOwner());
		}
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

	ActiveInteraction.Reset();
}

void UInteractionManager::StartHoldPhaseIfNeeded()
{
	if (!UsesHoldLifecycle(ActiveInteraction.Mode) || ActiveInteraction.bHoldStarted || ActiveInteraction.ElapsedTime < ActiveInteraction.TapThresholdSeconds)
	{
		return;
	}

	ActiveInteraction.bHoldStarted = true;
	ActiveInteraction.State        = EInteractionState::IS_InProgress;

	if (ActiveInteraction.Mode != EManagedInteractionMode::GroundTapOrHold)
	{
		if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
		{
			IInteractable::Execute_OnHoldInteractionStart(ActiveInteractableObjectPtr, GetOwner());
		}
	}

	const float HoldPhaseElapsed = FMath::Max(ActiveInteraction.ElapsedTime - ActiveInteraction.TapThresholdSeconds, 0.0f);
	const float HoldProgress = FMath::Clamp(HoldPhaseElapsed / FMath::Max(ActiveInteraction.HoldDurationSeconds, 0.01f), 0.0f, 1.0f);
	PushActiveProgress(HoldProgress, true);
}

void UInteractionManager::PushActiveProgress(float NewProgress, bool bForce)
{
	const float ClampedProgress = FMath::Clamp(NewProgress, 0.0f, 1.0f);
	ActiveInteraction.Progress = ClampedProgress;

	if (!bForce && FMath::Abs(ClampedProgress - ActiveInteraction.LastProgress) <= 0.005f)
	{
		return;
	}

	ActiveInteraction.LastProgress = ClampedProgress;
	SetWidgetHoldingState(ClampedProgress);
	OnHoldProgressChanged.Broadcast(ClampedProgress);

	if (UsesActorTarget(ActiveInteraction.Mode) && UsesHoldLifecycle(ActiveInteraction.Mode))
	{
		if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
		{
			IInteractable::Execute_OnHoldInteractionUpdate(ActiveInteractableObjectPtr, GetOwner(), ClampedProgress);
		}
	}
}

void UInteractionManager::CompleteActiveHoldInteraction()
{
	if (!UsesHoldLifecycle(ActiveInteraction.Mode) || ActiveInteraction.bHoldCompleted)
	{
		return;
	}

	ActiveInteraction.bHoldCompleted = true;
	ActiveInteraction.State          = EInteractionState::IS_Completed;
	PushActiveProgress(1.0f, true);

	if (ActiveInteraction.Mode == EManagedInteractionMode::GroundTapOrHold)
	{
		PickupGroundItemAndEquip(ActiveInteraction.GroundItemID) ? SetWidgetCompletedState() : SetWidgetCancelledState();
		ResetActiveInteractionState(true);
		return;
	}

	if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
	{
		IInteractable::Execute_OnHoldInteractionComplete(ActiveInteractableObjectPtr, GetOwner());
	}

	SetWidgetCompletedState();
	ResetActiveInteractionState(true);
}

void UInteractionManager::CancelActiveHoldInteraction(bool bShowCancelledState)
{
	if (UsesActorTarget(ActiveInteraction.Mode) && !ActiveInteraction.bHoldCompleted)
	{
		if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
		{
			IInteractable::Execute_OnHoldInteractionCancelled(ActiveInteractableObjectPtr, GetOwner());
		}
	}

	if (bShowCancelledState)
	{
		SetWidgetCancelledState();
	}

	ResetActiveInteractionState(true);
}

void UInteractionManager::EndActorContinuousInteraction()
{
	if (ActiveInteraction.Mode != EManagedInteractionMode::ActorContinuous)
	{
		return;
	}

	if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
	{
		IInteractable::Execute_OnContinuousInteractionEnd(ActiveInteractableObjectPtr, GetOwner());
	}

	ResetActiveInteractionState(true);
}

void UInteractionManager::UpdateActiveInteraction(float DeltaTime)
{
	if (!HasActiveInteraction() || DeltaTime <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	switch (ActiveInteraction.Mode)
	{
		case EManagedInteractionMode::GroundTapOrHold:
		case EManagedInteractionMode::ActorHold:
		case EManagedInteractionMode::ActorTapOrHold:
		{
			if (UsesActorTarget(ActiveInteraction.Mode) && !HasValidActiveInteractable())
			{
				ResetActiveInteractionState(true);
				return;
			}

			ActiveInteraction.ElapsedTime += DeltaTime;
			StartHoldPhaseIfNeeded();

			if (!ActiveInteraction.bHoldStarted)
			{
				return;
			}

			const float HoldPhaseElapsed = FMath::Max(ActiveInteraction.ElapsedTime - ActiveInteraction.TapThresholdSeconds, 0.0f);
			const float HoldProgress = FMath::Clamp(HoldPhaseElapsed / FMath::Max(ActiveInteraction.HoldDurationSeconds, 0.01f), 0.0f, 1.0f);
			PushActiveProgress(HoldProgress);

			if (!ActiveInteraction.bHoldCompleted && ActiveInteraction.ElapsedTime >= GetRequiredHoldSeconds())
			{
				CompleteActiveHoldInteraction();
			}

			break;
		}

		case EManagedInteractionMode::ActorMash:
		{
			if (!HasValidActiveInteractable())
			{
				ResetActiveInteractionState(true);
				return;
			}

			const bool bHadMashProgress = ActiveInteraction.MashProgressUnits > 0.0f;

			if (ActiveInteraction.MashDecayRate > 0.0f && ActiveInteraction.MashProgressUnits > 0.0f)
			{
				// ── Issue #4: Cache world time once instead of calling GetWorld() twice ──
				const float CurrentTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
				const float TimeSinceLastPress = ActiveInteraction.LastMashTime >= 0.0f
					? CurrentTimeSeconds - ActiveInteraction.LastMashTime
					: TNumericLimits<float>::Max();
				constexpr float MashDecayGraceSeconds = 0.15f;
				constexpr float MashMaxDecayPerFrame  = 0.25f;

				if (TimeSinceLastPress > MashDecayGraceSeconds)
				{
					const float DecayAmount = FMath::Min(ActiveInteraction.MashDecayRate * DeltaTime, MashMaxDecayPerFrame);
					ActiveInteraction.MashProgressUnits = FMath::Max(0.0f, ActiveInteraction.MashProgressUnits - DecayAmount);
				}
			}

			ActiveInteraction.MashCount = FMath::Clamp(FMath::FloorToInt(ActiveInteraction.MashProgressUnits + KINDA_SMALL_NUMBER), 0, ActiveInteraction.MashRequiredCount);
			ActiveInteraction.Progress  = ActiveInteraction.MashRequiredCount > 0
				? FMath::Clamp(ActiveInteraction.MashProgressUnits / static_cast<float>(ActiveInteraction.MashRequiredCount), 0.0f, 1.0f)
				: 0.0f;

			if (FMath::Abs(ActiveInteraction.Progress - ActiveInteraction.LastProgress) > 0.005f)
			{
				ActiveInteraction.LastProgress = ActiveInteraction.Progress;
				SetWidgetHoldingState(ActiveInteraction.Progress);
				if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
				{
					IInteractable::Execute_OnMashInteractionUpdate(ActiveInteractableObjectPtr, GetOwner(), ActiveInteraction.MashCount, ActiveInteraction.MashRequiredCount, ActiveInteraction.Progress);
				}
				OnHoldProgressChanged.Broadcast(ActiveInteraction.Progress);
			}

			if (bHadMashProgress && ActiveInteraction.MashProgressUnits <= KINDA_SMALL_NUMBER)
			{
				if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
				{
					IInteractable::Execute_OnMashInteractionFailed(ActiveInteractableObjectPtr, GetOwner());
				}
				SetWidgetCancelledState();
				ResetActiveInteractionState(true);
				return;
			}

			if (ActiveInteraction.Progress >= 1.0f)
			{
				if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
				{
					IInteractable::Execute_OnMashInteractionComplete(ActiveInteractableObjectPtr, GetOwner());
				}
				SetWidgetCompletedState();
				ResetActiveInteractionState(true);
			}

			break;
		}

		case EManagedInteractionMode::ActorContinuous:
		{
			if (!HasValidActiveInteractable())
			{
				ResetActiveInteractionState(true);
				return;
			}

			ActiveInteraction.ElapsedTime += DeltaTime;
			if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
			{
				IInteractable::Execute_OnContinuousInteractionUpdate(ActiveInteractableObjectPtr, GetOwner(), ActiveInteraction.ElapsedTime);
			}
			break;
		}

		case EManagedInteractionMode::None:
		default:
			break;
	}
}

void UInteractionManager::RefreshFocusedWidget()
{
	if (HasValidCurrentInteractable())
	{
		UpdateWidgetForActorInteractable(GetCurrentInteractableInterface());
	}
	else if (CurrentGroundItemID != INDEX_NONE)
	{
		UpdateWidgetForGroundItem(CurrentGroundItemID);
	}
	else
	{
		HideWidget();
	}
}

AActor* UInteractionManager::ResolveInteractionActor(const TScriptInterface<IInteractable>& Interactable) const
{
	return ResolveInteractionActor(Interactable.GetObject());
}

AActor* UInteractionManager::ResolveInteractionActor(UObject* InteractableObject) const
{
	if (!IsValid(InteractableObject))
	{
		return nullptr;
	}

	if (const UInteractableManager* InteractableComp = Cast<UInteractableManager>(InteractableObject))
	{
		return InteractableComp->GetOwner();
	}

	return Cast<AActor>(InteractableObject);
}

float UInteractionManager::GetRequiredHoldSeconds() const
{
	return ActiveInteraction.TapThresholdSeconds + FMath::Max(ActiveInteraction.HoldDurationSeconds, 0.01f);
}

bool UInteractionManager::HasActiveInteraction() const
{
	return ActiveInteraction.IsActive();
}

bool UInteractionManager::HasValidCurrentInteractable() const
{
	return CurrentInteractableObject.IsValid()
		&& CurrentInteractable.GetObject() == CurrentInteractableObject.Get()
		&& CurrentInteractable.GetInterface() != nullptr;
}

bool UInteractionManager::HasValidActiveInteractable() const
{
	return ActiveInteraction.TargetObject.IsValid()
		&& ActiveInteraction.Target.GetObject() == ActiveInteraction.TargetObject.Get()
		&& ActiveInteraction.Target.GetInterface() != nullptr;
}

UObject* UInteractionManager::GetCurrentInteractableObject() const
{
	return HasValidCurrentInteractable() ? CurrentInteractableObject.Get() : nullptr;
}

UObject* UInteractionManager::GetActiveInteractableObject() const
{
	return HasValidActiveInteractable() ? ActiveInteraction.TargetObject.Get() : nullptr;
}

bool UInteractionManager::UsesHoldLifecycle(EManagedInteractionMode Mode) const
{
	return Mode == EManagedInteractionMode::GroundTapOrHold
		|| Mode == EManagedInteractionMode::ActorHold
		|| Mode == EManagedInteractionMode::ActorTapOrHold;
}

bool UInteractionManager::UsesTapThreshold(EManagedInteractionMode Mode) const
{
	return Mode == EManagedInteractionMode::GroundTapOrHold
		|| Mode == EManagedInteractionMode::ActorTapOrHold;
}

bool UInteractionManager::UsesActorTarget(EManagedInteractionMode Mode) const
{
	return Mode == EManagedInteractionMode::ActorHold
		|| Mode == EManagedInteractionMode::ActorTapOrHold
		|| Mode == EManagedInteractionMode::ActorMash
		|| Mode == EManagedInteractionMode::ActorContinuous;
}

bool UInteractionManager::ShouldUpdatePromptWidgetFromFocus() const
{
	return !HasActiveInteraction();
}

// ═══════════════════════════════════════════════════════════════════════
// WIDGET MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════

void UInteractionManager::UpdateWidgetForActorInteractable(const TScriptInterface<IInteractable>& Interactable)
{
	if (!InteractionWidget || !Interactable.GetInterface())
	{
		return;
	}

	InteractionWidget->SetInteractionData(
		IInteractable::Execute_GetInputAction(Interactable.GetObject()),
		IInteractable::Execute_GetInteractionText(Interactable.GetObject()));
	InteractionWidget->SetWidgetState(EInteractionWidgetState::IWS_Idle);
	InteractionWidget->Show();
}

void UInteractionManager::UpdateWidgetForGroundItem(int32 GroundItemID)
{
	if (!InteractionWidget || GroundItemID == INDEX_NONE)
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

	if (!InteractionWidget->IsShown())
	{
		InteractionWidget->Show();
	}

	const EInteractionWidgetState WidgetState =
		(ActiveInteraction.Mode == EManagedInteractionMode::ActorMash)
			? EInteractionWidgetState::IWS_Mashing
			: EInteractionWidgetState::IWS_Holding;
	InteractionWidget->SetWidgetState(WidgetState);
	InteractionWidget->SetProgress(FMath::Clamp(Progress, 0.0f, 1.0f));
}

void UInteractionManager::SetWidgetCompletedState()
{
	if (!InteractionWidget)
	{
		return;
	}

	if (!InteractionWidget->IsShown())
	{
		InteractionWidget->Show();
	}
	InteractionWidget->SetWidgetState(EInteractionWidgetState::IWS_Completed);
}

void UInteractionManager::SetWidgetCancelledState()
{
	if (!InteractionWidget)
	{
		return;
	}

	if (!InteractionWidget->IsShown())
	{
		InteractionWidget->Show();
	}
	InteractionWidget->SetWidgetState(EInteractionWidgetState::IWS_Cancelled);
}

UItemInstance* UInteractionManager::GetGroundItemInstance(int32 GroundItemID) const
{
	if (GroundItemID == INDEX_NONE)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGroundItemSubsystem* Subsystem = World->GetSubsystem<UGroundItemSubsystem>();
	if (!Subsystem)
	{
		return nullptr;
	}

	return Subsystem->GetItemByID(GroundItemID);
}

// ═══════════════════════════════════════════════════════════════════════
// SERVER RPCs  (OPT-RPC: multiplayer foundation for ground item pickup)
// ═══════════════════════════════════════════════════════════════════════

void UInteractionManager::Server_PickupToInventory_Implementation(
	int32 ItemID, FVector ClientLocation)
{
	// Basic proximity validation — prevent clients picking up items across the map.
	// The tolerance is generous to allow for latency and prediction error.
	constexpr float MaxPickupDistSq = 800.f * 800.f; // ~8 m tolerance
	if (AActor* Owner = GetOwner())
	{
		const float DistSq = FVector::DistSquared(Owner->GetActorLocation(), ClientLocation);
		if (DistSq > MaxPickupDistSq)
		{
			UE_LOG(LogInteractionManager, Warning,
				TEXT("Server_PickupToInventory: Rejected — client location too far "
				     "(%.0f cm) for item %d"),
				FMath::Sqrt(DistSq), ItemID);
			return;
		}
	}

	// Delegate to the existing authority-path logic.
	PickupManager.PickupToInventory(ItemID);
}

void UInteractionManager::Server_PickupAndEquip_Implementation(
	int32 ItemID, FVector ClientLocation)
{
	constexpr float MaxPickupDistSq = 800.f * 800.f;
	if (AActor* Owner = GetOwner())
	{
		const float DistSq = FVector::DistSquared(Owner->GetActorLocation(), ClientLocation);
		if (DistSq > MaxPickupDistSq)
		{
			UE_LOG(LogInteractionManager, Warning,
				TEXT("Server_PickupAndEquip: Rejected — client location too far "
				     "(%.0f cm) for item %d"),
				FMath::Sqrt(DistSq), ItemID);
			return;
		}
	}

	PickupManager.PickupAndEquip(ItemID);
}

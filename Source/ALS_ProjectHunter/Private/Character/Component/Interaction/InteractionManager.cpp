// Character/Component/Interaction/InteractionManager.cpp

#include "Character/Component/Interaction/InteractionManager.h"

#include "EngineUtils.h"
#include "Character/Controller/HunterController.h"
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
	SetIsReplicatedByDefault(false);
	
	CurrentGroundItemID = INDEX_NONE;
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
	if (!bInteractionEnabled || bInteractInputHeld)
	{
		return;
	}

	bInteractInputHeld = true;

	// Resolve focus right before deciding what interaction path to enter.
	CheckForInteractables();

	if (CurrentInteractable.GetInterface())
	{
		const EInteractionType InteractionType = IInteractable::Execute_GetInteractionType(CurrentInteractable.GetObject());

		switch (InteractionType)
		{
			case EInteractionType::IT_Tap:
			case EInteractionType::IT_Toggle:
			{
				InteractWithActor(ResolveInteractionActor(CurrentInteractable));
				return;
			}

			case EInteractionType::IT_Hold:
				BeginActorHoldInteraction(CurrentInteractable, false);
				StartHoldPhaseIfNeeded();
				return;

			case EInteractionType::IT_TapOrHold:
				BeginActorHoldInteraction(CurrentInteractable, true);
				return;

			case EInteractionType::IT_Mash:
				BeginOrAdvanceActorMashInteraction(CurrentInteractable);
				return;

			case EInteractionType::IT_Continuous:
				BeginActorContinuousInteraction(CurrentInteractable);
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

	SyncHeldInputSecondsFromController();

	switch (ActiveInteractionMode)
	{
		case EManagedInteractionMode::ActorMash:
			// Mash progress is press-based. Releasing should not end the sequence.
			return;

		case EManagedInteractionMode::ActorContinuous:
			EndActorContinuousInteraction();
			return;

		case EManagedInteractionMode::GroundTapOrHold:
		{
			const int32 ItemID = ActiveGroundItemID;

			if (HeldInputSeconds < ActiveTapThresholdSeconds)
			{
				const bool bSuccess = PickupGroundItemToInventory(ItemID);
				bSuccess ? SetWidgetCompletedState() : SetWidgetCancelledState();
				ResetActiveInteractionState(true);
				return;
			}

			if (!bHoldCompleted)
			{
				SetWidgetCancelledState();
			}

			ResetActiveInteractionState(true);
			return;
		}

		case EManagedInteractionMode::ActorTapOrHold:
		{
			if (HeldInputSeconds < ActiveTapThresholdSeconds)
			{
				InteractWithActor(ResolveInteractionActor(ActiveInteractable));
				ResetActiveInteractionState(true);
				return;
			}

			if (!bHoldCompleted)
			{
				CancelActiveHoldInteraction(true);
				return;
			}

			ResetActiveInteractionState(true);
			return;
		}

		case EManagedInteractionMode::ActorHold:
		{
			if (!bHoldCompleted)
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
	if (CurrentInteractable.GetInterface())
	{
		return Cast<UInteractableManager>(CurrentInteractable.GetObject());
	}
	return nullptr;
}

float UInteractionManager::GetCurrentHoldProgress() const
{
	if (HasActiveInteraction())
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
	if (CurrentInteractable.GetObject() == NewInteractable.GetObject())
	{
		return;
	}

	if (CurrentInteractable.GetInterface())
	{
		IInteractable::Execute_OnEndFocus(CurrentInteractable.GetObject(), GetOwner());
		ResetHighlightStyle();
	}

	CurrentInteractable = NewInteractable;

	if (CurrentInteractable.GetInterface())
	{
		FInteractableHighlightStyle Style = IInteractable::Execute_GetHighlightStyle(CurrentInteractable.GetObject());
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

	bIsHolding = true;
	ActiveInteractionMode = EManagedInteractionMode::GroundTapOrHold;
	ActiveGroundItemID = GroundItemID;
	ActiveInteractable = nullptr;
	HeldInputSeconds = 0.0f;
	ActiveTapThresholdSeconds = FMath::Max(0.0f, PickupManager.TapHoldThreshold);
	ActiveHoldDurationSeconds = FMath::Max(0.01f, PickupManager.HoldToEquipDuration);
	ActiveProgress = 0.0f;
	bHoldStarted = false;
	bHoldCompleted = false;
	LastInteractionProgress = -1.0f;

	UpdateWidgetForGroundItem(GroundItemID);

	UE_LOG(LogInteractionManager, Verbose, TEXT("InteractionManager: Ground tap-or-hold started for item %d (Threshold: %.2fs, Hold: %.2fs)"),
		GroundItemID, ActiveTapThresholdSeconds, ActiveHoldDurationSeconds);
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
	ActiveGroundItemID = INDEX_NONE;
	HeldInputSeconds = 0.0f;
	ActiveTapThresholdSeconds = bAllowTapOnRelease
		? FMath::Max(0.0f, IInteractable::Execute_GetTapHoldThreshold(Interactable.GetObject()))
		: 0.0f;
	ActiveHoldDurationSeconds = FMath::Max(0.01f, IInteractable::Execute_GetHoldDuration(Interactable.GetObject()));
	ActiveProgress = 0.0f;
	bHoldStarted = false;
	bHoldCompleted = false;
	LastInteractionProgress = -1.0f;

	UpdateWidgetForActorInteractable(Interactable);

	UE_LOG(LogInteractionManager, Verbose, TEXT("InteractionManager: Actor hold started (AllowTap: %s, Threshold: %.2fs, Hold: %.2fs)"),
		bAllowTapOnRelease ? TEXT("Yes") : TEXT("No"),
		ActiveTapThresholdSeconds,
		ActiveHoldDurationSeconds);
}

void UInteractionManager::BeginActorContinuousInteraction(const TScriptInterface<IInteractable>& Interactable)
{
	if (!Interactable.GetInterface())
	{
		return;
	}

	ResetActiveInteractionState(true);

	bIsHolding = true;
	ActiveInteractionMode = EManagedInteractionMode::ActorContinuous;
	ActiveInteractable = Interactable;
	ActiveGroundItemID = INDEX_NONE;
	HeldInputSeconds = 0.0f;
	ActiveTapThresholdSeconds = 0.0f;
	ActiveHoldDurationSeconds = 0.0f;
	ActiveProgress = 0.0f;
	bHoldStarted = false;
	bHoldCompleted = false;
	LastInteractionProgress = -1.0f;

	UpdateWidgetForActorInteractable(Interactable);
	IInteractable::Execute_OnContinuousInteractionStart(ActiveInteractable.GetObject(), GetOwner());
}

void UInteractionManager::BeginOrAdvanceActorMashInteraction(const TScriptInterface<IInteractable>& Interactable)
{
	if (!Interactable.GetInterface())
	{
		return;
	}

	const bool bIsSameMashInteraction =
		ActiveInteractionMode == EManagedInteractionMode::ActorMash
		&& ActiveInteractable.GetObject() == Interactable.GetObject();

	if (!bIsSameMashInteraction)
	{
		ResetActiveInteractionState(true);

		bIsHolding = false;
		ActiveInteractionMode = EManagedInteractionMode::ActorMash;
		ActiveInteractable = Interactable;
		ActiveGroundItemID = INDEX_NONE;
		HeldInputSeconds = 0.0f;
		ActiveTapThresholdSeconds = 0.0f;
		ActiveHoldDurationSeconds = 0.0f;
		ActiveProgress = 0.0f;
		bHoldStarted = false;
		bHoldCompleted = false;
		LastInteractionProgress = -1.0f;
		MashProgressUnits = 0.0f;
		MashRequiredCount = FMath::Max(1, IInteractable::Execute_GetRequiredMashCount(Interactable.GetObject()));
		MashCurrentCount = 0;
		MashDecayRate = FMath::Max(0.0f, IInteractable::Execute_GetMashDecayRate(Interactable.GetObject()));

		UpdateWidgetForActorInteractable(Interactable);
		IInteractable::Execute_OnMashInteractionStart(ActiveInteractable.GetObject(), GetOwner());
	}

	MashProgressUnits = FMath::Clamp(MashProgressUnits + 1.0f, 0.0f, static_cast<float>(MashRequiredCount));
	MashCurrentCount = FMath::Clamp(FMath::FloorToInt(MashProgressUnits + KINDA_SMALL_NUMBER), 0, MashRequiredCount);
	ActiveProgress = MashRequiredCount > 0
		? FMath::Clamp(MashProgressUnits / static_cast<float>(MashRequiredCount), 0.0f, 1.0f)
		: 0.0f;
	LastInteractionProgress = ActiveProgress;

	SetWidgetHoldingState(ActiveProgress);
	IInteractable::Execute_OnMashInteractionUpdate(ActiveInteractable.GetObject(), GetOwner(), MashCurrentCount, MashRequiredCount, ActiveProgress);
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
	ActiveGroundItemID = INDEX_NONE;
	HeldInputSeconds = 0.0f;
	ActiveTapThresholdSeconds = 0.0f;
	ActiveHoldDurationSeconds = 0.0f;
	ActiveProgress = 0.0f;
	bHoldStarted = false;
	bHoldCompleted = false;
	LastInteractionProgress = -1.0f;
	MashProgressUnits = 0.0f;
	MashRequiredCount = 0;
	MashCurrentCount = 0;
	MashDecayRate = 0.0f;
}

void UInteractionManager::StartHoldPhaseIfNeeded()
{
	if (!UsesHoldLifecycle(ActiveInteractionMode) || bHoldStarted || HeldInputSeconds < ActiveTapThresholdSeconds)
	{
		return;
	}

	bHoldStarted = true;

	if (ActiveInteractionMode != EManagedInteractionMode::GroundTapOrHold && ActiveInteractable.GetInterface())
	{
		IInteractable::Execute_OnHoldInteractionStart(ActiveInteractable.GetObject(), GetOwner());
	}

	const float HoldPhaseElapsed = FMath::Max(HeldInputSeconds - ActiveTapThresholdSeconds, 0.0f);
	const float HoldProgress = FMath::Clamp(HoldPhaseElapsed / FMath::Max(ActiveHoldDurationSeconds, 0.01f), 0.0f, 1.0f);
	PushActiveProgress(HoldProgress, true);
}

void UInteractionManager::PushActiveProgress(float NewProgress, bool bForce)
{
	const float ClampedProgress = FMath::Clamp(NewProgress, 0.0f, 1.0f);
	ActiveProgress = ClampedProgress;

	if (!bForce && FMath::Abs(ClampedProgress - LastInteractionProgress) <= 0.005f)
	{
		return;
	}

	LastInteractionProgress = ClampedProgress;
	SetWidgetHoldingState(ClampedProgress);
	OnHoldProgressChanged.Broadcast(ClampedProgress);

	if (UsesActorTarget(ActiveInteractionMode) && UsesHoldLifecycle(ActiveInteractionMode) && ActiveInteractable.GetInterface())
	{
		IInteractable::Execute_OnHoldInteractionUpdate(ActiveInteractable.GetObject(), GetOwner(), ClampedProgress);
	}
}

void UInteractionManager::CompleteActiveHoldInteraction()
{
	if (!UsesHoldLifecycle(ActiveInteractionMode) || bHoldCompleted)
	{
		return;
	}

	bHoldCompleted = true;
	PushActiveProgress(1.0f, true);

	if (ActiveInteractionMode == EManagedInteractionMode::GroundTapOrHold)
	{
		PickupGroundItemAndEquip(ActiveGroundItemID) ? SetWidgetCompletedState() : SetWidgetCancelledState();
		ResetActiveInteractionState(true);
		return;
	}

	if (ActiveInteractable.GetInterface())
	{
		IInteractable::Execute_OnHoldInteractionComplete(ActiveInteractable.GetObject(), GetOwner());
	}

	SetWidgetCompletedState();
	ResetActiveInteractionState(true);
}

void UInteractionManager::CancelActiveHoldInteraction(bool bShowCancelledState)
{
	if (UsesActorTarget(ActiveInteractionMode) && ActiveInteractable.GetInterface() && !bHoldCompleted)
	{
		IInteractable::Execute_OnHoldInteractionCancelled(ActiveInteractable.GetObject(), GetOwner());
	}

	if (bShowCancelledState)
	{
		SetWidgetCancelledState();
	}

	ResetActiveInteractionState(true);
}

void UInteractionManager::EndActorContinuousInteraction()
{
	if (ActiveInteractionMode != EManagedInteractionMode::ActorContinuous)
	{
		return;
	}

	if (ActiveInteractable.GetInterface())
	{
		IInteractable::Execute_OnContinuousInteractionEnd(ActiveInteractable.GetObject(), GetOwner());
	}

	ResetActiveInteractionState(true);
}

void UInteractionManager::UpdateActiveInteraction(float DeltaTime)
{
	if (!HasActiveInteraction() || DeltaTime <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	switch (ActiveInteractionMode)
	{
		case EManagedInteractionMode::GroundTapOrHold:
		case EManagedInteractionMode::ActorHold:
		case EManagedInteractionMode::ActorTapOrHold:
		{
			if (UsesActorTarget(ActiveInteractionMode) && !ActiveInteractable.GetInterface())
			{
				ResetActiveInteractionState(true);
				return;
			}

			HeldInputSeconds += DeltaTime;
			StartHoldPhaseIfNeeded();

			if (!bHoldStarted)
			{
				return;
			}

			const float HoldPhaseElapsed = FMath::Max(HeldInputSeconds - ActiveTapThresholdSeconds, 0.0f);
			const float HoldProgress = FMath::Clamp(HoldPhaseElapsed / FMath::Max(ActiveHoldDurationSeconds, 0.01f), 0.0f, 1.0f);
			PushActiveProgress(HoldProgress);

			if (!bHoldCompleted && HeldInputSeconds >= GetRequiredHoldSeconds())
			{
				CompleteActiveHoldInteraction();
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

			const bool bHadMashProgress = MashProgressUnits > 0.0f;

			if (MashDecayRate > 0.0f && MashProgressUnits > 0.0f)
			{
				MashProgressUnits = FMath::Max(0.0f, MashProgressUnits - (MashDecayRate * DeltaTime));
			}

			MashCurrentCount = FMath::Clamp(FMath::FloorToInt(MashProgressUnits + KINDA_SMALL_NUMBER), 0, MashRequiredCount);
			ActiveProgress = MashRequiredCount > 0
				? FMath::Clamp(MashProgressUnits / static_cast<float>(MashRequiredCount), 0.0f, 1.0f)
				: 0.0f;

			if (FMath::Abs(ActiveProgress - LastInteractionProgress) > 0.005f)
			{
				LastInteractionProgress = ActiveProgress;
				SetWidgetHoldingState(ActiveProgress);
				IInteractable::Execute_OnMashInteractionUpdate(ActiveInteractable.GetObject(), GetOwner(), MashCurrentCount, MashRequiredCount, ActiveProgress);
				OnHoldProgressChanged.Broadcast(ActiveProgress);
			}

			if (bHadMashProgress && MashProgressUnits <= KINDA_SMALL_NUMBER)
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

		case EManagedInteractionMode::ActorContinuous:
		{
			if (!ActiveInteractable.GetInterface())
			{
				ResetActiveInteractionState(true);
				return;
			}

			HeldInputSeconds += DeltaTime;
			IInteractable::Execute_OnContinuousInteractionUpdate(ActiveInteractable.GetObject(), GetOwner(), HeldInputSeconds);
			break;
		}

		case EManagedInteractionMode::None:
		default:
			break;
	}
}

void UInteractionManager::RefreshFocusedWidget()
{
	if (CurrentInteractable.GetInterface())
	{
		UpdateWidgetForActorInteractable(CurrentInteractable);
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
	if (!Interactable.GetInterface())
	{
		return nullptr;
	}

	if (const UInteractableManager* InteractableComp = Cast<UInteractableManager>(Interactable.GetObject()))
	{
		return InteractableComp->GetOwner();
	}

	return Cast<AActor>(Interactable.GetObject());
}

float UInteractionManager::GetRequiredHoldSeconds() const
{
	return ActiveTapThresholdSeconds + FMath::Max(ActiveHoldDurationSeconds, 0.01f);
}

bool UInteractionManager::HasActiveInteraction() const
{
	return ActiveInteractionMode != EManagedInteractionMode::None;
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

void UInteractionManager::SyncHeldInputSecondsFromController()
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	const AHunterController* HunterController = Cast<AHunterController>(OwnerPawn->GetController());
	if (!HunterController)
	{
		return;
	}

	const UInputAction* InputAction = nullptr;

	if (ActiveInteractable.GetInterface())
	{
		InputAction = IInteractable::Execute_GetInputAction(ActiveInteractable.GetObject());
	}
	else if (ActiveGroundItemID != INDEX_NONE || CurrentGroundItemID != INDEX_NONE)
	{
		InputAction = GroundItemActionInput;
	}

	if (InputAction)
	{
		HeldInputSeconds = FMath::Max(HeldInputSeconds, HunterController->GetElapsedSeconds(InputAction));
	}
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
		(ActiveInteractionMode == EManagedInteractionMode::ActorMash)
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

	UGroundItemSubsystem* Subsystem = GetWorld()->GetSubsystem<UGroundItemSubsystem>();
	if (!Subsystem)
	{
		return nullptr;
	}

	return Subsystem->GetItemByID(GroundItemID);
}

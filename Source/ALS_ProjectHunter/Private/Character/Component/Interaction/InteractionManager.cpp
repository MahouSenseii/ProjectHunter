// Character/Component/Interaction/InteractionManager.cpp

#include "Character/Component/Interaction/InteractionManager.h"

#include "EngineUtils.h"
#include "Interactable/Interface/Interactable.h"
#include "Interactable/Component/InteractableManager.h"
#include "Interactable/Widget/InteractableWidget.h"
#include "Tower/Subsystem/GroundItemSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Interactable/Library/InteractionStructLibrary.h"

DEFINE_LOG_CATEGORY(LogInteractionManager);

namespace InteractionManagerPrivate
{
	constexpr float MaxClientLocationErrorSq = 800.f * 800.f;

	bool ValidateServerGroundItemPickup(const UInteractionManager* Manager, int32 ItemID,
		const FVector& ClientLocation, const TCHAR* Context)
	{
		if (!Manager)
		{
			return false;
		}

		AActor* Owner = Manager->GetOwner();
		UWorld* World = Manager->GetWorld();
		UGroundItemSubsystem* GroundItems = World ? World->GetSubsystem<UGroundItemSubsystem>() : nullptr;
		if (!Owner || !GroundItems)
		{
			UE_LOG(LogInteractionManager, Warning,
				TEXT("%s: Rejected item %d because owner or GroundItemSubsystem was unavailable."),
				Context, ItemID);
			return false;
		}

		const FVector ServerOwnerLocation = Owner->GetActorLocation();
		const float ClientErrorSq = FVector::DistSquared(ServerOwnerLocation, ClientLocation);
		if (ClientErrorSq > MaxClientLocationErrorSq)
		{
			UE_LOG(LogInteractionManager, Warning,
				TEXT("%s: Rejected item %d because client location was %.0f cm from server pawn."),
				Context, ItemID, FMath::Sqrt(ClientErrorSq));
			return false;
		}

		const FVector* ItemLocation = GroundItems->GetInstanceLocations().Find(ItemID);
		if (!ItemLocation)
		{
			UE_LOG(LogInteractionManager, Warning,
				TEXT("%s: Rejected item %d because the ground item location was not registered."),
				Context, ItemID);
			return false;
		}

		const float MaxPickupDistance = FMath::Max(Manager->PickupManager.PickupRadius, 800.f);
		const float ItemDistSq = FVector::DistSquared(ServerOwnerLocation, *ItemLocation);
		if (ItemDistSq > FMath::Square(MaxPickupDistance))
		{
			UE_LOG(LogInteractionManager, Warning,
				TEXT("%s: Rejected item %d because it was %.0f cm from the server pawn."),
				Context, ItemID, FMath::Sqrt(ItemDistSq));
			return false;
		}

		return true;
	}
}

// ═══════════════════════════════════════════════════════════════════════
// LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════

UInteractionManager::UInteractionManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);  // Required for Server RPCs on this component

	CurrentGroundItemID = INDEX_NONE;
	bSystemInitialized  = false;
	// ActiveInteraction is zero-initialised by its own default constructor.
}

void UInteractionManager::BeginPlay()
{
	Super::BeginPlay();

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		UE_LOG(LogInteractionManager, Warning, TEXT("InteractionManager: Owner is not a Pawn"));
		return;
	}
	
	if (!OwnerPawn->IsLocallyControlled())
	{
		UE_LOG(LogInteractionManager, Log,
			TEXT("InteractionManager: Not locally controlled — skipping initialization"));
		return;
	}

	InitializeInteractionSystem();
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
			true);

		UE_LOG(LogInteractionManager, Log,
			TEXT("InteractionManager: Manually initialized on %s (Frequency: %.2fs)"),
			*GetOwner()->GetName(), TraceManager.CheckFrequency);
	}
}

void UInteractionManager::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bInteractionEnabled && HasActiveInteraction())
	{
		UpdateActiveInteraction(DeltaTime);
	}

	// Keep the world-space widget anchored above the focused ground item every frame
	if (CurrentGroundItemID != INDEX_NONE)
	{
		WidgetPresenter.TickGroundItemWorldWidget(CurrentGroundItemID);

		// Also keep the screen-space HUD widget projected to the correct screen position
		if (WidgetPresenter.IsHUDWidgetShown())
		{
			WidgetPresenter.PositionWidgetAtGroundItem(CurrentGroundItemID);
		}
	}
}

void UInteractionManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetActiveInteractionState(true);
	bInteractInputHeld = false;

	// Clear all timers
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InteractionCheckTimer);
	}

	// End focus on current interactable
	if (UObject* CurrentInteractableTarget = GetCurrentInteractableObject())
	{
		IInteractable::Execute_OnEndFocus(CurrentInteractableTarget, GetOwner());
	}

	CurrentInteractable = nullptr;
	CurrentInteractableObject.Reset();
	CurrentGroundItemID = INDEX_NONE;

	// Destroy widget instances owned by the presenter
	WidgetPresenter.Shutdown();

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

	// Resolve focus right before deciding which interaction path to enter.
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
		if (!IInteractable::Execute_CanInteract(FocusedInteractableObject, GetOwner()))
		{
			return;
		}

		const EInteractionType InteractionType = IInteractable::Execute_GetInteractionType(FocusedInteractableObject);

		switch (InteractionType)
		{
			case EInteractionType::IT_Tap:
			case EInteractionType::IT_Toggle:
				InteractWithActor(ResolveInteractionActor(FocusedInteractableObject));
				return;

			case EInteractionType::IT_Hold:
				// IT_Hold calls StartHoldPhaseIfNeeded() immediately because TapThresholdSeconds
				// is 0 for pure holds — hold phase fires synchronously on press, before first tick.
				BeginActorHoldInteraction(FocusedInteractable, false);
				StartHoldPhaseIfNeeded();
				return;

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

		if (!bInteractionEnabled || !HasActiveInteraction())
	{
		return;
	}

	switch (ActiveInteraction.Mode)
	{
		case EManagedInteractionMode::ActorMash:
			// Mash progress is press-based — releasing does not end the sequence.
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
				bSuccess ? WidgetPresenter.SetCompletedState() : WidgetPresenter.SetCancelledState();
				ResetActiveInteractionState(true);
				return;
			}

			if (!ActiveInteraction.bHoldCompleted)
			{
				WidgetPresenter.SetCancelledState();
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
	if (!bInteractionEnabled || !IsLocallyControlled())
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		LastInteractionCheckTimeSeconds = World->GetTimeSeconds();
	}

	// Camera location for debug visualization
	FVector CameraLocation;
	FRotator CameraRotation;
	TraceManager.GetCameraViewPoint(CameraLocation, CameraRotation);

	if (bDebugEnabled)
	{
		DebugManager.DrawInteractionRange(CameraLocation, TraceManager.InteractionDistance);
	}

	// PRIORITY 1: actor-based interactables
	TScriptInterface<IInteractable> NewInteractable = TraceManager.TraceForActorInteractable();

	// PRIORITY 2: ground items (preferred: direct ISM trace; fallback: proximity)
	int32 NewGroundItemID = INDEX_NONE;
	if (!NewInteractable.GetInterface())
	{
		NewGroundItemID = TraceManager.FindGroundItemByTrace();

		if (NewGroundItemID == INDEX_NONE)
		{
			TraceManager.FindNearestGroundItem(NewGroundItemID);
		}
	}

	// Update actor interactable focus
	if (GetCurrentInteractableObject() != NewInteractable.GetObject())
	{
		UpdateFocusState(NewInteractable);
	}

	// Update ground item focus
	if (NewGroundItemID != CurrentGroundItemID)
	{
		UpdateGroundItemFocus(NewGroundItemID);
	}

	// Refresh prompt widget when not in an active interaction
	if (ShouldUpdatePromptWidgetFromFocus())
	{
		RefreshFocusedWidget();
	}

	// Debug visualization
	if (bDebugEnabled)
	{
		UInteractableManager* InteractableComp = GetCurrentInteractable();
		float Distance = 0.0f;

		if (InteractableComp)
		{
			Distance = FVector::Distance(CameraLocation, InteractableComp->GetOwner()->GetActorLocation());
			DebugManager.DrawInteractableInfo(InteractableComp, Distance);
		}

		if (CurrentGroundItemID != INDEX_NONE)
		{
			if (UGroundItemSubsystem* Subsystem = GetWorld()->GetSubsystem<UGroundItemSubsystem>())
			{
				const TMap<int32, FVector>& Locations = Subsystem->GetInstanceLocations();
				if (const FVector* LocationPtr = Locations.Find(CurrentGroundItemID))
				{
					DebugManager.DrawGroundItem(*LocationPtr, CurrentGroundItemID);
				}
			}
		}

		DebugManager.DisplayInteractionState(InteractableComp, Distance, CurrentGroundItemID);
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
	return HasActiveInteraction() ? ActiveInteraction.Progress : 0.0f;
}

bool UInteractionManager::IsHoldingInteraction() const
{
	return ActiveInteraction.Mode == EManagedInteractionMode::GroundTapOrHold
		|| ActiveInteraction.Mode == EManagedInteractionMode::ActorHold
		|| ActiveInteraction.Mode == EManagedInteractionMode::ActorTapOrHold
		|| ActiveInteraction.Mode == EManagedInteractionMode::ActorContinuous;
}

// ═══════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════

void UInteractionManager::InitializeInteractionSystem()
{
	if (bSystemInitialized)
	{
		UE_LOG(LogInteractionManager, Verbose, TEXT("InteractionManager: Already initialized, skipping"));
		return;
	}

	UE_LOG(LogInteractionManager, Log, TEXT("═══════════════════════════════════════════"));
	UE_LOG(LogInteractionManager, Log, TEXT("  INTERACTION MANAGER - Initializing"));
	UE_LOG(LogInteractionManager, Log, TEXT("═══════════════════════════════════════════"));

	InitializeSubManagers();
	InitializeWidget();
	ApplyQuickSettings();

	if (bInteractionEnabled)
	{
		GetWorld()->GetTimerManager().ClearTimer(InteractionCheckTimer);
		GetWorld()->GetTimerManager().SetTimer(
			InteractionCheckTimer,
			this,
			&UInteractionManager::CheckForInteractables,
			TraceManager.CheckFrequency,
			true);

		UE_LOG(LogInteractionManager, Log,
			TEXT("InteractionManager: Initialized on %s (Frequency: %.2fs)"),
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

	TraceManager.Initialize(Owner, World);
	ValidatorManager.Initialize(Owner, World);
	PickupManager.Initialize(Owner, World);
	DebugManager.Initialize(Owner, World);

	TraceManager.SetDebugManager(&DebugManager);

	UE_LOG(LogInteractionManager, Log, TEXT("InteractionManager: All sub-managers initialized"));
}

void UInteractionManager::InitializeWidget()
{
	// Delegate entirely to WidgetPresenter (PH-4.3)
	WidgetPresenter.Initialize(this, GetWorld());
}

void UInteractionManager::ApplyQuickSettings()
{
	DebugManager.DebugMode = bDebugEnabled
		? EInteractionDebugMode::Full
		: EInteractionDebugMode::None;
}

// ═══════════════════════════════════════════════════════════════════════
// FOCUS MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════

void UInteractionManager::UpdateFocusState(TScriptInterface<IInteractable> NewInteractable)
{
	if (GetCurrentInteractableObject() == NewInteractable.GetObject())
	{
		return;
	}

	if (UObject* PreviousInteractableObject = GetCurrentInteractableObject())
	{
		IInteractable::Execute_OnEndFocus(PreviousInteractableObject, GetOwner());
		WidgetPresenter.ResetHighlightStyle();
	}

	CurrentInteractable       = NewInteractable;
	CurrentInteractableObject = NewInteractable.GetObject();

	if (UObject* NewInteractableObject = GetCurrentInteractableObject())
	{
		FInteractableHighlightStyle Style = IInteractable::Execute_GetHighlightStyle(NewInteractableObject);
		Style.Color       = WidgetPresenter.PlayerHighlightColor;
		Style.OutlineWidth = WidgetPresenter.PlayerHighlightWidth;
		Style.Threshold   = WidgetPresenter.PlayerHighlightThreshold;

		if (Style.bEnableHighlight)
		{
			WidgetPresenter.ApplyHighlightStyle(Style);
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

// ═══════════════════════════════════════════════════════════════════════
// INTERACTION ROUTING
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
e		IInteractable::Execute_OnInteract(InteractableComp, Owner);

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

	if (const AActor* Owner = GetOwner(); Owner && !Owner->HasAuthority())
	{
		Server_PickupToInventory(ItemID, Owner->GetActorLocation());
		// Optimistically clear focus so the UI feels responsive
		if (CurrentGroundItemID == ItemID)
		{
			UpdateGroundItemFocus(INDEX_NONE);
		}
		return true; // Assume success — server validates
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

// ═══════════════════════════════════════════════════════════════════════
// ACTIVE INTERACTION STATE MACHINE
// ═══════════════════════════════════════════════════════════════════════

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

	WidgetPresenter.UpdateForGroundItem(GroundItemID);

	UE_LOG(LogInteractionManager, Verbose,
		TEXT("InteractionManager: Ground tap-or-hold started for item %d (Threshold: %.2fs, Hold: %.2fs)"),
		GroundItemID, ActiveInteraction.TapThresholdSeconds, ActiveInteraction.HoldDurationSeconds);
}

void UInteractionManager::BeginActorHoldInteraction(
	const TScriptInterface<IInteractable>& Interactable, bool bAllowTapOnRelease)
{
	if (!Interactable.GetInterface())
	{
		return;
	}

	ResetActiveInteractionState(true);

	ActiveInteraction.Mode = bAllowTapOnRelease
		? EManagedInteractionMode::ActorTapOrHold
		: EManagedInteractionMode::ActorHold;
	ActiveInteraction.Type = bAllowTapOnRelease
		? EInteractionType::IT_TapOrHold
		: EInteractionType::IT_Hold;
	ActiveInteraction.State               = EInteractionState::IS_Started;
	ActiveInteraction.Interactor          = GetOwner();
	ActiveInteraction.Target              = Interactable;
	ActiveInteraction.TargetObject        = Interactable.GetObject();
	ActiveInteraction.TapThresholdSeconds = bAllowTapOnRelease
		? FMath::Max(0.0f, IInteractable::Execute_GetTapHoldThreshold(Interactable.GetObject()))
		: 0.0f;
	ActiveInteraction.HoldDurationSeconds =
		FMath::Max(0.01f, IInteractable::Execute_GetHoldDuration(Interactable.GetObject()));

	WidgetPresenter.UpdateForActorInteractable(Interactable);

	UE_LOG(LogInteractionManager, Verbose,
		TEXT("InteractionManager: Actor hold started (AllowTap: %s, Threshold: %.2fs, Hold: %.2fs)"),
		bAllowTapOnRelease ? TEXT("Yes") : TEXT("No"),
		ActiveInteraction.TapThresholdSeconds,
		ActiveInteraction.HoldDurationSeconds);
}

void UInteractionManager::BeginActorContinuousInteraction(
	const TScriptInterface<IInteractable>& Interactable)
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

	WidgetPresenter.UpdateForActorInteractable(Interactable);

	if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
	{
		IInteractable::Execute_OnContinuousInteractionStart(ActiveInteractableObjectPtr, GetOwner());
	}
}

void UInteractionManager::BeginOrAdvanceActorMashInteraction(
	const TScriptInterface<IInteractable>& Interactable)
{
	if (!Interactable.GetInterface())
	{
		return;
	}

	const bool bIsSameMashInteraction =
		ActiveInteraction.Mode == EManagedInteractionMode::ActorMash
		&& GetActiveInteractableObject() == Interactable.GetObject();

	const float CurrentWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	if (!bIsSameMashInteraction)
	{
		ResetActiveInteractionState(true);

		ActiveInteraction.Mode              = EManagedInteractionMode::ActorMash;
		ActiveInteraction.Type              = EInteractionType::IT_Mash;
		ActiveInteraction.State             = EInteractionState::IS_Started;
		ActiveInteraction.Interactor        = GetOwner();
		ActiveInteraction.Target            = Interactable;
		ActiveInteraction.TargetObject      = Interactable.GetObject();
		ActiveInteraction.MashRequiredCount =
			FMath::Max(1, IInteractable::Execute_GetRequiredMashCount(Interactable.GetObject()));
		ActiveInteraction.MashDecayRate     =
			FMath::Max(0.0f, IInteractable::Execute_GetMashDecayRate(Interactable.GetObject()));
		ActiveInteraction.LastMashTime      = CurrentWorldTime;

		WidgetPresenter.UpdateForActorInteractable(Interactable);

		if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
		{
			IInteractable::Execute_OnMashInteractionStart(ActiveInteractableObjectPtr, GetOwner());
		}
	}

	ActiveInteraction.State        = EInteractionState::IS_InProgress;
	ActiveInteraction.LastMashTime = CurrentWorldTime;

	ActiveInteraction.MashProgressUnits = FMath::Clamp(
		ActiveInteraction.MashProgressUnits + 1.0f, 0.0f,
		static_cast<float>(ActiveInteraction.MashRequiredCount));
	ActiveInteraction.MashCount = FMath::Clamp(
		FMath::FloorToInt(ActiveInteraction.MashProgressUnits + KINDA_SMALL_NUMBER),
		0, ActiveInteraction.MashRequiredCount);
	ActiveInteraction.Progress = ActiveInteraction.MashRequiredCount > 0
		? FMath::Clamp(
			ActiveInteraction.MashProgressUnits / static_cast<float>(ActiveInteraction.MashRequiredCount),
			0.0f, 1.0f)
		: 0.0f;
	ActiveInteraction.LastProgress = ActiveInteraction.Progress;

	WidgetPresenter.SetHoldingState(ActiveInteraction.Progress, ActiveInteraction.Mode);

	if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
	{
		IInteractable::Execute_OnMashInteractionUpdate(ActiveInteractableObjectPtr, GetOwner(),
			ActiveInteraction.MashCount, ActiveInteraction.MashRequiredCount, ActiveInteraction.Progress);
	}
	OnHoldProgressChanged.Broadcast(ActiveInteraction.Progress);

	if (ActiveInteraction.Progress >= 1.0f)
	{
		if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
		{
			IInteractable::Execute_OnMashInteractionComplete(ActiveInteractableObjectPtr, GetOwner());
		}
		WidgetPresenter.SetCompletedState();
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
	if (!UsesHoldLifecycle(ActiveInteraction.Mode)
		|| ActiveInteraction.bHoldStarted
		|| ActiveInteraction.ElapsedTime < ActiveInteraction.TapThresholdSeconds)
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

	const float HoldPhaseElapsed = FMath::Max(
		ActiveInteraction.ElapsedTime - ActiveInteraction.TapThresholdSeconds, 0.0f);
	const float HoldProgress = FMath::Clamp(
		HoldPhaseElapsed / FMath::Max(ActiveInteraction.HoldDurationSeconds, 0.01f), 0.0f, 1.0f);
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
	WidgetPresenter.SetHoldingState(ClampedProgress, ActiveInteraction.Mode);
	OnHoldProgressChanged.Broadcast(ClampedProgress);

	if (UsesActorTarget(ActiveInteraction.Mode) && UsesHoldLifecycle(ActiveInteraction.Mode))
	{
		if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
		{
			IInteractable::Execute_OnHoldInteractionUpdate(
				ActiveInteractableObjectPtr, GetOwner(), ClampedProgress);
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
		PickupGroundItemAndEquip(ActiveInteraction.GroundItemID)
			? WidgetPresenter.SetCompletedState()
			: WidgetPresenter.SetCancelledState();
		ResetActiveInteractionState(true);
		return;
	}

	if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
	{
		IInteractable::Execute_OnHoldInteractionComplete(ActiveInteractableObjectPtr, GetOwner());
	}

	WidgetPresenter.SetCompletedState();
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
		WidgetPresenter.SetCancelledState();
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

			const float HoldPhaseElapsed = FMath::Max(
				ActiveInteraction.ElapsedTime - ActiveInteraction.TapThresholdSeconds, 0.0f);
			const float HoldProgress = FMath::Clamp(
				HoldPhaseElapsed / FMath::Max(ActiveInteraction.HoldDurationSeconds, 0.01f), 0.0f, 1.0f);
			PushActiveProgress(HoldProgress);

			if (!ActiveInteraction.bHoldCompleted
				&& ActiveInteraction.ElapsedTime >= GetRequiredHoldSeconds())
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
				const float CurrentTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
				const float TimeSinceLastPress = ActiveInteraction.LastMashTime >= 0.0f
					? CurrentTimeSeconds - ActiveInteraction.LastMashTime
					: TNumericLimits<float>::Max();

				constexpr float MashDecayGraceSeconds = 0.15f;
				constexpr float MashMaxDecayPerFrame  = 0.25f;

				if (TimeSinceLastPress > MashDecayGraceSeconds)
				{
					const float DecayAmount = FMath::Min(
						ActiveInteraction.MashDecayRate * DeltaTime, MashMaxDecayPerFrame);
					ActiveInteraction.MashProgressUnits =
						FMath::Max(0.0f, ActiveInteraction.MashProgressUnits - DecayAmount);
				}
			}

			ActiveInteraction.MashCount = FMath::Clamp(
				FMath::FloorToInt(ActiveInteraction.MashProgressUnits + KINDA_SMALL_NUMBER),
				0, ActiveInteraction.MashRequiredCount);
			ActiveInteraction.Progress = ActiveInteraction.MashRequiredCount > 0
				? FMath::Clamp(
					ActiveInteraction.MashProgressUnits / static_cast<float>(ActiveInteraction.MashRequiredCount),
					0.0f, 1.0f)
				: 0.0f;

			if (FMath::Abs(ActiveInteraction.Progress - ActiveInteraction.LastProgress) > 0.005f)
			{
				ActiveInteraction.LastProgress = ActiveInteraction.Progress;
				WidgetPresenter.SetHoldingState(ActiveInteraction.Progress, ActiveInteraction.Mode);

				if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
				{
					IInteractable::Execute_OnMashInteractionUpdate(ActiveInteractableObjectPtr, GetOwner(),
						ActiveInteraction.MashCount, ActiveInteraction.MashRequiredCount,
						ActiveInteraction.Progress);
				}
				OnHoldProgressChanged.Broadcast(ActiveInteraction.Progress);
			}

			if (bHadMashProgress && ActiveInteraction.MashProgressUnits <= KINDA_SMALL_NUMBER)
			{
				if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
				{
					IInteractable::Execute_OnMashInteractionFailed(ActiveInteractableObjectPtr, GetOwner());
				}
				WidgetPresenter.SetCancelledState();
				ResetActiveInteractionState(true);
				return;
			}

			if (ActiveInteraction.Progress >= 1.0f)
			{
				if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
				{
					IInteractable::Execute_OnMashInteractionComplete(ActiveInteractableObjectPtr, GetOwner());
				}
				WidgetPresenter.SetCompletedState();
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
				IInteractable::Execute_OnContinuousInteractionUpdate(
					ActiveInteractableObjectPtr, GetOwner(), ActiveInteraction.ElapsedTime);
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
	// Always reset the ground item world widget first — it will be re-shown by
	// UpdateForGroundItem if a ground item is still focused.
	WidgetPresenter.HideGroundItemWorldWidget();

	if (HasValidCurrentInteractable())
	{
		WidgetPresenter.UpdateForActorInteractable(GetCurrentInteractableInterface());
	}
	else if (CurrentGroundItemID != INDEX_NONE)
	{
		WidgetPresenter.UpdateForGroundItem(CurrentGroundItemID);
	}
	else
	{
		WidgetPresenter.HideAll();
	}
}

// ═══════════════════════════════════════════════════════════════════════
// STATE MACHINE HELPERS
// ═══════════════════════════════════════════════════════════════════════

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
	return ActiveInteraction.TapThresholdSeconds
		+ FMath::Max(ActiveInteraction.HoldDurationSeconds, 0.01f);
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
// SERVER RPCs
// ═══════════════════════════════════════════════════════════════════════

void UInteractionManager::Server_PickupToInventory_Implementation(
	int32 ItemID, FVector ClientLocation)
{
	if (!InteractionManagerPrivate::ValidateServerGroundItemPickup(
		this, ItemID, ClientLocation, TEXT("Server_PickupToInventory")))
	{
		return;
	}

	// Basic proximity validation — prevents clients picking up items across the map.
	constexpr float MaxPickupDistSq = 800.f * 800.f; // ~8 m tolerance
	if (AActor* Owner = GetOwner())
	{
		const float DistSq = FVector::DistSquared(Owner->GetActorLocation(), ClientLocation);
		if (DistSq > MaxPickupDistSq)
		{
			UE_LOG(LogInteractionManager, Warning,
				TEXT("Server_PickupToInventory: Rejected — client location too far (%.0f cm) for item %d"),
				FMath::Sqrt(DistSq), ItemID);
			return;
		}
	}

	PickupManager.PickupToInventory(ItemID);
}

void UInteractionManager::Server_PickupAndEquip_Implementation(
	int32 ItemID, FVector ClientLocation)
{
	if (!InteractionManagerPrivate::ValidateServerGroundItemPickup(
		this, ItemID, ClientLocation, TEXT("Server_PickupAndEquip")))
	{
		return;
	}

	constexpr float MaxPickupDistSq = 800.f * 800.f;
	if (AActor* Owner = GetOwner())
	{
		const float DistSq = FVector::DistSquared(Owner->GetActorLocation(), ClientLocation);
		if (DistSq > MaxPickupDistSq)
		{
			UE_LOG(LogInteractionManager, Warning,
				TEXT("Server_PickupAndEquip: Rejected — client location too far (%.0f cm) for item %d"),
				FMath::Sqrt(DistSq), ItemID);
			return;
		}
	}

	PickupManager.PickupAndEquip(ItemID);
}

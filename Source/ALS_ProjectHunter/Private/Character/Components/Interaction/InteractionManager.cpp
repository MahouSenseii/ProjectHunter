#include "Character/Components/Interaction/InteractionManager.h"

#include "EngineUtils.h"
#include "Interactable/Interface/Interactable.h"
#include "Interactable/Components/InteractableManager.h"
#include "UI/Interaction/InteractableWidget.h"
#include "Tower/Subsystems/GroundItemSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Interactable/Library/Structs/InteractionStructs.h"

DEFINE_LOG_CATEGORY(LogInteractionManager);

namespace InteractionManagerPrivate
{
	constexpr float MaxClientLocationErrorSq = 800.f * 800.f;

	bool HasSameCandidateOrder(
		const TArray<FGroundItemInteractionCandidate>& A,
		const TArray<FGroundItemInteractionCandidate>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (A[Index].ItemID != B[Index].ItemID)
			{
				return false;
			}
		}

		return true;
	}

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

UInteractionManager::UInteractionManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	CurrentGroundItemID = INDEX_NONE;
	bSystemInitialized  = false;
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
		// The server needs sub-managers initialized to handle RPCs
		// (ValidatorManager distance checks, PickupManager inventory calls).
		// Skip the timer and widget - those are client-only.
		InitializeSubManagers();
		ApplyQuickSettings();
		UE_LOG(LogInteractionManager, Log,
			TEXT("InteractionManager: Server/remote init on %s (sub-managers only)"),
			*GetOwner()->GetName());
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
		CurrentInteractionCheckInterval = -1.0f;
		UpdateInteractionCheckRate(false);

		UE_LOG(LogInteractionManager, Log,
			TEXT("InteractionManager: Manually initialized on %s (Idle: %.2fs | Active: %.2fs)"),
			*GetOwner()->GetName(), TraceManager.IdleCheckFrequency, TraceManager.CheckFrequency);
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

	// The camera moves every frame while the gathered candidates do not, so
	// re-scoring here (no spatial queries) is what makes focus follow the aim at
	// frame rate instead of stepping at CheckFrequency.
	if (bRescoreFocusEveryFrame && bSystemInitialized && !HasActiveInteraction())
	{
		EvaluateInteractionFocus(/*bRefreshCandidates=*/false);
	}

	if (CurrentGroundItemID != INDEX_NONE)
	{
		WidgetPresenter.TickGroundItemWorldWidget(CurrentGroundItemID);

		if (WidgetPresenter.IsHUDWidgetShown())
		{
			WidgetPresenter.PositionWidgetAtGroundItem(CurrentGroundItemID);
		}
	}
}

void UInteractionManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetActiveInteractionState();
	bInteractInputHeld = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InteractionCheckTimer);
	}
	CurrentInteractionCheckInterval = -1.0f;

	if (UObject* CurrentInteractableTarget = GetCurrentInteractableObject())
	{
		IInteractable::Execute_OnEndFocus(CurrentInteractableTarget, GetOwner());
	}

	CurrentInteractable = nullptr;
	CurrentInteractableObject.Reset();
	CurrentGroundItemID = INDEX_NONE;
	GroundItemCandidates.Reset();
	SelectedGroundItemCandidateIndex = INDEX_NONE;
	AutomaticGroundItemID = INDEX_NONE;
	ClearManualGroundItemSelection();
	TraceManager.InvalidateCandidateCache();

	WidgetPresenter.Shutdown();

	Super::EndPlay(EndPlayReason);
}

void UInteractionManager::OnInteractPressed()
{
	if (!bInteractionEnabled || bInteractInputHeld)
	{
		return;
	}

	bInteractInputHeld = true;

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
				ResetActiveInteractionState();
				return;
			}

			if (!ActiveInteraction.bHoldCompleted)
			{
				WidgetPresenter.SetCancelledState();
			}

			ResetActiveInteractionState();
			return;
		}

		case EManagedInteractionMode::ActorTapOrHold:
		{
			if (ActiveInteraction.ElapsedTime < ActiveInteraction.TapThresholdSeconds)
			{
				InteractWithActor(ResolveInteractionActor(GetActiveInteractableObject()));
				ResetActiveInteractionState();
				return;
			}

			if (!ActiveInteraction.bHoldCompleted)
			{
				CancelActiveHoldInteraction(true);
				return;
			}

			ResetActiveInteractionState();
			return;
		}

		case EManagedInteractionMode::ActorHold:
		{
			if (!ActiveInteraction.bHoldCompleted)
			{
				CancelActiveHoldInteraction(true);
				return;
			}

			ResetActiveInteractionState();
			return;
		}

		case EManagedInteractionMode::None:
		default:
			return;
	}
}

void UInteractionManager::CheckForInteractables()
{
	EvaluateInteractionFocus(/*bRefreshCandidates=*/true);
}

void UInteractionManager::EvaluateInteractionFocus(bool bRefreshCandidates)
{
	if (!bInteractionEnabled || !IsLocallyControlled())
	{
		return;
	}

	if (bRefreshCandidates)
	{
		if (const UWorld* World = GetWorld())
		{
			LastInteractionCheckTimeSeconds = World->GetTimeSeconds();
		}
	}

	// One player-centered proximity query gathers both actor interactables and
	// saved ground-item locations, then scores them with the same math.
	TScriptInterface<IInteractable> NewInteractable;
	int32 NewGroundItemID = INDEX_NONE;
	TArray<FGroundItemInteractionCandidate> NewGroundItemCandidates;
	bool bHasProximityCandidates = false;
	TraceManager.FindBestInteractionTarget(
		CurrentInteractable,
		CurrentGroundItemID,
		NewInteractable,
		NewGroundItemID,
		NewGroundItemCandidates,
		bHasProximityCandidates,
		bRefreshCandidates);

	// The fast evaluation loop only runs while something overlaps the logical
	// player sphere (or an interaction is still active). Empty-space discovery
	// uses the slower idle rate because ISM ground items do not emit overlaps.
	if (bRefreshCandidates)
	{
		UpdateInteractionCheckRate(bHasProximityCandidates || HasActiveInteraction());
	}

	const UObject* PreviousFocusObject = GetCurrentInteractableObject();
	const int32 PreviousGroundItemID = CurrentGroundItemID;

	// Don't shift focus while an interaction is in progress - the outline and
	// ground-item widget must stay on the active target until it completes.
	if (!HasActiveInteraction())
	{
		// A manual pick outranks the actor as well as aim scoring: without this
		// an actor interactable would take focus straight back on the next pass
		// and there would be no way to cycle to an item lying next to a chest.
		const bool bManualSelectionHeld =
			IsManualGroundItemSelectionLocked()
			&& NewGroundItemCandidates.ContainsByPredicate(
				[this](const FGroundItemInteractionCandidate& Candidate)
				{
					return Candidate.ItemID == ManualGroundItemSelectionID;
				});

		const TScriptInterface<IInteractable> DesiredInteractable =
			bManualSelectionHeld ? TScriptInterface<IInteractable>() : NewInteractable;

		if (GetCurrentInteractableObject() != DesiredInteractable.GetObject())
		{
			UpdateFocusState(DesiredInteractable);
		}

		// Candidates are kept even when an actor wins, so cycling can still reach
		// the items around it; the actor just keeps focus until the player cycles.
		ApplyGroundItemCandidates(
			NewGroundItemCandidates,
			NewGroundItemID,
			/*bAllowAutomaticSelection=*/!DesiredInteractable.GetObject());
	}

	// UpdateForActorInteractable has no early-out and reaches into Blueprint for
	// its text, so the per-frame path only pays for it when focus actually moved.
	const bool bFocusChanged = PreviousFocusObject != GetCurrentInteractableObject()
		|| PreviousGroundItemID != CurrentGroundItemID;

	if ((bRefreshCandidates || bFocusChanged) && ShouldUpdatePromptWidgetFromFocus())
	{
		RefreshFocusedWidget();
	}

	if (bRefreshCandidates && DebugManager.ShouldShowDebugTraces())
	{
		// The actual trace origin (ALS camera position), so the visualisation
		// matches where the trace really fires from.
		const FVector TraceOrigin = TraceManager.GetTraceStart();
		UInteractableManager* InteractableComp = GetCurrentInteractable();
		float Distance = 0.0f;

		if (InteractableComp)
		{
			Distance = FVector::Distance(TraceOrigin, InteractableComp->GetOwner()->GetActorLocation());
			DebugManager.DrawInteractableInfo(InteractableComp, Distance, TraceOrigin);
		}

		const bool bManualSelectionLocked = IsManualGroundItemSelectionLocked();
		const float ManualLockRemaining = GetManualGroundItemSelectionLockRemaining();
		const FGroundItemInteractionCandidate* SelectedCandidate =
			GroundItemCandidates.FindByPredicate(
				[this](const FGroundItemInteractionCandidate& Candidate)
				{
					return Candidate.ItemID == CurrentGroundItemID;
				});
		DebugManager.DrawGroundItemCandidateStack(
			TraceOrigin,
			GroundItemCandidates,
			CurrentGroundItemID,
			AutomaticGroundItemID,
			bManualSelectionLocked,
			ManualLockRemaining);

		DebugManager.DisplayInteractionState(
			InteractableComp,
			Distance,
			CurrentGroundItemID,
			GetGroundItemSelectionNumber(),
			GroundItemCandidates.Num(),
			AutomaticGroundItemID,
			SelectedCandidate,
			bHasProximityCandidates,
			bHasProximityCandidates
				? TraceManager.CheckFrequency
				: TraceManager.IdleCheckFrequency,
			bManualSelectionLocked,
			ManualLockRemaining);
	}
}

void UInteractionManager::CycleGroundItemFocus(int32 Direction)
{
	if (!bInteractionEnabled || HasActiveInteraction() || Direction == 0)
	{
		return;
	}

	if (GroundItemCandidates.IsEmpty())
	{
		CheckForInteractables();
	}

	if (GroundItemCandidates.IsEmpty())
	{
		return;
	}

	const int32 Step = Direction > 0 ? 1 : -1;
	const int32 StartingIndex = GroundItemCandidates.IsValidIndex(SelectedGroundItemCandidateIndex)
		? SelectedGroundItemCandidateIndex
		: (Step > 0 ? -1 : 0);
	const int32 NewIndex = (StartingIndex + Step + GroundItemCandidates.Num())
		% GroundItemCandidates.Num();

	SelectGroundItemCandidateIndex(NewIndex, true);
}

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

void UInteractionManager::InitializeInteractionSystem()
{
	if (bSystemInitialized)
	{
		UE_LOG(LogInteractionManager, Verbose, TEXT("InteractionManager: Already initialized, skipping"));
		return;
	}

	UE_LOG(LogInteractionManager, Log, TEXT("  INTERACTION MANAGER - Initializing"));

	InitializeSubManagers();
	InitializeWidget();
	ApplyQuickSettings();

	if (bInteractionEnabled)
	{
		CurrentInteractionCheckInterval = -1.0f;
		UpdateInteractionCheckRate(false);

		UE_LOG(LogInteractionManager, Log,
			TEXT("InteractionManager: Initialized on %s (Idle: %.2fs | Active: %.2fs)"),
			*GetOwner()->GetName(), TraceManager.IdleCheckFrequency, TraceManager.CheckFrequency);
	}

	bSystemInitialized = true;
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
	WidgetPresenter.Initialize(this, GetWorld());
}

void UInteractionManager::ApplyQuickSettings()
{
	DebugManager.DebugMode = bDebugEnabled
		? EInteractionDebugMode::Full
		: EInteractionDebugMode::None;

	// Keep debug shapes alive until the next timer tick so neither the quick
	// setting nor ALS's Show Traces toggle produces a flickering snapshot.
	DebugManager.DrawDuration = TraceManager.CheckFrequency;
}

void UInteractionManager::UpdateInteractionCheckRate(bool bHasProximityCandidates)
{
	UWorld* World = GetWorld();
	if (!World || !bInteractionEnabled)
	{
		return;
	}

	const float ActiveInterval = FMath::Max(TraceManager.CheckFrequency, 0.01f);
	const float IdleInterval = FMath::Max(TraceManager.IdleCheckFrequency, ActiveInterval);
	const float DesiredInterval = bHasProximityCandidates ? ActiveInterval : IdleInterval;
	DebugManager.DrawDuration = DesiredInterval;

	if (World->GetTimerManager().IsTimerActive(InteractionCheckTimer)
		&& FMath::IsNearlyEqual(CurrentInteractionCheckInterval, DesiredInterval))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		InteractionCheckTimer,
		this,
		&UInteractionManager::CheckForInteractables,
		DesiredInterval,
		true);
	CurrentInteractionCheckInterval = DesiredInterval;
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

void UInteractionManager::ApplyGroundItemCandidates(
	const TArray<FGroundItemInteractionCandidate>& NewCandidates,
	int32 NewAutomaticGroundItemID,
	bool bAllowAutomaticSelection)
{
	const bool bCandidateOrderChanged =
		!InteractionManagerPrivate::HasSameCandidateOrder(GroundItemCandidates, NewCandidates);
	const int32 OldSelectionIndex = SelectedGroundItemCandidateIndex;
	const int32 OldGroundItemID = CurrentGroundItemID;

	GroundItemCandidates = NewCandidates;
	AutomaticGroundItemID = FindGroundItemCandidateIndex(NewAutomaticGroundItemID) != INDEX_NONE
		? NewAutomaticGroundItemID
		: INDEX_NONE;

	// A manual pick is released only when its item leaves the list or the player
	// signals they are done with it. Aim scoring must never take it back on its
	// own: overlapping items score identically, so any automatic override would
	// snap focus to whichever one sorts first and make cycling useless.
	if (ManualGroundItemSelectionID != INDEX_NONE
		&& (FindGroundItemCandidateIndex(ManualGroundItemSelectionID) == INDEX_NONE
			|| !IsManualGroundItemSelectionLocked()))
	{
		ClearManualGroundItemSelection();
	}

	int32 DesiredGroundItemID = INDEX_NONE;
	if (ManualGroundItemSelectionID != INDEX_NONE)
	{
		DesiredGroundItemID = ManualGroundItemSelectionID;
	}
	else if (bAllowAutomaticSelection)
	{
		DesiredGroundItemID = AutomaticGroundItemID != INDEX_NONE
			? AutomaticGroundItemID
			: (GroundItemCandidates.IsEmpty() ? INDEX_NONE : GroundItemCandidates[0].ItemID);
	}

	if (GroundItemCandidates.IsEmpty())
	{
		DesiredGroundItemID = INDEX_NONE;
		AutomaticGroundItemID = INDEX_NONE;
		ClearManualGroundItemSelection();
	}

	SelectedGroundItemCandidateIndex = FindGroundItemCandidateIndex(DesiredGroundItemID);
	UpdateGroundItemFocus(DesiredGroundItemID);

	if (bCandidateOrderChanged
		|| OldSelectionIndex != SelectedGroundItemCandidateIndex
		|| OldGroundItemID != CurrentGroundItemID)
	{
		BroadcastGroundItemSelectionChanged();
	}
}

void UInteractionManager::BeginManualGroundItemSelection(int32 ItemID)
{
	ManualGroundItemSelectionID = ItemID;

	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		ManualSelectionPlayerLocation = OwnerPawn->GetActorLocation();
		ManualSelectionAimDirection = OwnerPawn->GetControlRotation().Vector();
	}

	const float LockDuration = FMath::Max(0.0f, ManualGroundItemSelectionLockDuration);
	const UWorld* World = GetWorld();
	ManualGroundItemSelectionLockEndTime = (LockDuration > 0.0f && World)
		? World->GetTimeSeconds() + LockDuration
		: -1.0f;
}

void UInteractionManager::ClearManualGroundItemSelection()
{
	ManualGroundItemSelectionID = INDEX_NONE;
	ManualSelectionAimDirection = FVector::ZeroVector;
	ManualSelectionPlayerLocation = FVector::ZeroVector;
	ManualGroundItemSelectionLockEndTime = -1.0f;
}

bool UInteractionManager::HasManualGroundItemSelectionDrifted() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return true;
	}

	if (ManualSelectionBreakDistance > 0.0f
		&& FVector::DistSquared(OwnerPawn->GetActorLocation(), ManualSelectionPlayerLocation)
			> FMath::Square(ManualSelectionBreakDistance))
	{
		return true;
	}

	if (ManualSelectionBreakAngle > 0.0f && !ManualSelectionAimDirection.IsNearlyZero())
	{
		// Control rotation rather than TraceManager's view point: this is const,
		// and it is the direction the player is steering rather than wherever
		// camera lag has left the camera this frame.
		const float MinDot = FMath::Cos(FMath::DegreesToRadians(
			FMath::Clamp(ManualSelectionBreakAngle, 0.0f, 180.0f)));

		if (FVector::DotProduct(OwnerPawn->GetControlRotation().Vector(),
				ManualSelectionAimDirection) < MinDot)
		{
			return true;
		}
	}

	return false;
}

void UInteractionManager::SelectGroundItemCandidateIndex(int32 NewIndex, bool bManualSelection)
{
	if (!GroundItemCandidates.IsValidIndex(NewIndex))
	{
		return;
	}

	if (bManualSelection)
	{
		BeginManualGroundItemSelection(GroundItemCandidates[NewIndex].ItemID);
	}

	if (HasValidCurrentInteractable())
	{
		UpdateFocusState(TScriptInterface<IInteractable>());
	}

	SelectedGroundItemCandidateIndex = NewIndex;
	UpdateGroundItemFocus(GroundItemCandidates[NewIndex].ItemID);
	BroadcastGroundItemSelectionChanged();

	if (ShouldUpdatePromptWidgetFromFocus())
	{
		RefreshFocusedWidget();
	}
}

void UInteractionManager::AdvanceGroundItemFocusAfterPickup(int32 PickedUpItemID)
{
	if (ManualGroundItemSelectionID == PickedUpItemID)
	{
		ClearManualGroundItemSelection();
	}

	const int32 RemovedIndex = FindGroundItemCandidateIndex(PickedUpItemID);
	if (RemovedIndex == INDEX_NONE)
	{
		if (CurrentGroundItemID == PickedUpItemID)
		{
			SelectedGroundItemCandidateIndex = INDEX_NONE;
			UpdateGroundItemFocus(INDEX_NONE);
			BroadcastGroundItemSelectionChanged();
		}
		return;
	}

	GroundItemCandidates.RemoveAt(RemovedIndex, 1, EAllowShrinking::No);

	if (GroundItemCandidates.IsEmpty())
	{
		SelectedGroundItemCandidateIndex = INDEX_NONE;
		AutomaticGroundItemID = INDEX_NONE;
		ClearManualGroundItemSelection();
		UpdateGroundItemFocus(INDEX_NONE);
		BroadcastGroundItemSelectionChanged();
		return;
	}

	SelectedGroundItemCandidateIndex = FMath::Min(RemovedIndex, GroundItemCandidates.Num() - 1);
	if (FindGroundItemCandidateIndex(AutomaticGroundItemID) == INDEX_NONE)
	{
		AutomaticGroundItemID = GroundItemCandidates[0].ItemID;
	}

	// Hold the item that slid into the emptied slot, so clearing a pile is
	// repeated taps in place rather than one pickup then a jump somewhere else.
	BeginManualGroundItemSelection(GroundItemCandidates[SelectedGroundItemCandidateIndex].ItemID);
	UpdateGroundItemFocus(GroundItemCandidates[SelectedGroundItemCandidateIndex].ItemID);
	BroadcastGroundItemSelectionChanged();
}

void UInteractionManager::BroadcastGroundItemSelectionChanged()
{
	OnGroundItemSelectionChanged.Broadcast(
		CurrentGroundItemID,
		GetGroundItemSelectionNumber(),
		GroundItemCandidates.Num());
}

int32 UInteractionManager::FindGroundItemCandidateIndex(int32 ItemID) const
{
	if (ItemID == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	return GroundItemCandidates.IndexOfByPredicate(
		[ItemID](const FGroundItemInteractionCandidate& Candidate)
		{
			return Candidate.ItemID == ItemID;
		});
}

bool UInteractionManager::IsManualGroundItemSelectionLocked() const
{
	if (ManualGroundItemSelectionID == INDEX_NONE)
	{
		return false;
	}

	// A configured duration is an optional extra timeout on top of the drift
	// check; at the default of 0 no end time is set and only drift releases it.
	if (ManualGroundItemSelectionLockEndTime >= 0.0f)
	{
		const UWorld* World = GetWorld();
		if (!World || World->GetTimeSeconds() >= ManualGroundItemSelectionLockEndTime)
		{
			return false;
		}
	}

	return !HasManualGroundItemSelectionDrifted();
}

float UInteractionManager::GetManualGroundItemSelectionLockRemaining() const
{
	const UWorld* World = GetWorld();
	if (!World || ManualGroundItemSelectionLockEndTime < 0.0f)
	{
		return 0.0f;
	}

	return FMath::Max(ManualGroundItemSelectionLockEndTime - World->GetTimeSeconds(), 0.0f);
}

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

	// Remote client: route the authoritative interaction through OUR Server RPC
	// (player-owned component - target-actor Server RPCs from clients are
	// dropped by the engine). The local execution below still runs for
	// immediate presentation; target actors gate gameplay on HasAuthority.
	if (!Owner->HasAuthority())
	{
		Server_InteractWithActor(TargetActor, ClientLocation);
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

	if (const AActor* Owner = GetOwner(); Owner && !Owner->HasAuthority())
	{
		Server_PickupToInventory(ItemID, Owner->GetActorLocation());
		AdvanceGroundItemFocusAfterPickup(ItemID);
		return true;
	}

	const bool bSuccess = PickupManager.PickupToInventory(ItemID);

	if (bDebugEnabled)
	{
		DebugManager.LogGroundItemPickup(ItemID, true, bSuccess);
	}

	if (bSuccess)
	{
		AdvanceGroundItemFocusAfterPickup(ItemID);
	}

	return bSuccess;
}

bool UInteractionManager::PickupGroundItemAndEquip(int32 ItemID)
{
	if (ItemID == INDEX_NONE)
	{
		return false;
	}

	AActor* Owner = GetOwner();
	if (Owner && !Owner->HasAuthority())
	{
		Server_PickupAndEquip(ItemID, Owner->GetActorLocation());
		AdvanceGroundItemFocusAfterPickup(ItemID);
		return true;
	}

	const bool bSuccess = PickupManager.PickupAndEquip(ItemID);

	if (bDebugEnabled)
	{
		DebugManager.LogGroundItemPickup(ItemID, false, bSuccess);
	}

	if (bSuccess)
	{
		AdvanceGroundItemFocusAfterPickup(ItemID);
	}

	return bSuccess;
}

void UInteractionManager::BeginGroundTapOrHoldInteraction(int32 GroundItemID)
{
	if (GroundItemID == INDEX_NONE)
	{
		return;
	}

	ResetActiveInteractionState();

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

	UObject* InteractableObject = Interactable.GetObject();
	if (!InteractableObject)
	{
		return;
	}

	const bool bIsSameHoldInteraction =
		UsesActorTarget(ActiveInteraction.Mode)
		&& UsesHoldLifecycle(ActiveInteraction.Mode)
		&& GetActiveInteractableObject() == InteractableObject;
	if (bIsSameHoldInteraction)
	{
		return;
	}

	ResetActiveInteractionState();

	ActiveInteraction.Mode = bAllowTapOnRelease
		? EManagedInteractionMode::ActorTapOrHold
		: EManagedInteractionMode::ActorHold;
	ActiveInteraction.Type = bAllowTapOnRelease
		? EInteractionType::IT_TapOrHold
		: EInteractionType::IT_Hold;
	ActiveInteraction.State               = EInteractionState::IS_Started;
	ActiveInteraction.Interactor          = GetOwner();
	ActiveInteraction.Target              = Interactable;
	ActiveInteraction.TargetObject        = InteractableObject;
	ActiveInteraction.TapThresholdSeconds = bAllowTapOnRelease
		? FMath::Max(0.0f, IInteractable::Execute_GetTapHoldThreshold(InteractableObject))
		: 0.0f;
	ActiveInteraction.HoldDurationSeconds =
		FMath::Max(0.01f, IInteractable::Execute_GetHoldDuration(InteractableObject));

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

	ResetActiveInteractionState();

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
		ResetActiveInteractionState();

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
			// Remote client: authoritative completion via our Server RPC;
			// local Execute stays for presentation.
			if (AActor* Owner = GetOwner(); Owner && !Owner->HasAuthority())
			{
				if (AActor* TargetActor = ResolveInteractionActor(ActiveInteractableObjectPtr))
				{
					Server_NotifyMashComplete(TargetActor, Owner->GetActorLocation());
				}
			}

			IInteractable::Execute_OnMashInteractionComplete(ActiveInteractableObjectPtr, GetOwner());
		}
		WidgetPresenter.SetCompletedState();
		ResetActiveInteractionState();
	}
}

void UInteractionManager::ResetActiveInteractionState()
{
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
		ResetActiveInteractionState();
		return;
	}

	if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
	{
		// Remote client: the authoritative completion goes through our Server
		// RPC; the local Execute below remains for presentation (widget /
		// component BP events). See SERVER RPCs note in the header.
		if (AActor* Owner = GetOwner(); Owner && !Owner->HasAuthority())
		{
			if (AActor* TargetActor = ResolveInteractionActor(ActiveInteractableObjectPtr))
			{
				Server_NotifyHoldComplete(TargetActor, Owner->GetActorLocation());
			}
		}

		IInteractable::Execute_OnHoldInteractionComplete(ActiveInteractableObjectPtr, GetOwner());
	}

	WidgetPresenter.SetCompletedState();
	ResetActiveInteractionState();
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

	ResetActiveInteractionState();
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

	ResetActiveInteractionState();
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
				ResetActiveInteractionState();
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
				ResetActiveInteractionState();
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
				ResetActiveInteractionState();
				return;
			}

			if (ActiveInteraction.Progress >= 1.0f)
			{
				if (UObject* ActiveInteractableObjectPtr = GetActiveInteractableObject())
				{
					IInteractable::Execute_OnMashInteractionComplete(ActiveInteractableObjectPtr, GetOwner());
				}
				WidgetPresenter.SetCompletedState();
				ResetActiveInteractionState();
			}

			break;
		}

		case EManagedInteractionMode::ActorContinuous:
		{
			if (!HasValidActiveInteractable())
			{
				ResetActiveInteractionState();
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
	if (HasValidCurrentInteractable())
	{
		WidgetPresenter.HideGroundItemWorldWidget();
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

void UInteractionManager::Server_PickupToInventory_Implementation(
	int32 ItemID, FVector ClientLocation)
{
	if (!InteractionManagerPrivate::ValidateServerGroundItemPickup(
		this, ItemID, ClientLocation, TEXT("Server_PickupToInventory")))
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
				TEXT("Server_PickupToInventory: Rejected - client location too far (%.0f cm) for item %d"),
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
				TEXT("Server_PickupAndEquip: Rejected - client location too far (%.0f cm) for item %d"),
				FMath::Sqrt(DistSq), ItemID);
			return;
		}
	}

	PickupManager.PickupAndEquip(ItemID);
}

// SERVER-SIDE ACTOR INTERACTION

bool UInteractionManager::ValidateServerInteraction(AActor* TargetActor)
{
	AActor* Owner = GetOwner();
	if (!Owner || !IsValid(TargetActor))
	{
		return false;
	}

	// Validate against the SERVER's view of the pawn - the client-reported
	// location is informational only and never trusted for range checks.
	return ValidatorManager.ValidateActorInteraction(
		TargetActor,
		Owner->GetActorLocation(),
		TraceManager.InteractionDistance);
}

UObject* UInteractionManager::ResolveInteractableObjectOnActor(AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return nullptr;
	}

	if (UInteractableManager* InteractableComp = TargetActor->FindComponentByClass<UInteractableManager>())
	{
		return InteractableComp;
	}

	if (TargetActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		return TargetActor;
	}

	return nullptr;
}

void UInteractionManager::Server_InteractWithActor_Implementation(AActor* TargetActor, FVector ClientLocation)
{
	if (!ValidateServerInteraction(TargetActor))
	{
		UE_LOG(LogInteractionManager, Warning,
			TEXT("Server_InteractWithActor: validation failed for target %s"),
			*GetNameSafe(TargetActor));
		return;
	}

	if (UObject* InteractableObject = ResolveInteractableObjectOnActor(TargetActor))
	{
		if (IInteractable::Execute_CanInteract(InteractableObject, GetOwner()))
		{
			IInteractable::Execute_OnInteract(InteractableObject, GetOwner());
		}
	}
}

void UInteractionManager::Server_NotifyHoldComplete_Implementation(AActor* TargetActor, FVector ClientLocation)
{
	if (!ValidateServerInteraction(TargetActor))
	{
		UE_LOG(LogInteractionManager, Warning,
			TEXT("Server_NotifyHoldComplete: validation failed for target %s"),
			*GetNameSafe(TargetActor));
		return;
	}

	if (UObject* InteractableObject = ResolveInteractableObjectOnActor(TargetActor))
	{
		if (IInteractable::Execute_CanInteract(InteractableObject, GetOwner()))
		{
			IInteractable::Execute_OnHoldInteractionComplete(InteractableObject, GetOwner());
		}
	}
}

void UInteractionManager::Server_NotifyMashComplete_Implementation(AActor* TargetActor, FVector ClientLocation)
{
	if (!ValidateServerInteraction(TargetActor))
	{
		UE_LOG(LogInteractionManager, Warning,
			TEXT("Server_NotifyMashComplete: validation failed for target %s"),
			*GetNameSafe(TargetActor));
		return;
	}

	if (UObject* InteractableObject = ResolveInteractableObjectOnActor(TargetActor))
	{
		if (IInteractable::Execute_CanInteract(InteractableObject, GetOwner()))
		{
			IInteractable::Execute_OnMashInteractionComplete(InteractableObject, GetOwner());
		}
	}
}

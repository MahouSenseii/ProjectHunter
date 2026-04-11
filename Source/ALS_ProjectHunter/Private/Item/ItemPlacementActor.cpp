// Item/ItemPlacementActor.cpp

#include "Item/ItemPlacementActor.h"

#include "Item/ItemInstance.h"
#include "Tower/Subsystem/GroundItemSubsystem.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

#if WITH_EDITORONLY_DATA
#include "Components/BillboardComponent.h"
#endif

AItemPlacementActor::AItemPlacementActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

#if WITH_EDITORONLY_DATA
	EditorBillboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorBillboard"));
	if (EditorBillboard)
	{
		EditorBillboard->SetupAttachment(SceneRoot);
		EditorBillboard->bHiddenInGame = true;
		EditorBillboard->bIsScreenSizeScaled = true;
	}
#endif
}

void AItemPlacementActor::BeginPlay()
{
	Super::BeginPlay();

	if (!bSpawnOnBeginPlay)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// ── Order-of-BeginPlay fix ────────────────────────────────────────────
	// UGroundItemSubsystem::EnsureISMContainerExists refuses to spawn its
	// ISM container actor until World->HasBegunPlay() is true.  That flag
	// is set AFTER every actor's BeginPlay has fired, so calling
	// SpawnIntoWorld() directly here would fail for any placement actor
	// that was placed in the level at design time (which is all of them).
	//
	// UWorld::OnWorldBeginPlay is broadcast exactly once, at the end of
	// UWorld::BeginPlay(), after HasBegunPlay() has flipped.  That's the
	// correct hook for "run this after everyone is alive."
	//
	// For placement actors spawned AT RUNTIME (after the world has already
	// begun play — e.g. streamed levels, quest rewards), HasBegunPlay() is
	// already true, so we can spawn immediately without deferring.
	if (World->HasBegunPlay())
	{
		SpawnIntoWorld();
	}
	else
	{
		WorldBeginPlayHandle = World->OnWorldBeginPlay.AddUObject(
			this, &AItemPlacementActor::HandleWorldBeginPlay);
	}
}

void AItemPlacementActor::HandleWorldBeginPlay()
{
	// Unbind first so we're safe against any future re-broadcast.
	if (UWorld* World = GetWorld())
	{
		World->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
	}
	WorldBeginPlayHandle.Reset();

	SpawnIntoWorld();
}

void AItemPlacementActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unbind the deferred hook in case we're destroyed before it ever fired.
	if (WorldBeginPlayHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
		}
		WorldBeginPlayHandle.Reset();
	}

	if (bRemoveOnDestroyed && RegisteredItemID != INDEX_NONE)
	{
		RemoveFromWorld();
	}

	Super::EndPlay(EndPlayReason);
}

int32 AItemPlacementActor::SpawnIntoWorld()
{
	// Idempotent — if we already registered an item, do nothing.
	if (RegisteredItemID != INDEX_NONE)
	{
		return RegisteredItemID;
	}

	// ── Validate ─────────────────────────────────────────────────────────
	if (BaseItemHandle.IsNull() || BaseItemHandle.DataTable == nullptr)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ItemPlacementActor '%s': BaseItemHandle is null or has no "
			     "DataTable assigned — nothing will be placed."),
			*GetName());
		return INDEX_NONE;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return INDEX_NONE;
	}

	UGroundItemSubsystem* GroundItems = World->GetSubsystem<UGroundItemSubsystem>();
	if (!GroundItems)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ItemPlacementActor '%s': UGroundItemSubsystem is unavailable."),
			*GetName());
		return INDEX_NONE;
	}

	// ── Construct the item instance ──────────────────────────────────────
	// Outer = the GroundItemSubsystem so the item's lifetime is tied to the
	// subsystem (which itself owns it via its UPROPERTY map).  This means
	// the item will outlive *this* actor cleanly if bRemoveOnDestroyed is
	// flipped off — useful for "drop the placement marker after spawn"
	// editor workflows.
	UItemInstance* NewItem = NewObject<UItemInstance>(GroundItems);
	if (!NewItem)
	{
		return INDEX_NONE;
	}

	NewItem->Initialize(BaseItemHandle, ItemLevel, Rarity, bGenerateAffixes);

	if (!NewItem->HasValidBaseData())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ItemPlacementActor '%s': Initialized item has no valid base "
			     "data — row handle probably points at a row that doesn't exist."),
			*GetName());
		return INDEX_NONE;
	}

	// Apply quantity for stackable items.
	if (Quantity > 1)
	{
		NewItem->SetQuantity(Quantity);
		NewItem->UpdateTotalWeight();
	}

	// ── Register with the ground subsystem at our transform ──────────────
	RegisteredItemID = GroundItems->AddItemToGround(
		NewItem,
		GetActorLocation(),
		GetActorRotation());

	if (RegisteredItemID == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ItemPlacementActor '%s': AddItemToGround returned INDEX_NONE."),
			*GetName());
	}
	else
	{
		UE_LOG(LogTemp, Log,
			TEXT("ItemPlacementActor '%s': spawned item ID %d at %s."),
			*GetName(), RegisteredItemID, *GetActorLocation().ToString());
	}

	return RegisteredItemID;
}

UItemInstance* AItemPlacementActor::RemoveFromWorld()
{
	if (RegisteredItemID == INDEX_NONE)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		RegisteredItemID = INDEX_NONE;
		return nullptr;
	}

	UGroundItemSubsystem* GroundItems = World->GetSubsystem<UGroundItemSubsystem>();
	if (!GroundItems)
	{
		RegisteredItemID = INDEX_NONE;
		return nullptr;
	}

	UItemInstance* Removed = GroundItems->RemoveItemFromGround(RegisteredItemID);
	RegisteredItemID = INDEX_NONE;
	return Removed;
}

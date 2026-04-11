// Item/WeaponPlacementActor.cpp

#include "Item/WeaponPlacementActor.h"

#include "Item/ItemInstance.h"
#include "Tower/Subsystem/GroundItemSubsystem.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

#if WITH_EDITORONLY_DATA
#include "Components/BillboardComponent.h"
#endif

AWeaponPlacementActor::AWeaponPlacementActor()
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

void AWeaponPlacementActor::BeginPlay()
{
	Super::BeginPlay();

	if (bSpawnOnBeginPlay)
	{
		SpawnIntoWorld();
	}
}

void AWeaponPlacementActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bRemoveOnDestroyed && RegisteredItemID != INDEX_NONE)
	{
		RemoveFromWorld();
	}

	Super::EndPlay(EndPlayReason);
}

int32 AWeaponPlacementActor::SpawnIntoWorld()
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
			TEXT("WeaponPlacementActor '%s': BaseItemHandle is null or has no "
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
			TEXT("WeaponPlacementActor '%s': UGroundItemSubsystem is unavailable."),
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
			TEXT("WeaponPlacementActor '%s': Initialized item has no valid base "
			     "data — row handle probably points at a row that doesn't exist."),
			*GetName());
		return INDEX_NONE;
	}

	// ── Register with the ground subsystem at our transform ──────────────
	RegisteredItemID = GroundItems->AddItemToGround(
		NewItem,
		GetActorLocation(),
		GetActorRotation());

	if (RegisteredItemID == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("WeaponPlacementActor '%s': AddItemToGround returned INDEX_NONE."),
			*GetName());
	}

	return RegisteredItemID;
}

UItemInstance* AWeaponPlacementActor::RemoveFromWorld()
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

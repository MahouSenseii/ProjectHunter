// Tower/Subsystem/GroundItemSubsystem.cpp

#include "Tower/Subsystem/GroundItemSubsystem.h"
#include "Tower/Actors/ISMContainerActor.h"
#include "Item/ItemInstance.h"
#include "Engine/World.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "DrawDebugHelpers.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGroundItemSubsystem, Log, All);
DEFINE_LOG_CATEGORY(LogGroundItemSubsystem);

// ═══════════════════════════════════════════════════════════════════════
// SUBSYSTEM LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════

void UGroundItemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	bIsProcessingRemoval = false;
	
	UE_LOG(LogGroundItemSubsystem, Log, TEXT("GroundItemSubsystem: Initialized"));
}

void UGroundItemSubsystem::Deinitialize()
{
	ClearAllItems();
	
	if (ISMContainerActor.IsValid())
	{
		ISMContainerActor->Destroy();
		ISMContainerActor.Reset();
	}
	
	Super::Deinitialize();
	
	UE_LOG(LogGroundItemSubsystem, Log, TEXT("GroundItemSubsystem: Deinitialized"));
}

// ═══════════════════════════════════════════════════════════════════════
// ISM MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════

void UGroundItemSubsystem::EnsureISMContainerExists()
{
	// WS-2 FIX: TWeakObjectPtr::IsValid() returns false if the actor was GC'd.
	if (ISMContainerActor.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogGroundItemSubsystem, Error, TEXT("EnsureISMContainerExists: World is null!"));
		return;
	}

	if (!World->HasBegunPlay())
	{
		UE_LOG(LogGroundItemSubsystem, Warning, TEXT("EnsureISMContainerExists: World hasn't begun play yet"));
		return;
	}

	FActorSpawnParameters Params;
	Params.Name = FName("GroundItems_ISMContainer");
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags = RF_Transient;
	
	ISMContainerActor = World->SpawnActor<AISMContainerActor>(
		AISMContainerActor::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		Params
	);
	
	if (ISMContainerActor.IsValid())
	{
		UE_LOG(LogGroundItemSubsystem, Log, TEXT("GroundItemSubsystem: Created ISM container actor"));
	}
	else
	{
		UE_LOG(LogGroundItemSubsystem, Error, TEXT("GroundItemSubsystem: Failed to spawn ISM container actor!"));
	}
}

UInstancedStaticMeshComponent* UGroundItemSubsystem::GetOrCreateISMComponent(UStaticMesh* Mesh)
{
	if (!Mesh)
	{
		return nullptr;
	}

	if (UInstancedStaticMeshComponent** FoundISM = MeshToISM.Find(Mesh))
	{
		if (*FoundISM && IsValid(*FoundISM))
		{
			return *FoundISM;
		}
	}

	EnsureISMContainerExists();

	if (!ISMContainerActor.IsValid())
	{
		return nullptr;
	}

	UInstancedStaticMeshComponent* NewISM = NewObject<UInstancedStaticMeshComponent>(
		ISMContainerActor.Get(),
		*FString::Printf(TEXT("ISM_%s"), *Mesh->GetName())
	);
	
	NewISM->SetStaticMesh(Mesh);
	NewISM->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	NewISM->SetCollisionResponseToAllChannels(ECR_Ignore);
	NewISM->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	NewISM->RegisterComponent();
	NewISM->AttachToComponent(ISMContainerActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

	MeshToISM.Add(Mesh, NewISM);

	UE_LOG(LogGroundItemSubsystem, Log, TEXT("Created ISM component for mesh: %s"), *Mesh->GetName());

	return NewISM;
}

void UGroundItemSubsystem::UpdateIndexAfterSwap(UInstancedStaticMeshComponent* ISMComponent,
                                                int32 RemovedIndex, int32 LastIndex)
{
	// P-3 FIX: After the manual swap-and-pop, the instance that was at LastIndex now
	// sits at RemovedIndex. Only that one entry needs its stored index updated.
	if (RemovedIndex == LastIndex)
	{
		// The removed instance WAS the last one; no swap occurred, nothing to update.
		return;
	}

	// OPT-ISM: The old linear scan over ItemISMData was O(n) in the number of
	// ground items.  This is fine for single removals but adds up during batch
	// operations.  We still do a linear scan here (since we don't maintain an
	// ISM-index→ItemID reverse map) but break early, keeping it O(n) worst case
	// with typically very fast early exits.
	for (TPair<int32, FGroundItemISMData>& Pair : ItemISMData)
	{
		FGroundItemISMData& Data = Pair.Value;
		if (Data.ISMComponent == ISMComponent && Data.InstanceIndex == LastIndex)
		{
			Data.InstanceIndex = RemovedIndex;
			UE_LOG(LogGroundItemSubsystem, Verbose,
				TEXT("UpdateIndexAfterSwap: Item %d moved from ISM index %d to %d after swap removal"),
				Pair.Key, LastIndex, RemovedIndex);
			return; // Only one item can hold LastIndex; stop once found.
		}
	}
}

// ═══════════════════════════════════════════════════════════════════════
// PRIMARY API
// ═══════════════════════════════════════════════════════════════════════

int32 UGroundItemSubsystem::AddItemToGround(UItemInstance* Item, FVector Location, FRotator Rotation)
{
	EnsureISMContainerExists();

	if (!ISMContainerActor.IsValid())
	{
		UE_LOG(LogGroundItemSubsystem, Error, TEXT("AddItemToGround: Cannot add item - no container actor!"));
		return -1;
	}

	if (!Item || !Item->HasValidBaseData())
	{
		UE_LOG(LogGroundItemSubsystem, Warning, TEXT("AddItemToGround: Invalid item!"));
		return -1;
	}

	UStaticMesh* Mesh = Item->GetGroundMesh();
	if (!Mesh)
	{
		UE_LOG(LogGroundItemSubsystem, Warning, TEXT("AddItemToGround: Item has no ground mesh!"));
		return -1;
	}

	UInstancedStaticMeshComponent* ISM = GetOrCreateISMComponent(Mesh);
	if (!ISM)
	{
		UE_LOG(LogGroundItemSubsystem, Error, TEXT("AddItemToGround: Failed to get/create ISM component!"));
		return -1;
	}

	FTransform Transform(Rotation, Location, FVector::OneVector);
	int32 ISMInstanceIndex = ISM->AddInstance(Transform);

	if (ISMInstanceIndex == INDEX_NONE)
	{
		UE_LOG(LogGroundItemSubsystem, Error, TEXT("AddItemToGround: Failed to add instance to ISM!"));
		return -1;
	}

	int32 ItemID = NextItemID++;

	GroundItems.Add(ItemID, Item);
	InstanceLocations.Add(ItemID, Location);
	ItemISMData.Add(ItemID, FGroundItemISMData(ISM, ISMInstanceIndex, Mesh));

	// P-3 FIX: Keep the reverse map in sync for O(1) GetInstanceID lookups.
	InstanceToIDMap.Add(Item, ItemID);

	UE_LOG(LogGroundItemSubsystem, Log, TEXT("AddItemToGround: Added item '%s' (ID: %d, ISMIndex: %d) at %s"),
		*Item->GetDisplayName().ToString(), ItemID, ISMInstanceIndex, *Location.ToString());

	return ItemID;
}

UItemInstance* UGroundItemSubsystem::RemoveItemFromGround(int32 ItemID)
{
	// ═══════════════════════════════════════════════
	// FIX: Guard against concurrent removal operations
	// ═══════════════════════════════════════════════
	if (bIsProcessingRemoval)
	{
		UE_LOG(LogGroundItemSubsystem, Warning,
			TEXT("RemoveItemFromGround: Already processing a removal, queuing item %d"), ItemID);

		// WS-1 FIX: Lock before touching PendingRemovals — async loaders or
		// parallel-for tasks could call RemoveItemFromGround concurrently.
		FScopeLock Lock(&PendingRemovalsCS);
		PendingRemovals.AddUnique(ItemID);
		return nullptr;
	}

	bIsProcessingRemoval = true;

	UItemInstance* Result = RemoveItemFromGroundInternal(ItemID);

	// Process any queued removals
	{
		FScopeLock Lock(&PendingRemovalsCS);
		while (PendingRemovals.Num() > 0)
		{
			int32 QueuedID = PendingRemovals.Pop();
			RemoveItemFromGroundInternal(QueuedID);
		}
	}

	bIsProcessingRemoval = false;
	
	return Result;
}

UItemInstance* UGroundItemSubsystem::RemoveItemFromGroundInternal(int32 ItemID)
{
	UItemInstance** FoundItem = GroundItems.Find(ItemID);
	if (!FoundItem)
	{
		UE_LOG(LogGroundItemSubsystem, Warning, TEXT("RemoveItemFromGround: Item ID %d not found"), ItemID);
		return nullptr;
	}

	UItemInstance* Item = *FoundItem;

	FGroundItemISMData* ISMData = ItemISMData.Find(ItemID);
	if (ISMData && ISMData->IsValid())
	{
		UInstancedStaticMeshComponent* ISM = ISMData->ISMComponent;
		int32 InstanceIndex = ISMData->InstanceIndex;
		const int32 LastIndex = ISM->GetInstanceCount() - 1;

		if (InstanceIndex >= 0 && InstanceIndex <= LastIndex)
		{
			// P-3 FIX: Emulate swap-and-pop so removal is effectively O(1).
			// UInstancedStaticMeshComponent has no RemoveInstanceSwap, so we do it
			// manually:
			//   1. If this isn't already the last slot, overwrite it with the last
			//      instance's world transform — the visual result is identical.
			//   2. Call RemoveInstance on the last slot: O(1) because no indices
			//      after it need shifting.
			//   3. UpdateIndexAfterSwap then fixes only the single bookkeeping entry
			//      whose index moved from LastIndex to InstanceIndex.
			if (InstanceIndex != LastIndex)
			{
				FTransform LastTransform;
				ISM->GetInstanceTransform(LastIndex, LastTransform, /*bWorldSpace=*/true);
				ISM->UpdateInstanceTransform(InstanceIndex, LastTransform,
				                             /*bWorldSpace=*/true,
				                             /*bMarkRenderStateDirty=*/false,
				                             /*bTeleport=*/true);
			}
			ISM->RemoveInstance(LastIndex);
			UpdateIndexAfterSwap(ISM, InstanceIndex, LastIndex);

			UE_LOG(LogGroundItemSubsystem, Log,
				TEXT("RemoveItemFromGround: Removed item ID %d (ISMIndex was %d, LastIndex was %d)"),
				ItemID, InstanceIndex, LastIndex);
		}
		else
		{
			UE_LOG(LogGroundItemSubsystem, Error,
				TEXT("RemoveItemFromGround: Invalid ISM index %d for item %d (ISM has %d instances)"),
				InstanceIndex, ItemID, ISM->GetInstanceCount());
		}
	}
	else
	{
		UE_LOG(LogGroundItemSubsystem, Warning, TEXT("RemoveItemFromGround: No valid ISM data for item ID %d"), ItemID);
	}

	// P-3 FIX: Remove from reverse lookup map to keep it in sync.
	if (Item)
	{
		InstanceToIDMap.Remove(Item);
	}

	GroundItems.Remove(ItemID);
	InstanceLocations.Remove(ItemID);
	ItemISMData.Remove(ItemID);

	return Item;
}

// ═══════════════════════════════════════════════════════════════════════
// BATCH OPERATIONS
// ═══════════════════════════════════════════════════════════════════════

TArray<UItemInstance*> UGroundItemSubsystem::RemoveMultipleItemsFromGround(const TArray<int32>& ItemIDs)
{
	TArray<UItemInstance*> RemovedItems;
	RemovedItems.Reserve(ItemIDs.Num());

	if (ItemIDs.Num() == 0)
	{
		return RemovedItems;
	}

	bIsProcessingRemoval = true;

	// OPT-ISM: Collect all affected ISM components so we can batch the render
	// state rebuild.  Without this, each RemoveInstance call can trigger an
	// individual MarkRenderStateDirty → N separate rebuilds for N removals.
	TSet<UInstancedStaticMeshComponent*> AffectedISMs;

	// Sort by ISM index descending for efficient removal
	TArray<TPair<int32, int32>> SortedItems;
	for (int32 ItemID : ItemIDs)
	{
		if (FGroundItemISMData* Data = ItemISMData.Find(ItemID))
		{
			SortedItems.Add(TPair<int32, int32>(ItemID, Data->InstanceIndex));
			if (Data->ISMComponent)
			{
				AffectedISMs.Add(Data->ISMComponent);
			}
		}
	}

	SortedItems.Sort([](const TPair<int32, int32>& A, const TPair<int32, int32>& B)
	{
		return A.Value > B.Value;
	});

	for (const TPair<int32, int32>& Pair : SortedItems)
	{
		if (UItemInstance* Item = RemoveItemFromGroundInternal(Pair.Key))
		{
			RemovedItems.Add(Item);
		}
	}

	// OPT-ISM: Single render state rebuild per affected ISM component
	// (instead of one per removal). This is the key perf win for batch pickup.
	for (UInstancedStaticMeshComponent* ISM : AffectedISMs)
	{
		if (ISM && IsValid(ISM))
		{
			ISM->MarkRenderStateDirty();
		}
	}

	bIsProcessingRemoval = false;

	return RemovedItems;
}

// ═══════════════════════════════════════════════════════════════════════
// QUERIES
// ═══════════════════════════════════════════════════════════════════════

UItemInstance* UGroundItemSubsystem::GetItemByID(int32 ItemID) const
{
	UItemInstance* const* FoundItem = GroundItems.Find(ItemID);
	return FoundItem ? *FoundItem : nullptr;
}

UItemInstance* UGroundItemSubsystem::GetNearestItem(FVector Location, float MaxDistance, int32& OutItemID)
{
	OutItemID = -1;
	float ClosestDistSq = MaxDistance * MaxDistance;
	UItemInstance* ClosestItem = nullptr;

	for (const TPair<int32, FVector>& Pair : InstanceLocations)
	{
		float DistSq = FVector::DistSquared(Location, Pair.Value);
		
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			OutItemID = Pair.Key;
			
			if (UItemInstance* const* Item = GroundItems.Find(Pair.Key))
			{
				ClosestItem = *Item;
			}
		}
	}

	return ClosestItem;
}

int32 UGroundItemSubsystem::GetItemsInRadius(FVector Location, float Radius, TArray<int32>& OutItemIDs)
{
	OutItemIDs.Reset();
	float RadiusSq = Radius * Radius;

	for (const TPair<int32, FVector>& Pair : InstanceLocations)
	{
		float DistSq = FVector::DistSquared(Location, Pair.Value);
		
		if (DistSq <= RadiusSq)
		{
			OutItemIDs.Add(Pair.Key);
		}
	}

	return OutItemIDs.Num();
}

TArray<UItemInstance*> UGroundItemSubsystem::GetItemInstancesInRadius(FVector Location, float Radius)
{
	TArray<UItemInstance*> ItemsInRange;
	float RadiusSq = Radius * Radius;

	for (const TPair<int32, FVector>& Pair : InstanceLocations)
	{
		float DistSq = FVector::DistSquared(Location, Pair.Value);
		
		if (DistSq <= RadiusSq)
		{
			if (UItemInstance* const* Found = GroundItems.Find(Pair.Key))
			{
				ItemsInRange.Add(*Found);
			}
		}
	}

	return ItemsInRange;
}

int32 UGroundItemSubsystem::GetInstanceID(UItemInstance* Item) const
{
	if (!Item)
	{
		return -1;
	}

	// P-3 FIX: O(1) reverse-map lookup replaces the previous O(n) linear scan.
	if (const int32* FoundID = InstanceToIDMap.Find(Item))
	{
		return *FoundID;
	}

	return -1;
}

void UGroundItemSubsystem::UpdateItemLocation(int32 ItemID, FVector NewLocation)
{
	FGroundItemISMData* ISMData = ItemISMData.Find(ItemID);
	if (!ISMData || !ISMData->IsValid())
	{
		UE_LOG(LogGroundItemSubsystem, Warning, TEXT("UpdateItemLocation: No valid ISM data for item ID %d"), ItemID);
		return;
	}

	UInstancedStaticMeshComponent* ISM = ISMData->ISMComponent;
	int32 InstanceIndex = ISMData->InstanceIndex;

	FTransform CurrentTransform;
	ISM->GetInstanceTransform(InstanceIndex, CurrentTransform, true);

	FTransform NewTransform = CurrentTransform;
	NewTransform.SetLocation(NewLocation);

	ISM->UpdateInstanceTransform(InstanceIndex, NewTransform, true);

	InstanceLocations.Add(ItemID, NewLocation);
}

void UGroundItemSubsystem::ClearAllItems()
{
	for (TPair<UStaticMesh*, UInstancedStaticMeshComponent*>& Pair : MeshToISM)
	{
		if (Pair.Value && IsValid(Pair.Value))
		{
			Pair.Value->ClearInstances();
		}
	}

	GroundItems.Empty();
	InstanceLocations.Empty();
	ItemISMData.Empty();
	InstanceToIDMap.Empty(); // P-3 FIX: keep reverse map in sync
	PendingRemovals.Empty();

	UE_LOG(LogGroundItemSubsystem, Log, TEXT("ClearAllItems: All ground items cleared"));
}

// ═══════════════════════════════════════════════════════════════════════
// DEBUG
// ═══════════════════════════════════════════════════════════════════════

#if WITH_EDITOR
void UGroundItemSubsystem::DebugDrawAllItems(float Duration)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (const TPair<int32, FVector>& Pair : InstanceLocations)
	{
		DrawDebugSphere(World, Pair.Value, 25.0f, 8, FColor::Yellow, false, Duration);
		
		if (UItemInstance* const* Found = GroundItems.Find(Pair.Key))
		{
			FString DebugText = FString::Printf(TEXT("[%d] %s"), Pair.Key, *(*Found)->GetDisplayName().ToString());
			DrawDebugString(World, Pair.Value + FVector(0, 0, 50), DebugText, nullptr, FColor::White, Duration);
		}
	}
	
	UE_LOG(LogGroundItemSubsystem, Log, TEXT("DebugDrawAllItems: Drew %d items for %.1fs"), InstanceLocations.Num(), Duration);
}
#endif

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
	
	if (ISMContainerActor)
	{
		ISMContainerActor->Destroy();
		ISMContainerActor = nullptr;
	}
	
	Super::Deinitialize();
	
	UE_LOG(LogGroundItemSubsystem, Log, TEXT("GroundItemSubsystem: Deinitialized"));
}

// ═══════════════════════════════════════════════════════════════════════
// ISM MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════

void UGroundItemSubsystem::EnsureISMContainerExists()
{
	if (ISMContainerActor && IsValid(ISMContainerActor))
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
	
	if (ISMContainerActor)
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
	
	if (!ISMContainerActor)
	{
		return nullptr;
	}

	UInstancedStaticMeshComponent* NewISM = NewObject<UInstancedStaticMeshComponent>(
		ISMContainerActor,
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

void UGroundItemSubsystem::ReindexAfterRemoval(UInstancedStaticMeshComponent* ISMComponent, int32 RemovedIndex)
{
	
	for (TPair<int32, FGroundItemISMData>& Pair : ItemISMData)
	{
		FGroundItemISMData& Data = Pair.Value;
		
		if (Data.ISMComponent != ISMComponent)
		{
			continue;
		}

		if (Data.InstanceIndex > RemovedIndex)
		{
			Data.InstanceIndex--;
			
			UE_LOG(LogGroundItemSubsystem, Verbose, TEXT("ReindexAfterRemoval: Item %d index shifted from %d to %d"),
				Pair.Key, Data.InstanceIndex + 1, Data.InstanceIndex);
		}
	}
}

// ═══════════════════════════════════════════════════════════════════════
// PRIMARY API
// ═══════════════════════════════════════════════════════════════════════

int32 UGroundItemSubsystem::AddItemToGround(UItemInstance* Item, FVector Location, FRotator Rotation)
{
	EnsureISMContainerExists();
	
	if (!ISMContainerActor)
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
		
		PendingRemovals.AddUnique(ItemID);
		return nullptr;
	}
	
	bIsProcessingRemoval = true;
	
	UItemInstance* Result = RemoveItemFromGroundInternal(ItemID);
	
	// Process any queued removals
	while (PendingRemovals.Num() > 0)
	{
		int32 QueuedID = PendingRemovals.Pop();
		RemoveItemFromGroundInternal(QueuedID);
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

		if (InstanceIndex >= 0 && InstanceIndex < ISM->GetInstanceCount())
		{
			ISM->RemoveInstance(InstanceIndex);
			ReindexAfterRemoval(ISM, InstanceIndex);
			
			UE_LOG(LogGroundItemSubsystem, Log, TEXT("RemoveItemFromGround: Removed item ID %d (ISMIndex was %d)"), 
				ItemID, InstanceIndex);
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
	
	// Sort by ISM index descending for efficient removal
	TArray<TPair<int32, int32>> SortedItems;
	for (int32 ItemID : ItemIDs)
	{
		if (FGroundItemISMData* Data = ItemISMData.Find(ItemID))
		{
			SortedItems.Add(TPair<int32, int32>(ItemID, Data->InstanceIndex));
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

	for (const TPair<int32, UItemInstance*>& Pair : GroundItems)
	{
		if (Pair.Value == Item)
		{
			return Pair.Key;
		}
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

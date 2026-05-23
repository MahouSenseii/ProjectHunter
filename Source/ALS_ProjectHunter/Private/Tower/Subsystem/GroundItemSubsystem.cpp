#include "Tower/Subsystem/GroundItemSubsystem.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Tower/Actors/ISMContainerActor.h"
#include "Item/ItemInstance.h"
#include "Item/Library/ItemStructs.h"
#include "Engine/World.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY(LogGroundItemSubsystem);

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

void UGroundItemSubsystem::EnsureISMContainerExists()
{
	if (ISMContainerActor.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		PH_LOG_ERROR(LogGroundItemSubsystem, "EnsureISMContainerExists failed: World was null.");
		return;
	}

	if (!World->HasBegunPlay())
	{
		PH_LOG_WARNING(LogGroundItemSubsystem, "EnsureISMContainerExists deferred because the world has not begun play yet.");
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
		PH_LOG_ERROR(LogGroundItemSubsystem, "EnsureISMContainerExists failed: Could not spawn the ISM container actor.");
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
	if (RemovedIndex == LastIndex)
	{
		return;
	}
	
	for (TPair<int32, FGroundItemISMData>& Pair : ItemISMData)
	{
		FGroundItemISMData& Data = Pair.Value;
		if (Data.ISMComponent == ISMComponent && Data.InstanceIndex == LastIndex)
		{
			Data.InstanceIndex = RemovedIndex;

			if (ISMContainerActor.IsValid())
			{
				ISMContainerActor->UpdateItemAnimationIndex(Pair.Key, RemovedIndex);
			}

			UE_LOG(LogGroundItemSubsystem, Verbose,
				TEXT("UpdateIndexAfterSwap: Item %d moved from ISM index %d to %d after swap removal"),
				Pair.Key, LastIndex, RemovedIndex);
			return;
		}
	}
}

int32 UGroundItemSubsystem::AddItemToGround(UItemInstance* Item, FVector Location, FRotator Rotation)
{
	EnsureISMContainerExists();

	if (!ISMContainerActor.IsValid())
	{
		PH_LOG_ERROR(LogGroundItemSubsystem, "AddItemToGround failed: The ISM container actor was unavailable.");
		return -1;
	}

	if (!Item || !Item->HasValidBaseData())
	{
		PH_LOG_WARNING(LogGroundItemSubsystem, "AddItemToGround failed: Item was invalid.");
		return -1;
	}

	UStaticMesh* Mesh = Item->GetGroundMesh();
	if (!Mesh)
	{
		PH_LOG_WARNING(LogGroundItemSubsystem, "AddItemToGround failed: Item=%s had no ground mesh.", *Item->GetDisplayName().ToString());
		return -1;
	}

	UInstancedStaticMeshComponent* ISM = GetOrCreateISMComponent(Mesh);
	if (!ISM)
	{
		PH_LOG_ERROR(LogGroundItemSubsystem, "AddItemToGround failed: Could not get or create an ISM component for Mesh=%s.", *GetNameSafe(Mesh));
		return -1;
	}

	FRotator FinalRotation = Rotation;
	if (const FItemBase* BaseData = Item->GetBaseData())
	{
		if (BaseData->bFlipGroundMeshRotation)
		{
			FinalRotation.Pitch += 180.0f;
		}
		FinalRotation += BaseData->GroundMeshRotationOffset;
	}

	FTransform Transform(FinalRotation, Location, FVector::OneVector);
	int32 ISMInstanceIndex = ISM->AddInstance(Transform);

	if (ISMInstanceIndex == INDEX_NONE)
	{
		PH_LOG_ERROR(LogGroundItemSubsystem, "AddItemToGround failed: Could not add an instance to the ISM for Item=%s.", *Item->GetDisplayName().ToString());
		return -1;
	}

	int32 ItemID = NextItemID++;

	GroundItems.Add(ItemID, Item);
	InstanceLocations.Add(ItemID, Location);
	ItemISMData.Add(ItemID, FGroundItemISMData(ISM, ISMInstanceIndex, Mesh));


	InstanceToIDMap.Add(Item, ItemID);

	if (ISMContainerActor.IsValid())
	{
		ISMContainerActor->RegisterItemForAnimation(ItemID, ISM, ISMInstanceIndex, Location, FinalRotation);
	}

	UE_LOG(LogGroundItemSubsystem, Log, TEXT("AddItemToGround: Added item '%s' (ID: %d, ISMIndex: %d) at %s"),
		*Item->GetDisplayName().ToString(), ItemID, ISMInstanceIndex, *Location.ToString());

	return ItemID;
}

UItemInstance* UGroundItemSubsystem::RemoveItemFromGround(int32 ItemID)
{
	if (bIsProcessingRemoval)
	{
		PH_LOG_WARNING(LogGroundItemSubsystem, "RemoveItemFromGround deferred ItemID=%d because a removal was already in progress.", ItemID);


		FScopeLock Lock(&PendingRemovalsCS);
		PendingRemovals.AddUnique(ItemID);
		return nullptr;
	}

	bIsProcessingRemoval = true;

	UItemInstance* Result = RemoveItemFromGroundInternal(ItemID);

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
		PH_LOG_WARNING(LogGroundItemSubsystem, "RemoveItemFromGround failed: ItemID=%d was not found.", ItemID);
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
			PH_LOG_ERROR(LogGroundItemSubsystem, "RemoveItemFromGround failed: ItemID=%d had invalid ISMIndex=%d while the component had %d instances.", ItemID, InstanceIndex, ISM->GetInstanceCount());
		}
	}
	else
	{
		PH_LOG_WARNING(LogGroundItemSubsystem, "RemoveItemFromGround found no valid ISM data for ItemID=%d.", ItemID);
	}

	
	if (Item)
	{
		InstanceToIDMap.Remove(Item);
	}

	if (ISMContainerActor.IsValid())
	{
		ISMContainerActor->UnregisterItemFromAnimation(ItemID);
	}

	GroundItems.Remove(ItemID);
	InstanceLocations.Remove(ItemID);
	ItemISMData.Remove(ItemID);

	return Item;
}

TArray<UItemInstance*> UGroundItemSubsystem::RemoveMultipleItemsFromGround(const TArray<int32>& ItemIDs)
{
	TArray<UItemInstance*> RemovedItems;
	RemovedItems.Reserve(ItemIDs.Num());

	if (ItemIDs.Num() == 0)
	{
		return RemovedItems;
	}

	bIsProcessingRemoval = true;

	TSet<UInstancedStaticMeshComponent*> AffectedISMs;
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


	if (const int32* FoundID = InstanceToIDMap.Find(Item))
	{
		return *FoundID;
	}

	return -1;
}

int32 UGroundItemSubsystem::FindItemByISMInstance(UInstancedStaticMeshComponent* ISMComponent, int32 InstanceIndex) const
{
	if (!ISMComponent || InstanceIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}


	for (const TPair<int32, FGroundItemISMData>& Pair : ItemISMData)
	{
		if (Pair.Value.ISMComponent == ISMComponent && Pair.Value.InstanceIndex == InstanceIndex)
		{
			return Pair.Key;
		}
	}

	return INDEX_NONE;
}

void UGroundItemSubsystem::UpdateItemLocation(int32 ItemID, FVector NewLocation)
{
	FGroundItemISMData* ISMData = ItemISMData.Find(ItemID);
	if (!ISMData || !ISMData->IsValid())
	{
		PH_LOG_WARNING(LogGroundItemSubsystem, "UpdateItemLocation failed: No valid ISM data was found for ItemID=%d.", ItemID);
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
	if (ISMContainerActor.IsValid())
	{
		ISMContainerActor->ClearAllAnimationState();
	}

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
	InstanceToIDMap.Empty();
	PendingRemovals.Empty();

	UE_LOG(LogGroundItemSubsystem, Log, TEXT("ClearAllItems: All ground items cleared"));
}

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

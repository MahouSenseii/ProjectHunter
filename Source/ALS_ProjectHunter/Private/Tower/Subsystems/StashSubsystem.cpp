#include "Tower/Subsystems/StashSubsystem.h"
#include "Tower/Subsystems/StashSaveGame.h"
#include "Item/ItemInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

DEFINE_LOG_CATEGORY(LogStashSubsystem);


void UStashSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UStashSubsystem::Deinitialize()
{
	FlushDirtyTabs();
	Super::Deinitialize();
}

void UStashSubsystem::LoadStashHandles(const FString& CharacterSlotName)
{
	ActiveSlotName = CharacterSlotName.TrimStartAndEnd();
	if (ActiveSlotName.IsEmpty())
	{
		ActiveSlotName = TEXT("Hunter");
		UE_LOG(LogStashSubsystem, Warning,
			TEXT("LoadStashHandles: Empty character slot name; using '%s'."), *ActiveSlotName);
	}
	TabHandles.Empty();
	LoadedTabs.Empty();
	bHandlesDirty = false;

	const FString HandleSlot = ActiveSlotName + TEXT("_StashHandles");

	if (UGameplayStatics::DoesSaveGameExist(HandleSlot, 0))
	{
		if (USaveGame* Raw = UGameplayStatics::LoadGameFromSlot(HandleSlot, 0))
		{
			if (UStashHandlesSaveGame* HandlesSave = Cast<UStashHandlesSaveGame>(Raw))
			{
				if (HandlesSave->SaveVersion > UStashHandlesSaveGame::CurrentSaveVersion)
				{
					UE_LOG(LogStashSubsystem, Error,
						TEXT("LoadStashHandles: Save version %d is newer than supported version %d."),
						HandlesSave->SaveVersion, UStashHandlesSaveGame::CurrentSaveVersion);
				}
				else
				{
					for (const FStashTabHandleSaveData& Saved : HandlesSave->Handles)
					{
						if (Saved.TabID == NAME_None || TabHandles.ContainsByPredicate(
							[&Saved](const FStashTabHandle& Existing) { return Existing.TabID == Saved.TabID; }))
						{
							continue;
						}
						FStashTabHandle Handle(Saved.TabID, Saved.TabName, Saved.TabType);
						Handle.CachedItemCount = FMath::Max(0, Saved.CachedItemCount);
						Handle.AccentColor     = Saved.AccentColor;
						TabHandles.Add(Handle);
					}
					UE_LOG(LogStashSubsystem, Log,
						TEXT("LoadStashHandles: Loaded %d handles from '%s'"),
						TabHandles.Num(), *HandleSlot);
				}
			}
		}
	}

	if (TabHandles.Num() == 0)
	{
		TabHandles.Add(FStashTabHandle(TEXT("Tab_0"), FText::FromString(TEXT("Main Stash")),       EStashTabType::STT_Normal));
		TabHandles.Add(FStashTabHandle(TEXT("Tab_1"), FText::FromString(TEXT("Currency")),          EStashTabType::STT_Currency));
		TabHandles.Add(FStashTabHandle(TEXT("Tab_2"), FText::FromString(TEXT("Gear Storage")),      EStashTabType::STT_Quad));
		bHandlesDirty = true;
	}

	UE_LOG(LogStashSubsystem, Log,
		TEXT("LoadStashHandles: %d tab handles ready (data NOT loaded - lazy)"),
		TabHandles.Num());
}

bool UStashSubsystem::RequestTabData(int32 TabIndex)
{
	if (!IsValidTabIndex(TabIndex))
	{
		return false;
	}

	FStashTabHandle& Handle = TabHandles[TabIndex];

	if (Handle.bIsLoaded)
	{
		return true;
	}

	const bool bLoaded = LoadTab(TabIndex);
	if (bLoaded)
	{
		Handle.bIsLoaded = true;
		OnStashTabLoaded.Broadcast(Handle.TabID);
		UE_LOG(LogStashSubsystem, Log,
			TEXT("RequestTabData: Tab '%s' loaded (%d items)"),
			*Handle.TabID.ToString(),
			LoadedTabs.Contains(Handle.TabID) ? LoadedTabs[Handle.TabID].Items.Num() : 0);
	}
	else
	{
		FStashTabData NewData(Handle.TabID);
		NewData.GridSize = (Handle.TabType == EStashTabType::STT_Quad)
			? FIntPoint(24, 24) : FIntPoint(12, 12);
		LoadedTabs.Add(Handle.TabID, NewData);
		Handle.bIsLoaded = true;
		OnStashTabLoaded.Broadcast(Handle.TabID);
	}

	return true;
}

FStashTabData* UStashSubsystem::GetLoadedTabData(int32 TabIndex)
{
	if (!IsValidTabIndex(TabIndex) || !TabHandles[TabIndex].bIsLoaded)
	{
		return nullptr;
	}
	return LoadedTabs.Find(TabHandles[TabIndex].TabID);
}

bool UStashSubsystem::IsTabLoaded(int32 TabIndex) const
{
	return IsValidTabIndex(TabIndex) && TabHandles[TabIndex].bIsLoaded;
}

bool UStashSubsystem::AddItemToTab(int32 TabIndex, UItemInstance* Item, FIntPoint GridPos)
{
	if (!Item)
	{
		return false;
	}

	FStashTabData* TabData = GetLoadedTabData(TabIndex);
	if (!TabData)
	{
		UE_LOG(LogStashSubsystem, Warning,
			TEXT("AddItemToTab: Tab %d not loaded - call RequestTabData first"), TabIndex);
		return false;
	}

	FIntPoint PlacePos = GridPos;
	if (PlacePos == FIntPoint(-1, -1))
	{
		if (!FindFreeGridPosition(*TabData, PlacePos))
		{
			UE_LOG(LogStashSubsystem, Warning, TEXT("AddItemToTab: Tab %d is full"), TabIndex);
			return false;
		}
	}

	for (const FStashItemEntry& Entry : TabData->Items)
	{
		if (Entry.GridPosition == PlacePos)
		{
			UE_LOG(LogStashSubsystem, Warning,
				TEXT("AddItemToTab: Position (%d,%d) in tab %d is occupied"),
				PlacePos.X, PlacePos.Y, TabIndex);
			return false;
		}
	}

	TabData->Items.Add(FStashItemEntry(Item, PlacePos));
	TabHandles[TabIndex].CachedItemCount++;
	bHandlesDirty = true;

	MarkTabDirty(TabIndex);
	OnStashItemAdded.Broadcast(TabHandles[TabIndex].TabID, Item);

	return true;
}

bool UStashSubsystem::AddItemToTabAutoPlace(int32 TabIndex, UItemInstance* Item)
{
	return AddItemToTab(TabIndex, Item, FIntPoint(-1, -1));
}

UItemInstance* UStashSubsystem::RemoveItemFromTab(int32 TabIndex, FIntPoint GridPos)
{
	FStashTabData* TabData = GetLoadedTabData(TabIndex);
	if (!TabData)
	{
		return nullptr;
	}

	for (int32 i = 0; i < TabData->Items.Num(); ++i)
	{
		if (TabData->Items[i].GridPosition == GridPos)
		{
			UItemInstance* Item = TabData->Items[i].Item;
			TabData->Items.RemoveAt(i);

			if (TabHandles[TabIndex].CachedItemCount > 0)
			{
				TabHandles[TabIndex].CachedItemCount--;
				bHandlesDirty = true;
			}

			MarkTabDirty(TabIndex);
			OnStashItemRemoved.Broadcast(TabHandles[TabIndex].TabID, Item);
			return Item;
		}
	}

	return nullptr;
}

bool UStashSubsystem::MoveItem(int32 FromTabIndex, FIntPoint FromPos,
	int32 ToTabIndex, FIntPoint ToPos)
{
	UItemInstance* Item = RemoveItemFromTab(FromTabIndex, FromPos);
	if (!Item)
	{
		return false;
	}

	if (!AddItemToTab(ToTabIndex, Item, ToPos))
	{
		AddItemToTab(FromTabIndex, Item, FromPos);
		return false;
	}

	return true;
}

void UStashSubsystem::MarkTabDirty(int32 TabIndex)
{
	if (IsValidTabIndex(TabIndex))
	{
		TabHandles[TabIndex].bIsDirty = true;
	}
}

void UStashSubsystem::FlushDirtyTabs()
{
	bool bAnySaved = false;
	for (int32 i = 0; i < TabHandles.Num(); ++i)
	{
		if (TabHandles[i].bIsDirty && TabHandles[i].bIsLoaded)
		{
			if (SaveTab(i))
			{
				TabHandles[i].bIsDirty = false;
				bAnySaved = true;
				UE_LOG(LogStashSubsystem, Log,
					TEXT("FlushDirtyTabs: Saved tab '%s'"), *TabHandles[i].TabID.ToString());
			}
		}
	}

	if (bAnySaved || bHandlesDirty)
	{
		if (SaveHandles())
		{
			bHandlesDirty = false;
		}
	}
}

void UStashSubsystem::UnloadCleanTabs()
{
	for (int32 i = 0; i < TabHandles.Num(); ++i)
	{
		if (TabHandles[i].bIsLoaded && !TabHandles[i].bIsDirty)
		{
			LoadedTabs.Remove(TabHandles[i].TabID);
			TabHandles[i].bIsLoaded = false;
			UE_LOG(LogStashSubsystem, Log,
				TEXT("UnloadCleanTabs: Unloaded clean tab '%s'"),
				*TabHandles[i].TabID.ToString());
		}
	}
}

int32 UStashSubsystem::AddTab(const FText& Name, EStashTabType Type)
{
	const FName NewID = *FString::Printf(TEXT("Tab_%d"), TabHandles.Num());
	TabHandles.Add(FStashTabHandle(NewID, Name, Type));
	bHandlesDirty = true;
	return TabHandles.Num() - 1;
}

void UStashSubsystem::RenameTab(int32 TabIndex, const FText& NewName)
{
	if (IsValidTabIndex(TabIndex))
	{
		TabHandles[TabIndex].TabName = NewName;
		bHandlesDirty = true;
	}
}

FString UStashSubsystem::BuildTabSlotName(FName TabID) const
{
	return ActiveSlotName + TEXT("_Stash_") + TabID.ToString();
}

bool UStashSubsystem::FindFreeGridPosition(const FStashTabData& Tab, FIntPoint& OutPos) const
{
	TSet<FIntPoint> Occupied;
	for (const FStashItemEntry& Entry : Tab.Items)
	{
		Occupied.Add(Entry.GridPosition);
	}

	for (int32 Row = 0; Row < Tab.GridSize.Y; ++Row)
	{
		for (int32 Col = 0; Col < Tab.GridSize.X; ++Col)
		{
			const FIntPoint Pos(Col, Row);
			if (!Occupied.Contains(Pos))
			{
				OutPos = Pos;
				return true;
			}
		}
	}
	return false;
}

bool UStashSubsystem::SaveTab(int32 TabIndex)
{
	if (!IsValidTabIndex(TabIndex) || ActiveSlotName.IsEmpty())
	{
		return false;
	}

	const FStashTabHandle& Handle = TabHandles[TabIndex];
	const FStashTabData* TabData = LoadedTabs.Find(Handle.TabID);
	if (!TabData)
	{
		return false;
	}

	UStashTabSaveGame* SaveObj = Cast<UStashTabSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UStashTabSaveGame::StaticClass()));
	if (!SaveObj)
	{
		return false;
	}

	SaveObj->TabID    = TabData->TabID;
	SaveObj->GridSize = TabData->GridSize;
	bool bAllItemsSerialized = true;

	for (const FStashItemEntry& Entry : TabData->Items)
	{
		if (!Entry.Item)
		{
			continue;
		}

		FStashItemSaveData ItemSave;
		ItemSave.GridPosition  = Entry.GridPosition;
		ItemSave.ItemClassPath = FSoftClassPath(Entry.Item->GetClass());

		TArray<uint8> Bytes;
		FMemoryWriter MemWriter(Bytes, true);
		FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
		Ar.ArIsSaveGame = true;
		Entry.Item->Serialize(Ar);
		if (Ar.IsError() || Bytes.IsEmpty())
		{
			UE_LOG(LogStashSubsystem, Error,
				TEXT("SaveTab: Failed to serialize item '%s' in tab '%s'."),
				*Entry.Item->GetName(), *Handle.TabID.ToString());
			bAllItemsSerialized = false;
			continue;
		}
		ItemSave.ItemBytes = MoveTemp(Bytes);

		SaveObj->Items.Add(MoveTemp(ItemSave));
	}
	if (!bAllItemsSerialized)
	{
		UE_LOG(LogStashSubsystem, Error,
			TEXT("SaveTab: Aborted slot write for '%s' so unserializable items are not silently lost."),
			*Handle.TabID.ToString());
		return false;
	}

	const FString SlotName = BuildTabSlotName(Handle.TabID);
	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, 0);
	if (!bSaved)
	{
		UE_LOG(LogStashSubsystem, Error, TEXT("SaveTab: Failed to write slot '%s'."), *SlotName);
		return false;
	}

	UE_LOG(LogStashSubsystem, Log,
		TEXT("SaveTab: Saved %d items for tab '%s' -> slot '%s'"),
		SaveObj->Items.Num(), *Handle.TabID.ToString(), *SlotName);
	return true;
}

bool UStashSubsystem::SaveHandles()
{
	UStashHandlesSaveGame* SaveObj = Cast<UStashHandlesSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UStashHandlesSaveGame::StaticClass()));
	if (!SaveObj)
	{
		return false;
	}

	for (const FStashTabHandle& Handle : TabHandles)
	{
		FStashTabHandleSaveData Data;
		Data.TabID           = Handle.TabID;
		Data.TabName         = Handle.TabName;
		Data.TabType         = Handle.TabType;
		Data.CachedItemCount = Handle.CachedItemCount;
		Data.AccentColor     = Handle.AccentColor;
		SaveObj->Handles.Add(Data);
	}

	const FString Slot = ActiveSlotName + TEXT("_StashHandles");
	if (!UGameplayStatics::SaveGameToSlot(SaveObj, Slot, 0))
	{
		UE_LOG(LogStashSubsystem, Error, TEXT("SaveHandles: Failed to write slot '%s'."), *Slot);
		return false;
	}

	UE_LOG(LogStashSubsystem, Log,
		TEXT("SaveHandles: Saved %d handles -> slot '%s'"),
		SaveObj->Handles.Num(), *Slot);
	return true;
}

bool UStashSubsystem::LoadTab(int32 TabIndex)
{
	const FString SlotName = BuildTabSlotName(TabHandles[TabIndex].TabID);
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		return false;
	}

	USaveGame* Raw = UGameplayStatics::LoadGameFromSlot(SlotName, 0);
	UStashTabSaveGame* TabSave = Raw ? Cast<UStashTabSaveGame>(Raw) : nullptr;
	if (!TabSave)
	{
		UE_LOG(LogStashSubsystem, Warning,
			TEXT("LoadTab: Failed to load/cast save game from slot '%s'"), *SlotName);
		return false;
	}
	if (TabSave->SaveVersion > UStashTabSaveGame::CurrentSaveVersion)
	{
		UE_LOG(LogStashSubsystem, Error,
			TEXT("LoadTab: Save version %d is newer than supported version %d in slot '%s'."),
			TabSave->SaveVersion, UStashTabSaveGame::CurrentSaveVersion, *SlotName);
		return false;
	}

	const FName ExpectedTabID = TabHandles[TabIndex].TabID;
	FStashTabData TabData(ExpectedTabID);
	TabData.GridSize.X = FMath::Clamp(TabSave->GridSize.X, 1, 64);
	TabData.GridSize.Y = FMath::Clamp(TabSave->GridSize.Y, 1, 64);
	TSet<FIntPoint> OccupiedPositions;

	for (const FStashItemSaveData& ItemSave : TabSave->Items)
	{
		if (ItemSave.ItemBytes.IsEmpty()
			|| ItemSave.GridPosition.X < 0 || ItemSave.GridPosition.Y < 0
			|| ItemSave.GridPosition.X >= TabData.GridSize.X
			|| ItemSave.GridPosition.Y >= TabData.GridSize.Y
			|| OccupiedPositions.Contains(ItemSave.GridPosition))
		{
			UE_LOG(LogStashSubsystem, Warning,
				TEXT("LoadTab: Skipping invalid or overlapping item entry at (%d,%d) in '%s'."),
				ItemSave.GridPosition.X, ItemSave.GridPosition.Y, *SlotName);
			continue;
		}

		UClass* ItemClass = ItemSave.ItemClassPath.TryLoadClass<UItemInstance>();
		if (!ItemClass)
		{
			UE_LOG(LogStashSubsystem, Warning,
				TEXT("LoadTab: Could not resolve item class '%s' - skipping"),
				*ItemSave.ItemClassPath.ToString());
			continue;
		}

		UItemInstance* Item = NewObject<UItemInstance>(this, ItemClass);
		if (!Item)
		{
			continue;
		}

		FMemoryReader MemReader(ItemSave.ItemBytes, true);
		FObjectAndNameAsStringProxyArchive Ar(MemReader, true);
		Ar.ArIsSaveGame = true;
		Item->Serialize(Ar);
		if (Ar.IsError())
		{
			UE_LOG(LogStashSubsystem, Warning,
				TEXT("LoadTab: Item payload was corrupt in slot '%s' - skipping."), *SlotName);
			continue;
		}

		// Run any version migration (added in ItemInstance serialization versioning)
		Item->PostLoadInit();

		TabData.Items.Add(FStashItemEntry(Item, ItemSave.GridPosition));
		OccupiedPositions.Add(ItemSave.GridPosition);
	}

	LoadedTabs.Add(TabData.TabID, MoveTemp(TabData));
	TabHandles[TabIndex].CachedItemCount = LoadedTabs[ExpectedTabID].Items.Num();

	UE_LOG(LogStashSubsystem, Log,
		TEXT("LoadTab: Loaded %d items from slot '%s'"),
		LoadedTabs[TabHandles[TabIndex].TabID].Items.Num(), *SlotName);
	return true;
}

bool UStashSubsystem::IsValidTabIndex(int32 TabIndex) const
{
	return TabHandles.IsValidIndex(TabIndex);
}

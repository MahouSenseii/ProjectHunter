#include "Item/ItemInitializationHandler.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Item/Generation/AffixGenerator.h"
#include "Item/ItemInstance.h"
#include "Item/ItemStackingHandler.h"
#include "Systems/Item/Library/ItemLog.h"

bool FItemInitializationHandler::MigrateToCurrentVersion(UItemInstance& Item)
{
	if (Item.SerializationVersion >= UItemInstance::ITEM_CURRENT_VERSION)
	{
		return false;
	}

	bool bMigrated = false;

	if (Item.SerializationVersion < 1)
	{
		Item.SerializationVersion = 1;
		bMigrated = true;
	}

	if (bMigrated)
	{
		UE_LOG(LogItemInstance, Log,
			TEXT("MigrateToCurrentVersion: item '%s' migrated to version %d"),
			*Item.UniqueID.ToString(), Item.SerializationVersion);
	}

	return bMigrated;
}

void FItemInitializationHandler::PostLoadInit(UItemInstance& Item)
{
	MigrateToCurrentVersion(Item);
	Item.InvalidateBaseCache();
}

void FItemInitializationHandler::Initialize(UItemInstance& Item, FDataTableRowHandle InBaseItemHandle, int32 InItemLevel, EItemRarity InRarity, bool bGenerateAffixes)
{
	InitializeWithCorruption(
		Item,
		InBaseItemHandle,
		InItemLevel,
		InRarity,
		bGenerateAffixes,
		0.0f,
		false);
}

void FItemInitializationHandler::InitializeWithCorruption(UItemInstance& Item, FDataTableRowHandle InBaseItemHandle, int32 InItemLevel, EItemRarity InRarity, bool bGenerateAffixes, float CorruptionChance, bool bForceCorrupted)
{
	Item.BaseItemHandle = InBaseItemHandle;
	Item.ItemLevel = FMath::Clamp(InItemLevel, 1, 100);
	Item.Rarity = InRarity;
	CorruptionChance = FMath::Clamp(CorruptionChance, 0.0f, 1.0f);

	if (!Item.HasValidBaseData())
	{
		PH_LOG_ERROR(LogItemInstance, "InitializeWithCorruption failed: Invalid base item handle %s.",
			*InBaseItemHandle.RowName.ToString());
		return;
	}

	FItemBase* Base = Item.GetBaseData();
	if (!Base)
	{
		return;
	}

	if (Item.Rarity == EItemRarity::IR_None)
	{
		Item.Rarity = Base->ItemRarity;
	}

	switch (Base->ItemType)
	{
	case EItemType::IT_Weapon:
	case EItemType::IT_Armor:
	case EItemType::IT_Accessory:
		{
			Item.Durability = FItemDurability();
			Item.Durability.SetMaxDurability(Base->MaxDurability);

			if (bGenerateAffixes && Item.Rarity > EItemRarity::IR_GradeF)
			{
				FAffixGenerator Generator;
				Item.Stats = Generator.GenerateAffixes(
					*Base,
					Item.ItemLevel,
					Item.Rarity,
					Item.Seed,
					CorruptionChance,
					bForceCorrupted);

				CalculateCorruptionState(Item);
			}
			else
			{
				Item.Stats.Implicits = Base->ImplicitMods;
				for (FPHAttributeData& Implicit : Item.Stats.Implicits)
				{
					Implicit.RollValue();
					Implicit.GenerateUID();
				}
			}

			Item.bIdentified = !Base->bCanBeIdentified;
			break;
		}

	case EItemType::IT_Consumable:
		Item.Quantity = 1;
		Item.RemainingUses = Base->ConsumableData.MaxUses > 0 ? Base->ConsumableData.MaxUses : 1;
		Item.bIdentified = true;
		break;

	case EItemType::IT_Material:
	case EItemType::IT_Currency:
		Item.Quantity = 1;
		Item.bIdentified = true;
		break;

	case EItemType::IT_Quest:
	case EItemType::IT_Key:
		Item.Quantity = 1;
		Item.bIsKeyItem = true;
		Item.bIsTradeable = false;
		Item.bIsSoulbound = true;
		Item.bIdentified = true;
		break;

	default:
		Item.Quantity = 1;
		Item.bIdentified = true;
		break;
	}

	const EItemType ResolvedType = Base->ItemType;
	if (ResolvedType != EItemType::IT_Quest && ResolvedType != EItemType::IT_Key)
	{
		Item.bIsTradeable = Base->bIsTradeable;
	}

	FItemStackingHandler::UpdateTotalWeight(Item);
	Item.bCacheDirty = true;
}

void FItemInitializationHandler::CalculateCorruptionState(UItemInstance& Item)
{
	Item.bHasCorruptedAffixes = false;
	Item.TotalCorruptionPoints = 0;

	for (const FPHAttributeData& Affix : Item.Stats.Prefixes)
	{
		const int32 Points = Affix.GetRankPointValue();
		if (Points < 0)
		{
			Item.bHasCorruptedAffixes = true;
			Item.TotalCorruptionPoints += Points;
		}
	}

	for (const FPHAttributeData& Affix : Item.Stats.Suffixes)
	{
		const int32 Points = Affix.GetRankPointValue();
		if (Points < 0)
		{
			Item.bHasCorruptedAffixes = true;
			Item.TotalCorruptionPoints += Points;
		}
	}

	for (const FPHAttributeData& Affix : Item.Stats.Crafted)
	{
		const int32 Points = Affix.GetRankPointValue();
		if (Points < 0)
		{
			Item.bHasCorruptedAffixes = true;
			Item.TotalCorruptionPoints += Points;
		}
	}

	if (Item.bHasCorruptedAffixes)
	{
		Item.bCanBeModified = false;
		UE_LOG(LogItemInstance, Log, TEXT("ItemInstance: Corruption detected! Points: %d"), Item.TotalCorruptionPoints);
	}
}

TArray<FPHAttributeData> FItemInitializationHandler::GetCorruptedAffixes(const UItemInstance& Item)
{
	TArray<FPHAttributeData> Corrupted;

	for (const FPHAttributeData& Affix : Item.Stats.Prefixes)
	{
		if (Affix.IsCorruptedAffix())
		{
			Corrupted.Add(Affix);
		}
	}

	for (const FPHAttributeData& Affix : Item.Stats.Suffixes)
	{
		if (Affix.IsCorruptedAffix())
		{
			Corrupted.Add(Affix);
		}
	}

	for (const FPHAttributeData& Affix : Item.Stats.Crafted)
	{
		if (Affix.IsCorruptedAffix())
		{
			Corrupted.Add(Affix);
		}
	}

	return Corrupted;
}

void FItemInitializationHandler::PrepareForSave(UItemInstance& Item)
{
	Item.AppliedEffectHandles.Empty();
	Item.bEffectsActive = false;
	Item.InvalidateBaseCache();
}

void FItemInitializationHandler::PostLoadInitialize(UItemInstance& Item)
{
	Item.bCacheDirty = true;
	Item.InvalidateBaseCache();

	if (!Item.HasValidBaseData())
	{
		PH_LOG_ERROR(LogItemInstance, "PostLoad failed: Base data no longer exists for %s.",
			*Item.UniqueID.ToString());
	}

	CalculateCorruptionState(Item);
}

#include "Item/Helpers/ItemInitializationHelper.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Item/Generation/AffixGenerator.h"
#include "Item/ItemInstance.h"
#include "Item/Helpers/ItemStackingHelper.h"
#include "Item/Library/ItemLog.h"
#include "Item/Library/FunctionLibraries/ItemPowerFunctionLibrary.h"

bool FItemInitializationHelper::MigrateToCurrentVersion(UItemInstance& Item)
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

	if (Item.SerializationVersion < 2)
	{
		const bool bRevealAllAffixes = Item.bIdentified || Item.bForceAllAffixesIdentified;
		Item.Stats.SetAllIdentified(bRevealAllAffixes);
		Item.SerializationVersion = 2;
		Item.RefreshIdentificationState();
		bMigrated = true;
	}

	if (Item.SerializationVersion < 3)
	{
		// Older saves predate explicit item-power authoring. Preserve their rough
		// strength until they are recrafted by assigning a conservative legacy value.
		Item.Stats.ForEachMutableStat([](FPHAttributeData& Affix)
		{
			if (FMath::IsNearlyZero(Affix.PowerValue))
			{
				const int32 Rank = Affix.GetRankPointValue();
				Affix.PowerValue = (Affix.IsCorruptedAffix() || Rank < 0)
					? -FMath::Max(1.0f, static_cast<float>(FMath::Abs(Rank)))
					: 10.0f + (2.0f * FMath::Max(0, Rank));
			}
		});
		Item.SerializationVersion = 3;
		bMigrated = true;
	}

	if (Item.SerializationVersion < 4)
	{
		// Legacy range modifiers only stored one rolled value. Preserve their
		// effective value for both endpoints until the item is rerolled.
		Item.Stats.ForEachMutableStat([](FPHAttributeData& Affix)
		{
			if (Affix.UsesValueRange())
			{
				Affix.RolledSecondaryStatValue = Affix.RolledStatValue;
			}
		});
		Item.SerializationVersion = 4;
		bMigrated = true;
	}

	if (Item.SerializationVersion < 5)
	{
		// Version 5 adds explicit gain-as-extra semantics to conversion affixes.
		// Existing conversion affixes remain ordinary conversion by default.
		Item.SerializationVersion = 5;
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

void FItemInitializationHelper::PostLoadInit(UItemInstance& Item)
{
	MigrateToCurrentVersion(Item);
	Item.InvalidateBaseCache();
	CalculateCorruptionState(Item);
	Item.RefreshIdentificationState();
	UItemPowerFunctionLibrary::RecalculateItemGrade(&Item);
}

void FItemInitializationHelper::Initialize(UItemInstance& Item, FDataTableRowHandle InBaseItemHandle, int32 InItemLevel, EItemRarity InRarity, bool bGenerateAffixes)
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

void FItemInitializationHelper::InitializeWithCorruption(UItemInstance& Item, FDataTableRowHandle InBaseItemHandle, int32 InItemLevel, EItemRarity InRarity, bool bGenerateAffixes, float CorruptionChance, bool bForceCorrupted)
{
	Item.BaseItemHandle = InBaseItemHandle;
	Item.ItemLevel = FMath::Clamp(InItemLevel, 1, 100);
	Item.Rarity = InRarity;
	CorruptionChance = FMath::Clamp(CorruptionChance, 0.0f, 1.0f);

	// Honor the SetSeed contract ("0 = generate random"): items initialized
	// without an explicit seed (e.g. direct Blueprint Initialize calls) get a
	// real random seed HERE so the stored value reproduces this exact item.
	if (Item.Seed == 0)
	{
		Item.Seed = FMath::RandRange(1, MAX_int32 - 1);
	}

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
			Item.bForceAllAffixesIdentified = Base->bForceAllAffixesIdentified;

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
				// Seeded so Grade-F / no-affix items are reproducible too.
				FRandomStream ImplicitStream(Item.Seed);
				Item.Stats.Implicits = Base->ImplicitMods;
				for (FPHAttributeData& Implicit : Item.Stats.Implicits)
				{
					Implicit.RollValue(ImplicitStream);
					Implicit.GenerateUID(ImplicitStream);
				}
			}

			const bool bRevealAllAffixes = !Base->bCanBeIdentified || Base->bForceAllAffixesIdentified;
			Item.Stats.SetAllIdentified(bRevealAllAffixes);
			Item.RefreshIdentificationState();
			UItemPowerFunctionLibrary::RecalculateItemGrade(&Item);
			break;
		}

	case EItemType::IT_Consumable:
		Item.Quantity = 1;
		Item.RemainingUses = Base->ConsumableData.MaxUses > 0 ? Base->ConsumableData.MaxUses : 1;
		Item.bForceAllAffixesIdentified = false;
		Item.bIdentified = true;
		break;

	case EItemType::IT_Material:
	case EItemType::IT_Currency:
		Item.Quantity = 1;
		Item.bForceAllAffixesIdentified = false;
		Item.bIdentified = true;
		break;

	case EItemType::IT_Quest:
	case EItemType::IT_Key:
		Item.Quantity = 1;
		Item.bIsKeyItem = true;
		Item.bIsTradeable = false;
		Item.bIsSoulbound = true;
		Item.bForceAllAffixesIdentified = false;
		Item.bIdentified = true;
		break;

	default:
		Item.Quantity = 1;
		Item.bForceAllAffixesIdentified = false;
		Item.bIdentified = true;
		break;
	}

	const EItemType ResolvedType = Base->ItemType;
	if (ResolvedType != EItemType::IT_Quest && ResolvedType != EItemType::IT_Key)
	{
		Item.bIsTradeable = Base->bIsTradeable;
	}

	FItemStackingHelper::UpdateTotalWeight(Item);
	Item.bCacheDirty = true;
}

void FItemInitializationHelper::CalculateCorruptionState(UItemInstance& Item)
{
	Item.bHasCorruptedAffixes = false;
	Item.TotalCorruptionPoints = 0;

	Item.Stats.ForEachStat([&Item](const FPHAttributeData& Affix)
	{
		const int32 Points = Affix.GetRankPointValue();
		if (Affix.IsCorruptedAffix() || Points < 0)
		{
			Item.bHasCorruptedAffixes = true;
			// Type-authored corruption may intentionally use RP_0. It still locks
			// crafting, while negative rank points continue to describe severity.
			Item.TotalCorruptionPoints += FMath::Min(0, Points);
		}
	});

	if (Item.bHasCorruptedAffixes)
	{
		Item.bCanBeModified = false;
		UE_LOG(LogItemInstance, Log, TEXT("ItemInstance: Corruption detected! Points: %d"), Item.TotalCorruptionPoints);
	}
}

TArray<FPHAttributeData> FItemInitializationHelper::GetCorruptedAffixes(const UItemInstance& Item)
{
	TArray<FPHAttributeData> Corrupted;
	Item.Stats.ForEachStat([&Corrupted](const FPHAttributeData& Affix)
	{
		if (Affix.IsCorruptedAffix() || Affix.GetRankPointValue() < 0)
		{
			Corrupted.Add(Affix);
		}
	});

	return Corrupted;
}

void FItemInitializationHelper::PrepareForSave(UItemInstance& Item)
{
	Item.AppliedEffectHandles.Empty();
	Item.bEffectsActive = false;
	Item.InvalidateBaseCache();
}

void FItemInitializationHelper::PostLoadInitialize(UItemInstance& Item)
{
	Item.bCacheDirty = true;
	Item.InvalidateBaseCache();

	if (!Item.HasValidBaseData())
	{
		PH_LOG_ERROR(LogItemInstance, "PostLoad failed: Base data no longer exists for %s.",
			*Item.UniqueID.ToString());
	}

	CalculateCorruptionState(Item);
	Item.RefreshIdentificationState();
	UItemPowerFunctionLibrary::RecalculateItemGrade(&Item);
}

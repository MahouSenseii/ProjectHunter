#include "Item/ItemInstance.h"

#include "Engine/Texture2D.h"
#include "Item/Helpers/ItemInitializationHelper.h"
#include "Item/ItemNameBuilder.h"
#include "Item/Helpers/ItemStackingHelper.h"
#include "Item/Helpers/ItemUsageHelper.h"
#include "Item/ItemValueCalculator.h"
#include "Item/Library/FunctionLibraries/ItemEnumFunctionLibrary.h"
#include "Item/Library/FunctionLibraries/ItemReinforcementFunctionLibrary.h"
#include "Item/Library/FunctionLibraries/ItemPowerFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Item/Library/ItemLog.h"

DEFINE_LOG_CATEGORY(LogItemInstance);

UItemInstance::UItemInstance()
{
	UniqueID = FGuid::NewGuid();
	Seed = FMath::Rand();
}

void UItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UItemInstance, BaseItemHandle);
	DOREPLIFETIME(UItemInstance, UniqueID);
	DOREPLIFETIME(UItemInstance, Seed);
	DOREPLIFETIME(UItemInstance, SerializationVersion);
	DOREPLIFETIME(UItemInstance, Quantity);
	DOREPLIFETIME(UItemInstance, TotalWeight);
	DOREPLIFETIME(UItemInstance, ItemLevel);
	DOREPLIFETIME(UItemInstance, Rarity);
	DOREPLIFETIME(UItemInstance, ItemPowerScore);
	DOREPLIFETIME(UItemInstance, bIdentified);
	DOREPLIFETIME(UItemInstance, bForceAllAffixesIdentified);
	DOREPLIFETIME(UItemInstance, DisplayName);
	DOREPLIFETIME(UItemInstance, bHasNameBeenGenerated);
	DOREPLIFETIME(UItemInstance, Stats);
	DOREPLIFETIME(UItemInstance, RemainingUses);
	DOREPLIFETIME(UItemInstance, LastUseTime);
	DOREPLIFETIME(UItemInstance, Durability);
	DOREPLIFETIME(UItemInstance, bHasCorruptedAffixes);
	DOREPLIFETIME(UItemInstance, TotalCorruptionPoints);
	DOREPLIFETIME(UItemInstance, bCanBeModified);
	DOREPLIFETIME(UItemInstance, RuneCraftingData);
	DOREPLIFETIME(UItemInstance, QuestID);
	DOREPLIFETIME(UItemInstance, bIsKeyItem);
	DOREPLIFETIME(UItemInstance, bIsTradeable);
	DOREPLIFETIME(UItemInstance, bIsSoulbound);
	DOREPLIFETIME(UItemInstance, ValueModifier);
}

bool UItemInstance::MigrateToCurrentVersion()
{
	return FItemInitializationHelper::MigrateToCurrentVersion(*this);
}

void UItemInstance::PostLoadInit()
{
	FItemInitializationHelper::PostLoadInit(*this);
}

float UItemInstance::GetReinforcementMultiplier() const
{
	return UItemReinforcementFunctionLibrary::CalculateReinforcementMultiplier(Quality);
}

bool UItemInstance::RecalculateItemGrade()
{
	return UItemPowerFunctionLibrary::RecalculateItemGrade(this);
}

void UItemInstance::Initialize(FDataTableRowHandle InBaseItemHandle, int32 InItemLevel, EItemRarity InRarity, bool bGenerateAffixes)
{
	FItemInitializationHelper::Initialize(*this, InBaseItemHandle, InItemLevel, InRarity, bGenerateAffixes);
}

void UItemInstance::InitializeWithCorruption(FDataTableRowHandle InBaseItemHandle, int32 InItemLevel, EItemRarity InRarity, bool bGenerateAffixes, float CorruptionChance, bool bForceCorrupted)
{
	FItemInitializationHelper::InitializeWithCorruption(*this, InBaseItemHandle, InItemLevel, InRarity, bGenerateAffixes, CorruptionChance, bForceCorrupted);
}

void UItemInstance::CalculateCorruptionState()
{
	FItemInitializationHelper::CalculateCorruptionState(*this);
}

TArray<FPHAttributeData> UItemInstance::GetCorruptedAffixes() const
{
	return FItemInitializationHelper::GetCorruptedAffixes(*this);
}

FText UItemInstance::GetDisplayName()
{
	return FItemNameBuilder::GetDisplayName(*this);
}

void UItemInstance::RegenerateDisplayName()
{
	FItemNameBuilder::RegenerateDisplayName(*this);
}

FText UItemInstance::GenerateRareName() const
{
	return FItemNameBuilder::GenerateRareName(*this);
}

UStaticMesh* UItemInstance::GetGroundMesh() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->StaticMesh.Get() : nullptr;
}

USkeletalMesh* UItemInstance::GetEquippedMesh() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->SkeletalMesh.Get() : nullptr;
}

UTexture2D* UItemInstance::GetInventoryIcon() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->ItemImage : nullptr;
}

FLinearColor UItemInstance::GetRarityColor() const
{
	if (bHasCorruptedAffixes)
	{
		return FLinearColor(0.5f, 0.0f, 0.3f, 1.0f);
	}

	return UItemEnumFunctionLibrary::GetItemRarityColor(Rarity);
}

FText UItemInstance::GetBaseItemName() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->ItemName : FText::FromString("Unknown");
}

EItemType UItemInstance::GetItemType() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->ItemType : EItemType::IT_None;
}

EItemSubType UItemInstance::GetItemSubType() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->ItemSubType : EItemSubType::IST_None;
}

EEquipmentSlot UItemInstance::GetEquipmentSlot() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->EquipmentSlot : EEquipmentSlot::ES_None;
}

int32 UItemInstance::GetMaxStackSize() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->MaxStackSize : 1;
}

float UItemInstance::GetBaseWeight() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->BaseWeight : 0.0f;
}

bool UItemInstance::bIsTwoHanded() const
{
	FItemBase* Base = GetBaseData();
	if (!Base)
	{
		return false;
	}

	return Base->WeaponHandle == EWeaponHandle::WH_TwoHanded;
}

void UItemInstance::UpdateTotalWeight()
{
	FItemStackingHelper::UpdateTotalWeight(*this);
}

void UItemInstance::ApplyAffixesToCharacter(UAbilitySystemComponent* ASC)
{
	FItemUsageHelper::ApplyAffixesToCharacter(*this, ASC);
}

void UItemInstance::RemoveAffixesFromCharacter(UAbilitySystemComponent* ASC)
{
	FItemUsageHelper::RemoveAffixesFromCharacter(*this, ASC);
}

bool UItemInstance::UseConsumable(AActor* Target)
{
	return FItemUsageHelper::UseConsumable(*this, Target);
}

bool UItemInstance::CanUseConsumable() const
{
	return FItemUsageHelper::CanUseConsumable(*this);
}

float UItemInstance::GetCooldownProgress() const
{
	return FItemUsageHelper::GetCooldownProgress(*this);
}

bool UItemInstance::ReduceUses(int32 Amount)
{
	return FItemUsageHelper::ReduceUses(*this, Amount);
}

bool UItemInstance::ApplyConsumableEffects(AActor* Target)
{
	return FItemUsageHelper::ApplyConsumableEffects(*this, Target);
}

void UItemInstance::Identify()
{
	IdentifyAllAffixes();
}

bool UItemInstance::IdentifyAffix(FGuid AffixUID)
{
	if (!IsEquipment() || !AffixUID.IsValid())
	{
		return false;
	}

	const bool bIdentifiedAffix = Stats.IdentifyStatByUID(AffixUID);
	if (bIdentifiedAffix)
	{
		RefreshIdentificationState();
		RegenerateDisplayName();
	}

	return bIdentifiedAffix;
}

void UItemInstance::IdentifyAllAffixes()
{
	if (!IsEquipment())
	{
		return;
	}

	Stats.SetAllIdentified(true);
	RefreshIdentificationState();
	RegenerateDisplayName();
}

void UItemInstance::SetForceAllAffixesIdentified(bool bInForceAllAffixesIdentified)
{
	bForceAllAffixesIdentified = bInForceAllAffixesIdentified;
	if (bForceAllAffixesIdentified)
	{
		Stats.SetAllIdentified(true);
	}

	RefreshIdentificationState();
	RegenerateDisplayName();
}

void UItemInstance::RefreshIdentificationState()
{
	if (bForceAllAffixesIdentified)
	{
		Stats.SetAllIdentified(true);
	}

	bIdentified = !HasUnidentifiedAffixes();
}

bool UItemInstance::IsIdentified() const
{
	return !HasUnidentifiedAffixes();
}

bool UItemInstance::HasUnidentifiedAffixes() const
{
	return IsEquipment() && !bForceAllAffixesIdentified && Stats.HasUnidentifiedStats();
}

bool UItemInstance::IsEquipment() const
{
	const EItemType Type = GetItemType();
	return Type == EItemType::IT_Weapon ||
		Type == EItemType::IT_Armor ||
		Type == EItemType::IT_Accessory;
}

bool UItemInstance::IsConsumable() const
{
	return GetItemType() == EItemType::IT_Consumable;
}

bool UItemInstance::IsMaterial() const
{
	return GetItemType() == EItemType::IT_Material;
}

bool UItemInstance::IsQuestItem() const
{
	return GetItemType() == EItemType::IT_Quest || bIsKeyItem;
}

bool UItemInstance::IsCurrency() const
{
	return GetItemType() == EItemType::IT_Currency;
}

bool UItemInstance::IsKeyItem() const
{
	return GetItemType() == EItemType::IT_Key || bIsKeyItem;
}

bool UItemInstance::CanBeEquipped() const
{
	return IsEquipment() && !IsBroken();
}

bool UItemInstance::IsStackable() const
{
	return FItemStackingHelper::IsStackable(*this);
}

bool UItemInstance::CanStackWith(const UItemInstance* Other) const
{
	return FItemStackingHelper::CanStackWith(*this, Other);
}

bool UItemInstance::IsConsumed() const
{
	return FItemStackingHelper::IsConsumed(*this);
}

int32 UItemInstance::AddToStack(int32 Amount)
{
	return FItemStackingHelper::AddToStack(*this, Amount);
}

int32 UItemInstance::RemoveFromStack(int32 Amount)
{
	return FItemStackingHelper::RemoveFromStack(*this, Amount);
}

UItemInstance* UItemInstance::SplitStack(int32 Amount)
{
	return FItemStackingHelper::SplitStack(*this, Amount);
}

int32 UItemInstance::GetRemainingStackSpace() const
{
	return FItemStackingHelper::GetRemainingStackSpace(*this);
}

int32 UItemInstance::GetCalculatedValue() const
{
	return FItemValueCalculator::GetCalculatedValue(*this);
}

int32 UItemInstance::GetSellValue(float SellPercentage) const
{
	return FItemValueCalculator::GetSellValue(*this, SellPercentage);
}

FItemBase* UItemInstance::GetBaseData() const
{
	if (!bCacheDirty && CachedBaseData)
	{
		return CachedBaseData;
	}

	if (!BaseItemHandle.IsNull())
	{
		CachedBaseData = BaseItemHandle.GetRow<FItemBase>("GetBaseData");
		bCacheDirty = false;
		return CachedBaseData;
	}

	return nullptr;
}

bool UItemInstance::GetBaseDataBP(FItemBase& OutBaseData) const
{
	FItemBase* Base = GetBaseData();
	if (Base)
	{
		OutBaseData = *Base;
		return true;
	}

	return false;
}

bool UItemInstance::HasValidBaseData() const
{
	return GetBaseData() != nullptr;
}

void UItemInstance::InvalidateBaseCache()
{
	bCacheDirty = true;
	CachedBaseData = nullptr;
}

void UItemInstance::PrepareForSave()
{
	FItemInitializationHelper::PrepareForSave(*this);
}

void UItemInstance::PostLoadInitialize()
{
	FItemInitializationHelper::PostLoadInitialize(*this);
}

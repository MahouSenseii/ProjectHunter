#include "Item/Library/FunctionLibraries/ItemFunctionLibrary.h"
#include "Item/ItemInstance.h"
#include "Item/Library/FunctionLibraries/ItemAffixFunctionLibrary.h"
#include "Item/Library/FunctionLibraries/ItemBaseFunctionLibrary.h"
#include "Item/Library/FunctionLibraries/ItemCalculationFunctionLibrary.h"
#include "Item/Library/FunctionLibraries/ItemComparisonFunctionLibrary.h"
#include "Item/Library/FunctionLibraries/ItemEnumFunctionLibrary.h"
#include "Item/Library/FunctionLibraries/ItemNameFunctionLibrary.h"
#include "Item/Library/FunctionLibraries/ItemRequirementFunctionLibrary.h"

FLinearColor UItemFunctionLibrary::GetRarityColor(EItemRarity Rarity)
{
	return UItemEnumFunctionLibrary::GetItemRarityColor(Rarity);
}

FText UItemFunctionLibrary::GetRarityDisplayName(EItemRarity Rarity)
{
	return UItemEnumFunctionLibrary::GetItemRarityDisplayName(Rarity);
}

FText UItemFunctionLibrary::GetAffixCountText(EItemRarity Rarity)
{
	return UItemAffixFunctionLibrary::GetAffixCountText(Rarity);
}

FString UItemFunctionLibrary::FormatAffixValue(
	float Value,
	EAttributeDisplayFormat Format,
	FName AttributeName,
	float MinValue,
	float MaxValue,
	const FText& CustomText)
{
	return UItemAffixFunctionLibrary::FormatAffixValue(
		Value,
		Format,
		AttributeName,
		MinValue,
		MaxValue,
		CustomText);
}

FString UItemFunctionLibrary::FormatAffixText(const FPHAttributeData& Affix)
{
	return UItemAffixFunctionLibrary::FormatAffixText(Affix);
}

FString UItemFunctionLibrary::GetModifyTypeSymbol(EModifyType ModifyType)
{
	return UItemAffixFunctionLibrary::GetModifyTypeSymbol(ModifyType);
}

int32 UItemFunctionLibrary::GetRankPointsValue(ERankPoints Points)
{
	return UItemAffixFunctionLibrary::GetRankPointsValue(Points);
}

FText UItemFunctionLibrary::GetTierName(ERankPoints Points)
{
	return UItemAffixFunctionLibrary::GetTierName(Points);
}

bool UItemFunctionLibrary::CompareAffixRank(const FPHAttributeData& AffixA, const FPHAttributeData& AffixB)
{
	return UItemAffixFunctionLibrary::CompareAffixRank(AffixA, AffixB);
}

FText UItemFunctionLibrary::GenerateItemName(
	const FPHItemStats& ItemStats,
	const FItemBase& ItemBase,
	EItemRarity Rarity)
{
	return UItemNameFunctionLibrary::GenerateItemName(ItemStats, ItemBase, Rarity);
}

FText UItemFunctionLibrary::GenerateLegendaryName(int32 Seed)
{
	return UItemNameFunctionLibrary::GenerateLegendaryName(Seed);
}

FText UItemFunctionLibrary::GetPrefixName(const FPHAttributeData& Affix)
{
	return UItemNameFunctionLibrary::GetPrefixName(Affix);
}

FText UItemFunctionLibrary::GetSuffixName(const FPHAttributeData& Affix)
{
	return UItemNameFunctionLibrary::GetSuffixName(Affix);
}

FDamageRange UItemFunctionLibrary::CalculateFinalDamage(
	FDamageRange BaseDamage,
	float FlatAdded,
	float IncreasedPercent,
	float MorePercent)
{
	return UItemCalculationFunctionLibrary::CalculateFinalDamage(
		BaseDamage,
		FlatAdded,
		IncreasedPercent,
		MorePercent);
}

float UItemFunctionLibrary::CalculateDPS(FDamageRange DamageRange, float AttackSpeed)
{
	return UItemCalculationFunctionLibrary::CalculateDPS(DamageRange, AttackSpeed);
}

FDamageRange UItemFunctionLibrary::CalculateCriticalDamage(
	FDamageRange BaseDamage,
	float CritMultiplier)
{
	return UItemCalculationFunctionLibrary::CalculateCriticalDamage(BaseDamage, CritMultiplier);
}

float UItemFunctionLibrary::CalculateFinalResistance(
	float BaseResistance,
	float FlatAdded,
	float IncreasedPercent)
{
	return UItemCalculationFunctionLibrary::CalculateFinalResistance(
		BaseResistance,
		FlatAdded,
		IncreasedPercent);
}

float UItemFunctionLibrary::CalculateArmorReduction(float Armor, float IncomingDamage)
{
	return UItemCalculationFunctionLibrary::CalculateArmorReduction(Armor, IncomingDamage);
}

float UItemFunctionLibrary::CalculateMaxWeightFromStrength(
	int32 Strength,
	float WeightPerStrength)
{
	return UItemCalculationFunctionLibrary::CalculateMaxWeightFromStrength(Strength, WeightPerStrength);
}

float UItemFunctionLibrary::GetOverweightPercentage(float CurrentWeight, float MaxWeight)
{
	return UItemCalculationFunctionLibrary::GetOverweightPercentage(CurrentWeight, MaxWeight);
}

bool UItemFunctionLibrary::MeetsItemRequirements(
	const FItemStatRequirement& Requirements,
	int32 HunterLevel,
	int32 Strength,
	int32 Dexterity,
	int32 Intelligence,
	int32 Endurance,
	int32 Affliction,
	int32 Luck,
	int32 Covenant)
{
	return UItemRequirementFunctionLibrary::MeetsItemRequirements(
		Requirements,
		HunterLevel,
		Strength,
		Dexterity,
		Intelligence,
		Endurance,
		Affliction,
		Luck,
		Covenant);
}

int32 UItemFunctionLibrary::GetRequiredLevel(const FItemStatRequirement& Requirements)
{
	return UItemRequirementFunctionLibrary::GetRequiredLevel(Requirements);
}

bool UItemFunctionLibrary::IsItemBaseValid(const FItemBase& ItemBase)
{
	return UItemBaseFunctionLibrary::IsItemBaseValid(ItemBase);
}

bool UItemFunctionLibrary::IsItemBaseValidForInventory(const FItemBase& ItemBase)
{
	return UItemBaseFunctionLibrary::IsItemBaseValidForInventory(ItemBase);
}

bool UItemFunctionLibrary::IsItemBaseWeapon(const FItemBase& ItemBase)
{
	return UItemBaseFunctionLibrary::IsItemBaseWeapon(ItemBase);
}

bool UItemFunctionLibrary::IsItemBaseArmor(const FItemBase& ItemBase)
{
	return UItemBaseFunctionLibrary::IsItemBaseArmor(ItemBase);
}

bool UItemFunctionLibrary::IsItemBaseAccessory(const FItemBase& ItemBase)
{
	return UItemBaseFunctionLibrary::IsItemBaseAccessory(ItemBase);
}

bool UItemFunctionLibrary::IsItemBaseEquippable(const FItemBase& ItemBase)
{
	return UItemBaseFunctionLibrary::IsItemBaseEquippable(ItemBase);
}

bool UItemFunctionLibrary::IsItemBaseConsumable(const FItemBase& ItemBase)
{
	return UItemBaseFunctionLibrary::IsItemBaseConsumable(ItemBase);
}

bool UItemFunctionLibrary::IsItemBaseMaterial(const FItemBase& ItemBase)
{
	return UItemBaseFunctionLibrary::IsItemBaseMaterial(ItemBase);
}

bool UItemFunctionLibrary::IsItemBaseCurrency(const FItemBase& ItemBase)
{
	return UItemBaseFunctionLibrary::IsItemBaseCurrency(ItemBase);
}

bool UItemFunctionLibrary::DoesItemBaseUseRuntimeActor(const FItemBase& ItemBase)
{
	return UItemBaseFunctionLibrary::DoesItemBaseUseRuntimeActor(ItemBase);
}

TSubclassOf<AActor> UItemFunctionLibrary::GetItemBaseRuntimeActorClass(const FItemBase& ItemBase)
{
	return UItemBaseFunctionLibrary::GetItemBaseRuntimeActorClass(ItemBase);
}

FName UItemFunctionLibrary::GetItemBaseSocketForContext(const FItemBase& ItemBase, FName Context)
{
	return UItemBaseFunctionLibrary::GetItemBaseSocketForContext(ItemBase, Context);
}

float UItemFunctionLibrary::GetItemBaseCalculatedValue(
	const FItemBase& ItemBase,
	int32 Quantity,
	EItemRarity InstanceRarity)
{
	return UItemBaseFunctionLibrary::GetItemBaseCalculatedValue(ItemBase, Quantity, InstanceRarity);
}

float UItemFunctionLibrary::GetItemBaseTotalWeight(const FItemBase& ItemBase, int32 Quantity)
{
	return UItemBaseFunctionLibrary::GetItemBaseTotalWeight(ItemBase, Quantity);
}

void UItemFunctionLibrary::GetAffixCountByRarity(
	EItemRarity Rarity,
	int32& OutMinPrefixes,
	int32& OutMaxPrefixes,
	int32& OutMinSuffixes,
	int32& OutMaxSuffixes)
{
	UItemAffixFunctionLibrary::GetAffixCountByRarity(
		Rarity,
		OutMinPrefixes,
		OutMaxPrefixes,
		OutMinSuffixes,
		OutMaxSuffixes);
}

float UItemFunctionLibrary::GetRarityValueMultiplier(EItemRarity Rarity)
{
	return UItemCalculationFunctionLibrary::GetRarityValueMultiplier(Rarity);
}

int32 UItemFunctionLibrary::CompareItemDamage(const FItemBase& ItemA, const FItemBase& ItemB)
{
	return UItemComparisonFunctionLibrary::CompareItemDamage(ItemA, ItemB);
}

int32 UItemFunctionLibrary::CompareItemValue(const FItemBase& ItemA, const FItemBase& ItemB)
{
	return UItemComparisonFunctionLibrary::CompareItemValue(ItemA, ItemB);
}

int32 UItemFunctionLibrary::CompareItemInstanceValue(const UItemInstance* ItemA, const UItemInstance* ItemB)
{
	return UItemComparisonFunctionLibrary::CompareItemInstanceValue(ItemA, ItemB);
}

int32 UItemFunctionLibrary::CompareItemInstanceRarity(const UItemInstance* ItemA, const UItemInstance* ItemB)
{
	return UItemComparisonFunctionLibrary::CompareItemInstanceRarity(ItemA, ItemB);
}

int32 UItemFunctionLibrary::CompareItemInstanceWeight(const UItemInstance* ItemA, const UItemInstance* ItemB)
{
	return UItemComparisonFunctionLibrary::CompareItemInstanceWeight(ItemA, ItemB);
}

EDefenseType UItemFunctionLibrary::DamageTypeToResistance(EDamageType DamageType)
{
	return UItemEnumFunctionLibrary::DamageTypeToResistance(DamageType);
}

FText UItemFunctionLibrary::GetItemTypeName(EItemType ItemType)
{
	return UItemEnumFunctionLibrary::GetItemTypeName(ItemType);
}

FText UItemFunctionLibrary::GetItemSubTypeName(EItemSubType SubType)
{
	return UItemEnumFunctionLibrary::GetItemSubTypeName(SubType);
}

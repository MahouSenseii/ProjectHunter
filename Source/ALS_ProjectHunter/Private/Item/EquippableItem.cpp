// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/EquippableItem.h"

#include "AbilitySystem/PHAttributeSet.h"
#include "Character/PHBaseCharacter.h"
#include "Library/PHItemFunctionLibrary.h"

UEquippableItem::UEquippableItem(): StatsDataTable(nullptr)
{
}

void UEquippableItem::Initialize( UItemDefinitionAsset*& ItemInfo)
{
	Super::Initialize(ItemInfo);
	if (!ItemInfo->Equip.Affixes.bAffixesGenerated)
	{
		ItemInfos->Equip.Affixes = UPHItemFunctionLibrary::GenerateStats(StatsDataTable);
		ItemInfos = UPHItemFunctionLibrary::GenerateItemName(ItemInfo->Equip.Affixes, ItemInfo);
	}
}


bool UEquippableItem::CanEquipItem(const APHBaseCharacter* Character) const
{
    if (!Character) return false;

    const UPHAttributeSet* AttributeSet = Cast<UPHAttributeSet>(Character->GetAttributeSet());
    if (!AttributeSet) return false;

    // Check if the player's stats meet the item's requirements
    if (AttributeSet->GetStrength() < ItemInfos->Equip.StatRequirements.RequiredStrength) return false;
    if (AttributeSet->GetIntelligence() < ItemInfos->Equip.StatRequirements.RequiredIntelligence) return false;
    if (AttributeSet->GetDexterity() < ItemInfos->Equip.StatRequirements.RequiredDexterity) return false;
    if (AttributeSet->GetEndurance() < ItemInfos->Equip.StatRequirements.RequiredEndurance) return false;
    if (AttributeSet->GetAffliction() < ItemInfos->Equip.StatRequirements.RequiredAffliction) return false;
    if (AttributeSet->GetLuck() < ItemInfos->Equip.StatRequirements.RequiredLuck) return false;
    if (AttributeSet->GetCovenant() < ItemInfos->Equip.StatRequirements.RequiredCovenant) return false;

    return true;
}

const FItemStatRequirement& UEquippableItem::GetStatRequirements() const
{
	return ItemInfos->Equip.StatRequirements;
}

void UEquippableItem::RerollAllMods()
{
	UPHItemFunctionLibrary::RerollModifiers(this, StatsDataTable, true, true, {});
	UPHItemFunctionLibrary::GenerateItemName(ItemInfos->Equip.Affixes, GetItemInfo());
}

TArray<FPHAttributeData> UEquippableItem::GetItemStats() const
{
	return ItemInfos->Equip.Affixes.GetAllStats();
}

const FPHItemStats& UEquippableItem::GetFullItemStats() const
{
	return ItemInfos->Equip.Affixes;
}




TArray<FPHAttributeData> UEquippableItem::GetItemStatsByTag(FGameplayTag Tag) const
{
	TArray<FPHAttributeData> MatchingStats;

	const auto AllStats = GetItemStats(); // Combines Prefixes + Suffixes

	for (const FPHAttributeData& Stat : AllStats)
	{
		if (Stat.AffectedTags.Contains(Tag))
		{
			MatchingStats.Add(Stat);
		}
	}

	return MatchingStats;
}

TArray<FPHAttributeData> UEquippableItem::GetItemStatsByAffixType(EPrefixSuffix Type) const
{
	TArray<FPHAttributeData> Filtered;

	const auto AllStats = GetItemStats();

	for (const FPHAttributeData& Stat : AllStats)
	{
		if (Stat.PrefixSuffix == Type)
		{
			Filtered.Add(Stat);
		}
	}

	return Filtered;
}

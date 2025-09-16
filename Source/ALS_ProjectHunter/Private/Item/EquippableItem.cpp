// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/EquippableItem.h"

#include "AbilitySystem/PHAttributeSet.h"
#include "Character/PHBaseCharacter.h"
#include "Library/PHItemFunctionLibrary.h"

UEquippableItem::UEquippableItem(): StatsDataTable(nullptr)
{
}

void UEquippableItem::Initialize( FItemInformation& ItemInfo)
{
	Super::Initialize(ItemInfo);
	if (!ItemInfo.ItemData.Affixes.bAffixesGenerated)
	{
		ItemInfos.ItemData.Affixes = UPHItemFunctionLibrary::GenerateStats(StatsDataTable);
		ItemInfos = UPHItemFunctionLibrary::GenerateItemName(ItemInfo.ItemData.Affixes, ItemInfo);
	}
}


bool UEquippableItem::CanEquipItem(const APHBaseCharacter* Character) const
{
    if (!Character) return false;

    const UPHAttributeSet* AttributeSet = Cast<UPHAttributeSet>(Character->GetAttributeSet());
    if (!AttributeSet) return false;

    // Check if the player's stats meet the item's requirements
    if (AttributeSet->GetStrength() < ItemInfos.ItemData.StatRequirements.RequiredStrength) return false;
    if (AttributeSet->GetIntelligence() < ItemInfos.ItemData.StatRequirements.RequiredIntelligence) return false;
    if (AttributeSet->GetDexterity() < ItemInfos.ItemData.StatRequirements.RequiredDexterity) return false;
    if (AttributeSet->GetEndurance() < ItemInfos.ItemData.StatRequirements.RequiredEndurance) return false;
    if (AttributeSet->GetAffliction() < ItemInfos.ItemData.StatRequirements.RequiredAffliction) return false;
    if (AttributeSet->GetLuck() < ItemInfos.ItemData.StatRequirements.RequiredLuck) return false;
    if (AttributeSet->GetCovenant() < ItemInfos.ItemData.StatRequirements.RequiredCovenant) return false;

    return true;
}

FItemStatRequirement UEquippableItem::GetStatRequirements() const
{
	return ItemInfos.ItemData.StatRequirements;
}

void UEquippableItem::RerollAllMods()
{
	UPHItemFunctionLibrary::RerollModifiers(this, StatsDataTable, true, true, {});
	UPHItemFunctionLibrary::GenerateItemName(ItemInfos.ItemData.Affixes, GetItemInfo());
}

TArray<FPHAttributeData> UEquippableItem::GetItemStats() const
{
	return ItemInfos.ItemData.Affixes.GetAllStats();
}

const FPHItemStats& UEquippableItem::GetFullItemStats() const
{
	return ItemInfos.ItemData.Affixes;
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

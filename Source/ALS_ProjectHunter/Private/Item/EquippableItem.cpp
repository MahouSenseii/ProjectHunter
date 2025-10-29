// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/EquippableItem.h"

#include "AbilitySystem/PHAttributeSet.h"
#include "Character/PHBaseCharacter.h"
#include "Library/PHItemFunctionLibrary.h"

UEquippableItem::UEquippableItem()
{
}


void UEquippableItem::Initialize(UItemDefinitionAsset* ItemInfo)
{
	ItemDefinition = ItemInfo;  // Store reference (const)
	
	if (RuntimeData.UniqueID.IsEmpty())
	{
		RuntimeData.UniqueID = FGuid::NewGuid().ToString();
	}
	
	if (!RuntimeData.bHasNameBeenGenerated)
	{
		FPHItemStats CombinedStats;
		CombinedStats.Prefixes = RuntimeData.Prefixes;
		CombinedStats.Suffixes = RuntimeData.Suffixes;
        
		RuntimeData.DisplayName = UPHItemFunctionLibrary::GenerateItemName(
			CombinedStats,
			ItemDefinition
		);
		RuntimeData.bHasNameBeenGenerated = true;
	}
}


bool UEquippableItem::CanEquipItem(const APHBaseCharacter* Character) const
{
    if (!Character) return false;

    const UPHAttributeSet* AttributeSet = Cast<UPHAttributeSet>(Character->GetAttributeSet());
    if (!AttributeSet) return false;

    // Check if the player's stats meet the item's requirements
    if (AttributeSet->GetStrength() < ItemDefinition->Equip.StatRequirements.RequiredStrength) return false;
    if (AttributeSet->GetIntelligence() < ItemDefinition->Equip.StatRequirements.RequiredIntelligence) return false;
    if (AttributeSet->GetDexterity() < ItemDefinition->Equip.StatRequirements.RequiredDexterity) return false;
    if (AttributeSet->GetEndurance() < ItemDefinition->Equip.StatRequirements.RequiredEndurance) return false;
    if (AttributeSet->GetAffliction() < ItemDefinition->Equip.StatRequirements.RequiredAffliction) return false;
    if (AttributeSet->GetLuck() < ItemDefinition->Equip.StatRequirements.RequiredLuck) return false;
    if (AttributeSet->GetCovenant() < ItemDefinition->Equip.StatRequirements.RequiredCovenant) return false;

    return true;
}

const FItemStatRequirement& UEquippableItem::GetStatRequirements() const
{
	return ItemDefinition->Equip.StatRequirements;
}

void UEquippableItem::RerollAllMods()
{
	UPHItemFunctionLibrary::RerollModifiers(this, ItemDefinition->StatsDataTableTable, true, true, {});
	UPHItemFunctionLibrary::GenerateItemName(ItemDefinition->Equip.Affixes, GetItemInfo());
}

TArray<FPHAttributeData> UEquippableItem::GetItemStats() const
{
	return ItemDefinition->Equip.Affixes.GetAllStats();
}

const FPHItemStats& UEquippableItem::GetFullItemStats() const
{
	return ItemDefinition->Equip.Affixes;
}




TArray<FPHAttributeData> UEquippableItem::GetItemStatsByTag(FGameplayTag Tag) const
{
	TArray<FPHAttributeData> MatchingStats;
	const auto AllStats = GetItemStats();
    
	for (const FPHAttributeData& Stat : AllStats)
	{
		if (Stat.AffectedTags.HasTag(Tag))
		{
			MatchingStats.Add(Stat);
		}
	}
    
	return MatchingStats;
}

TArray<FPHAttributeData> UEquippableItem::GetItemStatsByAffixType(const EAffixes Type) const
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

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

	if(!ItemStats.bHasGenerated)
	{
		ItemStats = UPHItemFunctionLibrary::GenerateStats(StatsDataTable);
		ItemInfo = UPHItemFunctionLibrary::GenerateItemName(ItemStats, ItemInfo);
	}
}

bool UEquippableItem::CanEquipItem(const APHBaseCharacter* Character) const
{
    if (!Character) return false;

    const UPHAttributeSet* AttributeSet = Cast<UPHAttributeSet>(Character->GetAttributeSet());
    if (!AttributeSet) return false;

    // Check if the player's stats meet the item's requirements
    if (AttributeSet->GetStrength() < EquippableData.StatRequirements.RequiredStrength) return false;
    if (AttributeSet->GetIntelligence() < EquippableData.StatRequirements.RequiredIntelligence) return false;
    if (AttributeSet->GetDexterity() < EquippableData.StatRequirements.RequiredDexterity) return false;
    if (AttributeSet->GetEndurance() < EquippableData.StatRequirements.RequiredEndurance) return false;
    if (AttributeSet->GetAffliction() < EquippableData.StatRequirements.RequiredAffliction) return false;
    if (AttributeSet->GetLuck() < EquippableData.StatRequirements.RequiredLuck) return false;
    if (AttributeSet->GetCovenant() < EquippableData.StatRequirements.RequiredCovenant) return false;

    return true;
}

FItemStatRequirement UEquippableItem::GetStatRequirements() const
{
	return EquippableData.StatRequirements;
}


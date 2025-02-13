// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/PHItemFunctionLibrary.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Character/PHBaseCharacter.h"
#include "Item/ConsumableItem.h"
#include "AbilitySystemComponent.h"
#include "Item/WeaponItem.h"

bool UPHItemFunctionLibrary::AreItemSlotsEqual(FItemInformation FirstItem, FItemInformation SecondItem)
{
	if((FirstItem.EquipmentSlot == SecondItem.EquipmentSlot)|| (FirstItem.EquipmentSlot == EEquipmentSlot::ES_MainHand && SecondItem.EquipmentSlot == EEquipmentSlot::ES_OffHand)
		|| (SecondItem.EquipmentSlot == EEquipmentSlot::ES_MainHand && FirstItem.EquipmentSlot == EEquipmentSlot::ES_OffHand))
	{

		return  true;
	}


	return false;
}


UBaseItem* UPHItemFunctionLibrary::GetItemInformation(FItemInformation ItemInfo, FEquippableItemData EquippableItemData,
                                                      FWeaponItemData WeaponItemData, FConsumableItemData ConsumableItemData)
{
    switch (ItemInfo.EquipmentSlot)
    {
    case EEquipmentSlot::ES_Belt:
    case EEquipmentSlot::ES_Boots:
    case EEquipmentSlot::ES_Chestplate:
    case EEquipmentSlot::ES_Head:
    case EEquipmentSlot::ES_Neck:
    case EEquipmentSlot::ES_Legs:
    case EEquipmentSlot::ES_Cloak:
    case EEquipmentSlot::ES_Gloves:
    case EEquipmentSlot::ES_Ring:
        return CreateEquippableItem(ItemInfo, EquippableItemData);

    case EEquipmentSlot::ES_Flask:
        return CreateConsumableItem(ItemInfo, ConsumableItemData);

    case EEquipmentSlot::ES_MainHand:
    case EEquipmentSlot::ES_OffHand:
        return CreateWeaponItem(ItemInfo, EquippableItemData, WeaponItemData);

    case EEquipmentSlot::ES_None:
    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown Equipment Slot: %d"), static_cast<int32>(ItemInfo.EquipmentSlot));
        return nullptr;
    }
}

UEquippableItem* UPHItemFunctionLibrary::CreateEquippableItem(const FItemInformation& ItemInfo, const FEquippableItemData& EquippableItemData)
{
    UEquippableItem* NewEquipItem = NewObject<UEquippableItem>();
    NewEquipItem->SetItemInfo(ItemInfo);
    NewEquipItem->SetEquippableData(EquippableItemData);
    return NewEquipItem;
}

UConsumableItem* UPHItemFunctionLibrary::CreateConsumableItem(const FItemInformation& ItemInfo, const FConsumableItemData ConsumableItemData)
{
    UConsumableItem* NewConsumableItem = NewObject<UConsumableItem>();
    NewConsumableItem->SetItemInfo(ItemInfo);
    NewConsumableItem->SetConsumableData(ConsumableItemData);
    return NewConsumableItem;
}

UWeaponItem* UPHItemFunctionLibrary::CreateWeaponItem( const FItemInformation& ItemInfo, const FEquippableItemData& EquippableItemData, const FWeaponItemData& WeaponItemData)
{
    UWeaponItem* NewWeaponItem = NewObject<UWeaponItem>();
    NewWeaponItem->SetItemInfo(ItemInfo);
    NewWeaponItem->SetEquippableData(EquippableItemData);
    NewWeaponItem->SetWeaponData(WeaponItemData);
    return NewWeaponItem;
}

TMap<FString, int> UPHItemFunctionLibrary::CalculateTotalDamage(const int MinDamage, const int MaxDamage, const APHBaseCharacter* Actor)
{
    TMap<FString, int> TotalDamage;

    // Validate Actor and its AttributeSet
    if (!Actor || !Actor->GetAbilitySystemComponent())
    {
        UE_LOG(LogTemp, Warning, TEXT("Actor or AbilitySystemComponent is NULL!"));
        return TotalDamage;
    }

    // Get Attribute Set from Character
    const UPHAttributeSet* OwnerAttributeSet = Cast<UPHAttributeSet>(Actor->GetAttributeSet());
    if (!OwnerAttributeSet)
    {
        UE_LOG(LogTemp, Warning, TEXT("OwnerAttributeSet is NULL!"));
        return TotalDamage;
    }

    // Fetch Global Damage Multiplier
    const float GlobalDamage = OwnerAttributeSet->GetGlobalDamages();

    // Loop through all damage types dynamically
    for (const TPair<FString, FGameplayAttribute>& Pair : OwnerAttributeSet->BaseDamageAttributesMap)
    {
        FString DamageType = Pair.Key;

        // Get Base, Flat, and Percent attributes safely
        const float BaseDamage = OwnerAttributeSet->GetAttributeValue(Pair.Value);
        float FlatBonus = 0.0f;
        float PercentBonus = 0.0f;
        const float ElementalBonus = OwnerAttributeSet->GetElementalDamage();

        // Check if keys exist in maps to prevent crashes
        if (OwnerAttributeSet->FlatDamageAttributesMap.Contains(DamageType))
        {
            FlatBonus = OwnerAttributeSet->GetAttributeValue(OwnerAttributeSet->FlatDamageAttributesMap[DamageType]);
        }

        if (OwnerAttributeSet->PercentDamageAttributesMap.Contains(DamageType))
        {
            PercentBonus = OwnerAttributeSet->GetAttributeValue(OwnerAttributeSet->PercentDamageAttributesMap[DamageType]);
        }

        // Random weapon damage in range from MinDamage to MaxDamage
        const int RandomWeaponDamage = FMath::RandRange(MinDamage, MaxDamage);

        // Calculate final damage
        float BonusDamage = BaseDamage + FlatBonus;
        if (PercentBonus != 0.0f)
        {
            BonusDamage *= (1.0f + PercentBonus);
        }

        if (DamageType != "Physical" && ElementalBonus != 0.0f)
        {
            BonusDamage *= (1.0f + ElementalBonus);
        }

        const float FinalDamage = (RandomWeaponDamage + BonusDamage) * (1.0f + GlobalDamage);
        TotalDamage.Add(DamageType, FMath::Max(1, FMath::RoundToInt(FinalDamage)));
    }

    return TotalDamage;
}

FText UPHItemFunctionLibrary::FormatAttributeText(const FPHAttributeData& AttributeData)
{
    // If the stat is not identified, return masked text
    if (!AttributeData.bIsIdentified)
    {
        return FText::FromString(TEXT("??????"));
    }

    // Roll a value within the given range
    const float RolledValue1 = FMath::RandRange(AttributeData.MinStatChanged, AttributeData.MaxStatChanged);

    // Handle different display formats
    switch (AttributeData.DisplayFormat)
    {
        case EAttributeDisplayFormat::Additive:
            return FText::Format(FText::FromString(TEXT("+{0} TO {1}")),
                                 FText::AsNumber(FMath::RoundToInt(RolledValue1)),
                                 FText::FromName(AttributeData.AttributeName));

        case EAttributeDisplayFormat::Percent:
            return FText::Format(FText::FromString(TEXT("+{0}% TO {1}")),
                                 FText::AsNumber(FMath::RoundToInt(RolledValue1)),
                                 FText::FromName(AttributeData.AttributeName));

        case EAttributeDisplayFormat::MinMax:
            if (AttributeData.bIsRange)
            {
                const float RolledValue2 = FMath::RandRange(AttributeData.MinStatChanged, AttributeData.MaxStatChanged);
                return FText::Format(FText::FromString(TEXT("ADD {0} TO {1} {2}")),
                                     FText::AsNumber(FMath::RoundToInt(RolledValue1)),
                                     FText::AsNumber(FMath::RoundToInt(RolledValue2)),
                                     FText::FromName(AttributeData.AttributeName));
            }
            else
            {
                return FText::Format(FText::FromString(TEXT("+{0} {1}")),
                                     FText::AsNumber(FMath::RoundToInt(RolledValue1)),
                                     FText::FromName(AttributeData.AttributeName));
            }
            
        default:
            return FText::FromString(TEXT("INVALID ATTRIBUTE FORMAT"));
    }
}


void UPHItemFunctionLibrary::IdentifyStat(float ChanceToIdentify, FPHItemStats& StatsToCheck)
{
    TArray<FPHAttributeData*> AllStats;

    // Gather all un-identified stats into a list
    for (FPHAttributeData& Stat : StatsToCheck.PrefixStats)
    {
        if (!Stat.bIsIdentified)
        {
            AllStats.Add(&Stat);
        }
    }

    for (FPHAttributeData& Stat : StatsToCheck.SuffixStats)
    {
        if (!Stat.bIsIdentified)
        {
            AllStats.Add(&Stat);
        }
    }

    // Continue attempting to identify until we fail a roll
    while (AllStats.Num() > 0)
    {
        // Select a random stat
        const int32 RandomIndex = FMath::RandRange(0, AllStats.Num() - 1);
        FPHAttributeData* SelectedStat = AllStats[RandomIndex];

        // Roll to identify
        if (FMath::FRand() <= ChanceToIdentify)
        {
            SelectedStat->bIsIdentified = true;
            ChanceToIdentify *= 0.5f; // Halve the chance after success
        }
        else
        {
            break; // Stop checking once we fail a roll
        }

        // Remove the checked stat so we don’t process it again
        AllStats.RemoveAt(RandomIndex);
    }
}

FPHItemStats UPHItemFunctionLibrary::GenerateStats(const UDataTable* StatsThatCanBeGenerated)
{
    FPHItemStats GeneratedStats;

    if (!StatsThatCanBeGenerated)
    {
        
        return GeneratedStats; // Early exit if no data table
    }

    TArray<FName> RowNames = StatsThatCanBeGenerated->GetRowNames();
    float ChanceToAddStat = 0.5f; // Start at 50% chance

    while (RowNames.Num() > 0)
    {
        // Roll to determine whether we add a stat
        if (FMath::FRand() > ChanceToAddStat)
        {
            break; // Stop generating stats if we fail a roll
        }

        // Pick a random row
        const int32 RandomIndex = FMath::RandRange(0, RowNames.Num() - 1);
        const FName SelectedRow = RowNames[RandomIndex];

        // Retrieve the stat data from the table
        if (const FPHAttributeData* StatData = StatsThatCanBeGenerated->FindRow<FPHAttributeData>(SelectedRow, TEXT("Generating Stats")))
        {
            // Assign stat based on its PrefixSuffix type
            if (StatData->PrefixSuffix == EPrefixSuffix::Prefix)
            {
                GeneratedStats.PrefixStats.Add(*StatData);
            }
            else if (StatData->PrefixSuffix == EPrefixSuffix::Suffix)
            {
                GeneratedStats.SuffixStats.Add(*StatData);
            }
        }

        // Reduce chance for the next stat
        ChanceToAddStat *= 0.5f;

        // Remove used row to prevent duplicate stats
        RowNames.RemoveAt(RandomIndex);
        GeneratedStats.bHasGenerated = true;
    }
    return GeneratedStats;
}

int32 UPHItemFunctionLibrary::GetRankPointsValue(ERankPoints Rank)
{
    return static_cast<int32>(Rank) * 5;
}

EItemRarity UPHItemFunctionLibrary::DetermineWeaponRank(const int32 BaseRankPoints, const FPHItemStats& Stats)
{
    int32 TotalRankPoints = BaseRankPoints; // Start with base weapon points

    // Sum up RankPoints from all prefixes and suffixes
    for (const FPHAttributeData& Prefix : Stats.PrefixStats)
    {
        TotalRankPoints += GetRankPointsValue(Prefix.RankPoints);
    }
    for (const FPHAttributeData& Suffix : Stats.SuffixStats)
    {
        TotalRankPoints += GetRankPointsValue(Suffix.RankPoints);
    }

    // Determine rank based on adjusted thresholds
    if (TotalRankPoints >= 205) return EItemRarity::IR_GradeS;
    if (TotalRankPoints >= 175) return EItemRarity::IR_GradeA;
    if (TotalRankPoints >= 135) return EItemRarity::IR_GradeB;
    if (TotalRankPoints >= 95) return EItemRarity::IR_GradeC;
    if (TotalRankPoints >= 55) return EItemRarity::IR_GradeD;

    return EItemRarity::IR_GradeF; // Default to F if below 55 points
}

FItemInformation UPHItemFunctionLibrary::GenerateItemName(const FPHItemStats& ItemStats, FItemInformation& ItemInfo)
{
    // Initialize highest ranked prefix and suffix
    const FPHAttributeData* HighestPrefix = nullptr;
    const FPHAttributeData* HighestSuffix = nullptr;

    // Find the highest ranking prefix
    for (const FPHAttributeData& Prefix : ItemStats.PrefixStats)
    {
        if (!HighestPrefix || Prefix.RankPoints > HighestPrefix->RankPoints)
        {
            HighestPrefix = &Prefix;
        }
    }

    // Find the highest ranking suffix
    for (const FPHAttributeData& Suffix : ItemStats.SuffixStats)
    {
        if (!HighestSuffix || Suffix.RankPoints > HighestSuffix->RankPoints)
        {
            HighestSuffix = &Suffix;
        }
    }

    // Ensure we have valid names
    const FText PrefixName = (HighestPrefix) ? FText::FromName(HighestPrefix->AttributeName) : FText::FromString(TEXT(""));
    const FText SuffixName = (HighestSuffix) ? FText::FromName(HighestSuffix->AttributeName) : FText::FromString(TEXT(""));

    // Convert enums to readable text
    const FText RarityName = UEnum::GetDisplayValueAsText(ItemInfo.ItemRarity);
    const FText ItemTypeName = UEnum::GetDisplayValueAsText(ItemInfo.ItemType);

    // Construct the full item name
    const FText ItemName = FText::Format(FText::FromString(TEXT("{0} {1} {2} {3}")),
                                         RarityName, PrefixName, ItemTypeName, SuffixName);

    // Assign the generated name to the item
    ItemInfo.ItemName = ItemName;

    return ItemInfo;
}





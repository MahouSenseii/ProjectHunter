// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/PHItemFunctionLibrary.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Character/PHBaseCharacter.h"
#include "Item/ConsumableItem.h"
#include "AbilitySystemComponent.h"
#include "PHGameplayTags.h"
#include "Interactables/Pickups/WeaponPickup.h"
#include "Item/EquippedObject.h"
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


UBaseItem* UPHItemFunctionLibrary::GetItemInformation(
	FItemInformation ItemInfo,
	FEquippableItemData EquippableItemData,
	FConsumableItemData ConsumableItemData)
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
	case EEquipmentSlot::ES_MainHand:
	case EEquipmentSlot::ES_OffHand:
		return CreateEquippableItem(ItemInfo, EquippableItemData);

	case EEquipmentSlot::ES_Flask:
		return CreateConsumableItem(ItemInfo, ConsumableItemData);

	case EEquipmentSlot::ES_None:
	default:
		UE_LOG(LogTemp, Warning, TEXT("Unknown or None Equipment Slot: %d"), static_cast<int32>(ItemInfo.EquipmentSlot));
		return nullptr;
	}
}


UEquippableItem* UPHItemFunctionLibrary::CreateEquippableItem(
	const FItemInformation& ItemInfo,
	const FEquippableItemData& EquippableItemData)
{
	if (!EquippableItemData.EquipClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipClass is not set on EquippableItemData!"));
		return nullptr;
	}

	UEquippableItem* NewEquipItem = NewObject<UEquippableItem>(GetTransientPackage(), EquippableItemData.EquipClass);
	if (!NewEquipItem)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create UEquippableItem object."));
		return nullptr;
	}

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
    for (const TPair<FString, FGameplayAttribute>& Pair : FPHGameplayTags::BaseDamageToAttributesMap)
    {
        FString DamageType = Pair.Key;

        // Get Base, Flat, and Percent attributes safely
        const float BaseDamage = OwnerAttributeSet->GetAttributeValue(Pair.Value);
        float FlatBonus = 0.0f;
        float PercentBonus = 0.0f;
        const float ElementalBonus = OwnerAttributeSet->GetElementalDamage();

        // Check if keys exist in maps to prevent crashes
        if ( FPHGameplayTags::FlatDamageToAttributesMap.Contains(DamageType))
        {
            FlatBonus = OwnerAttributeSet->GetAttributeValue( FPHGameplayTags::FlatDamageToAttributesMap[DamageType]);
        }

        if ( FPHGameplayTags::PercentDamageToAttributesMap.Contains(DamageType))
        {
            PercentBonus = OwnerAttributeSet->GetAttributeValue( FPHGameplayTags::PercentDamageToAttributesMap[DamageType]);
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
	if (!AttributeData.bIsIdentified)
	{
		return FText::FromString(TEXT("??????"));
	}

	const FText StatName = FText::FromName(AttributeData.AttributeName); // Swap to DisplayName if you add that

	const int32 Rolled = FMath::RoundToInt(AttributeData.RolledStatValue);
	const int32 Min = FMath::RoundToInt(AttributeData.MinStatChanged);
	const int32 Max = FMath::RoundToInt(AttributeData.MaxStatChanged);

	switch (AttributeData.DisplayFormat)
	{
	case EAttributeDisplayFormat::Additive:
		return FText::Format(
			FText::FromString(TEXT("+{0} {1}")),
			FText::AsNumber(Rolled),
			StatName
		);

	case EAttributeDisplayFormat::FlatNegative:
		return FText::Format(
			FText::FromString(TEXT("-{0} {1}")),
			FText::AsNumber(Rolled),
			StatName
		);

	case EAttributeDisplayFormat::Percent:
		return FText::Format(
			FText::FromString(TEXT("+{0}% {1}")),
			FText::AsNumber(Rolled),
			StatName
		);

	case EAttributeDisplayFormat::MinMax:
		if (AttributeData.bDisplayAsRange)
		{
			return FText::Format(
				FText::FromString(TEXT("Adds {0}–{1} {2}")),
				FText::AsNumber(Min),
				FText::AsNumber(Max),
				StatName
			);
		}
		else
		{
			return FText::Format(
				FText::FromString(TEXT("+{0} {1}")),
				FText::AsNumber(Rolled),
				StatName
			);
		}

	case EAttributeDisplayFormat::Increase:
		return FText::Format(
			FText::FromString(TEXT("{0}% increased {1}")),
			FText::AsNumber(Rolled),
			StatName
		);

	case EAttributeDisplayFormat::More:
		return FText::Format(
			FText::FromString(TEXT("{0}% more {1}")),
			FText::AsNumber(Rolled),
			StatName
		);

	case EAttributeDisplayFormat::Less:
		return FText::Format(
			FText::FromString(TEXT("{0}% less {1}")),
			FText::AsNumber(Rolled),
			StatName
		);

	case EAttributeDisplayFormat::Chance:
		return FText::Format(
			FText::FromString(TEXT("{0}% chance to {1}")),
			FText::AsNumber(Rolled),
			StatName
		);

	case EAttributeDisplayFormat::Duration:
		return FText::Format(
			FText::FromString(TEXT("{0}s duration to {1}")),
			FText::AsNumber(AttributeData.RolledStatValue), // Duration may be fractional
			StatName
		);

	case EAttributeDisplayFormat::Cooldown:
		return FText::Format(
			FText::FromString(TEXT("{0}s cooldown on {1}")),
			FText::AsNumber(AttributeData.RolledStatValue), // Same here
			StatName
		);

	case EAttributeDisplayFormat::CustomText:
		// Optional: implement a custom text override on your struct like AttributeData.CustomDisplayFormatText
		return FText::FromString(TEXT("Custom Attribute Format"));

	default:
		return FText::FromString(TEXT("Invalid Attribute Format"));
	}
}




void UPHItemFunctionLibrary::IdentifyStat(float ChanceToIdentify, FPHItemStats& StatsToCheck)
{
    TArray<FPHAttributeData*> AllStats;

    // Gather all un-identified stats into a list
    for (FPHAttributeData& Stat : StatsToCheck.Prefixes)
    {
        if (!Stat.bIsIdentified)
        {
            AllStats.Add(&Stat);
        }
    }

    for (FPHAttributeData& Stat : StatsToCheck.Suffixes)
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

	const int32 MaxPrefixes = 3;
	const int32 MaxSuffixes = 3;

	int32 PrefixCount = 0;
	int32 SuffixCount = 0;
	

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
        	if (PrefixCount >= MaxPrefixes && SuffixCount >= MaxSuffixes)
        	{
        		break;
        	}
        	
            // Assign stat based on its PrefixSuffix type
        	if (StatData->PrefixSuffix == EPrefixSuffix::Prefix && PrefixCount < MaxPrefixes)
        	{
        		GeneratedStats.Prefixes.Add(*StatData);
        		PrefixCount++;
        	}
        	else if (StatData->PrefixSuffix == EPrefixSuffix::Suffix && SuffixCount < MaxSuffixes)
        	{
        		GeneratedStats.Suffixes.Add(*StatData);
        		SuffixCount++;
        	}

        }

        // Reduce chance for the next stat
        ChanceToAddStat *= 0.5f;

        // Remove used row to prevent duplicate stats
        RowNames.RemoveAt(RandomIndex);
        GeneratedStats.bAffixesGenerated = true;
    }
    return GeneratedStats;
}

int32 UPHItemFunctionLibrary::GetRankPointsValue(ERankPoints Rank)
{
	switch (Rank)
	{
	case ERankPoints::RP_Neg30: return -30;
	case ERankPoints::RP_Neg25: return -25;
	case ERankPoints::RP_Neg20: return -20;
	case ERankPoints::RP_Neg15: return -15;
	case ERankPoints::RP_Neg10: return -10;
	case ERankPoints::RP_Neg5:  return -5;
	case ERankPoints::RP_0:     return 0;
	case ERankPoints::RP_5:     return 5;
	case ERankPoints::RP_10:    return 10;
	case ERankPoints::RP_15:    return 15;
	case ERankPoints::RP_20:    return 20;
	case ERankPoints::RP_25:    return 25;
	case ERankPoints::RP_30:    return 30;
	default:                       return 0;
	}
}

EItemRarity UPHItemFunctionLibrary::DetermineWeaponRank(const int32 BaseRankPoints, const FPHItemStats& Stats)
{
    int32 TotalRankPoints = BaseRankPoints; // Start with base weapon points

    // Sum up RankPoints from all prefixes and suffixes
    for (const FPHAttributeData& Prefix : Stats.Prefixes)
    {
        TotalRankPoints += GetRankPointsValue(Prefix.RankPoints);
    }
    for (const FPHAttributeData& Suffix : Stats.Suffixes)
    {
        TotalRankPoints += GetRankPointsValue(Suffix.RankPoints);
    }

    // Determine rank based on adjusted thresholds
	if (TotalRankPoints >= 55) return EItemRarity::IR_GradeD;
	if (TotalRankPoints >= 95) return EItemRarity::IR_GradeC;
	if (TotalRankPoints >= 135) return EItemRarity::IR_GradeB;
	if (TotalRankPoints >= 175) return EItemRarity::IR_GradeA;
    if (TotalRankPoints >= 205) return EItemRarity::IR_GradeS;



    return EItemRarity::IR_GradeF; // Default to F if below 55 points
}

FItemInformation UPHItemFunctionLibrary::GenerateItemName(const FPHItemStats& ItemStats, FItemInformation& ItemInfo)
{
	const FText UnknownAffixText = NSLOCTEXT("Item", "UnidentifiedAffix", "???");

	const FPHAttributeData* HighestPrefix = nullptr;
	const FPHAttributeData* HighestSuffix = nullptr;

	for (const FPHAttributeData& Prefix : ItemStats.Prefixes)
	{
		if (!HighestPrefix || Prefix.RankPoints > HighestPrefix->RankPoints)
		{
			HighestPrefix = &Prefix;
		}
	}

	for (const FPHAttributeData& Suffix : ItemStats.Suffixes)
	{
		if (!HighestSuffix || Suffix.RankPoints > HighestSuffix->RankPoints)
		{
			HighestSuffix = &Suffix;
		}
	}

	const FText PrefixName = (HighestPrefix)
		? (HighestPrefix->bIsIdentified
			? FText::FromName(HighestPrefix->AttributeName)
			: UnknownAffixText)
		: FText::GetEmpty();

	const FText SuffixName = (HighestSuffix)
		? (HighestSuffix->bIsIdentified
			? FText::FromName(HighestSuffix->AttributeName)
			: UnknownAffixText)
		: FText::GetEmpty();
	
	const FText ItemSubTypeName = UEnum::GetDisplayValueAsText(ItemInfo.ItemSubType);

	// 🔥 Build final name 
	FString FullName;

	if (!PrefixName.IsEmpty())
	{
		FullName += PrefixName.ToString() + TEXT(" ");
	}

	FullName += ItemSubTypeName.ToString();

	if (!SuffixName.IsEmpty())
	{
		FullName += TEXT(" of ") + SuffixName.ToString();
	}

	ItemInfo.ItemName = FText::FromString(FullName);
	return ItemInfo;
}





void UPHItemFunctionLibrary::RerollModifiers(
	UEquippableItem* Item,
	const UDataTable* ModPool,
	bool bRerollPrefixes,
	bool bRerollSuffixes,
	const TArray<FGuid>& LockedModifiers
)
{
	if (!Item || !ModPool) return;

	FEquippableItemData EquipData = Item->GetEquippableData();
	FPHItemStats& Affixes = EquipData.Affixes;

	if (bRerollPrefixes)
	{
		TArray<FPHAttributeData> NewPrefixes;

		for (const FPHAttributeData& Mod : Affixes.Prefixes)
		{
			if (LockedModifiers.Contains(Mod.ModifierUID))
			{
				NewPrefixes.Add(Mod); // Preserve locked mod
			}
			else
			{
				NewPrefixes.Add(RollSingleMod(ModPool, true));
			}
		}

		Affixes.Prefixes = NewPrefixes;
	}

	if (bRerollSuffixes)
	{
		TArray<FPHAttributeData> NewSuffixes;

		for (const FPHAttributeData& Mod : Affixes.Suffixes)
		{
			if (LockedModifiers.Contains(Mod.ModifierUID))
			{
				NewSuffixes.Add(Mod); // Preserve locked mod
			}
			else
			{
				NewSuffixes.Add(RollSingleMod(ModPool, false));
			}
		}

		Affixes.Suffixes = NewSuffixes;
	}

	// Push updated data back into the item
	Item->SetEquippableData(EquipData);
}




FPHAttributeData UPHItemFunctionLibrary::RollSingleMod(const UDataTable* ModPool, bool bIsPrefix)
{
    TArray<FPHAttributeData*> AllRows;
    ModPool->GetAllRows<FPHAttributeData>(TEXT("ModRoll"), AllRows);

    // Filter valid prefix/suffix mods
    TArray<FPHAttributeData*> ValidRows;
    for (FPHAttributeData* Row : AllRows)
    {
        if ((bIsPrefix && Row->PrefixSuffix == EPrefixSuffix::Prefix) ||
            (!bIsPrefix && Row->PrefixSuffix == EPrefixSuffix::Suffix))
        {
            ValidRows.Add(Row);
        }
    }

    if (ValidRows.Num() == 0) return FPHAttributeData();

    // Pick one randomly
    const int32 RollIndex = FMath::RandRange(0, ValidRows.Num() - 1);
    FPHAttributeData RolledMod = *ValidRows[RollIndex];

    // Generate runtime UID
    RolledMod.ModifierUID = FGuid::NewGuid();

    // Roll value (optional: float or int)
    const float RolledValue = RolledMod.bRollsAsInteger
                                  ? static_cast<float>(FMath::RandRange(static_cast<int32>(RolledMod.MinStatChanged), static_cast<int32>(RolledMod.MaxStatChanged)))
                                  : FMath::FRandRange(RolledMod.MinStatChanged, RolledMod.MaxStatChanged);

    RolledMod.MinStatChanged = RolledValue;
    RolledMod.MaxStatChanged = RolledValue;

    return RolledMod;
}

FName UPHItemFunctionLibrary::GetSocketNameForSlot(EEquipmentSlot Slot)
{
	static const TMap<EEquipmentSlot, FName> SlotToSocketMap = {
		{EEquipmentSlot::ES_Head, "HeadSocket"},
		{EEquipmentSlot::ES_Gloves, "GlovesSocket"},
		{EEquipmentSlot::ES_Neck, "NeckSocket"},
		{EEquipmentSlot::ES_Chestplate, "ChestSocket"},
		{EEquipmentSlot::ES_Legs, "LegsSocket"},
		{EEquipmentSlot::ES_Boots, "BootsSocket"},
		{EEquipmentSlot::ES_MainHand, "MainHandSocket"},
		{EEquipmentSlot::ES_OffHand, "OffHandSocket"},
		{EEquipmentSlot::ES_Ring, "RingSocket"},
		{EEquipmentSlot::ES_Flask, "FlaskSocket"},
		{EEquipmentSlot::ES_Belt, "BeltSocket"},
		{EEquipmentSlot::ES_None, "None"}
	};

	const FName* FoundSocket = SlotToSocketMap.Find(Slot);
	return FoundSocket ? *FoundSocket : FName("None");
}

float UPHItemFunctionLibrary::GetStatValueByAttribute(const FEquippableItemData& Data, const FGameplayAttribute& Attribute)
{
	float Total = 0.f;

	auto Accumulate = [&](const TArray<FPHAttributeData>& Stats)
	{
		for (const auto& Stat : Stats)
		{
			if (Stat.ModifiedAttribute == Attribute)
			{
				Total += Stat.RolledStatValue;
			}
		}
	};

	Accumulate(Data.Affixes.Prefixes);
	Accumulate(Data.Affixes.Suffixes);
	Accumulate(Data.Affixes.Implicits);
	Accumulate(Data.Affixes.Crafted);

	return Total;
}
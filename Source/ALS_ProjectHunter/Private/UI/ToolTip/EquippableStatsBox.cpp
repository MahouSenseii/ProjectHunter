// EquippableStatsBox.cpp

#include "UI/ToolTip/EquippableStatsBox.h"
#include "UI/ToolTip/MinMaxBox.h"
#include "UI/ToolTip/RequirementsBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Item/Data/UItemDefinitionAsset.h"

void UEquippableStatsBox::NativeConstruct()
{
    Super::NativeConstruct();
    
    //  Set default MinMaxBox class if not set
    if (!MinMaxBoxClass)
    {
        MinMaxBoxClass = UMinMaxBox::StaticClass();
    }
}

void UEquippableStatsBox::SetItemData(const UItemDefinitionAsset* Definition, const FItemInstanceData& InInstanceData)
{
    if (!Definition)
    {
        UE_LOG(LogTemp, Warning, TEXT("SetItemData: Null definition"));
        return;
    }

    //  Store both definition and instance
    ItemDefinition = Definition;
    InstanceData = InInstanceData;

    //  Update requirements box
    if (RequirementsBox)
    {
        // RequirementsBox->SetItemRequirements is called from the tooltip
    }
}

void UEquippableStatsBox::CreateMinMaxBoxByDamageTypes()
{
    if (!ItemDefinition)
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateMinMaxBoxByDamageTypes: No item definition"));
        return;
    }

    //  Get base damage from DEFINITION (static weapon stats)
    const TMap<EDamageTypes, FDamageRange>& DamageMap = ItemDefinition->Equip.WeaponBaseStats.BaseDamage;
    bool bHasElementalDamage = false;

    //  Get damage modifiers from INSTANCE DATA (rolled affixes)
    auto GetDamageModifier = [this](EDamageTypes DamageType) -> FDamageRange
    {
        FDamageRange Modifier;
        Modifier.Min = 0.0f;
        Modifier.Max = 0.0f;

        // Helper to accumulate damage from affix list
        auto Accumulate = [&](const TArray<FPHAttributeData>& Stats)
        {
            for (const auto& Stat : Stats)
            {
                if (!Stat.bAffectsBaseWeaponStatsDirectly) continue;
                
                const FString AttrName = Stat.ModifiedAttribute.GetName();
                const FString DamageTypeName = UEnum::GetValueAsString(DamageType);
                
                if (AttrName.Contains(DamageTypeName, ESearchCase::IgnoreCase))
                {
                    Modifier.Min += Stat.RolledStatValue;
                    Modifier.Max += Stat.RolledStatValue;
                }
            }
        };

        //  Accumulate from instance data affixes
        Accumulate(InstanceData.Prefixes);
        Accumulate(InstanceData.Suffixes);
        Accumulate(InstanceData.Implicits);
        Accumulate(InstanceData.Crafted);

        return Modifier;
    };

    // Handle Physical Damage
    if (const FDamageRange* PhysicalDamage = DamageMap.Find(EDamageTypes::DT_Physical))
    {
        //  Add instance modifiers to base damage
        FDamageRange Modifier = GetDamageModifier(EDamageTypes::DT_Physical);
        float FinalMin = PhysicalDamage->Min + Modifier.Min;
        float FinalMax = PhysicalDamage->Max + Modifier.Max;

        if (!FMath::IsNearlyZero(FinalMin) || !FMath::IsNearlyZero(FinalMax))
        {
            const FString FormattedMin = FString::Printf(TEXT("(%d"), FMath::RoundToInt(FinalMin));
            const FString FormattedMax = FString::Printf(TEXT("%d)"), FMath::RoundToInt(FinalMax));

            if (PhysicalDamageValueMin) PhysicalDamageValueMin->SetText(FText::FromString(FormattedMin));
            if (PhysicalDamageValueMax) PhysicalDamageValueMax->SetText(FText::FromString(FormattedMax));
        }
        else if (PhysicalDamageBox)
        {
            PhysicalDamageBox->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    else if (PhysicalDamageBox)
    {
        PhysicalDamageBox->SetVisibility(ESlateVisibility::Collapsed);
    }

    // Handle Elemental Damage
    for (const TPair<EDamageTypes, FDamageRange>& Pair : DamageMap)
    {
        const EDamageTypes DamageType = Pair.Key;
        if (DamageType == EDamageTypes::DT_Physical) continue;

        //  Add instance modifiers to base damage
        FDamageRange Modifier = GetDamageModifier(DamageType);
        float FinalMin = Pair.Value.Min + Modifier.Min;
        float FinalMax = Pair.Value.Max + Modifier.Max;

        if (FMath::IsNearlyZero(FinalMin) && FMath::IsNearlyZero(FinalMax)) continue;

        bHasElementalDamage = true;

        // Create MinMaxBox
        UMinMaxBox* MinMaxBox = CreateWidget<UMinMaxBox>(this, MinMaxBoxClass);
        if (!MinMaxBox)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create MinMaxBox for damage type %d"), static_cast<int32>(DamageType));
            continue;
        }

        MinMaxBox->Init();
        MinMaxBox->SetMinMaxText(FinalMin, FinalMax);
        MinMaxBox->SetColorBaseOnType(DamageType);
        MinMaxBox->SetFontSize(18.0f);

        if (EDBox)
        {
            if (UHorizontalBox* HBox = Cast<UHorizontalBox>(EDBox))
            {
                if (UHorizontalBoxSlot* HBSlot = HBox->AddChildToHorizontalBox(MinMaxBox))
                {
                    HBSlot->SetHorizontalAlignment(HAlign_Right);
                    HBSlot->SetVerticalAlignment(VAlign_Fill);
                }
            }
            else
            {
                EDBox->AddChild(MinMaxBox);
            }
        }
    }

    // Remove EDBox if no elemental damage was found
    if (!bHasElementalDamage && EDBox)
    {
        EDBox->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UEquippableStatsBox::SetMinMaxForOtherStats()
{
    if (!ItemDefinition)
    {
        UE_LOG(LogTemp, Warning, TEXT("SetMinMaxForOtherStats: No item definition"));
        return;
    }

    //  Show/hide box based on value, with optional percent display
    auto SetStatRow = [](UHorizontalBox* Box, UTextBlock* TextBlock, float Value, bool bAsPercent = false)
    {
        if (!Box || !TextBlock) return;

        if (FMath::Abs(Value) < 0.01f)
        {
            Box->SetVisibility(ESlateVisibility::Collapsed);
        }
        else
        {
            Box->SetVisibility(ESlateVisibility::Visible);
            if (bAsPercent)
            {
                const int32 RoundedPercent = FMath::RoundToInt(Value);
                TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), RoundedPercent)));
            }
            else
            {
                TextBlock->SetText(FText::AsNumber(FMath::RoundToInt(Value)));
            }
        }
    };

    //  Helper: Sum affix contributions from INSTANCE DATA
    auto GetStatModifierFromAffixes = [this](const FGameplayAttribute& Attribute) -> float
    {
        float Total = 0.f;

        auto Accumulate = [&](const TArray<FPHAttributeData>& Stats)
        {
            for (const auto& Stat : Stats)
            {
                if (Stat.ModifiedAttribute == Attribute && Stat.bAffectsBaseWeaponStatsDirectly)
                {
                    Total += Stat.RolledStatValue;
                }
            }
        };

        //  Read from INSTANCE DATA, not definition!
        Accumulate(InstanceData.Prefixes);
        Accumulate(InstanceData.Suffixes);
        Accumulate(InstanceData.Implicits);
        Accumulate(InstanceData.Crafted);

        return Total;
    };

    //  Final values = Base (from definition) + Modifiers (from instance)
    const float FinalArmor        = ItemDefinition->Equip.ArmorBaseStats.Armor + GetStatModifierFromAffixes(UPHAttributeSet::GetArmourAttribute());
    const float FinalPoise        = ItemDefinition->Equip.ArmorBaseStats.Poise + GetStatModifierFromAffixes(UPHAttributeSet::GetPoiseAttribute());
    const float FinalCritChance   = ItemDefinition->Equip.WeaponBaseStats.CritChance + GetStatModifierFromAffixes(UPHAttributeSet::GetCritChanceAttribute());
    const float FinalAttackSpeed  = ItemDefinition->Equip.WeaponBaseStats.AttackSpeed + GetStatModifierFromAffixes(UPHAttributeSet::GetAttackSpeedAttribute());
    const float FinalCastTime     = GetStatModifierFromAffixes(UPHAttributeSet::GetCastSpeedAttribute());
    const float FinalManaCost     = GetStatModifierFromAffixes(UPHAttributeSet::GetManaCostChangesAttribute());
    const float FinalStaminaCost  = GetStatModifierFromAffixes(UPHAttributeSet::GetStaminaCostChangesAttribute());
    const float FinalWeaponRange  = ItemDefinition->Equip.WeaponBaseStats.WeaponRange + GetStatModifierFromAffixes(UPHAttributeSet::GetAttackRangeAttribute());

    // Apply to UI
    SetStatRow(ArmourBox,      ArmourValue,      FinalArmor, false);
    SetStatRow(PoiseBox,       PoiseValue,       FinalPoise, false);
    SetStatRow(CritChanceBox,  CritChanceValue,  FinalCritChance, true);
    SetStatRow(AtkSpeedBox,    AtkSpeedValue,    FinalAttackSpeed, false);
    SetStatRow(CastTimeBox,    CastTimeValue,    FinalCastTime, false);
    SetStatRow(ManaCostBox,    ManaCostValue,    FinalManaCost, false);
    SetStatRow(StaminaCostBox, StaminaCostValue, FinalStaminaCost, false);
    SetStatRow(WeaponRangeBox, WeaponRangeValue, FinalWeaponRange, false);
}

void UEquippableStatsBox::CreateResistanceBoxes()
{
    if (!ResistanceBox || !ResistanceBoxContainer || !MinMaxBoxClass || !ItemDefinition)
    {
        return;
    }

    ResistanceBoxContainer->ClearChildren();
    bool bHasAnyResistance = false;

    //  Get base resistances from DEFINITION
    for (const TPair<EDefenseTypes, float>& Pair : ItemDefinition->Equip.ArmorBaseStats.Resistances)
    {
        if (FMath::IsNearlyZero(Pair.Value)) continue;

        bHasAnyResistance = true;

        UMinMaxBox* MinMaxBox = CreateWidget<UMinMaxBox>(this, MinMaxBoxClass);
        if (!MinMaxBox) continue;

        MinMaxBox->Init();
        MinMaxBox->SetMinMaxText(Pair.Value, 0.0f);
        MinMaxBox->SetColorBaseOnType(static_cast<EDamageTypes>(Pair.Key));

        ResistanceBoxContainer->AddChild(MinMaxBox);
    }

    if (!bHasAnyResistance)
    {
        ResistanceBox->SetVisibility(ESlateVisibility::Collapsed);
    }
    else
    {
        ResistanceBox->SetVisibility(ESlateVisibility::Visible);
    }
}
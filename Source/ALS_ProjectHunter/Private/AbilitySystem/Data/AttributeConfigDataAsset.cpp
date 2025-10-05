// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Data/AttributeConfigDataAsset.h"
#include "PHGameplayTags.h"


UAttributeConfigDataAsset::UAttributeConfigDataAsset()
{
    // Constructor can be empty or set default values if needed
}

FAttributeInitConfig UAttributeConfigDataAsset::FindAttributeConfig(FGameplayTag AttributeTag, bool& bFound) const
{
    bFound = false;
    
    // Search in primary attributes
    for (const FAttributeInitConfig& Config : Attributes)
    {
        if (Config.AttributeTag == AttributeTag)
        {
            bFound = true;
            return Config;
        }
    }

    // Return empty config if not found
    return FAttributeInitConfig();
}

bool UAttributeConfigDataAsset::ValidateConfiguration(TArray<FString>& OutErrors) const
{
    OutErrors.Empty();
    bool bValid = true;
    
    // Validate primary attributes
    for (int32 i = 0; i < Attributes.Num(); i++)
    {
        const FAttributeInitConfig& Config = Attributes[i];
        
        if (!Config.AttributeTag.IsValid())
        {
            OutErrors.Add(FString::Printf(TEXT("Attributes[%d]: Invalid AttributeTag"), i));
            bValid = false;
            
        }
        
        
        
        if (Config.MinValue > Config.MaxValue)
        {
            OutErrors.Add(FString::Printf(TEXT("Attributes[%d]: %s Category"), i, *Config.AttributeTag.ToString()));
            bValid = false;
        
            OutErrors.Add(FString::Printf(TEXT("Attributes[%d] %s: MinValue (%.2f) > MaxValue (%.2f)"), 
                i, *Config.DisplayName, Config.MinValue, Config.MaxValue));
            bValid = false;
        }
        
        if (Config.DefaultValue < Config.MinValue || Config.DefaultValue > Config.MaxValue)
        {
            OutErrors.Add(FString::Printf(TEXT("Attributes[%d] %s: DefaultValue (%.2f) outside range [%.2f, %.2f]"), 
                i, *Config.DisplayName, Config.DefaultValue, Config.MinValue, Config.MaxValue));
            bValid = false;
        }
    }
    
    return bValid;
}

#if WITH_EDITOR
void UAttributeConfigDataAsset::AutoPopulateFromGameplayTags()
{
    const auto& PHTags = FPHGameplayTags::Get();
    
    Attributes.Empty();
    
    Attributes = {
        // === PRIMARY ATTRIBUTES ===
        {EAttributeCategory::Primary, PHTags.Attributes_Primary_Strength, 10.0f, 0.0f, 999.0f, 
            TEXT("Strength"), TEXT("Physical power and melee damage")},
        {EAttributeCategory::Primary, PHTags.Attributes_Primary_Intelligence, 10.0f, 0.0f, 999.0f, 
            TEXT("Intelligence"), TEXT("Magic power and mana")},
        {EAttributeCategory::Primary, PHTags.Attributes_Primary_Dexterity, 10.0f, 0.0f, 999.0f, 
            TEXT("Dexterity"), TEXT("Speed and critical chance")},
        {EAttributeCategory::Primary, PHTags.Attributes_Primary_Endurance, 10.0f, 0.0f, 999.0f, 
            TEXT("Endurance"), TEXT("Health and stamina")},
        {EAttributeCategory::Primary, PHTags.Attributes_Primary_Affliction, 10.0f, 0.0f, 999.0f, 
            TEXT("Affliction"), TEXT("Dark magic and status effects")},
        {EAttributeCategory::Primary, PHTags.Attributes_Primary_Luck, 10.0f, 0.0f, 999.0f, 
            TEXT("Luck"), TEXT("Critical chance and loot")},
        {EAttributeCategory::Primary, PHTags.Attributes_Primary_Covenant, 10.0f, 0.0f, 999.0f, 
            TEXT("Covenant"), TEXT("Divine magic and minions")},

        // === VITAL MAX ATTRIBUTES ===
        {EAttributeCategory::VitalMax, PHTags.Attributes_Secondary_Vital_MaxHealth, 100.0f, 1.0f, 99999.0f, 
            TEXT("Max Health"), TEXT("Maximum health points")},
        {EAttributeCategory::VitalMax, PHTags.Attributes_Secondary_Vital_MaxMana, 100.0f, 1.0f, 99999.0f, 
            TEXT("Max Mana"), TEXT("Maximum mana points")},
        {EAttributeCategory::VitalMax, PHTags.Attributes_Secondary_Vital_MaxStamina, 100.0f, 1.0f, 99999.0f, 
            TEXT("Max Stamina"), TEXT("Maximum stamina points")},

        // === VITAL CURRENT ATTRIBUTES (These get set to max on init) ===
        {EAttributeCategory::VitalCurrent, PHTags.Attributes_Vital_Health, 100.0f, 0.0f, 99999.0f, 
            TEXT("Health"), TEXT("Current health points")},
        {EAttributeCategory::VitalCurrent, PHTags.Attributes_Vital_Mana, 100.0f, 0.0f, 99999.0f, 
            TEXT("Mana"), TEXT("Current mana points")},
        {EAttributeCategory::VitalCurrent, PHTags.Attributes_Vital_Stamina, 100.0f, 0.0f, 99999.0f, 
            TEXT("Stamina"), TEXT("Current stamina points")},
    
        // === SECONDARY CURRENT ATTRIBUTES ===
        {EAttributeCategory::SecondaryCurrent, PHTags.Attributes_Secondary_Vital_HealthRegenRate, 1.0f, 0.1f, 9999.0f, 
            TEXT("Health Regen Rate"), TEXT("Seconds between health regeneration")},
        {EAttributeCategory::SecondaryCurrent, PHTags.Attributes_Secondary_Vital_HealthRegenAmount, 1.0f, 0.0f, 9999.0f, 
            TEXT("Health Regen Amount"), TEXT("Health regenerated per tick")},
        {EAttributeCategory::SecondaryCurrent, PHTags.Attributes_Secondary_Vital_ManaRegenRate, 1.0f, 0.1f, 99999.0f, 
            TEXT("Mana Regen Rate"), TEXT("Seconds between mana regeneration")},
        {EAttributeCategory::SecondaryCurrent, PHTags.Attributes_Secondary_Vital_ManaRegenAmount, 1.0f, 0.0f, 9999.0f, 
            TEXT("Mana Regen Amount"), TEXT("Mana regenerated per tick")},
        {EAttributeCategory::SecondaryCurrent, PHTags.Attributes_Secondary_Vital_StaminaRegenRate, 1.0f, 0.1f, 9999.0f, 
            TEXT("Stamina Regen Rate"), TEXT("Seconds between stamina regeneration")},
        {EAttributeCategory::SecondaryCurrent, PHTags.Attributes_Secondary_Vital_StaminaRegenAmount, 1.0f, 0.0f, 9999.0f, 
            TEXT("Stamina Regen Amount"), TEXT("Stamina regenerated per tick")},
        {EAttributeCategory::SecondaryCurrent, PHTags.Attributes_Secondary_Vital_StaminaDegenAmount, 1.0f, 0.0f, 9999.0f, 
            TEXT("Stamina Degen Amount"), TEXT("Stamina lost per tick")},
        {EAttributeCategory::SecondaryCurrent, PHTags.Attributes_Secondary_Vital_StaminaDegenRate, 1.0f, 0.0f, 9999.0f, 
            TEXT("Stamina Degen Rate"), TEXT("Seconds between stamina degeneration")},
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_GlobalDefenses, 0.0f, 0.0f, 9999.0f,
            TEXT("Global Defenses"), TEXT("General damage reduction against all damage types")},
        // === ARMOUR SYSTEM ===
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_Armour, 0.0f, 0.0f, 9999.0f,
            TEXT("Armour"), TEXT("Base physical damage reduction")},
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_ArmourFlatBonus, 0.0f, 0.0f, 9999.0f,
            TEXT("Armour (Flat Bonus)"), TEXT("Additional flat armour bonus")},
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_ArmourPercentBonus, 0.0f, 0.0f, 500.0f,
            TEXT("Armour (% Bonus)"), TEXT("Percentage increase to total armour")},

        // === FLAT RESISTANCES ===
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_FireResistanceFlat, 0.0f, 0.0f, 9999.0f,
            TEXT("Fire Resistance (Flat)"), TEXT("Flat fire damage reduction")},
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_IceResistanceFlat, 0.0f, 0.0f, 9999.0f,
            TEXT("Ice Resistance (Flat)"), TEXT("Flat ice damage reduction")},
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_LightResistanceFlat, 0.0f, 0.0f, 9999.0f,
            TEXT("Light Resistance (Flat)"), TEXT("Flat light damage reduction")},
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_LightningResistanceFlat, 0.0f, 0.0f, 9999.0f,
            TEXT("Lightning Resistance (Flat)"), TEXT("Flat lightning damage reduction")},
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_CorruptionResistanceFlat, 0.0f, 0.0f, 9999.0f,
            TEXT("Corruption Resistance (Flat)"), TEXT("Flat corruption damage reduction")},
        
        // === PERCENTAGE RESISTANCES ===
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_FireResistancePercentage, 0.0f, -100.0f, 95.0f,
            TEXT("Fire Resistance (%)"), TEXT("Percentage fire damage reduction")},
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_IceResistancePercentage, 0.0f, -100.0f, 95.0f,
            TEXT("Ice Resistance (%)"), TEXT("Percentage ice damage reduction")},
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_LightResistancePercentage, 0.0f, -100.0f, 95.0f,
            TEXT("Light Resistance (%)"), TEXT("Percentage light damage reduction")},
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_LightningResistancePercentage, 0.0f, -100.0f, 95.0f,
            TEXT("Lightning Resistance (%)"), TEXT("Percentage lightning damage reduction")},
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_CorruptionResistancePercentage, 0.0f, -100.0f, 95.0f,
            TEXT("Corruption Resistance (%)"), TEXT("Percentage corruption damage reduction")},

        // === MAXIMUM RESISTANCE CAPS ===
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_MaxFireResistance, 0.0f, 0.0f, 95.0f,
            TEXT("Max Fire Resistance"), TEXT("Maximum fire resistance percentage cap")},
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_MaxIceResistance, 0.0f, 0.0f, 95.0f,
            TEXT("Max Ice Resistance"), TEXT("Maximum ice resistance percentage cap")},
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_MaxLightResistance, 0.0, 0.0f, 95.0f,
            TEXT("Max Light Resistance"), TEXT("Maximum light resistance percentage cap")},
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_MaxLightningResistance, 0.0f, 0.0f, 95.0f,
            TEXT("Max Lightning Resistance"), TEXT("Maximum lightning resistance percentage cap")},
        {EAttributeCategory::SecondaryCurrent,PHTags.Attributes_Secondary_Resistances_MaxCorruptionResistance, 0.0f, 0.0f, 95.0f,
            TEXT("Max Corruption Resistance"), TEXT("Maximum corruption resistance percentage cap")},
    };
    
    UE_LOG(LogTemp, Log, TEXT("Auto-populated AttributeConfig: %d Attributes"), 
          Attributes.Num());
}
#endif
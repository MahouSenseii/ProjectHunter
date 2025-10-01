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
    for (const FAttributeInitConfig& Config : PrimaryAttributes)
    {
        if (Config.AttributeTag == AttributeTag)
        {
            bFound = true;
            return Config;
        }
    }
    
    // Search in secondary attributes
    for (const FAttributeInitConfig& Config : SecondaryAttributes)
    {
        if (Config.AttributeTag == AttributeTag)
        {
            bFound = true;
            return Config;
        }
    }
    
    // Search in vital attributes
    for (const FAttributeInitConfig& Config : VitalAttributes)
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
    for (int32 i = 0; i < PrimaryAttributes.Num(); i++)
    {
        const FAttributeInitConfig& Config = PrimaryAttributes[i];
        
        if (!Config.AttributeTag.IsValid())
        {
            OutErrors.Add(FString::Printf(TEXT("Primary[%d]: Invalid AttributeTag"), i));
            bValid = false;
        }
        
        if (Config.MinValue > Config.MaxValue)
        {
            OutErrors.Add(FString::Printf(TEXT("Primary[%d] %s: MinValue (%.2f) > MaxValue (%.2f)"), 
                i, *Config.DisplayName, Config.MinValue, Config.MaxValue));
            bValid = false;
        }
        
        if (Config.DefaultValue < Config.MinValue || Config.DefaultValue > Config.MaxValue)
        {
            OutErrors.Add(FString::Printf(TEXT("Primary[%d] %s: DefaultValue (%.2f) outside range [%.2f, %.2f]"), 
                i, *Config.DisplayName, Config.DefaultValue, Config.MinValue, Config.MaxValue));
            bValid = false;
        }
    }
    
    // Validate secondary attributes
    for (int32 i = 0; i < SecondaryAttributes.Num(); i++)
    {
        const FAttributeInitConfig& Config = SecondaryAttributes[i];
        
        if (!Config.AttributeTag.IsValid())
        {
            OutErrors.Add(FString::Printf(TEXT("Secondary[%d]: Invalid AttributeTag"), i));
            bValid = false;
        }
        
        if (Config.MinValue > Config.MaxValue)
        {
            OutErrors.Add(FString::Printf(TEXT("Secondary[%d] %s: MinValue (%.2f) > MaxValue (%.2f)"), 
                i, *Config.DisplayName, Config.MinValue, Config.MaxValue));
            bValid = false;
        }
        
        if (Config.DefaultValue < Config.MinValue || Config.DefaultValue > Config.MaxValue)
        {
            OutErrors.Add(FString::Printf(TEXT("Secondary[%d] %s: DefaultValue (%.2f) outside range [%.2f, %.2f]"), 
                i, *Config.DisplayName, Config.DefaultValue, Config.MinValue, Config.MaxValue));
            bValid = false;
        }
    }
    
    // Validate vital attributes  
    for (int32 i = 0; i < VitalAttributes.Num(); i++)
    {
        const FAttributeInitConfig& Config = VitalAttributes[i];
        
        if (!Config.AttributeTag.IsValid())
        {
            OutErrors.Add(FString::Printf(TEXT("Vital[%d]: Invalid AttributeTag"), i));
            bValid = false;
        }
        
        if (Config.MinValue > Config.MaxValue)
        {
            OutErrors.Add(FString::Printf(TEXT("Vital[%d] %s: MinValue (%.2f) > MaxValue (%.2f)"), 
                i, *Config.DisplayName, Config.MinValue, Config.MaxValue));
            bValid = false;
        }
        
        if (Config.DefaultValue < Config.MinValue || Config.DefaultValue > Config.MaxValue)
        {
            OutErrors.Add(FString::Printf(TEXT("Vital[%d] %s: DefaultValue (%.2f) outside range [%.2f, %.2f]"), 
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
    
    // Clear existing arrays
    PrimaryAttributes.Empty();
    SecondaryAttributes.Empty();
    VitalAttributes.Empty();
    
    // Auto-populate primary attributes
    PrimaryAttributes = {
        {PHTags.Attributes_Primary_Strength, 0.0f, 0.0f, 999.0f, TEXT("Strength"), TEXT("Physical power and melee damage")},
        {PHTags.Attributes_Primary_Intelligence, 0.0f, 0.0f, 999.0f, TEXT("Intelligence"), TEXT("Magic power and mana")},
        {PHTags.Attributes_Primary_Dexterity, 0.0f, 0.0f, 999.0f, TEXT("Dexterity"), TEXT("Speed and critical chance")},
        {PHTags.Attributes_Primary_Endurance, 0.0f, 0.0f, 999.0f, TEXT("Endurance"), TEXT("Health and stamina")},
        {PHTags.Attributes_Primary_Affliction, 0.0f, 0.0f, 999.0f, TEXT("Affliction"), TEXT("Dark magic and status effects")},
        {PHTags.Attributes_Primary_Luck, 0.0f, 0.0f, 999.0f, TEXT("Luck"), TEXT("Critical chance and loot")},
        {PHTags.Attributes_Primary_Covenant, 0.0f, 0.0f, 999.0f, TEXT("Covenant"), TEXT("Divine magic and minions")}
    };
    
    // Auto-populate vital attributes
    VitalAttributes = {
        {PHTags.Attributes_Vital_Health, 50.0f, 0.0f, 9999.0f, TEXT("Health"), TEXT("Current health points")},
        {PHTags.Attributes_Vital_Mana, 50.0f, 0.0f, 9999.0f, TEXT("Mana"), TEXT("Current mana points")},
        {PHTags.Attributes_Vital_Stamina, 50.0f, 0.0f, 9999.0f, TEXT("Stamina"), TEXT("Current stamina points")}
    };
    
    // Autopopulate some key secondary attributes (you can expand this list)
    SecondaryAttributes = {
        {PHTags.Attributes_Secondary_Vital_HealthRegenRate,1.0f, 0.1f, 9999.0f, TEXT("Health Regen Rate"), TEXT("Seconds between health regeneration")},
        {PHTags.Attributes_Secondary_Vital_HealthRegenAmount, 1.0f, 0.0f, 9999.0f, TEXT("Health Regen Amount"), TEXT("Health regenerated per tick")},
        {PHTags.Attributes_Secondary_Vital_ManaRegenRate, 1.0f, 0.1f, 99999.0f, TEXT("Mana Regen Rate"), TEXT("Seconds between mana regeneration")},
        {PHTags.Attributes_Secondary_Vital_ManaRegenAmount, 1.0f, 0.0f, 9999.0f, TEXT("Mana Regen Amount"), TEXT("Mana regenerated per tick")},
        {PHTags.Attributes_Secondary_Vital_StaminaRegenRate, 1.0f, 0.1f, 9999.0f, TEXT("Stamina Regen Rate"), TEXT("Seconds between stamina regeneration")},
        {PHTags.Attributes_Secondary_Vital_StaminaRegenAmount, 1.0f, 0.0f, 9999.0f, TEXT("Stamina Regen Amount"), TEXT("Stamina regenerated per tick")},
        {PHTags.Attributes_Secondary_Vital_StaminaDegenAmount, 1.0f, 0.0f, 9999.0f, TEXT("Stamina Degen Amount"), TEXT("Stamina lost per tick")},
        {PHTags.Attributes_Secondary_Vital_StaminaDegenRate, 1.0f, 0.0f, 9999.0f, TEXT("Stamina Degen Rate"), TEXT("Seconds between stamina degeneration")},

        // === GENERAL DEFENSES ===
        {PHTags.Attributes_Secondary_Resistances_GlobalDefenses, 0.0f, 0.0f, 9999.0f, TEXT("Global Defenses"), TEXT("General damage reduction against all damage types")},

        // === ARMOUR SYSTEM ===
        {PHTags.Attributes_Secondary_Resistances_Armour, 0.0f, 0.0f, 9999.0f, TEXT("Armour"), TEXT("Base physical damage reduction")},
        {PHTags.Attributes_Secondary_Resistances_ArmourFlatBonus, 0.0f, 0.0f, 9999.0f, TEXT("Armour (Flat Bonus)"), TEXT("Additional flat armour bonus")},
        {PHTags.Attributes_Secondary_Resistances_ArmourPercentBonus, 0.0f, 0.0f, 500.0f, TEXT("Armour (% Bonus)"), TEXT("Percentage increase to total armour")},

        // === FLAT RESISTANCES ===
        {PHTags.Attributes_Secondary_Resistances_FireResistanceFlat, 0.0f, 0.0f, 9999.0f, TEXT("Fire Resistance (Flat)"), TEXT("Flat fire damage reduction")},
        {PHTags.Attributes_Secondary_Resistances_IceResistanceFlat, 0.0f, 0.0f, 9999.0f, TEXT("Ice Resistance (Flat)"), TEXT("Flat ice damage reduction")},
        {PHTags.Attributes_Secondary_Resistances_LightResistanceFlat, 0.0f, 0.0f, 9999.0f, TEXT("Light Resistance (Flat)"), TEXT("Flat light damage reduction")},
        {PHTags.Attributes_Secondary_Resistances_LightningResistanceFlat, 0.0f, 0.0f, 9999.0f, TEXT("Lightning Resistance (Flat)"), TEXT("Flat lightning damage reduction")},
        {PHTags.Attributes_Secondary_Resistances_CorruptionResistanceFlat, 0.0f, 0.0f, 9999.0f, TEXT("Corruption Resistance (Flat)"), TEXT("Flat corruption damage reduction")},
        
        // === PERCENTAGE RESISTANCES ===
        {PHTags.Attributes_Secondary_Resistances_FireResistancePercentage, 0.0f, -100.0f, 95.0f, TEXT("Fire Resistance (%)"), TEXT("Percentage fire damage reduction")},
        {PHTags.Attributes_Secondary_Resistances_IceResistancePercentage, 0.0f, -100.0f, 95.0f, TEXT("Ice Resistance (%)"), TEXT("Percentage ice damage reduction")},
        {PHTags.Attributes_Secondary_Resistances_LightResistancePercentage, 0.0f, -100.0f, 95.0f, TEXT("Light Resistance (%)"), TEXT("Percentage light damage reduction")},
        {PHTags.Attributes_Secondary_Resistances_LightningResistancePercentage, 0.0f, -100.0f, 95.0f, TEXT("Lightning Resistance (%)"), TEXT("Percentage lightning damage reduction")},
        {PHTags.Attributes_Secondary_Resistances_CorruptionResistancePercentage, 0.0f, -100.0f, 95.0f, TEXT("Corruption Resistance (%)"), TEXT("Percentage corruption damage reduction")},

        // === MAXIMUM RESISTANCE CAPS ===
        {PHTags.Attributes_Secondary_Resistances_MaxFireResistance, 0.0f, 0.0f, 95.0f, TEXT("Max Fire Resistance"), TEXT("Maximum fire resistance percentage cap")},
        {PHTags.Attributes_Secondary_Resistances_MaxIceResistance, 0.0f, 0.0f, 95.0f, TEXT("Max Ice Resistance"), TEXT("Maximum ice resistance percentage cap")},
        {PHTags.Attributes_Secondary_Resistances_MaxLightResistance, 0.0, 0.0f, 95.0f, TEXT("Max Light Resistance"), TEXT("Maximum light resistance percentage cap")},
        {PHTags.Attributes_Secondary_Resistances_MaxLightningResistance, 0.0f, 0.0f, 95.0f, TEXT("Max Lightning Resistance"), TEXT("Maximum lightning resistance percentage cap")},
        {PHTags.Attributes_Secondary_Resistances_MaxCorruptionResistance, 0.0f, 0.0f, 95.0f, TEXT("Max Corruption Resistance"), TEXT("Maximum corruption resistance percentage cap")},
    };
    
    UE_LOG(LogTemp, Log, TEXT("Auto-populated AttributeConfig: %d Primary, %d Secondary, %d Vital"), 
           PrimaryAttributes.Num(), SecondaryAttributes.Num(), VitalAttributes.Num());
}
#endif
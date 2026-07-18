#pragma once

#include "CoreMinimal.h"
#include "LootEnums.generated.h"

UENUM(BlueprintType)
enum class EDropRarity : uint8
{
	DR_Common UMETA(DisplayName = "Common"),
	DR_Uncommon UMETA(DisplayName = "Uncommon"),
	DR_Rare UMETA(DisplayName = "Rare"),
	DR_Epic UMETA(DisplayName = "Epic"),
	DR_Legendary UMETA(DisplayName = "Legendary"),
	DR_Mythical UMETA(DisplayName = "Mythical")
};

UENUM(BlueprintType)
enum class ELootSourceType : uint8
{
	LST_None UMETA(DisplayName = "None"),
	LST_NPC UMETA(DisplayName = "NPC"),
	LST_Chest UMETA(DisplayName = "Chest"),
	LST_Breakable UMETA(DisplayName = "Breakable"),
	LST_Boss UMETA(DisplayName = "Boss"),
	LST_Quest UMETA(DisplayName = "Quest"),
	LST_Crafting UMETA(DisplayName = "Crafting"),
	LST_Shop UMETA(DisplayName = "Shop")
};

UENUM(BlueprintType)
enum class ELootSelectionMethod : uint8
{
	LSM_Weighted UMETA(DisplayName = "Weighted Random"),
	LSM_Sequential UMETA(DisplayName = "Sequential"),
	LSM_GuaranteedOne UMETA(DisplayName = "Guaranteed One"),
	LSM_All UMETA(DisplayName = "All")
};

UENUM(BlueprintType)
enum class ECorruptionType : uint8
{
	CT_None UMETA(DisplayName = "None"),
	CT_Minor UMETA(DisplayName = "Minor"),
	CT_Major UMETA(DisplayName = "Major"),
	CT_Abyssal UMETA(DisplayName = "Abyssal")
};

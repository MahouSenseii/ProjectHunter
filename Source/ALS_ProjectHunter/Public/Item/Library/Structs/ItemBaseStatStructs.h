// Item/Library/Structs/ItemBaseStatStructs.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/UnrealType.h"
#include "ItemBaseStatStructs.generated.h"

USTRUCT(BlueprintType)
struct FBaseWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
	float MinPhysicalDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
	float MaxPhysicalDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
	float MinFireDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
	float MaxFireDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
	float MinIceDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
	float MaxIceDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
	float MinLightningDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
	float MaxLightningDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
	float MinLightDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
	float MaxLightDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
	float MinCorruptionDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
	float MaxCorruptionDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Attack", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float AttackSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Attack", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float CriticalStrikeChance = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Attack", meta = (ClampMin = "0.1"))
	float Range = 1.0f;

	FBaseWeaponStats() = default;

#if WITH_EDITOR
	void OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName)
	{
		ValidateMinMaxRanges();
	}

	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
	{
		ValidateMinMaxRanges();
	}

private:
	void ValidateMinMaxRanges()
	{
		if (MaxPhysicalDamage < MinPhysicalDamage)
		{
			MaxPhysicalDamage = MinPhysicalDamage;
		}
		if (MaxFireDamage < MinFireDamage)
		{
			MaxFireDamage = MinFireDamage;
		}
		if (MaxIceDamage < MinIceDamage)
		{
			MaxIceDamage = MinIceDamage;
		}
		if (MaxLightningDamage < MinLightningDamage)
		{
			MaxLightningDamage = MinLightningDamage;
		}
		if (MaxLightDamage < MinLightDamage)
		{
			MaxLightDamage = MinLightDamage;
		}
		if (MaxCorruptionDamage < MinCorruptionDamage)
		{
			MaxCorruptionDamage = MinCorruptionDamage;
		}
	}
#endif
};

USTRUCT(BlueprintType)
struct FBaseArmorStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
	float Armor = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor|Resistances")
	float FireResistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor|Resistances")
	float IceResistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor|Resistances")
	float LightningResistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor|Resistances")
	float LightResistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor|Resistances")
	float CorruptionResistance = 0.0f;

	FBaseArmorStats() = default;
};

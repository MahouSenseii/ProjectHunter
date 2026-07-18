// Item/Library/Structs/ItemDurabilityStructs.h
#pragma once

#include "CoreMinimal.h"
#include "ItemDurabilityStructs.generated.h"

USTRUCT(BlueprintType)
struct FItemDurability
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Durability")
	float CurrentDurability = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Durability")
	float MaxDurability = 100.0f;

	FItemDurability() = default;

	void SetMaxDurability(float NewMax)
	{
		MaxDurability = NewMax;
		CurrentDurability = MaxDurability;
	}

	void Reduce(float Amount)
	{
		CurrentDurability = FMath::Max(0.0f, CurrentDurability - Amount);
	}

	void Repair(float Amount)
	{
		CurrentDurability = FMath::Min(MaxDurability, CurrentDurability + Amount);
	}

	void RepairFull()
	{
		CurrentDurability = MaxDurability;
	}

	bool IsBroken() const
	{
		return CurrentDurability <= 0.0f;
	}

	float GetDurabilityPercent() const
	{
		return MaxDurability > 0.0f ? (CurrentDurability / MaxDurability) : 0.0f;
	}
};

// Item/Library/Structs/ItemConsumableStructs.h
#pragma once

#include "CoreMinimal.h"
#include "ItemConsumableStructs.generated.h"

class UGameplayEffect;

USTRUCT(BlueprintType)
struct FConsumableData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	int32 MaxUses = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	float Cooldown = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	bool bConsumedOnUse = true;

	FConsumableData() = default;
};

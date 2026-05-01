#pragma once

#include "CoreMinimal.h"
#include "Equipment/Library/EquipmentEnums.h"
#include "EquipmentStructs.generated.h"

class UItemInstance;

/** Replicated flat equipment entry because TMap does not replicate. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FEquipmentSlotEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EEquipmentSlot Slot = EEquipmentSlot::ES_None;

	UPROPERTY(BlueprintReadOnly)
	UItemInstance* Item = nullptr;

	FEquipmentSlotEntry() = default;

	FEquipmentSlotEntry(EEquipmentSlot InSlot, UItemInstance* InItem)
		: Slot(InSlot)
		, Item(InItem)
	{
	}
};


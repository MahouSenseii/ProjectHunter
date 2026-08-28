#pragma once

#include "Item/Library/Structs/ItemRequirementStructs.h"

class UEquipmentManager;
class UItemInstance;

/** Reads the equipment owner's live stats and evaluates an item's authored requirements. */
class FEquipmentRequirementEvaluator
{
public:
	static FItemRequirementCheckResult Evaluate(const UEquipmentManager& Manager, const UItemInstance* Item);
};

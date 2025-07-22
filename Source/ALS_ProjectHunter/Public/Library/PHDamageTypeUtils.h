#pragma once

#include "CoreMinimal.h"
#include "Library/PHItemEnumLibrary.h"

/** Utility helper for converting damage type enums to name strings */
FORCEINLINE FString DamageTypeToString(EDamageTypes Type)
{
    return UEnum::GetValueAsString(Type).RightChop(15);
}

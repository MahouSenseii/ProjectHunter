#pragma once

#include "CoreMinimal.h"
#include "PHAbilityEnums.generated.h"

UENUM(BlueprintType)
enum class EPHAbilityActivationPolicy : uint8
{
	OnInputTriggered,
	WhileInputActive,
	OnSpawn
};

UENUM(BlueprintType)
enum class EPHAbilityActivationGroup : uint8
{
	Independent,
	Exclusive_Replaceable,
	Exclusive_Blocking,

	MAX UMETA(Hidden)
};

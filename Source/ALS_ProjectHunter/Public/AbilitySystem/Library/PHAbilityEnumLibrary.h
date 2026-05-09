// AbilitySystem/Library/PHAbilityEnumLibrary.h
// Activation policy and group enums for UPHGameplayAbility.
//
// Dependency rule: no project includes — only CoreMinimal.
// Include this alone when you need to pass or switch on ability activation
// enums without pulling in the full UPHGameplayAbility header.

#pragma once

#include "CoreMinimal.h"
#include "PHAbilityEnumLibrary.generated.h"

UENUM(BlueprintType)
enum class EPHAbilityActivationPolicy : uint8
{
	/** Activated when the bound input action is triggered. */
	OnInputTriggered,

	/** Active as long as the bound input action is held. */
	WhileInputActive,

	/** Activated automatically on spawn / grant. */
	OnSpawn
};

UENUM(BlueprintType)
enum class EPHAbilityActivationGroup : uint8
{
	/** Can run alongside any other ability. */
	Independent,

	/** Only one Exclusive ability can be active; a new one replaces the current one. */
	Exclusive_Replaceable,

	/** Only one Exclusive ability can be active; new activations are blocked. */
	Exclusive_Blocking,

	MAX UMETA(Hidden)
};

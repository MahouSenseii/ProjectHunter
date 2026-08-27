// Shared combat debug switch.
//
// Hunter.Debug.Combat is consumed by both FCombatOutgoingDamageCalculator and
// FCombatIncomingDamageResolver so a single toggle prints the whole per-stage
// breakdown for one hit. The console variable is registered exactly once, in
// CombatDebug.cpp - UE keys console objects globally by name, so registering
// the same name from two translation units leaves only one live and silently
// desyncs the two halves of the log.
#pragma once

#include "CoreMinimal.h"

namespace PHCombatDebug
{
	/** True when Hunter.Debug.Combat is set. Always false in shipping. */
	ALS_PROJECTHUNTER_API bool IsCombatDebugLoggingEnabled();
}

#include "Combat/Library/CombatDebug.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarDebugCombat(
		TEXT("Hunter.Debug.Combat"),
		0,
		TEXT("Log the per-stage combat damage breakdown for every ApplyHit\n")
		TEXT("0: Disabled (default)\n")
		TEXT("1: Log base roll, conversion, scaling, crit, mitigation, block, and routing"),
		ECVF_Cheat
	);
}
#endif

namespace PHCombatDebug
{
	bool IsCombatDebugLoggingEnabled()
	{
#if !UE_BUILD_SHIPPING
		return CVarDebugCombat.GetValueOnGameThread() != 0;
#else
		return false;
#endif
	}
}

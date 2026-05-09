// AI/Library/MobStructs.cpp
// Out-of-line definitions for structs declared in MobStructs.h.
//
// This file exists specifically so FMobTypeEntry::IsEligible() can include
// the full APHBaseCharacter definition (needed by TSubclassOf<APHBaseCharacter>::
// operator bool / operator*) without forcing that dependency into the header,
// which is intentionally forward-declaration-only for APHBaseCharacter.

#include "AI/Library/MobStructs.h"
#include "Character/PHBaseCharacter.h"

bool FMobTypeEntry::IsEligible(float CurrentTime) const
{
	if (!MobClass || SpawnWeight <= 0) { return false; }
	if (SpawnCooldown <= 0.0f)         { return true;  }
	return (LastSpawnTime < 0.0f) || ((CurrentTime - LastSpawnTime) >= SpawnCooldown);
}

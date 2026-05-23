#include "AI/Library/MobStructs.h"
#include "Character/PHBaseCharacter.h"

bool FMobTypeEntry::IsEligible(float CurrentTime) const
{
	if (!MobClass || SpawnWeight <= 0) { return false; }
	if (SpawnCooldown <= 0.0f)         { return true;  }
	return (LastSpawnTime < 0.0f) || ((CurrentTime - LastSpawnTime) >= SpawnCooldown);
}

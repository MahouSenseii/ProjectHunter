#include "Tower/Library/FunctionLibraries/RunSeedFunctionLibrary.h"

namespace RunSeedPrivate
{
	// Stream labels. Distinct labels stop sibling streams at the same index from
	// drawing identical numbers.
	const FName FloorLabel(TEXT("Floor"));
	const FName EncounterLabel(TEXT("Encounter"));
	const FName MonsterLabel(TEXT("Monster"));
	const FName ModifierLabel(TEXT("Modifier"));
	const FName RewardLabel(TEXT("Reward"));
	const FName LootLabel(TEXT("Loot"));
}

int32 URunSeedFunctionLibrary::DeriveSeed(const int32 ParentSeed, const FName Label, const int32 Index)
{
	uint32 Hash = static_cast<uint32>(ParentSeed);
	Hash = HashCombineFast(Hash, GetTypeHash(Label));
	Hash = HashCombineFast(Hash, static_cast<uint32>(Index));

	// FRandomStream(0) reseeds itself from the global RNG, which would silently
	// break determinism. Never hand back zero.
	const int32 Result = static_cast<int32>(Hash & MAX_int32);
	return Result != 0 ? Result : 1;
}

int32 URunSeedFunctionLibrary::DeriveFloorSeed(const int32 RunSeed, const int32 FloorNumber)
{
	return DeriveSeed(RunSeed, RunSeedPrivate::FloorLabel, FloorNumber);
}

int32 URunSeedFunctionLibrary::DeriveEncounterSeed(const int32 FloorSeed, const int32 EncounterIndex)
{
	return DeriveSeed(FloorSeed, RunSeedPrivate::EncounterLabel, EncounterIndex);
}

int32 URunSeedFunctionLibrary::DeriveMonsterSeed(const int32 EncounterSeed, const int32 MonsterIndex)
{
	return DeriveSeed(EncounterSeed, RunSeedPrivate::MonsterLabel, MonsterIndex);
}

int32 URunSeedFunctionLibrary::DeriveModifierSeed(const int32 MonsterSeed)
{
	return DeriveSeed(MonsterSeed, RunSeedPrivate::ModifierLabel, 0);
}

int32 URunSeedFunctionLibrary::DeriveRewardSeed(const int32 FloorSeed)
{
	return DeriveSeed(FloorSeed, RunSeedPrivate::RewardLabel, 0);
}

int32 URunSeedFunctionLibrary::DeriveLootSeed(const int32 RewardSeed, const int32 DropIndex)
{
	return DeriveSeed(RewardSeed, RunSeedPrivate::LootLabel, DropIndex);
}

FRandomStream URunSeedFunctionLibrary::MakeFloorStream(const int32 RunSeed, const int32 FloorNumber)
{
	return FRandomStream(DeriveFloorSeed(RunSeed, FloorNumber));
}

FRandomStream URunSeedFunctionLibrary::MakeModifierStream(const int32 MonsterSeed)
{
	return FRandomStream(DeriveModifierSeed(MonsterSeed));
}

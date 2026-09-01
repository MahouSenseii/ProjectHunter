#include "Tower/Library/FunctionLibraries/RunSeedFunctionLibrary.h"

#include "Hash/Fnv.h"
#include "Templates/TypeHash.h"

namespace RunSeedPrivate
{
	// Stream labels. Distinct labels stop sibling streams at the same index from
	// drawing identical numbers.
	const FName FloorLabel(TEXT("Floor"));
	const FName LayoutLabel(TEXT("Layout"));
	const FName EncounterLabel(TEXT("Encounter"));
	const FName MonsterLabel(TEXT("Monster"));
	const FName ModifierLabel(TEXT("Modifier"));
	const FName RewardLabel(TEXT("Reward"));
	const FName LootLabel(TEXT("Loot"));
}

int32 URunSeedFunctionLibrary::DeriveSeed(const int32 ParentSeed, const FName Label, const int32 Index)
{
	// FName IDs depend on load order. Hash canonical text with a persistent hash
	// and combine function so a saved run seed also works in another process.
	const FString LabelText = Label.ToString().ToLower();
	const uint32 LabelHash = UE::HashStringFNV1a32(FStringView(LabelText));
	uint32 Hash = static_cast<uint32>(ParentSeed);
	Hash = HashCombine(Hash, LabelHash);
	Hash = HashCombine(Hash, static_cast<uint32>(Index));

	// ProjectHunter spawn/modifier callers use zero to request an unseeded roll.
	const int32 Result = static_cast<int32>(Hash & MAX_int32);
	return Result != 0 ? Result : 1;
}

int32 URunSeedFunctionLibrary::DeriveFloorSeed(const int32 RunSeed, const int32 FloorNumber)
{
	return DeriveSeed(RunSeed, RunSeedPrivate::FloorLabel, FloorNumber);
}

int32 URunSeedFunctionLibrary::DeriveLayoutSeed(const int32 FloorSeed)
{
	return DeriveSeed(FloorSeed, RunSeedPrivate::LayoutLabel, 0);
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

FRandomStream URunSeedFunctionLibrary::MakeLayoutStream(const int32 FloorSeed)
{
	return FRandomStream(DeriveLayoutSeed(FloorSeed));
}

FRandomStream URunSeedFunctionLibrary::MakeModifierStream(const int32 MonsterSeed)
{
	return FRandomStream(DeriveModifierSeed(MonsterSeed));
}

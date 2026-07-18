#include "Loot/Library/FunctionLibraries/LootSpawnFunctionLibrary.h"

FVector ULootSpawnFunctionLibrary::GetCircularScatterLocation(const FVector& Location, float SpreadRadius, FRandomStream& RandStream)
{
	FVector SpawnLocation = Location;

	if (SpreadRadius > 0.0f)
	{
		FVector RandomDir = RandStream.VRand();
		RandomDir.Z = 0.0f;
		RandomDir.Normalize();

		SpawnLocation += RandomDir * RandStream.FRandRange(0.0f, SpreadRadius);
	}

	return SpawnLocation;
}

FVector ULootSpawnFunctionLibrary::GetSpawnLocationFromSettings(const FLootSpawnSettings& SpawnSettings, FRandomStream& RandStream)
{
	FVector SpawnLocation = SpawnSettings.SpawnLocation;
	SpawnLocation.Z += SpawnSettings.HeightOffset;

	if (SpawnSettings.bUseSpawnBox)
	{
		const FVector Extent = SpawnSettings.SpawnBoxExtent;
		SpawnLocation.X += RandStream.FRandRange(-Extent.X, Extent.X);
		SpawnLocation.Y += RandStream.FRandRange(-Extent.Y, Extent.Y);
		if (Extent.Z > 0.0f)
		{
			SpawnLocation.Z += RandStream.FRandRange(-Extent.Z, Extent.Z);
		}
	}
	else if (SpawnSettings.ScatterRadius > 0.0f)
	{
		FVector RandomDir = RandStream.VRand();
		RandomDir.Z = 0.0f;
		RandomDir.Normalize();
		SpawnLocation += RandomDir * RandStream.FRandRange(0.0f, SpawnSettings.ScatterRadius);
	}

	return SpawnLocation;
}

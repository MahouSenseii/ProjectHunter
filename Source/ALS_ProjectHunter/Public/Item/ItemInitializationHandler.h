#pragma once

#include "CoreMinimal.h"

class UItemInstance;
enum class EItemRarity : uint8;
struct FDataTableRowHandle;
struct FPHAttributeData;

class ALS_PROJECTHUNTER_API FItemInitializationHandler
{
public:
	static bool MigrateToCurrentVersion(UItemInstance& Item);
	static void PostLoadInit(UItemInstance& Item);
	static void Initialize(UItemInstance& Item, FDataTableRowHandle InBaseItemHandle, int32 InItemLevel, EItemRarity InRarity, bool bGenerateAffixes);
	static void InitializeWithCorruption(UItemInstance& Item, FDataTableRowHandle InBaseItemHandle, int32 InItemLevel, EItemRarity InRarity, bool bGenerateAffixes, float CorruptionChance, bool bForceCorrupted);
	static void CalculateCorruptionState(UItemInstance& Item);
	static TArray<FPHAttributeData> GetCorruptedAffixes(const UItemInstance& Item);
	static void PrepareForSave(UItemInstance& Item);
	static void PostLoadInitialize(UItemInstance& Item);
};

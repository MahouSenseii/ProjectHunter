// Item/Library/Structs/ItemStatsStructs.h
#pragma once

#include "CoreMinimal.h"
#include "Item/Library/Structs/ItemAttributeStructs.h"
#include "ItemStatsStructs.generated.h"


/**
 * All stats/affixes on an item
 */
USTRUCT(BlueprintType)
struct FPHItemStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Stats")
	TArray<FPHAttributeData> Prefixes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Stats")
	TArray<FPHAttributeData> Suffixes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Stats")
	TArray<FPHAttributeData> Implicits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Stats")
	TArray<FPHAttributeData> Crafted;

	/**
	 * Enchant slot. Holds up to one enchantment. Separate from Crafted so Blueprint UI
	 * can display enchantments in their own tooltip section.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Stats")
	TArray<FPHAttributeData> Enchants;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Stats")
	bool bAffixesGenerated = false;

	FPHItemStats() = default;

	/** Get total count of all stats (zero allocation) */
	FORCEINLINE int32 GetTotalStatCount() const
	{
		return Implicits.Num() + Prefixes.Num() + Suffixes.Num() + Crafted.Num() + Enchants.Num();
	}

	/** Get all stats combined - OPTIMIZED with Reserve() */
	TArray<FPHAttributeData> GetAllStats() const
	{
		TArray<FPHAttributeData> All;
		All.Reserve(GetTotalStatCount());
		All.Append(Implicits);
		All.Append(Prefixes);
		All.Append(Suffixes);
		All.Append(Crafted);
		All.Append(Enchants);
		return All;
	}

	/** Zero-allocation iteration over all stats */
	template<typename Func>
	void ForEachStat(Func&& Callback) const
	{
		for (const FPHAttributeData& Stat : Implicits) { Callback(Stat); }
		for (const FPHAttributeData& Stat : Prefixes) { Callback(Stat); }
		for (const FPHAttributeData& Stat : Suffixes) { Callback(Stat); }
		for (const FPHAttributeData& Stat : Crafted) { Callback(Stat); }
		for (const FPHAttributeData& Stat : Enchants) { Callback(Stat); }
	}

	/** Zero-allocation mutable iteration over all stats */
	template<typename Func>
	void ForEachMutableStat(Func&& Callback)
	{
		for (FPHAttributeData& Stat : Implicits) { Callback(Stat); }
		for (FPHAttributeData& Stat : Prefixes) { Callback(Stat); }
		for (FPHAttributeData& Stat : Suffixes) { Callback(Stat); }
		for (FPHAttributeData& Stat : Crafted) { Callback(Stat); }
		for (FPHAttributeData& Stat : Enchants) { Callback(Stat); }
	}

	/** Zero-allocation iteration with index */
	template<typename Func>
	void ForEachStatIndexed(Func&& Callback) const
	{
		int32 Index = 0;
		for (const FPHAttributeData& Stat : Implicits) { Callback(Stat, Index++); }
		for (const FPHAttributeData& Stat : Prefixes) { Callback(Stat, Index++); }
		for (const FPHAttributeData& Stat : Suffixes) { Callback(Stat, Index++); }
		for (const FPHAttributeData& Stat : Crafted) { Callback(Stat, Index++); }
		for (const FPHAttributeData& Stat : Enchants) { Callback(Stat, Index++); }
	}

	/** Zero-allocation find with predicate */
	template<typename Predicate>
	const FPHAttributeData* FindStat(Predicate&& Pred) const
	{
		for (const FPHAttributeData& Stat : Implicits) { if (Pred(Stat)) return &Stat; }
		for (const FPHAttributeData& Stat : Prefixes) { if (Pred(Stat)) return &Stat; }
		for (const FPHAttributeData& Stat : Suffixes) { if (Pred(Stat)) return &Stat; }
		for (const FPHAttributeData& Stat : Crafted) { if (Pred(Stat)) return &Stat; }
		for (const FPHAttributeData& Stat : Enchants) { if (Pred(Stat)) return &Stat; }
		return nullptr;
	}

	/** Find stat by name */
	const FPHAttributeData* FindStatByName(FName AttributeName) const
	{
		return FindStat([AttributeName](const FPHAttributeData& Stat) {
			return Stat.AttributeName == AttributeName;
		});
	}

	/** Get affix count (prefixes + suffixes only, excluding implicits/crafted/enchants) */
	int32 GetTotalAffixCount() const
	{
		return Prefixes.Num() + Suffixes.Num();
	}

	/** Check for unidentified stats - OPTIMIZED with early exit */
	bool HasUnidentifiedStats() const
	{
		for (const FPHAttributeData& Stat : Implicits) { if (!Stat.bIsIdentified) return true; }
		for (const FPHAttributeData& Stat : Prefixes) { if (!Stat.bIsIdentified) return true; }
		for (const FPHAttributeData& Stat : Suffixes) { if (!Stat.bIsIdentified) return true; }
		for (const FPHAttributeData& Stat : Crafted) { if (!Stat.bIsIdentified) return true; }
		for (const FPHAttributeData& Stat : Enchants) { if (!Stat.bIsIdentified) return true; }
		return false;
	}

	/** Set every stat/affix identification state */
	void SetAllIdentified(bool bInIdentified)
	{
		ForEachMutableStat([bInIdentified](FPHAttributeData& Stat) {
			Stat.bIsIdentified = bInIdentified;
		});
	}

	/** Identify one stat/affix by its generated runtime UID */
	bool IdentifyStatByUID(FGuid AttributeUID)
	{
		bool bFound = false;
		ForEachMutableStat([AttributeUID, &bFound](FPHAttributeData& Stat) {
			if (!bFound && Stat.AttributeUID == AttributeUID)
			{
				Stat.bIsIdentified = true;
				bFound = true;
			}
		});
		return bFound;
	}

	/** Total rank point value - OPTIMIZED with ForEachStat */
	float GetTotalAffixValue() const
	{
		float Total = 0.0f;
		ForEachStat([&Total](const FPHAttributeData& Stat) {
			Total += Stat.GetRankPointValue();
		});
		return Total;
	}

	/** Sum all bonuses for a specific attribute */
	float GetTotalValueForAttribute(FName AttributeName) const
	{
		float Total = 0.0f;
		ForEachStat([&Total, AttributeName](const FPHAttributeData& Stat) {
			if (Stat.AttributeName == AttributeName)
			{
				Total += Stat.RolledStatValue;
			}
		});
		return Total;
	}

	/** Check if empty */
	bool IsEmpty() const
	{
		return GetTotalStatCount() == 0;
	}

	/** Clear all stats */
	void Clear()
	{
		Implicits.Empty();
		Prefixes.Empty();
		Suffixes.Empty();
		Crafted.Empty();
		Enchants.Empty();
		bAffixesGenerated = false;
	}
};

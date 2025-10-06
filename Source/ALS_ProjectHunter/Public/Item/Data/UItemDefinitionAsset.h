#pragma once

#include "CoreMinimal.h"
#include "Library/PHItemStructLibrary.h"
#include "UItemDefinitionAsset.generated.h"


struct FPHAttributeData;
struct FItemBase;
struct FEquippableItemData;
/**
 * @class UItemDefinitionAsset
 * @brief Represents an item definition asset with static/author-time data, generation attributes, and asset management functionality.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UItemDefinitionAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// Static/author-time data (meshes, type, slot, base stats, passives)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FItemBase Base;         
               
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FEquippableItemData Equip;             

	// Affix pools (PoE-style)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Generation")
	UDataTable* StatsDataTableTable = nullptr;    

	// Optional: fixed implicits unique to this base
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Generation")
	TArray<FPHAttributeData> Implicits;    

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Stats")
	FPHItemStats ItemStats;
	
	UFUNCTION(BlueprintPure)
	bool IsValidDefinition() const;
	
	/** Get the combined base item information (what ItemInstance expects) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Definition")
	UItemDefinitionAsset* GetBaseItemInfo() const;

	/** Get just the base item data */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Definition")
	const FItemBase& GetBase() const { return Base; }

	/** Get just the equippable data */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Definition")
	const FEquippableItemData& GetEquip() const { return Equip; }

	/** Get the affix table for generation */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Definition")
	UDataTable* GetStatsDataTable() const { return StatsDataTableTable; }

	/** Get the fixed implicits */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Definition")
	const TArray<FPHAttributeData>& GetImplicits() const { return Implicits; }

	// Optional: Primary Asset Type (for Asset Manager)
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("ItemDef"), GetFName());
	}
};
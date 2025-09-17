// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Library/PHItemStructLibrary.h"
#include "UItemDefinitionAsset.generated.h"

/**
 * @class UItemDefinitionAsset
 * @brief Represents an item definition asset with static/author-time data, generation attributes, and asset management functionality.
 *
 * The UUItemDefinitionAsset class is derived from the UPrimaryDataAsset and is used to define item properties and data
 * essential for gameplay systems, including base item attributes, equippable data, affix properties, and implicit
 * attributes. It also provides functionality for validation and asset management support.
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

	// Affix pools (PoE-style) – you already use a UDataTable*
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Generation")
	UDataTable* AffixTable = nullptr;    

	// Optional: fixed implicits unique to this base
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Generation")
	TArray<FPHAttributeData> Implicits;    

	UFUNCTION(BlueprintPure)
	bool IsValidDefinition() const;

	// Optional: Primary Asset Type (for Asset Manager)
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("ItemDef"), GetFName());
	}
};

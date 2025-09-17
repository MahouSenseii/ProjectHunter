// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Item/BaseItem.h"
#include "EquippableItem.generated.h"

class APHBaseCharacter;
/**
 * 
 */
UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UEquippableItem : public UBaseItem
{
	GENERATED_BODY()


public:	
	// Sets default values for this actor's properties
	UEquippableItem();
	
	virtual void Initialize( FItemInformation& ItemInfo) override;

	UFUNCTION(BlueprintCallable)
	bool CanEquipItem(const APHBaseCharacter* Character) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Requirements", meta = (DisplayName = "Get Stat Requirements"))
	const FItemStatRequirement& GetStatRequirements() const;
	

	UFUNCTION(BlueprintCallable, Category = "Modifiers")
	void RerollAllMods();
	

	UFUNCTION(BlueprintCallable, Category = "Item")
	TArray<FPHAttributeData> GetItemStats() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Stats", meta = (DisplayName = "Get Full Item Stats"))
	const FPHItemStats& GetFullItemStats() const;

	UFUNCTION(BlueprintCallable, Category = "Item")
	TArray<FPHAttributeData> GetItemStatsByTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category = "Item")
	TArray<FPHAttributeData> GetItemStatsByAffixType(EPrefixSuffix Type) const;


protected:

	// Set in BP will be all stats that the item can generate
	UPROPERTY(EditAnywhere, Category = "ItemInfo|Stats")
	UDataTable* StatsDataTable;
	
};

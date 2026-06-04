// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Character/HUD/HunterHUDBaseWidget.h"
#include "MenuBaseWidget.generated.h"

class UStatsManager;
class UInventoryManager;
class UEquipmentManager;
class APHBaseCharacter;
/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UMenuBaseWidget : public UHunterHUDBaseWidget
{
	GENERATED_BODY()
	

	
public:
	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;

	// ─────────────────────────────────────────────────────────────────────────
	// Getters
	// ─────────────────────────────────────────────────────────────────────────
	
	// Will return the Equipment manager
	UFUNCTION(BlueprintCallable, Category = "Getters")
	UEquipmentManager* GetEquipmentManager() const { return EquipmentManager; }
	
	// Will return the Inventory manager
	UFUNCTION(BlueprintCallable, Category = "Getters")
	UInventoryManager* GetInventoryManager() const { return InventoryManager; }
	
	// Will return the Stats manager
	UFUNCTION(BlueprintCallable, Category = "Getters")
	UStatsManager* GetStatsManager() const { return StatsManager; }
	
	
	
protected:

	
	UPROPERTY(BlueprintReadOnly, Category = "SavedComponents")
	TObjectPtr<UEquipmentManager> EquipmentManager;
	
	UPROPERTY(BlueprintReadOnly, Category = "SavedComponents")
	TObjectPtr<UInventoryManager> InventoryManager;
	
	UPROPERTY(BlueprintReadOnly, Category = "SavedComponents")
	TObjectPtr<UStatsManager> StatsManager;
};


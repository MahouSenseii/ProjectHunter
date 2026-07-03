// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Character/HUD/HunterHUDBaseWidget.h"
#include "PHMenuPageWidgetBase.generated.h"

class UStatsManager;
class UInventoryManager;
class UEquipmentManager;
class APHBaseCharacter;
/**
 * @class UPHMenuPageWidgetBase
 * @brief Serves as the base class for menu page widgets within the application.
 *
 * This class provides the foundational functionalities and structure for menu page widgets.
 * It is intended to be extended by specific menu page implementations to include additional
 * behaviors and interactions as required.
 *
 * The UPHMenuPageWidgetBase class is designed for use in UI frameworks that manage
 * widget-based menu systems. It provides common methods and properties that child
 * classes can leverage or override to customize functionality and appearance.
 *
 * @note This class is typically inherited from and not used directly.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHMenuPageWidgetBase : public UHunterHUDBaseWidget
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


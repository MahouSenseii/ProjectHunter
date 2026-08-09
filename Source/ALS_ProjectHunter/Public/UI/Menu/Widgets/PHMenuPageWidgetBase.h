#pragma once

#include "CoreMinimal.h"
#include "UI/HUD/HunterHUDBaseWidget.h"
#include "PHMenuPageWidgetBase.generated.h"

class APHBaseCharacter;
class UEquipmentManager;
class UInventoryManager;
class UStatsManager;

UCLASS()
class ALS_PROJECTHUNTER_API UPHMenuPageWidgetBase : public UHunterHUDBaseWidget
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;

	UFUNCTION(BlueprintCallable, Category = "Getters")
	UEquipmentManager* GetEquipmentManager() const { return EquipmentManager; }

	UFUNCTION(BlueprintCallable, Category = "Getters")
	UInventoryManager* GetInventoryManager() const { return InventoryManager; }

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

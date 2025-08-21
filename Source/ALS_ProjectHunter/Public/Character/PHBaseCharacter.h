// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Character/ALSCharacter.h"
#include "Components/EquipmentManager.h"
#include "Components/StatsManager.h"
#include "Interfaces/CombatSubInterface.h"
#include "PaperSpriteComponent.h"
#include "Player/PHPlayerController.h"
#include "UI/ToolTip/EquippableToolTip.h"
#include "AbilitySystemComponent.h"
#include "Containers/Map.h"
#include "GameplayTagContainer.h" 
#include "GameplayEffectTypes.h" 
#include "PHBaseCharacter.generated.h"


class UInteractableManager;
class UCombatManager;
class USpringArmComponent;
class UWidgetManager;
class AEquipmentPickup;
class UAbilitySystemComponent;
class UAttributeSet;
class UConsumableToolTip;

/**
 *  APHBaseCharacter inherits from AALSCharacter and adds properties for the Gameplay Ability System.
 */
UCLASS()
class ALS_PROJECTHUNTER_API APHBaseCharacter : public AALSCharacter, public IAbilitySystemInterface, public ICombatSubInterface
{
	GENERATED_BODY()

public:
	/** Constructor */
	APHBaseCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/** Called when possessed by a new controller */
	virtual void PossessedBy(AController* NewController) override;

	/** Ability System */
	UFUNCTION(BlueprintCallable, Category = "GAS")
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category = "GAS")
	UAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** Equipment & Inventory */
	UFUNCTION(BlueprintCallable, Category = "Manager")
	UEquipmentManager* GetEquipmentManager() const { return EquipmentManager; }

	UFUNCTION(BlueprintCallable, Category = "Manager")
	UInventoryManager* GetInventoryManager() { return InventoryManager; }

	UFUNCTION(BlueprintCallable, Category = "Manager")
	UCombatManager* GetCombatManager() const { return CombatManager; }

	UFUNCTION(BlueprintCallable, Category = "Manager")
	UStatsManager* GetStatsManager() const { return StatsManager; }

	/** Player Level */
	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual int32 GetPlayerLevel() override;

	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual  int32 GetCurrentXP() override;

	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual  int32 GetXPFNeededForNextLevel() override;

	/** Tooltip UI */
	UFUNCTION()
	void OpenToolTip(UInteractableManager* InteractableManager);

	UFUNCTION()
	void CloseToolTip(UInteractableManager* InteractableManager);
	

	/** Regeneration & Tick */
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanSprint() const override;

	//Changed Handle
	virtual void OnGaitChanged(EALSGait PreviousGait) override;

	

protected:
	/** Begin Play */
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	

	/** Ability System Initialization */
	virtual void InitAbilityActorInfo();

protected:
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Controller")
	APHPlayerController* CurrentController;

	
private:
	/** Gameplay Ability System */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAttributeSet> AttributeSet;
	
	/** Managers */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Manager", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEquipmentManager> EquipmentManager;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Manager", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryManager> InventoryManager;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Manager", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatManager> CombatManager;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Manager", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStatsManager> StatsManager;
	

	/** UI - ToolTips */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "ToolTip", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPHBaseToolTip> CurrentToolTip;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ToolTip", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UEquippableToolTip> EquippableToolTipClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ToolTip", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UConsumableToolTip> ConsumableToolTipClass;
	

	/** Internal State */
	/** If true, this character is controlled by a real player and expects ASC/Attributes from the PlayerState */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Defaults", meta = (AllowPrivateAccess = "true"))
	bool bIsPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults", meta = (AllowPrivateAccess = "true"))
	int32 Level = 1;
	
       bool bIsInRecovery = false;


	};
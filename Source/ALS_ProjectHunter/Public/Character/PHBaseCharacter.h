// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Character/ALSCharacter.h"
#include "Components/EquipmentManager.h"
#include "Interfaces/CombatSubInterface.h"
#include "PaperSpriteComponent.h"
#include "Player/PHPlayerController.h"
#include "UI/ToolTip/EquippableToolTip.h"
#include "AbilitySystemComponent.h"
#include "Containers/Map.h"
#include "GameplayTagContainer.h" 
#include "GameplayEffectTypes.h" 
#include "Library/PHCharacterStructLibrary.h"
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

	/** Player Level */
	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual int32 GetPlayerLevel() override;

	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual  int32 GetCurrentXP() override;

	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual  int32 GetXPFNeededForNextLevel() override;

	/** Tooltip UI */
	UFUNCTION()
	void OpenToolTip(const UInteractableManager* InteractableManager);

	UFUNCTION()
	void CloseToolTip(UInteractableManager* InteractableManager);
	
	/** Poise Functions */
	UFUNCTION(BlueprintCallable, Category = "Poise")
	float GetCurrentPoiseDamage() const;

	UFUNCTION(BlueprintCallable, Category = "Poise")
	float GetTimeSinceLastPoiseHit() const;

	UFUNCTION(BlueprintCallable, Category = "Poise")
	void SetTimeSinceLastPoiseHit(float NewTime);
	

	/** Regeneration & Tick */
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanSprint() const override;

	//Changed Handle
	virtual void OnGaitChanged(EALSGait PreviousGait) override;

	/** Primary attributes set at character spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Init")
	FInitialPrimaryAttributes PrimaryInitAttributes;

	/** Secondary attributes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Init")
	FInitialSecondaryAttributes SecondaryInitAttributes;

	/** Vital attributes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Init")
	FInitialVitalAttributes VitalInitAttributes;

	// Store active periodic effects by SetByCaller ta
	TMultiMap<FGameplayTag, FActiveGameplayEffectHandle> ActivePeriodicEffects;



protected:
	/** Begin Play */
	virtual void BeginPlay() override;

	/** Ability System Initialization */
	virtual void InitAbilityActorInfo();
	
	/** Effect Application */
	UFUNCTION(BlueprintCallable)
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float InLevel) const;

	UFUNCTION(BlueprintCallable)
	FActiveGameplayEffectHandle ApplyEffectToSelfWithReturn(TSubclassOf<UGameplayEffect> InEffect, float InLevel);

	/** Default Attribute Initialization */
	void InitializeDefaultAttributes() const;
	
	/** Callback for Poise Replication */
	UFUNCTION()
	void OnRep_PoiseDamage();

	
protected:
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Controller")
	APHPlayerController* CurrentController;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS")
	TArray<FInitialGameplayEffectInfo> StartupEffects;
	
	
private:
	/** Gameplay Ability System */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAttributeSet> AttributeSet;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GAS|Attributes", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DefaultPrimaryAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GAS|Attributes", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DefaultSecondaryCurrentAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GAS|Attributes", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DefaultSecondaryMaxAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GAS|Attributes", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DefaultVitalAttributes;
	

	/** Managers */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Manager", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEquipmentManager> EquipmentManager;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Manager", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryManager> InventoryManager;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Manager", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatManager> CombatManager;

	/** MiniMap */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "MiniMap", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneCaptureComponent2D> MiniMapCapture = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "MiniMap", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> MiniMapSpringArm = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaperSpriteComponent> MiniMapIndicator;

	/** UI - ToolTips */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "ToolTip", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPHBaseToolTip> CurrentToolTip;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ToolTip", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UEquippableToolTip> EquippableToolTipClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ToolTip", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UConsumableToolTip> ConsumableToolTipClass;

	/** Animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* StaggerMontage;

	/** Poise System */
	UPROPERTY(ReplicatedUsing = OnRep_PoiseDamage, meta = (AllowPrivateAccess = "true"))
	float CurrentPoiseDamage = 0.0f;

	UPROPERTY(meta = (AllowPrivateAccess = "true"))
	float TimeSinceLastPoiseHit = 0.0f;

	/** Internal State */
	/** If true, this character is controlled by a real player and expects ASC/Attributes from the PlayerState */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Defaults", meta = (AllowPrivateAccess = "true"))
	bool bIsPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults", meta = (AllowPrivateAccess = "true"))
	int32 Level = 1;
	bool bIsInRecovery = false;
	float TimeSinceLastRecovery = 0.0;
	
};
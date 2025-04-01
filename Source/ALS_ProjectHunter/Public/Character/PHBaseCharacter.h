// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Character/ALSCharacter.h"
#include "Components/EquipmentManager.h"
#include "Interfaces/CombatSubInterface.h"
#include "PaperSpriteComponent.h"
#include "Player/PHPlayerController.h"
#include "UI/ToolTip/EquippableToolTip.h"
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
	void OpenToolTip(UInteractableManager* InteractableManager);

	UFUNCTION()
	void CloseToolTip(UInteractableManager* InteractableManager);

	/** MiniMap Setup */
	UFUNCTION()
	void SetupMiniMapCamera();

	/** Poise Functions */
	UFUNCTION(BlueprintCallable, Category = "Poise")
	float GetCurrentPoiseDamage() const;

	UFUNCTION(BlueprintCallable, Category = "Poise")
	float GetTimeSinceLastPoiseHit() const;

	UFUNCTION(BlueprintCallable, Category = "Poise")
	void SetTimeSinceLastPoiseHit(float NewTime);

	UFUNCTION(BlueprintCallable, Category = "Poise")
	void ApplyPoiseDamage(float PoiseDamage);

	UFUNCTION(BlueprintCallable, Category = "Poise")
	float GetPoisePercentage() const;

	/** Stagger */
	UFUNCTION(BlueprintCallable, Category = "Poise")
	UAnimMontage* GetStaggerAnimation();

	/** Regeneration & Tick */
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanSprint() const override;

	/** Equipment Handles */

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetStatBase(const FGameplayAttribute& Attr) const;

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void ApplyFlatStatModifier(const FGameplayAttribute& Attr, float InValue) const;

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void RemoveFlatStatModifier(const FGameplayAttribute& Attribute, float Delta);


protected:
	/** Begin Play */
	virtual void BeginPlay() override;

	/** Ability System Initialization */
	virtual void InitAbilityActorInfo();

	/** Regeneration */
	void SetRegenerationTimer(FTimerHandle& TimerHandle, void (APHBaseCharacter::*RegenFunction)() const, float RegenRate);
	void HealthRegenRateChange();
	void ManaRegenRateChange();
	void StaminaRegenRateChange();
	void HealthRegeneration() const;
	void ManaRegeneration() const;
	void StaminaRegeneration() const;
	void StaminaDegen(float DeltaTime);

	/** Effect Application */
	UFUNCTION(BlueprintCallable)
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float InLevel) const;

	/** Default Attribute Initialization */
	void InitializeDefaultAttributes() const;

	/** Callback for Poise Replication */
	UFUNCTION()
	void OnRep_PoiseDamage();

	
protected:
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Controller")
	APHPlayerController* CurrentController;
	
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

	/** Regeneration Effects */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vital", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> HealthRegenEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vital", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> ManaRegenEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vital", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> StaminaRegenEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vital", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> StaminaDegenEffect;

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
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Defaults", meta = (AllowPrivateAccess = "true"))
	bool bIsPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults", meta = (AllowPrivateAccess = "true"))
	int32 Level = 1;

	bool bIsInRecovery = false;
	float TimeSinceLastRecovery = 0.0;

	/** Timers */
	FTimerHandle HealthRegenTimer;
	FTimerHandle ManaRegenTimer;
	FTimerHandle StaminaRegenTimer;
};

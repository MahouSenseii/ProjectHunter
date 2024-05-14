// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Character/ALSCharacter.h"
#include "Interfaces/CombatSubInterface.h"
#include "PHBaseCharacter.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;

/**
 *  APHBaseCharacter inherits from AALSCharacter and adds properties for the Gameplay Ability System.
 */
UCLASS()
class ALS_PROJECTHUNTER_API APHBaseCharacter : public AALSCharacter, public IAbilitySystemInterface, public ICombatSubInterface
{
	GENERATED_BODY()

public:

	// Constructor for APHBaseCharacter
	APHBaseCharacter(const FObjectInitializer& ObjectInitializer);
	
	UFUNCTION(BlueprintCallable)
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable)
	UAttributeSet* GetAttributeSet() const { return AttributeSet; }
	
protected:

	virtual void BeginPlay() override;

	virtual bool CanSprint() const override;

	//TimerHandles
	FTimerHandle HealthRegenTimer;
	FTimerHandle ManaRegenTimer;
	FTimerHandle StaminaRegenTimer;
	
	virtual void Tick(float DeltaSeconds) override;
	void SetRegenerationTimer(FTimerHandle& TimerHandle,void (APHBaseCharacter::*RegenFunction)() const, float RegenRate);
	void HealthRegenRateChange();
	void ManaRegenRateChange();
	void StaminaRegenRateChange();
	void HealthRegeneration() const;
	void ManaRegeneration() const;
	void StaminaRegeneration() const;
	void StaminaDegen(const float DeltaTime);

	// getter for InRecovery
	UFUNCTION(BlueprintCallable)
	bool GetIsInRecovery() const { return  bIsInRecovery; }
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS")
	TObjectPtr<UAttributeSet> AttributeSet;

	virtual void InitAbilityActorInfo();
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultPrimaryAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultSecondaryCurrentAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultSecondaryMaxAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultVitalAttributes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vital")
	TSubclassOf<UGameplayEffect> HealthRegenEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vital")
	TSubclassOf<UGameplayEffect> ManaRegenEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vital")
	TSubclassOf<UGameplayEffect> StaminaRegenEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vital")
	TSubclassOf<UGameplayEffect> StaminaDegenEffect;
	
	UFUNCTION(BlueprintCallable)
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level) const;
	
	void InitializeDefaultAttributes() const;	

private:
	
	bool bIsInRecovery = false;
	float TimeSinceLastRecovery = 0.0;
};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"
#include "StatsManager.generated.h"

class APHBaseCharacter;
class UAbilitySystemComponent;
class UGameplayEffect;

DECLARE_LOG_CATEGORY_EXTERN(LogStatsManager, Log, All);

/**
 * Handles attribute modifications through the Gameplay Ability System.
 * Supports flat, percentage, and override modifiers.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UStatsManager : public UActorComponent
{
	GENERATED_BODY()

public:
	/* ============================= */
	/* ===   Lifecycle           === */
	/* ============================= */
	
	UStatsManager();
	virtual void BeginPlay() override;

	/* ============================= */
	/* ===   Initialization      === */
	/* ============================= */

	/** Initialize the stats manager with an ability system component */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void Initialize();

	/** Initialize attributes from a data asset configuration */
	UFUNCTION(BlueprintCallable, Category = "Stats|Initialization")
	void InitializeAttributesFromConfig(UAttributeConfigDataAsset* Config);

	/** Initialize current vitals (Health, Mana, Stamina) to their max values */
	UFUNCTION(BlueprintCallable, Category = "Stats|Initialization")
	void InitializeCurrentVitalsToMax();

	/** Initialize attribute-to-effect-class mapping */
	void InitializeAttributeToEffectClassMap();

	/* ============================= */
	/* ===   Regen / Degen       === */
	/* ============================= */

	/** Initialize and start regeneration effects for health, mana, and stamina */
	UFUNCTION(BlueprintCallable, Category = "Stats|Regeneration")
	void InitializeRegenAndDegenEffects();

	/** Stop all regeneration effects */
	UFUNCTION(BlueprintCallable, Category = "Stats|Regeneration")
	void StopAllRegenEffects();

	/* ============================= */
	/* ===   Setup / Access      === */
	/* ============================= */

	/** Set the ability system component */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void SetASC(UAbilitySystemComponent* InASC) { ASC = InASC; }

	/** Get the ability system component */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	UAbilitySystemComponent* GetASC() { return ASC; }

	/** Get the owner character */
	UFUNCTION(BlueprintCallable, Category = "Setup")
	APHBaseCharacter* GetOwnerCharacter() { return Owner; }

	/** Set the owner character */
	UFUNCTION(BlueprintCallable, Category = "Setup")
	void SetOwnerCharacter(APHBaseCharacter* InOwner) { Owner = InOwner; }

	/* ============================= */
	/* ===   Attribute Access    === */
	/* ============================= */

	/** Get the base value of an attribute (without modifiers) */
	UFUNCTION(BlueprintPure, Category = "Stats|Attributes")
	float GetAttributeBase(const FGameplayAttribute& Attribute) const;

	/** Get the current value of an attribute (with all modifiers applied) */
	UFUNCTION(BlueprintPure, Category = "Stats|Attributes")
	float GetAttributeCurrent(const FGameplayAttribute& Attribute) const;

	/** Check if an attribute exists on this component's attribute set */
	UFUNCTION(BlueprintPure, Category = "Stats|Attributes")
	bool HasAttribute(const FGameplayAttribute& Attribute) const;

	/** Find gameplay attribute by gameplay tag */
	UFUNCTION(BlueprintCallable, Category = "Stats|Helper")
	FGameplayAttribute FindAttributeByTag(const FGameplayTag& Tag) const;

	/** Get the effect class mapped to a specific attribute */
	TSubclassOf<UGameplayEffect> GetEffectClassForAttribute(const FGameplayAttribute& Attribute) const;

	/* ============================= */
	/* ===   Direct Modification === */
	/* ============================= */

	/** 
	 * Directly set the base value of an attribute.
	 * Warning: This bypasses gameplay effects and should be used sparingly.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Attributes")
	void SetAttributeBase(const FGameplayAttribute& Attribute, float NewValue);

	/* ============================= */
	/* ===   Modifier Application === */
	/* ============================= */

	/**
	 * Apply a flat additive modifier to an attribute (e.g., +10 Health).
	 * Uses AddBase operation - applied before multiplicative modifiers.
	 * @return Handle to the applied effect for later removal
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Modifiers")
	FActiveGameplayEffectHandle ApplyFlatModifier(
		const FGameplayAttribute& Attribute, 
		float Value,
		bool bIsTemporary = true);

	/**
	 * Apply a flat modifier to final result (e.g., +10 after all calculations).
	 * Uses AddFinal operation - applied last.
	 * @return Handle to the applied effect for later removal
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Modifiers")
	FActiveGameplayEffectHandle ApplyFlatModifierFinal(
		const FGameplayAttribute& Attribute, 
		float Value,
		bool bIsTemporary = true);

	/**
	 * Apply an additive percentage modifier (e.g., +25% stacks additively with other percentages).
	 * Uses MultiplyAdditive operation - 50% + 50% = 100% bonus (x2.0 total).
	 * @param Percentage - Value as percentage (e.g., 25.0 for +25%)
	 * @return Handle to the applied effect for later removal
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Modifiers")
	FActiveGameplayEffectHandle ApplyPercentageModifier(
		const FGameplayAttribute& Attribute, 
		float Percentage,
		bool bIsTemporary = true);

	/**
	 * Apply a compound multiplier (e.g., 1.5x that stacks multiplicatively).
	 * Uses MultiplyCompound operation - 1.5x * 1.5x = 2.25x total.
	 * @param Multiplier - Direct multiplier value (e.g., 1.5 for 150%)
	 * @return Handle to the applied effect for later removal
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Modifiers")
	FActiveGameplayEffectHandle ApplyMultiplierModifier(
		const FGameplayAttribute& Attribute, 
		float Multiplier,
		bool bIsTemporary = true);

	/**
	 * Apply an override modifier that sets the attribute to a specific value.
	 * This overrides all other modifiers.
	 * @return Handle to the applied effect for later removal
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Modifiers")
	FActiveGameplayEffectHandle ApplyOverrideModifier(
		const FGameplayAttribute& Attribute, 
		float Value);

	/* ============================= */
	/* ===   Modifier Removal    === */
	/* ============================= */

	/**
	 * Remove a specific modifier by its handle.
	 * @return True if the effect was successfully removed
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Modifiers")
	bool RemoveModifier(FActiveGameplayEffectHandle Handle);

	/**
	 * Remove all modifiers affecting a specific attribute.
	 * @return Number of effects removed
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Modifiers")
	int32 RemoveAllModifiersFromAttribute(const FGameplayAttribute& Attribute);

	/**
	 * Remove all temporary modifiers.
	 * @return Number of effects removed
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Modifiers")
	int32 RemoveAllTemporaryModifiers();

	/* ============================= */
	/* ===   Effect Application  === */
	/* ============================= */

	/**
	 * Apply a gameplay effect to self.
	 * @return Handle to the applied effect
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Effects")
	FActiveGameplayEffectHandle ApplyGameplayEffectToSelf(
		TSubclassOf<UGameplayEffect> EffectClass, 
		float Level = 1.0f);

	/**
	 * Apply a gameplay effect spec to self (for advanced usage).
	 * @return Handle to the applied effect
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Effects")
	FActiveGameplayEffectHandle ApplyGameplayEffectSpecToSelf(
		const FGameplayEffectSpec& Spec);

protected:
	/* ============================= */
	/* ===   Protected Functions === */
	/* ============================= */

	/**
	 * Apply an initialization effect with SetByCaller values.
	 * @param EffectClass - The gameplay effect to apply
	 * @param AttributeValues - Map of gameplay tags to values
	 * @return True if successfully applied
	 */
	bool ApplyInitializationEffect(
		const TSubclassOf<UGameplayEffect>& EffectClass,
		const TMap<FGameplayTag, float>& AttributeValues);

	/**
	 * Simple initialization - just applies the effect without SetByCaller.
	 * Use this when your GE has hardcoded values or uses attribute-based magnitudes.
	 */
	bool ApplySimpleInitEffect(const TSubclassOf<UGameplayEffect>& EffectClass) const;

	/** Helper to apply a regeneration/degeneration effect with SetByCaller values */
	FActiveGameplayEffectHandle ApplyRegenEffect(TSubclassOf<UGameplayEffect> EffectClass) const;

	/** Callback for when sprinting state changes */
	void OnSprintingTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	/** Validate that ASC is ready */
	bool IsInitialized() const { return ASC != nullptr; }

	/* ============================= */
	/* ===   Properties          === */
	/* ============================= */

	/** Reference to the Ability System Component */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	TObjectPtr<UAbilitySystemComponent> ASC;

	/** Reference to the character that owns this stats manager */
	UPROPERTY(BlueprintReadOnly, Category = "Setup")
	TObjectPtr<APHBaseCharacter> Owner;

	/** Optional: Data asset for attribute initialization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Initialization")
	TObjectPtr<UAttributeConfigDataAsset> AttributeConfig;

	/** Maps attributes to their corresponding effect classes */
	UPROPERTY()
	TMap<FGameplayAttribute, TSubclassOf<UGameplayEffect>> AttributeToEffectClassMap;

	/** Tracks all active temporary modifiers for bulk removal */
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> TemporaryModifiers;

	/* ============================= */
	/* ===   GameplayEffect Classes === */
	/* ============================= */

	// Equipment Effects
	UPROPERTY(EditDefaultsOnly, Category = "Classes|GameplayEffect|Equipment")
	TSubclassOf<UGameplayEffect> EquipmentDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Classes|GameplayEffect|Equipment")
	TSubclassOf<UGameplayEffect> EquipmentPercentBonusDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Classes|GameplayEffect|Equipment")
	TSubclassOf<UGameplayEffect> EquipmentFlatBonusDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Classes|GameplayEffect|Equipment")
	TSubclassOf<UGameplayEffect> EquipmentPercentResistances;

	UPROPERTY(EditDefaultsOnly, Category = "Classes|GameplayEffect|Equipment")
	TSubclassOf<UGameplayEffect> EquipmentFlatResistances;
	
	UPROPERTY(EditDefaultsOnly, Category = "Classes|GameplayEffect|Equipment")
	TSubclassOf<UGameplayEffect> EquipmentAttributes;
	
	UPROPERTY(EditDefaultsOnly, Category = "Classes|GameplayEffect|Equipment")
	TSubclassOf<UGameplayEffect> EquipmentPiercingDamage;

	// Regeneration Effects
	UPROPERTY(EditDefaultsOnly, Category = "Classes|GameplayEffect|Regeneration")
	TSubclassOf<UGameplayEffect> GE_HealthRegen;

	UPROPERTY(EditDefaultsOnly, Category = "Classes|GameplayEffect|Regeneration")
	TSubclassOf<UGameplayEffect> GE_ManaRegen;

	UPROPERTY(EditDefaultsOnly, Category = "Classes|GameplayEffect|Regeneration")
	TSubclassOf<UGameplayEffect> GE_StaminaRegen;

	// Degeneration Effects
	UPROPERTY(EditDefaultsOnly, Category = "Classes|GameplayEffect|Degeneration")
	TSubclassOf<UGameplayEffect> GE_StaminaDegen;

	/* ============================= */
	/* ===   Active Effect Handles === */
	/* ============================= */

	UPROPERTY()
	FActiveGameplayEffectHandle HealthRegenHandle;

	UPROPERTY()
	FActiveGameplayEffectHandle ManaRegenHandle;

	UPROPERTY()
	FActiveGameplayEffectHandle StaminaRegenHandle;

	UPROPERTY()
	FActiveGameplayEffectHandle StaminaDegenHandle;

	/* ============================= */
	/* ===   Gameplay Tags       === */
	/* ============================= */

	UPROPERTY(EditDefaultsOnly, Category = "Stats|Tags")
	FGameplayTag Tag_Sprinting;

	UPROPERTY(EditDefaultsOnly, Category = "Stats|Tags")
	FGameplayTag Tag_CannotRegenHP;

	UPROPERTY(EditDefaultsOnly, Category = "Stats|Tags")
	FGameplayTag Tag_CannotRegenMana;

	UPROPERTY(EditDefaultsOnly, Category = "Stats|Tags")
	FGameplayTag Tag_CannotRegenStamina;

	/** Delegate handle for sprinting tag listener */
	FDelegateHandle SprintingTagDelegateHandle;
};
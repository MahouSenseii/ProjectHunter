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
	UStatsManager();

	virtual void BeginPlay() override;

	/* ============================= */
	/* ===   Initialization      === */
	/* ============================= */

	/** Initialize the stats manager with an ability system component */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void Initialize();

	/* ============================= */
	/* ===   Attribute Init      === */
	/* ============================= */

	/**
	 * Initialize attributes from a data asset configuration.
	 * More flexible than using default effects directly.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Initialization")
	void InitializeAttributesFromConfig(UAttributeConfigDataAsset* Config);

	UFUNCTION(BlueprintCallable, Category = "Stats|Helper")
	FGameplayAttribute FindAttributeByTag(const FGameplayTag& Tag) const;


	UFUNCTION(BlueprintCallable, Category = "Stats|Initialization")
	void InitializeCurrentVitalsToMax();

	/** Set the ability system component */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void SetASC (UAbilitySystemComponent* InASC) { ASC = InASC; }

	/* ============================= */
	/* =======   Setup      ======== */
	/* ============================= */
	UFUNCTION(BlueprintCallable, Category = "Setup")
	APHBaseCharacter* GetOwnerCharacter() {return  Owner;}

	UFUNCTION(BlueprintCallable, Category = "Setup")
	void SetOwnerCharacter(APHBaseCharacter* InOwner) {Owner = InOwner;}
	
	/** Optional: Data asset for more complex initialization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Initialization")
	TObjectPtr<UAttributeConfigDataAsset> AttributeConfig;
	
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

	
	UFUNCTION(BlueprintCallable, Category = "Regen")
	void InitRegen();
	

protected:


	/**
	 * Apply an initialization effect with SetByCaller values.
	 * @param EffectClass - The gameplay effect to apply
	 * @param AttributeValues - Map of gameplay tags to values
	 * @return True if successfully applied
	 */
	bool ApplyInitializationEffect(
		const TSubclassOf<UGameplayEffect>& EffectClass,
		const TMap<FGameplayTag, float>& AttributeValues) ;

	/**
	 * Simple initialization - just applies the effect without SetByCaller.
	 * Use this when your GE has hardcoded values or uses attribute-based magnitudes.
	 */
	bool ApplySimpleInitEffect(const TSubclassOf<UGameplayEffect>& EffectClass) const;

	
	/** Reference to the Ability System Component */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	TObjectPtr<UAbilitySystemComponent> ASC;

	/** Tracks all active temporary modifiers for bulk removal */
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> TemporaryModifiers;

	/* ============================= */
	/* ===   Internal Helpers    === */
	/* ============================= */

	

	/** Create a dynamic instant gameplay effect for modifying attributes */
	static UGameplayEffect* CreateModifierEffect(
		const FGameplayAttribute& Attribute,
		EGameplayModOp::Type ModifierOp,
		float Magnitude);

	/** Validate that ASC is ready */
	bool IsInitialized() const { return ASC != nullptr; }

	

	/**
	 * Maintains a reference to the character that owns this stats manager.
	 * Used to link the stats manager's functionality to the associated character.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Setup")
	TObjectPtr<APHBaseCharacter> Owner;

	UPROPERTY(EditDefaultsOnly, Category = "Regen")
	TSubclassOf<UGameplayEffect> HealthRegenEffectClass;
    
	UPROPERTY(EditDefaultsOnly, Category = "Regen")
	TSubclassOf<UGameplayEffect> ManaRegenEffectClass;
    
	UPROPERTY(EditDefaultsOnly, Category = "Regen")
	TSubclassOf<UGameplayEffect> StaminaRegenEffectClass;
    
	UPROPERTY(EditDefaultsOnly, Category = "Regen")
	TSubclassOf<UGameplayEffect> ArcaneShieldRegenEffectClass;
};
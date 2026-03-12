// Character/Component/StatsManager.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/BaseStatsData.h"
#include "GameplayEffectTypes.h"
#include "StatsManager.generated.h"

// Forward declarations
class UAbilitySystemComponent;
class UHunterAttributeSet;
class UItemInstance;
class UGameplayEffect;
struct FPHAttributeData;
struct FGameplayAttribute;

DECLARE_LOG_CATEGORY_EXTERN(LogStatsManager, Log, All);
/**
 * Stats Manager Component
 * Handles all stat queries, attribute access, and stat-related calculations
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UStatsManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatsManager();

	virtual void BeginPlay() override;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* EQUIPMENT INTEGRATION (Required by EquipmentManager)                    */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/**
	 * Apply stats from an equipped item via GAS
	 * Called by EquipmentManager when item is equipped
	 * Works with FPHAttributeData arrays (Prefixes/Suffixes/Implicits/Crafted)
	 * @param Item - Item that was equipped
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Equipment")
	void ApplyEquipmentStats(UItemInstance* Item);

	/**
	 * Remove stats from an unequipped item via GAS
	 * Called by EquipmentManager when item is unequipped
	 * @param Item - Item that was unequipped
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Equipment")
	void RemoveEquipmentStats(UItemInstance* Item);

	/**
	 * Refresh all equipment stats (useful after stat recalculation)
	 * Removes all equipment effects and reapplies them
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Equipment")
	void RefreshEquipmentStats();

	/**
	 * Check if item's stats are currently applied
	 */
	UFUNCTION(BlueprintPure, Category = "Stats|Equipment")
	bool HasEquipmentStatsApplied(UItemInstance* Item) const;

	/**
	 * Apply a gameplay effect class to this component's owner.
	 * Intended as a simple Blueprint testing hook.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Effects")
	bool ApplyGameplayEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass, float Level = 1.0f);

	/**
	 * Apply a gameplay effect class to another actor that exposes an ASC.
	 * Intended as a simple Blueprint testing hook.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stats|Effects")
	bool ApplyGameplayEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> EffectClass, float Level = 1.0f);

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* PRIMARY ATTRIBUTES (7)                                                  */
	/* ═══════════════════════════════════════════════════════════════════════ */

	UFUNCTION(BlueprintPure, Category = "Stats|Primary")
	float GetStrength() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Primary")
	float GetIntelligence() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Primary")
	float GetDexterity() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Primary")
	float GetEndurance() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Primary")
	float GetAffliction() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Primary")
	float GetLuck() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Primary")
	float GetCovenant() const;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* SECONDARY/DERIVED ATTRIBUTES                                            */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/**
	 * Get Magic Find stat (affects loot quality and quantity)
	 * FIX: Added for LootChest integration
	 */
	UFUNCTION(BlueprintPure, Category = "Stats|Secondary")
	float GetMagicFind() const;

	/**
	 * Get Item Find stat (affects drop rates)
	 */
	UFUNCTION(BlueprintPure, Category = "Stats|Secondary")
	float GetItemFind() const;

	/**
	 * Get Gold Find stat (affects currency drops)
	 */
	UFUNCTION(BlueprintPure, Category = "Stats|Secondary")
	float GetGoldFind() const;

	/**
	 * Get Experience Bonus stat
	 */
	UFUNCTION(BlueprintPure, Category = "Stats|Secondary")
	float GetExperienceBonus() const;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* VITAL ATTRIBUTES                                                        */
	/* ═══════════════════════════════════════════════════════════════════════ */

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetMana() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetMaxMana() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetManaPercent() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetStamina() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetMaxStamina() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetStaminaPercent() const;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* GENERIC ATTRIBUTE ACCESS                                                */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/**
	 * Get any attribute by name
	 * @param AttributeName - Name of the attribute (e.g., "Strength", "MagicFind")
	 * @return Attribute value, or 0 if not found
	 */
	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetAttributeByName(FName AttributeName) const;

	/**
	 * Check if character meets stat requirements
	 * @param Requirements - Map of AttributeName → RequiredValue
	 * @return True if all requirements are met
	 */
	UFUNCTION(BlueprintPure, Category = "Stats")
	bool MeetsStatRequirements(const TMap<FName, float>& Requirements) const;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* POWER CALCULATIONS                                                      */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/**
	 * Calculate overall power level (for matchmaking, scaling, etc.)
	 * @return Calculated power level
	 */
	UFUNCTION(BlueprintPure, Category = "Stats|Power")
	float GetPowerLevel() const;

	/**
	 * Compare power ratio with another actor
	 * @param OtherActor - Actor to compare against
	 * @return Ratio (>1 = stronger, <1 = weaker)
	 */
	UFUNCTION(BlueprintPure, Category = "Stats|Power")
	float GetPowerRatioAgainst(AActor* OtherActor) const;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* INITIALIZATION                                                          */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Initialize stats from data asset */
	void InitializeFromDataAsset(UBaseStatsData* InStatsData);

	/** Initialize stats from map */
	void InitializeFromMap(const TMap<FName, float>& StatsMap) const;

	/** Set single stat value */
	void SetStatValue(FName AttributeName, float Value) const;

protected:
	/* ═══════════════════════════════════════════════════════════════════════ */
	/* INTERNAL HELPERS                                                        */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Get AttributeSet from owner */
	UHunterAttributeSet* GetAttributeSet() const;

	/** Get AbilitySystemComponent from owner */
	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	/**
	 * Create a gameplay effect for equipment stats
	 * FIX: Uses unique naming based on item GUID to prevent collisions
	 * @param Item - Item to create effect for
	 * @param Stats - Array of FPHAttributeData from item
	 * @return Gameplay effect spec handle
	 */
	FGameplayEffectSpecHandle CreateEquipmentEffect(UItemInstance* Item, const TArray<FPHAttributeData>& Stats);

	/**
	 * Apply a single stat modifier to effect
	 * @param Effect - UGameplayEffect to modify
	 * @param Stat - FPHAttributeData with modifier info
	 * @param Attribute - Target gameplay attribute
	 * @return True if modifier was added successfully
	 */
	bool ApplyStatModifier(UGameplayEffect* Effect, const FPHAttributeData& Stat, const FGameplayAttribute& Attribute);

	/** Cached references */
	UPROPERTY()
	TObjectPtr<UHunterAttributeSet> CachedAttributeSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Cached References")
	UBaseStatsData* StatsData;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;

	/**
	 * Active equipment effects
	 * Maps item GUID to gameplay effect handle
	 * Used to remove effects when equipment is unequipped
	 */
	UPROPERTY()
	TMap<FGuid, FActiveGameplayEffectHandle> ActiveEquipmentEffects;
};

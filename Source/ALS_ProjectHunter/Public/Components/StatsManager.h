// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Library/AttributeStructsLibrary.h"
#include "StatsManager.generated.h"

class UPHAbilitySystemComponent;
class APHBaseCharacter;
class UAttributeConfigDataAsset;
DECLARE_LOG_CATEGORY_EXTERN(LogStatsManager, Log, All);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALS_PROJECTHUNTER_API UStatsManager : public UActorComponent
{
	GENERATED_BODY()

public:

	/* ============================= */
	/* ===   Helper Functions    === */
	/* ============================ */
	
	UFUNCTION(BlueprintCallable, Category = "GAS|Attributes")
	void DebugPrintStartupEffects() const;
	
	/* =========================== */
	/* ===   Core Functions    === */
	/* =========================== */
	
	// Sets default values for this component's properties
	UStatsManager();

	// Called when the game starts
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/* =========================== */
	/* ===   Initialization    === */
	/* =========================== */
	
	UFUNCTION(BlueprintCallable, Category="GAS")
	void InitStatsManager(UPHAbilitySystemComponent* InASC);

	void InitializeDefaultAttributes();

	// Initialize with level scaling (for mobs)
	UFUNCTION(BlueprintCallable, Category = "Mob Configuration")
	void InitializeMobAttributesAtLevel(int32 Level);
	
	/* =========================== */
	/* ===   Attribute Access  === */
	/* =========================== */
	
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetStatBase(const FGameplayAttribute& Attr) const;

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void ApplyFlatStatModifier(const FGameplayAttribute& Attr, float InValue) const;

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void RemoveFlatStatModifier(const FGameplayAttribute& Attribute, float Delta);

	/* =========================== */
	/* ===   Effect Application === */
	/* =========================== */
	
	UFUNCTION(BlueprintCallable)
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float InLevel) const;

	UFUNCTION(BlueprintCallable)
	FActiveGameplayEffectHandle ApplyEffectToSelfWithReturn(TSubclassOf<UGameplayEffect> InEffect, float InLevel);

	/* =========================== */
	/* ===   Periodic Effects  === */
	/* =========================== */
	
	UFUNCTION(BlueprintCallable, Category="GAS|Vital")
	void ApplyPeriodicEffectToSelf(const FInitialGameplayEffectInfo& EffectInfo);

	// Clears all periodic effects currently applied to the character
	UFUNCTION(BlueprintCallable, Category = "GAS|Periodic")
	void ClearAllPeriodicEffects();
	
	// Checks if any valid periodic effect exists for the given tag
	UFUNCTION(BlueprintCallable, Category = "GAS|Periodic")
	bool HasValidPeriodicEffect(FGameplayTag EffectTag) const;

	// Removes all invalid handles from the periodic effect map
	UFUNCTION(BlueprintCallable, Category = "GAS|Periodic")
	void PurgeInvalidPeriodicHandles();

	// Removes all active periodic effects matching the given tag
	UFUNCTION(BlueprintCallable, Category = "GAS|Periodic")
	void RemovePeriodicEffectByTag(FGameplayTag EffectTag);

	void ReapplyAllStartupRegenEffects();
	void StartSprintStaminaDrain();
	void StopSprintStaminaDrain();
	bool CanStartSprinting()const;
	void CheckStaminaForSprint();

	/* =========================== */
	/* ===   Event Callbacks   === */
	/* =========================== */
	
	// Triggered when a regen/degen tag changes (can force reapplication)
	UFUNCTION(BlueprintCallable, Category = "GAS|Periodic")
	void OnRegenTagChanged(FGameplayTag ChangedTag, int32 NewCount);

	// Triggered when any rate/amount attribute changes
	void OnAnyVitalPeriodicStatChanged(const FOnAttributeChangeData& Data);

protected:
	/* =========================== */
	/* ===   Protected Utils   === */
	/* =========================== */
	
	APHBaseCharacter* GetOwnerSafe() const;

	// Helper function for data asset initialization
	void InitializeAttributesFromConfig(const TSubclassOf<UGameplayEffect>& EffectClass, 
	                                   const TArray<FAttributeInitConfig>& AttributeConfigs,
	                                   const FString& CategoryName) const;

	// Validation helper
	static float ValidateInitValue(float Value, const FString& AttributeName);

	// Simple level scaling for regular data assets
	void ApplyLevelScalingToConfig() const;

public:
	/* =========================== */
	/* ===   Blueprint Exposed === */
	/* =========================== */
	
	UPROPERTY(BlueprintReadOnly, Category = "Owner")
	TObjectPtr<APHBaseCharacter> Owner;

	UPROPERTY(BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UPHAbilitySystemComponent> ASC;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS")
	TArray<FInitialGameplayEffectInfo> StartupEffects;

	/* =========================== */
	/* ===   Configuration      === */
	/* =========================== */
	
	// Data asset reference replaces all the manual structs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	TObjectPtr<UAttributeConfigDataAsset> AttributeConfig;

	// Level for mob scaling
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	int32 MobLevel = 1;

	// Additional scaling multiplier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float DifficultyMultiplier = 1.0f;

	// Elite status for bonus stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	bool bIsElite = false;

	// Variant string for special types
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	FString MobVariant = TEXT("Normal");

	/* =========================== */
	/* ===   Default Effects   === */
	/* =========================== */
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GAS|Attributes", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DefaultPrimaryAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GAS|Attributes", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DefaultSecondaryCurrentAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GAS|Attributes", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DefaultSecondaryMaxAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GAS|Attributes", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DefaultVitalAttributes;

	UPROPERTY(EditDefaultsOnly, Category = "Stamina|Sprint")
	TSubclassOf<UGameplayEffect> SprintStaminaDrainEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Stamina|Sprint")
	float MinimumStaminaToStartSprint = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Stamina|Sprint")
	bool bStopSprintingWhenStaminaDepleted = true;

	FActiveGameplayEffectHandle ActiveSprintDrainHandle;
	FTimerHandle StaminaCheckTimer;
	
private:
	/* =========================== */
	/* ===   Private Data      === */
	/* =========================== */
	
	// Store active periodic effects by SetByCaller tag
	TMultiMap<FGameplayTag, FActiveGameplayEffectHandle> ActivePeriodicEffects;

	UPROPERTY()
	bool bIsInitialized = false;
    
	UPROPERTY()
	bool bIsInitializingAttributes = false;
};
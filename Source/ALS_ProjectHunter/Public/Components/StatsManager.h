// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Library/AttributeStructsLibrary.h"
#include "StatsManager.generated.h"

class UPHAbilitySystemComponent;
class APHBaseCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALS_PROJECTHUNTER_API UStatsManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UStatsManager();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetStatBase(const FGameplayAttribute& Attr) const;

	

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void ApplyFlatStatModifier(const FGameplayAttribute& Attr, float InValue) const;

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void RemoveFlatStatModifier(const FGameplayAttribute& Attribute, float Delta);

	/** Effect Application */
	UFUNCTION(BlueprintCallable)
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float InLevel) const;

	UFUNCTION(BlueprintCallable)
	FActiveGameplayEffectHandle ApplyEffectToSelfWithReturn(TSubclassOf<UGameplayEffect> InEffect, float InLevel);

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
	void RemoveAllPeriodicEffectsByTag(FGameplayTag EffectTag);

	// Triggered when a regen/degen tag changes (can force reapplication)
	UFUNCTION(BlueprintCallable, Category = "GAS|Periodic")
	void OnRegenTagChanged(FGameplayTag ChangedTag, int32 NewCount);

	// Triggered when any rate/amount attribute changes
	void OnAnyVitalPeriodicStatChanged(const FOnAttributeChangeData& Data);


	/** Default Attribute Initialization */
	void InitializeDefaultAttributes();
	void ReapplyAllStartupRegenEffects();
	
	
	/** Primary attributes set at character spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Init")
	FInitialPrimaryAttributes PrimaryInitAttributes;

	/** Secondary attributes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Init")
	FInitialSecondaryAttributes SecondaryInitAttributes;
	

	/** Vital attributes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Init")
	FInitialVitalAttributes VitalInitAttributes;
	

	
	/* ====================================== */
	/* ====     Default Attributes      ==== */
	/* ==================================== */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GAS|Attributes", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DefaultPrimaryAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GAS|Attributes", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DefaultSecondaryCurrentAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GAS|Attributes", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DefaultSecondaryMaxAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GAS|Attributes", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DefaultVitalAttributes;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;


	UFUNCTION(BlueprintCallable, Category="GAS")
	void InitStatsManager(UPHAbilitySystemComponent* InASC);
	
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	/* =========================== */
	/* ===     Utilities       === */
	/* =========================== */
	UPROPERTY(BlueprintReadOnly, Category = "Owner")
	TObjectPtr<APHBaseCharacter> Owner;

	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS")
	TArray<FInitialGameplayEffectInfo> StartupEffects;

	UPROPERTY(BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UPHAbilitySystemComponent> ASC;
	
	// Store active periodic effects by SetByCaller ta
	TMultiMap<FGameplayTag, FActiveGameplayEffectHandle> ActivePeriodicEffects;

	
};

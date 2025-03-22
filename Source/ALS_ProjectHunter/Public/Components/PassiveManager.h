// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemComponent.h"
#include "PassiveManager.generated.h"

struct FPHCharacterPassive;
struct FPassiveEffectInfo;

USTRUCT()
struct FPassiveHandleList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> Handles;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALS_PROJECTHUNTER_API UPassiveManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPassiveManager();

	// Runtime Operations
	UFUNCTION(BlueprintCallable, Category = "Passives")
	void ApplyPassive(const FPassiveEffectInfo& Passive);

	UFUNCTION(BlueprintCallable, Category = "Passives")
	void ApplyPassiveGroup(const FPHCharacterPassive& PassiveGroup);

	UFUNCTION(BlueprintCallable, Category = "Passives")
	void RemovePassiveByID(FName PassiveID);

	UFUNCTION(BlueprintCallable, Category = "Passives")
	void RemoveAllPassives();

	// Passive Tracking
	UPROPERTY(BlueprintReadOnly, Category = "Passives")
	TSet<FName> UnlockedPassiveIDs;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;


public:
	
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASC;

	/** Map of applied passive handles keyed by PassiveID */
	UPROPERTY()
	TMap<FName, FPassiveHandleList> AppliedPassives;
};

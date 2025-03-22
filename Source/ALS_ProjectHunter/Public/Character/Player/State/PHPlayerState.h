// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"

#include "GameFramework/PlayerState.h"
#include "PHPlayerState.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class ULevelUpInfo;

/**
 * 
 */

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerStatChanged, int32 /*StatValue*/)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLevelChanged, int32 /*StatValue*/, bool /*bLevelUp*/)

UCLASS()
class ALS_PROJECTHUNTER_API APHPlayerState : public APlayerState, public  IAbilitySystemInterface
{
	GENERATED_BODY()

public:

	APHPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable) virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UFUNCTION(BlueprintCallable) UAttributeSet* GetAttributeSet() const { return AttributeSet;}
	
	FOnPlayerStatChanged OnXPChangedDelegate;
	FOnLevelChanged OnLevelChangedDelegate;
	FOnPlayerStatChanged OnAttributePointsChangedDelegate;
	FOnPlayerStatChanged OnMaxAttributePointsChangedDelegate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leveling")
	TObjectPtr<ULevelUpInfo> LevelUpInfo;

	UFUNCTION(BlueprintCallable, Category = "Leveling")
	bool TryLevelUp();


	FORCEINLINE int32 GetPlayerLevel() const { return Level; }
	FORCEINLINE int32 GetXP() const { return XP; }
	FORCEINLINE int32 GetAttributePoints() const { return AttributePoints; }
	FORCEINLINE int32 GetMaxAttributePoints() const { return MaxAttributePoints; }
	FORCEINLINE int32 GetXPForNextLevel() const;
	void AddToXP(int32 InXp);
	void SetXP(int32 InXP);

	
	void AddToLevel();
	void SetLevel(int32 InLevel);
	
	void SetAttributePoints(int32 InPoints);
	void SetMaxAttributePoints(int32 InPoints);

private:
	
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Level)
	int32 Level = 1;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_XP)
	int32 XP = 0;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_AttributePoints)
	int32 AttributePoints = 0;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_MaxAttributePoints)
	int32 MaxAttributePoints = 0;

	UFUNCTION()
	void OnRep_Level(int32 OldLevel) const;

	UFUNCTION()
	void OnRep_XP(int32 OldXP) const;

	UFUNCTION()
	void OnRep_AttributePoints(int32 InPoints) const;

	UFUNCTION()
	void OnRep_MaxAttributePoints(int32 InPoints);
	
protected:
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS")
	TObjectPtr<UAttributeSet> AttributeSet;
	
	
};

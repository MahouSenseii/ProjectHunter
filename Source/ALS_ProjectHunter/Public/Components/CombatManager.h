// Copyright@2024 Quentin Davis 
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/PHBaseCharacter.h"
#include "Library/PHCombatStructLibrary.h"
#include "CombatManager.generated.h"

class UUserWidget;
class UAbilitySystemComponent;
struct FGameplayAttribute;

/** Broadcast after damage is applied (server-side listeners only) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamageApplied, float, TotalDamage, EDamageTypes, HighestDamageType, bool, IsCrit);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UCombatManager : public UActorComponent
{
	GENERATED_BODY()

public:
	/* =========================== */
	/* === Constructor & Setup === */
	/* =========================== */

	/** Sets default values for this component's properties */
	UCombatManager();

	/** Called after damage application to check death state, ragdoll, etc. */
	UFUNCTION(BlueprintCallable, Category="Combat")
	void CheckAliveStatus();

	/** Returns true if the owner is currently blocking */
	UFUNCTION(BlueprintCallable, Category="Combat")
	bool IsBlocking() const { return bIsBlocking; }

	/** Set the blocking state */
	UFUNCTION(BlueprintCallable, Category="Combat")
	void SetBlocking(bool bNewBlocking) { bIsBlocking = bNewBlocking; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/** Chance roll for applying a status effect by damage type */
	UFUNCTION()
	static bool RollForStatusEffect(const APHBaseCharacter* Attacker, EDamageTypes DamageType);

	/** Calculate the final damage values after bonuses and resistances */
	UFUNCTION(BlueprintCallable, Category="Combat")
	FDamageHitResultByType CalculateDamage(const APHBaseCharacter* Attacker, const APHBaseCharacter* Defender) const;

	/** Apply calculated damage, handle blocking/poise/stagger, broadcast, popups, alignment */
	UFUNCTION(BlueprintCallable, Category="Combat")
	void ApplyDamage(const APHBaseCharacter* Attacker, APHBaseCharacter* Defender);

	/** (Legacy helper) Spawn a damage popup at the owner location (used by server listeners or local testing) */
	UFUNCTION(BlueprintCallable, Category="Combat|UI")
	void InitPopup(float DamageAmount, EDamageTypes DamageType, bool bIsCrit);

	/** Returns the damage type with the highest value in a hit result */
	UFUNCTION(BlueprintPure, Category="Combat")
	static EDamageTypes GetHighestDamageType(const FDamageHitResultByType& HitResult);

	/** Tracks hits on the DEFENDER and flips alignment after a threshold */
	UFUNCTION()
	void HandleAlignmentShift(APHBaseCharacter* Defender);

public:
	/* ================== */
	/* === Anim / UI  === */
	/* ================== */

	/** Stagger montages played when poise breaks or block fails */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim", meta=(AllowPrivateAccess="true"))
	UAnimMontage* StaggerMontage = nullptr;

	/** Damage popup widget class */
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UUserWidget> DamagePopupClass;

	
	/** Event fired after damage has been applied (server-side) */
	UPROPERTY(BlueprintAssignable, Category="Combat")
	FOnDamageApplied OnDamageApplied;

	/** Cached owner */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Owner", meta=(AllowPrivateAccess="true"))
	TObjectPtr<APHBaseCharacter> OwnerCharacter = nullptr;

	/** Increase combo counter and schedule a reset */
	UFUNCTION(BlueprintCallable, Category="Combat")
	void IncreaseComboCounter(float Amount = 1.0f);


protected:
	/* ==================== */
	/* === State / Meta === */
	/* ==================== */

	/** How many times this character has been hit while still not marked as an enemy */
	UPROPERTY(BlueprintReadOnly, Category="Combat")
	int32 TimesHitBeforeEnemy = 0;

private:
	/** True when the character is actively blocking with a shield */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Combat", meta=(AllowPrivateAccess="true"))
	bool bIsBlocking = false;

	/** Reset the combo counter to 0 */
	UFUNCTION()
	void ResetComboCounter() const;

	/** Time in seconds before the combo counter resets */
	UPROPERTY(EditAnywhere, Category="Combat", meta=(AllowPrivateAccess="true"))
	float ComboResetTime = 5.0f;

	/** Cached combo counter */
	UPROPERTY(BlueprintReadOnly, Category="Combat", meta=(AllowPrivateAccess="true"))
	int32 ComboCount = 0;
	
	UPROPERTY()
	FTimerHandle ComboResetTimerHandle;
};

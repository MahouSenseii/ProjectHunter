#pragma once

#include "CoreMinimal.h"
#include "Combat/Library/Structs/CombatStructs.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "UCombatStatusEffectApplier.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

DECLARE_LOG_CATEGORY_EXTERN(LogCombatStatusEffectApplier, Log, All);

// SetByCaller tag names must match the Gameplay Effect assets exactly.
namespace CombatStatusSetByCallerTags
{
	const FName Bleed_DamagePerTick      = TEXT("DoT.Bleed.DamagePerTick");
	const FName Ignite_DamagePerTick     = TEXT("DoT.Ignite.DamagePerTick");
	const FName Poison_DamagePerTick     = TEXT("DoT.Poison.DamagePerTick");
	const FName Corruption_DamagePerTick = TEXT("DoT.Corruption.DamagePerTick");
	const FName Chill_SlowFraction       = TEXT("DoT.Chill.Magnitude");
	const FName Shock_AmpFraction        = TEXT("DoT.Shock.Magnitude");
}

/**
 * Owned helper. UCombatManager creates exactly one of these as its own
 * sub-object (see UCombatManager::CombatStatus) instead of a sibling
 * character component, so status effects still have one owner without a
 * second component on the actor. This is never registered/attached the way
 * a normal actor component is (CombatManager just owns it as a plain
 * sub-object), but it MUST stay UActorComponent-derived: existing saved
 * Blueprints (e.g. ALS_BaseCharacterBP) already have a serialized
 * legacy CombatStatusManager export from before this class was reparented here.
 * Changing this class's layout breaks that old serialized data with a fatal
 * "Serial size mismatch" LinkerLoad error. Kept BlueprintType/UFUNCTION so
 * IsBleeding/IsFrozen/etc. stay Blueprint-callable for the status-icon HUD.
 */
UCLASS(ClassGroup=(ProjectHunter))
class ALS_PROJECTHUNTER_API UCombatStatusEffectApplier : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatStatusEffectApplier();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat Status|Effects")
	TSubclassOf<UGameplayEffect> BleedEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat Status|Effects")
	TSubclassOf<UGameplayEffect> IgniteEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat Status|Effects")
	TSubclassOf<UGameplayEffect> PoisonEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat Status|Effects")
	TSubclassOf<UGameplayEffect> CorruptionEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat Status|Effects")
	TSubclassOf<UGameplayEffect> ChillEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat Status|Effects")
	TSubclassOf<UGameplayEffect> FreezeEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat Status|Effects")
	TSubclassOf<UGameplayEffect> PetrifyEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat Status|Effects")
	TSubclassOf<UGameplayEffect> ShockEffectClass;

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	FCombatStatusApplyResult ApplyBleed(AActor* Target, float DamagePerTick,
		float Duration, AActor* Instigator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	FCombatStatusApplyResult ApplyIgnite(AActor* Target, float DamagePerTick,
		float Duration, AActor* Instigator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	FCombatStatusApplyResult ApplyPoison(AActor* Target, float DamagePerTick,
		float Duration, AActor* Instigator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	FCombatStatusApplyResult ApplyCorruption(AActor* Target, float DamagePerTick,
		float Duration, AActor* Instigator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	FCombatStatusApplyResult ApplyChill(AActor* Target, float SlowFraction,
		float Duration, AActor* Instigator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	FCombatStatusApplyResult ApplyFreeze(AActor* Target, float Duration,
		AActor* Instigator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	FCombatStatusApplyResult ApplyPetrify(AActor* Target, float Duration,
		AActor* Instigator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	FCombatStatusApplyResult ApplyShock(AActor* Target, float AmpFraction,
		float Duration, AActor* Instigator = nullptr);

	UFUNCTION(BlueprintPure, Category = "Combat Status")
	bool IsBleeding(AActor* Target) const;

	UFUNCTION(BlueprintPure, Category = "Combat Status")
	bool IsIgnited(AActor* Target) const;

	UFUNCTION(BlueprintPure, Category = "Combat Status")
	int32 GetPoisonStacks(AActor* Target) const;

	UFUNCTION(BlueprintPure, Category = "Combat Status")
	bool IsCorrupted(AActor* Target) const;

	UFUNCTION(BlueprintPure, Category = "Combat Status")
	bool IsChilled(AActor* Target) const;

	UFUNCTION(BlueprintPure, Category = "Combat Status")
	bool IsFrozen(AActor* Target) const;

	UFUNCTION(BlueprintPure, Category = "Combat Status")
	bool IsPetrified(AActor* Target) const;

	UFUNCTION(BlueprintPure, Category = "Combat Status")
	bool IsShocked(AActor* Target) const;

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void CureBleed(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void CureIgnite(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void CurePoison(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void CureCorruption(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void RemoveChill(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void RemoveFreeze(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void RemovePetrify(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void RemoveShock(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void CleanseAll(AActor* Target);

private:
	FCombatStatusApplyResult ApplyDoTEffect(
		TSubclassOf<UGameplayEffect> EffectClass,
		AActor* Target,
		float SetByCallerValue,
		FName SetByCallerTag,
		float Duration,
		AActor* Instigator) const;

	void RemoveEffectByClass(AActor* Target,
		TSubclassOf<UGameplayEffect> EffectClass) const;

	bool HasActiveEffect(AActor* Target,
		TSubclassOf<UGameplayEffect> EffectClass) const;

	static UAbilitySystemComponent* GetTargetASC(AActor* Target);
};

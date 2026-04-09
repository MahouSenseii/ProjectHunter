#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Systems/Combat/Library/CombatStructs.h"
#include "GameplayEffect.h"
#include "CombatManager.generated.h"

class AActor;
class UAbilitySystemComponent;
class UHunterAttributeSet;

DECLARE_LOG_CATEGORY_EXTERN(LogCombatManager, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UCombatManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatManager();

	/**
	 * B-1 FIX: Blueprint-configurable Instant Gameplay Effect used to apply resolved damage
	 * through the GAS pipeline so PreAttributeChange clamping fires correctly.
	 *
	 * Configure this GE in the Blueprint default with three Additive modifiers:
	 *   Attribute: Health       | Magnitude: SetByCaller (Data.Damage.Health)
	 *   Attribute: ArcaneShield | Magnitude: SetByCaller (Data.Damage.ArcaneShield)
	 *   Attribute: Stamina      | Magnitude: SetByCaller (Data.Damage.Stamina)
	 *
	 * CombatManager will set each magnitude to the negative of the resolved damage amount
	 * (e.g. -50.0 to subtract 50 health). Duration must be Instant.
	 *
	 * If left unset the component falls back to SetNumericAttributeBase with a warning.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat",
		meta = (DisplayName = "Damage Application GE"))
	TSubclassOf<UGameplayEffect> DamageApplicationGE;

	/**
	 * I-01 FIX: Blueprint-configurable Instant GE for on-hit resource recovery
	 * (LifeOnHit, ManaOnHit, StaminaOnHit). Routes recovery through GAS so
	 * PreAttributeChange clamping fires instead of using SetNumericAttributeBase directly.
	 *
	 * Configure this GE with three Additive modifiers:
	 *   Attribute: Health  | Magnitude: SetByCaller (Data.Recovery.Health)
	 *   Attribute: Mana    | Magnitude: SetByCaller (Data.Recovery.Mana)
	 *   Attribute: Stamina | Magnitude: SetByCaller (Data.Recovery.Stamina)
	 *
	 * Duration must be Instant. If left unset, on-hit recovery is skipped with a warning.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat",
		meta = (DisplayName = "Recovery Application GE"))
	TSubclassOf<UGameplayEffect> RecoveryApplicationGE;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool ResolveHit(AActor* SourceActor, AActor* TargetActor, FCombatResolveResult& OutResult);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	FCombatDamagePacket BuildOutgoingDamagePacket(AActor* SourceActor, AActor* TargetActor) const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	FCombatDamagePacket ApplyDamageConversion(const FCombatDamagePacket& InPacket, AActor* SourceActor) const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	FCombatResolveResult MitigateDamagePacket(const FCombatDamagePacket& InPacket, AActor* SourceActor, AActor* TargetActor) const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ApplyResolvedDamage(AActor* SourceActor, AActor* TargetActor, const FCombatResolveResult& Result) const;

protected:
	static UAbilitySystemComponent* GetAbilitySystemComponentFromActor(const AActor* Actor);
	static const UHunterAttributeSet* GetHunterAttributeSetFromActor(const AActor* Actor);

	FCombatDamagePacket BuildOutgoingDamagePacketFromAttributes(const UHunterAttributeSet* SourceAttributes, AActor* SourceActor, AActor* TargetActor) const;
	FCombatDamagePacket ApplyDamageConversionFromAttributes(const FCombatDamagePacket& InPacket, const UHunterAttributeSet* SourceAttributes, AActor* SourceActor) const;
	FCombatResolveResult MitigateDamagePacketAgainstAttributes(const FCombatDamagePacket& InPacket, AActor* SourceActor, AActor* TargetActor, const UHunterAttributeSet* SourceAttributes, const UHunterAttributeSet* TargetAttributes) const;

	static float RollDamageRange(float MinDamage, float MaxDamage);
	float CalculateOutgoingDamageForType(EHunterDamageType DamageType, const UHunterAttributeSet* SourceAttributes) const;
	static float GetResistanceValue(EHunterDamageType DamageType, const UHunterAttributeSet* TargetAttributes);
	float GetResistanceCap(EHunterDamageType DamageType, const UHunterAttributeSet* TargetAttributes) const;
	static float GetPierceValue(EHunterDamageType DamageType, const UHunterAttributeSet* SourceAttributes);
	bool IsActorBlocking(AActor* Actor) const;
	bool CanBlockHit(AActor* SourceActor, AActor* TargetActor, const UHunterAttributeSet* TargetAttributes) const;
	float GetBlockTypeMultiplier(EHunterDamageType DamageType, const UHunterAttributeSet* TargetAttributes) const;
	float GetMoreDamageMultiplier(EHunterDamageType DamageType, const UHunterAttributeSet* SourceAttributes) const;
	float GetDamageTakenMultiplier(EHunterDamageType DamageType, const UHunterAttributeSet* TargetAttributes) const;
	void ApplyBlockingToMitigatedResult(AActor* SourceActor, AActor* TargetActor, const UHunterAttributeSet* TargetAttributes, FCombatResolveResult& InOutResult) const;
	void ApplyStaminaBlockCost(const UHunterAttributeSet* TargetAttributes, FCombatResolveResult& InOutResult) const;

	void ResolveCriticalStrike(FCombatDamagePacket& Packet, const UHunterAttributeSet* SourceAttributes) const;

	// OPT: Accept pre-fetched pointers from ResolveHit to avoid redundant ASC lookups
	// (GetHunterAttributeSetFromActor was called 2-3x per hit). Defaults to nullptr
	// so call sites that don't have them cached still work correctly.
	void ApplyOnHitEffects(
		AActor* SourceActor,
		AActor* TargetActor,
		const FCombatResolveResult& Result,
		UAbilitySystemComponent* CachedSourceASC = nullptr,
		const UHunterAttributeSet* CachedSourceAttributes = nullptr) const;
	void ApplyAilments(AActor* SourceActor, AActor* TargetActor, const FCombatResolveResult& Result) const;
	void ApplyReflect(AActor* SourceActor, AActor* TargetActor, const FCombatResolveResult& Result) const;
};

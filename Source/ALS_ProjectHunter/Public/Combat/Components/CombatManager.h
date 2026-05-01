// ProjectHunter combat state owner for hit resolution and damage application.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Library/CombatStructs.h"
#include "GameplayEffect.h"
#include "CombatManager.generated.h"

class AActor;
class UAbilitySystemComponent;
class UHunterAttributeSet;

DECLARE_LOG_CATEGORY_EXTERN(LogCombatManager, Log, All);

UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UCombatIncomingHitEditContext : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Packet")
	TObjectPtr<AActor> AttackerActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Packet")
	TObjectPtr<AActor> DefenderActor = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Combat|Packet")
	FCombatHitPacket HitPacket;

	UPROPERTY(BlueprintReadWrite, Category = "Combat|Packet")
	bool bApplyHit = true;

	UFUNCTION(BlueprintCallable, Category = "Combat|Packet")
	void RejectHit();
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEditIncomingHitPacket, UCombatIncomingHitEditContext*, Context);

UCLASS(ClassGroup=(Custom), Blueprintable, meta=(BlueprintSpawnableComponent))
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

	/**
	 * Instant GE used to pay skill activation costs.
	 *
	 * Configure this GE with three Additive modifiers:
	 *   Attribute: Health  | Magnitude: SetByCaller (Data.Cost.Health)
	 *   Attribute: Mana    | Magnitude: SetByCaller (Data.Cost.Mana)
	 *   Attribute: Stamina | Magnitude: SetByCaller (Data.Cost.Stamina)
	 *
	 * Duration must be Instant. If left unset, costs fall back to direct attribute
	 * mutation so Mana costs still hit Mana rather than ArcaneShield.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat",
		meta = (DisplayName = "Cost Application GE"))
	TSubclassOf<UGameplayEffect> CostApplicationGE;

	/**
	 * Instant GE used to apply reflected damage back to the original attacker.
	 * Must be non-recursive — this GE should NOT trigger ApplyAilments or ApplyReflect.
	 *
	 * Configure with one Additive modifier:
	 *   Attribute: Health | Magnitude: SetByCaller (Data.Damage.Health)
	 *
	 * Duration must be Instant. If left unset, reflect damage is logged but not applied.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat",
		meta = (DisplayName = "Reflect Application GE"))
	TSubclassOf<UGameplayEffect> ReflectApplicationGE;

	// Server-side Blueprint edit point. Bind in BP to modify Context.HitPacket before
	// CombatManager calculates conversion, scaling, mitigation, costs, and application.
	UPROPERTY(BlueprintAssignable, Category = "Combat|Packet")
	FOnEditIncomingHitPacket OnEditIncomingHitPacket;

	// One Blueprint packet node. Fill Offense, Defense, Cost, HitResponse, and
	// ailment flags here, then feed the result into ApplyHit.
	UFUNCTION(BlueprintPure, Category = "Combat|Packet", meta = (NativeMakeFunc))
	static FCombatHitPacket MakeCombatHitPacket(
		const FCombatOffensePacket& Offense,
		const FCombatDefensePacket& Defense,
		const FCombatCostPacket& Cost,
		EHitResponse HitResponse = EHitResponse::Normal,
		bool bCanApplyAilments = true);

	// Main hit entry point. Combines the BP-authored packet with attacker stats,
	// defender stats, and the configured GameplayEffects, then applies the result.
	UFUNCTION(BlueprintCallable, Category = "Combat|Packet")
	bool ApplyHit(AActor* AttackerActor, AActor* DefenderActor,
		const FCombatHitPacket& HitPacket, FCombatResolveResult& OutResult);

protected:
	static UAbilitySystemComponent* GetAbilitySystemComponentFromActor(const AActor* Actor);
	static const UHunterAttributeSet* GetHunterAttributeSetFromActor(const AActor* Actor);

	// SkillPacket = nullptr → attribute-only path (no skill layering)
	FCombatDamagePacket BuildOutgoingDamagePacketFromAttributes(
		const UHunterAttributeSet* SourceAttributes,
		AActor* SourceActor,
		AActor* TargetActor,
		const FSkillDamagePacket* SkillPacket = nullptr) const;

	// Expects an unscaled base packet. Conversion runs before type-specific increased,
	// more, and crit so converted damage scales only as its final damage type.
	FCombatDamagePacket ApplyDamageConversionFromAttributes(const FCombatDamagePacket& InPacket, const UHunterAttributeSet* SourceAttributes, AActor* SourceActor) const;
	FCombatDamageTransformRules BuildDamageConversionRulesFromAttributes(const UHunterAttributeSet* SourceAttributes) const;
	FCombatDamageTransformRules CombineDamageTransformRules(
		const FCombatDamageTransformRules& A,
		const FCombatDamageTransformRules& B) const;
	FCombatDamagePacket ApplyDamageTransformRules(
		const FCombatDamagePacket& InPacket,
		const FCombatDamageTransformRules& ConversionRules,
		const FCombatDamageTransformRules& GainAsExtraRules) const;
	FCombatDamagePacket BuildOutgoingDamagePacketFromHitPacket(
		const FCombatHitPacket& HitPacket,
		const UHunterAttributeSet* AttackerAttributes,
		AActor* AttackerActor) const;

	FCombatResolveResult MitigateDamagePacketAgainstAttributes(
		const FCombatDamagePacket& InPacket,
		AActor* SourceActor,
		AActor* TargetActor,
		const UHunterAttributeSet* SourceAttributes,
		const UHunterAttributeSet* TargetAttributes,
		const FCombatHitPacket* HitPacket = nullptr) const;

	void ApplyResolvedDamage(AActor* SourceActor, AActor* TargetActor, const FCombatResolveResult& Result) const;

	static float RollDamageRange(float MinDamage, float MaxDamage);

	// SkillPacket = nullptr → use full weapon range from attributes (no skill base damage).
	float CalculateBaseDamageForType(
		EHunterDamageType DamageType,
		const UHunterAttributeSet* SourceAttributes,
		const FSkillDamagePacket* SkillPacket = nullptr) const;

	float CalculateScaledDamageForType(
		EHunterDamageType DamageType,
		float BaseDamage,
		const UHunterAttributeSet* SourceAttributes,
		const FSkillDamagePacket* SkillPacket = nullptr) const;

	float CalculateScaledDamageForHitPacketType(
		EHunterDamageType DamageType,
		float BaseDamage,
		const UHunterAttributeSet* SourceAttributes,
		const FCombatHitPacket& HitPacket) const;

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

	// bCanCrit = false skips the crit roll entirely (e.g. skill sets bCanCrit = false)
	void ResolveCriticalStrike(FCombatDamagePacket& Packet, const UHunterAttributeSet* SourceAttributes,
		bool bCanCrit = true, const FSkillDamagePacket* SkillPacket = nullptr) const;

	void ResolveHitPacketCriticalStrike(FCombatDamagePacket& Packet,
		const UHunterAttributeSet* AttackerAttributes,
		const FCombatHitPacket& HitPacket) const;

	// Evaluates stagger eligibility after mitigation. Sets OutResult.bShouldStagger when stamina will be depleted
	// and the target does not have the State_Self_ExecutingSkill gameplay tag.
	void EvaluateStagger(AActor* TargetActor, const UHunterAttributeSet* TargetAttributes,
		FCombatResolveResult& InOutResult) const;

	void ApplyHitResponse(const FCombatHitPacket& HitPacket, FCombatResolveResult& InOutResult) const;

	// Deducts StaminaCost / ManaCost / HealthCost from the source actor's ASC once
	// at activation time after applying the matching CostChanges attributes.
	void ApplySkillCostToSource(AActor* SourceActor, const FSkillDamagePacket& SkillPacket) const;

	// Accepts pre-fetched pointers from ApplyHit to avoid redundant ASC lookups.
	void ApplyOnHitEffects(
		AActor* SourceActor,
		AActor* TargetActor,
		const FCombatResolveResult& Result,
		UAbilitySystemComponent* CachedSourceASC = nullptr,
		const UHunterAttributeSet* CachedSourceAttributes = nullptr) const;

	// Respects Result.bShouldApplyAilments and Result.HitResponse.
	// Parry path: ailments roll by flat chance only (Elden Ring style — no buildup roll).
	// Invincible path: fully skipped (bShouldApplyAilments == false).
	void ApplyAilments(AActor* SourceActor, AActor* TargetActor, const FCombatResolveResult& Result) const;

	void ApplyReflect(AActor* SourceActor, AActor* TargetActor, const FCombatResolveResult& Result) const;
};

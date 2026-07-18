// Shared combat structs used across ProjectHunter combat systems.
#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Combat/Library/Enums/CombatEnums.h"
#include "CombatStructs.generated.h"

class AActor;

/**
 * Animation-authored additive increased damage percentages per damage type.
 * These join the attacker's additive increased pool for a single hit.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FAnimationBaseDamageMulti
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float Physical = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float Fire = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float Ice = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float Lightning = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float Light = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float Corruption = 0.f;
};

/** Animation-authored penetration percentages, clamped by the combat pipeline. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FAnimationPiercingMulti
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float ArmourPiercing = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float Fire = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float Ice = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float Lightning = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float Light = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float Corruption = 0.f;
};

/** Animation-authored critical strike behavior for one hit. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FAnimationCritInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	bool bCanCrit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	bool bForceCrit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float CritChance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float CritMultiplier = 0.f;
};

/**
 * Skill category flags for one hit. Each true flag pulls the matching
 * conditional damage attributes into the additive pool.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FAnimationSkillTags
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	bool bIsMelee = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	bool bIsRanged = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	bool bIsSpell = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	bool bIsArea = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	bool bIsDamageOverTime = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	bool bIsChainHit = false;
};

/**
 * Blueprint-authored hit input. Animation notifies fill this per swing; weapon
 * damage, scaling, mitigation, block, ailments, leech, and reflect are resolved
 * from attacker and defender attributes.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FAnimationDamageInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	FAnimationBaseDamageMulti BaseMulti;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	FAnimationPiercingMulti Piercing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	FAnimationCritInfo Crit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	FAnimationSkillTags Tags;
};

/** Per-type damage snapshot produced before mitigation. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatDamagePacket
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Physical = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Fire = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Ice = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Lightning = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Light = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Corruption = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bCrit = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float CritMultiplierApplied = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float TotalPreMitigation = 0.f;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatResolveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	FCombatDamagePacket PreMitigationPacket;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float PhysicalTaken = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float FireTaken = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float IceTaken = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float LightningTaken = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float LightTaken = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float CorruptionTaken = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float PhysicalBlocked = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float FireBlocked = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float IceBlocked = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float LightningBlocked = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float LightBlocked = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float CorruptionBlocked = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float TotalDamageBeforeBlock = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float TotalDamageAfterBlock = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float TotalBlockedAmount = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float DamageToStamina = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float DamageToArcaneShield = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float DamageToHealth = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float TotalDamageTaken = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float HealthAfterHit = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bKilledTarget = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bWasCrit = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bWasBlocked = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bGuardBroken = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	EHitResponse HitResponse = EHitResponse::Normal;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bShouldApplyAilments = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bShouldStagger = false;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatDamagePopupData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage Popup")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage Popup")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage Popup")
	FCombatResolveResult ResolveResult;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage Popup")
	float TotalDamage = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage Popup")
	EHunterDamageType DominantDamageType = EHunterDamageType::Physical;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage Popup")
	FLinearColor DisplayColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage Popup")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage Popup")
	bool bWasCrit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage Popup")
	bool bWasBlocked = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage Popup")
	bool bKilledTarget = false;
};

/** Result of a UCombatStatusEffectApplier Apply* call. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatStatusApplyResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Status")
	bool bApplied = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Status")
	FActiveGameplayEffectHandle EffectHandle;
};

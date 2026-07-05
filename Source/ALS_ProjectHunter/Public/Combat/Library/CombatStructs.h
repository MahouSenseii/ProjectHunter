// Shared combat structs used across ProjectHunter combat systems.
#pragma once

#include "CoreMinimal.h"
#include "Combat/Library/CombatEnumLibrary.h"
#include "ActiveGameplayEffectHandle.h"
#include "CombatStructs.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct FCombatAffiliation
{
	GENERATED_BODY()

	// The permanent faction identity of the actor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	EFaction Faction = EFaction::Neutral;

	// The current alignment state toward a specific context.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	ECombatAlignment Alignment = ECombatAlignment::None;

	bool operator==(const FCombatAffiliation& Other) const
	{
		return Faction == Other.Faction && Alignment == Other.Alignment;
	}

	bool operator!=(const FCombatAffiliation& Other) const
	{
		return !(*this == Other);
	}
};

/**
 * Animation-authored additive "increased damage" percentages per damage type.
 * Each value joins the attacker's additive increased pool for that type
 * (gear + passives + this), so 50 here behaves exactly like "50% increased
 * Fire damage" on an item, scoped to this one swing.
 * 0 = the animation adds nothing for that type.
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

/**
 * Animation-authored penetration percentages. Added on top of the attacker's
 * piercing attributes, then clamped 0–100 by the pipeline. Resistance piercing
 * lowers the defender's effective resistance of that element; ArmourPiercing
 * reduces the armour value used against the physical portion of the hit.
 */
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

/**
 * Animation-authored critical strike behavior for one hit.
 * CritChance adds percentage points to the attacker's crit chance attributes.
 * CritMultiplier adds to the crit damage factor (0.25 = +25% crit damage on
 * top of the attacker's CritMultiplier attribute).
 * bForceCrit only fires when bCanCrit is also true.
 * Damage-over-time hits never crit regardless of these values.
 */
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
 * conditional "increased damage" attribute into the additive pool
 * (MeleeDamage, RangedDamage, SpellDamage, AreaDamage, DamageOverTime,
 * ChainDamage) and drives tag-specific rules (spells use spell crit
 * attributes, damage-over-time hits cannot crit).
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
 * The single Blueprint-authored input for UCombatManager::ApplyHit.
 * Animation notifies fill this per swing; everything else (weapon damage,
 * flat added damage, conversion, increased/more scaling, resistances,
 * block, ailments, leech, reflect) is resolved from attacker and defender
 * attributes inside CombatManager.
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

/**
 * Per-type damage snapshot produced by the outgoing damage stages.
 * Carried in FCombatResolveResult::PreMitigationPacket for UI/debug breakdown.
 */
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

	// How the defender resolved this hit. Drives damage routing and ailment rules.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	EHitResponse HitResponse = EHitResponse::Normal;

	// True = ailment chance rolls run (Normal / Parry / Blocked paths).
	// False = ailments fully skipped (Invincible).
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bShouldApplyAilments = true;

	// Set when the hit depleted stamina and the target was not executing a skill.
	// Blueprint reads this to play the stagger montage.
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

/** Result of a UCombatStatusManager Apply* call. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatStatusApplyResult
{
	GENERATED_BODY()

	/** True if the effect was successfully applied to the target. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat Status")
	bool bApplied = false;

	/** Handle of the active GE instance (invalid if bApplied == false). */
	UPROPERTY(BlueprintReadOnly, Category = "Combat Status")
	FActiveGameplayEffectHandle EffectHandle;
};

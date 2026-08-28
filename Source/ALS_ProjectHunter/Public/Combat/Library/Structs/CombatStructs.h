// Shared combat structs used across ProjectHunter combat systems.
#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"
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

/**
 * Skill-authored base damage before attacker modifiers. Attack skills normally
 * leave this at zero and scale weapon damage; spells can set weapon
 * effectiveness to zero and author their base damage here.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FAnimationSkillBaseDamage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation", meta = (ClampMin = "0.0"))
	float Physical = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation", meta = (ClampMin = "0.0"))
	float Fire = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation", meta = (ClampMin = "0.0"))
	float Ice = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation", meta = (ClampMin = "0.0"))
	float Lightning = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation", meta = (ClampMin = "0.0"))
	float Light = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation", meta = (ClampMin = "0.0"))
	float Corruption = 0.f;
};

/** One Blueprint-authored damage conversion or gain-as-extra rule. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatDamageConversionRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Conversion")
	EHunterDamageType From = EHunterDamageType::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Conversion")
	EHunterDamageType To = EHunterDamageType::Fire;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Conversion",
		meta = (ClampMin = "0.0", Units = "Percent"))
	float Percent = 0.f;

	/** Gain-as-extra copies damage without removing it from the source type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Conversion")
	bool bGainAsExtra = false;
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

/** Where an attack landed relative to the defender's facing. */
UENUM(BlueprintType)
enum class EHitDirection : uint8
{
	Front UMETA(DisplayName = "Front"),
	Flank UMETA(DisplayName = "Flank"),
	Rear  UMETA(DisplayName = "Rear")
};

/**
 * Tunable positional damage rules.
 *
 * Rear hits deal more damage. This is a damage modifier only - it is NOT a
 * Souls-style backstab execution, and deliberately has no animation, camera or
 * cinematic component. If a dedicated backstab is added later, Blueprint drives
 * the attack flow and C++ keeps owning validation and damage.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatPositionalRules
{
	GENERATED_BODY()

	/** Master toggle for positional damage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Positional")
	bool bEnablePositionalDamage = true;

	/**
	 * Total cone behind the defender that counts as a rear hit, in degrees.
	 * 120 means the attacker must be within 60 degrees of directly behind.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Positional",
		meta = (ClampMin = "0.0", ClampMax = "360.0", Units = "Degrees"))
	float RearAttackAngle = 120.f;

	/**
	 * Total cone in front of the defender that counts as a front hit, in degrees.
	 * Anything between the front and rear cones is a flank.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Positional",
		meta = (ClampMin = "0.0", ClampMax = "360.0", Units = "Degrees"))
	float FrontAttackAngle = 180.f;

	/** Damage ratio for a rear hit. 1.0 neutral, 1.5 = +50% from behind. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Positional",
		meta = (ClampMin = "0.0", UIMin = "1.0", UIMax = "3.0"))
	float BackDamageMultiplier = 1.5f;

	/** Damage ratio for a flank hit. 1.0 neutral. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Positional",
		meta = (ClampMin = "0.0", UIMin = "1.0", UIMax = "3.0"))
	float FlankDamageMultiplier = 1.f;
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

	/** Additive crit chance bonus for this hit, in percentage points. 5 = +5% chance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation",
		meta = (ToolTip = "Additive crit chance bonus in percentage points. 5 means +5% chance. 0 = no bonus."))
	float CritChance = 0.f;

	/**
	 * Additive crit multiplier bonus for this hit, as a ratio delta above neutral.
	 * 0 = no bonus, 0.5 = +50% crit damage, 1.0 = +100% crit damage.
	 * This is a DELTA, unlike the CritMultiplier attribute which is an absolute
	 * ratio (1.5 = 150%). Do not enter 50 here expecting +50%.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation",
		meta = (UIMin = "0.0", UIMax = "3.0",
			ToolTip = "Additive crit multiplier bonus as a ratio delta. 0 = no bonus, 0.5 = +50% crit damage. NOT a percentage - do not enter 50."))
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

	/** Base damage supplied by the skill itself. Useful for spells and weapon-independent attacks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	FAnimationSkillBaseDamage SkillBaseDamage;

	/** Percentage of the equipped weapon damage range used by this hit. 100 is a normal weapon hit; 0 ignores the weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation",
		meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "500.0", Units = "Percent"))
	float WeaponDamageEffectivenessPercent = 100.f;

	/** Percentage of flat added damage from attacker attributes used by this hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation",
		meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "500.0", Units = "Percent"))
	float AddedDamageEffectivenessPercent = 100.f;

	/** Weapon hand used by this hit. Automatic prefers two-hand, then main hand, then off hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	ECombatWeaponSource WeaponSource = ECombatWeaponSource::Automatic;

	/** Skill-inherent conversion runs before character/equipment conversion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	TArray<FCombatDamageConversionRule> SkillDamageConversions;

	/**
	 * Poise/posture damage for this hit. Zero derives it from damage that gets
	 * through block, keeping existing Blueprint attacks useful without setup.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation", meta = (ClampMin = "0.0"))
	float PoiseDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	FAnimationPiercingMulti Piercing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	FAnimationCritInfo Crit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	FAnimationSkillTags Tags;
};

/**
 * Identity and deterministic inputs for one authoritative attack. Reuse the
 * same AttackId and RandomSeed for every trace sample in one swing; the combat
 * manager will reject duplicate targets unless bAllowRepeatHit is enabled.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatHitContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit")
	FGuid AttackId;

	/** Non-zero deterministic seed. The server creates one when this is zero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit")
	int32 RandomSeed = 0;

	/** Increment for intentional repeat hits from the same attack (for example a channel tick). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit", meta = (ClampMin = "0"))
	int32 HitIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit")
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit")
	FVector ImpactDirection = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit")
	FGameplayTagContainer SkillTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hit")
	bool bAllowRepeatHit = false;
};

/** Data-driven defaults used when attacker attributes do not override duration. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatAilmentTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Ailments", meta = (ClampMin = "0.0"))
	float BleedDuration = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Ailments", meta = (ClampMin = "0.0"))
	float IgniteDuration = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Ailments", meta = (ClampMin = "0.0"))
	float FreezeDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Ailments", meta = (ClampMin = "0.0"))
	float ShockDuration = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Ailments", meta = (ClampMin = "0.0"))
	float PetrifyDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Ailments", meta = (ClampMin = "0.0"))
	float CorruptionDuration = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Ailments", meta = (ClampMin = "0.0"))
	float ChillDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Ailments", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BleedDamagePerTickFraction = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Ailments", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IgniteDamagePerTickFraction = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Ailments", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CorruptionDamagePerTickFraction = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Ailments", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShockDamageTakenFraction = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Ailments", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChillSlowFraction = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Ailments")
	bool bColdDamageAlwaysChills = true;
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

	void Scale(const float Multiplier)
	{
		const float SafeMultiplier = FMath::Max(0.f, Multiplier);
		Physical *= SafeMultiplier;
		Fire *= SafeMultiplier;
		Ice *= SafeMultiplier;
		Lightning *= SafeMultiplier;
		Light *= SafeMultiplier;
		Corruption *= SafeMultiplier;
		TotalPreMitigation *= SafeMultiplier;
	}
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

	/** Damage actually removed after current shield and health cap overkill. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float TotalDamageApplied = 0.f;

	/** Poise/posture damage after the defender's PoiseResistance. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float EffectivePoiseDamage = 0.f;

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

	/** Which side the hit landed on. Drives the positional damage bonus and hit reactions. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	EHitDirection HitDirection = EHitDirection::Front;

	/** Positional ratio actually applied to this hit. 1.0 when the hit was frontal. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float PositionalMultiplierApplied = 1.f;

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

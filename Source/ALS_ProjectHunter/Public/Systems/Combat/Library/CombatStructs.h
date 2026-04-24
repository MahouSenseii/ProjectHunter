#pragma once
#include "CoreMinimal.h"
#include "Combat/Library/CombatEnumLibrary.h"
#include "CombatStructs.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct FCombatAffiliation
{
	GENERATED_BODY()

public:

	// The permanent faction identity of the actor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	EFaction Faction = EFaction::Neutral;

	// The current alignment state toward a specific context
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	ECombatAlignment Alignment = ECombatAlignment::None;
	
	bool operator==(const FCombatAffiliation& Other) const
	{
		return Faction == Other.Faction &&
			   Alignment == Other.Alignment;
	}

	bool operator!=(const FCombatAffiliation& Other) const
	{
		return !(*this == Other);
	}
};


UENUM(BlueprintType)
enum class EHunterDamageType : uint8
{
	Physical UMETA(DisplayName = "Physical"),
	Fire UMETA(DisplayName = "Fire"),
	Ice UMETA(DisplayName = "Ice"),
	Lightning UMETA(DisplayName = "Lightning"),
	Light UMETA(DisplayName = "Light"),
	Corruption UMETA(DisplayName = "Corruption")
};

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
struct ALS_PROJECTHUNTER_API FCombatDamageTypeValues
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
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatDamageTypeMultipliers
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Physical = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Fire = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Ice = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Lightning = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Light = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Corruption = 1.f;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatDamageTransformRules
{
	GENERATED_BODY()

	// Percent of Physical damage converted or gained as each destination type.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Transform")
	FCombatDamageTypeValues FromPhysical;

	// Percent of Fire damage converted or gained as each destination type.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Transform")
	FCombatDamageTypeValues FromFire;

	// Percent of Ice damage converted or gained as each destination type.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Transform")
	FCombatDamageTypeValues FromIce;

	// Percent of Lightning damage converted or gained as each destination type.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Transform")
	FCombatDamageTypeValues FromLightning;

	// Percent of Light damage converted or gained as each destination type.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Transform")
	FCombatDamageTypeValues FromLight;

	// Percent of Corruption damage converted or gained as each destination type.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Transform")
	FCombatDamageTypeValues FromCorruption;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatOffensePacket
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	FCombatDamagePacket BaseDamage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	bool bAddAttackerAttributeDamage = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	bool bApplyAttackerDamageConversion = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	bool bApplyPacketDamageConversion = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	bool bApplyPacketGainAsExtra = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	bool bApplyAttackerAttributeModifiers = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense", meta = (ClampMin = "0.0"))
	float WeaponDamageEffectiveness = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense|Conversion")
	FCombatDamageTransformRules DamageConversionPercent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense|Gain As Extra")
	FCombatDamageTransformRules GainAsExtraPercent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	float GlobalIncreasedPercent = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	float ElementalIncreasedPercent = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	FCombatDamageTypeValues TypeIncreasedPercent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	float GlobalMoreMultiplier = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	float ElementalMoreMultiplier = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	FCombatDamageTypeMultipliers TypeMoreMultiplier;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	FCombatDamageTypeValues ResistancePiercingPercent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	float ArmourPiercingPercent = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	bool bCanCrit = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	bool bForceCrit = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	float CritChanceBonus = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	float CritMultiplierBonus = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Offense")
	float CritMultiplierOverride = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Tags")
	bool bIsMelee = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Tags")
	bool bIsRanged = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Tags")
	bool bIsSpell = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Tags")
	bool bIsArea = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Tags")
	bool bIsProjectile = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Tags")
	bool bIsDamageOverTime = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Tags")
	bool bIsChainHit = false;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatDefensePacket
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Defense")
	bool bIgnoreBlock = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Defense")
	bool bOverrideResistances = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Defense", meta = (EditCondition = "bOverrideResistances"))
	FCombatDamageTypeValues ResistanceOverride;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Defense")
	FCombatDamageTypeValues ResistanceBonus;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Defense")
	FCombatDamageTypeMultipliers DamageTakenMultiplier;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Defense")
	bool bOverrideArmour = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Defense", meta = (EditCondition = "bOverrideArmour"))
	float ArmourOverride = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Defense")
	float ArmourFlatBonus = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Defense")
	float ArmourPercentBonus = 0.f;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatCostPacket
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Cost")
	bool bPayOnApply = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Cost")
	float StaminaCost = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Cost")
	float ManaCost = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Cost")
	float HealthCost = 0.f;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatHitPacket
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Packet")
	FCombatOffensePacket Offense;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Packet")
	FCombatDefensePacket Defense;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Packet")
	FCombatCostPacket Cost;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Packet")
	EHitResponse HitResponse = EHitResponse::Normal;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat|Packet")
	bool bCanApplyAilments = true;
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
	bool bKilledTarget = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bWasCrit = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bWasBlocked = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bGuardBroken = false;

	// ── Hit Response ──────────────────────────────────────────────────────────
	// Determines how CombatManager routes damage and ailment application.
	// Set by FCombatHitPacket before ApplyResolvedDamage runs.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	EHitResponse HitResponse = EHitResponse::Normal;

	// ── Ailment Gate ──────────────────────────────────────────────────────────
	// True  = ailment chance rolls run normally (Normal + Parry paths)
	// False = ailments fully skipped (Invincible, certain absorb scenarios)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bShouldApplyAilments = true;

	// ── Stagger Gate ──────────────────────────────────────────────────────────
	// Set by CombatManager when stamina is depleted and State_Self_ExecutingSkill
	// tag is absent. BP reads this to play the stagger montage.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bShouldStagger = false;
};

// Legacy/internal skill-data bridge used by CombatManager's attribute-scaling helpers.
// New Blueprint-authored hit flows should build FCombatHitPacket and call ApplyHit.
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FSkillDamagePacket
{
	GENERATED_BODY()

	// ── Skill base damage range — C++ rolls between Min and Max ──────────────
	// These are the skill's own damage independent of weapon stats.
	// Set both to 0 for a skill that only scales from weapon (pure weapon skill).
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage")
	float MinPhysical = 0.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage")
	float MaxPhysical = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage")
	float MinFire = 0.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage")
	float MaxFire = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage")
	float MinIce = 0.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage")
	float MaxIce = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage")
	float MinLightning = 0.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage")
	float MaxLightning = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage")
	float MinLight = 0.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage")
	float MaxLight = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage")
	float MinCorruption = 0.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage")
	float MaxCorruption = 0.f;

	// ── Weapon damage effectiveness ───────────────────────────────────────────
	// Fraction of the character's weapon damage that feeds into this skill.
	// 1.0 = 100% weapon damage, 0.6 = 60% (PoE2 style), 0.0 = pure skill damage
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float WeaponDamageEffectiveness = 1.0f;

	// ── "More" multipliers — stack MULTIPLICATIVELY with each other ───────────
	// Each source multiplies independently. e.g. FireMore=1.3 * CharFireMore=1.2 = 1.56x
	// Use for skill-specific "X% more Fire damage" modifiers.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage|More")
	float PhysicalMore = 1.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage|More")
	float FireMore = 1.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage|More")
	float IceMore = 1.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage|More")
	float LightningMore = 1.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage|More")
	float LightMore = 1.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage|More")
	float CorruptionMore = 1.f;

	// ── "Increased" bonuses — pool ADDITIVELY with character's Increased ──────
	// Summed with character's type Increased% before applying as one multiplier.
	// e.g. SkillFireInc=20 + CharFireInc=30 = 1.50x fire multiplier total.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage|Increased")
	float PhysicalIncreasedPercent = 0.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage|Increased")
	float FireIncreasedPercent = 0.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage|Increased")
	float IceIncreasedPercent = 0.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage|Increased")
	float LightningIncreasedPercent = 0.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage|Increased")
	float LightIncreasedPercent = 0.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Damage|Increased")
	float CorruptionIncreasedPercent = 0.f;

	// ── Resource costs — deducted from source once on skill activation ────────
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Cost")
	float StaminaCost = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Cost")
	float ManaCost = 0.f;

	// Blood magic / sacrifice skills — costs health to use
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Cost")
	float HealthCost = 0.f;

	// Skill category flags let Blueprint-authored skills opt into conditional stats.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Tags")
	bool bIsMelee = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Tags")
	bool bIsRanged = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Tags")
	bool bIsSpell = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Tags")
	bool bIsArea = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Tags")
	bool bIsProjectile = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Tags")
	bool bIsDamageOverTime = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Tags")
	bool bIsChainHit = false;

	// ── Flags ──────────────────────────────────────────────────────────────────
	// False = use only skill min/max, ignore weapon stats entirely.
	// Good for spells, traps, fixed-damage abilities.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Flags")
	bool bLayerCharacterStats = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Flags")
	bool bCanCrit = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill|Flags")
	bool bCanApplyAilments = true;
};

#pragma once

#include "CoreMinimal.h"
#include "Combat/Library/Structs/CombatStructs.h"
#include "GameplayTagContainer.h"
#include "PHSkillStructs.generated.h"

class UTexture2D;

/** Base resource costs authored on a skill. Character cost modifiers are applied at resolve time. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHSkillCostData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Costs", meta = (ClampMin = "0.0"))
	float Mana = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Costs", meta = (ClampMin = "0.0"))
	float Health = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Costs", meta = (ClampMin = "0.0"))
	float Stamina = 0.f;
};

/** Projectile behavior authored by the skill before character bonuses. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHSkillProjectileData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Projectile", meta = (ClampMin = "0"))
	int32 Count = 0;

	/** Base projectile movement speed in world units per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Projectile", meta = (ClampMin = "0.0", Units = "cm/s"))
	float Speed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Projectile", meta = (ClampMin = "0"))
	int32 ChainCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Projectile", meta = (ClampMin = "0"))
	int32 ForkCount = 0;
};

/** Aura behavior authored by the skill before character bonuses. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHSkillAuraData
{
	GENERATED_BODY()

	/** Neutral is 1.0. Character AuraEffect applies as an increased/reduced percentage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Aura", meta = (ClampMin = "0.0"))
	float EffectMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Aura", meta = (ClampMin = "0.0"))
	float Radius = 0.f;
};

/**
 * Authored defaults for one gameplay ability.
 *
 * This struct is data only. The ability, projectile, aura, and combat owners
 * decide when and how to consume the resolved values.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHSkillData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Identity")
	FName SkillId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Identity", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Identity")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Uses per second before attack-speed or cast-speed modifiers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Timing", meta = (ClampMin = "0.01"))
	float BaseUseRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Timing", meta = (ClampMin = "0.0", Units = "s"))
	float BaseCooldownSeconds = 0.f;

	/** Attack skills can inherit the selected weapon's locally resolved attack rate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Attack")
	bool bUseWeaponAttackRate = true;

	/** Attack skills can inherit the selected weapon's locally resolved range. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Attack")
	bool bUseWeaponRange = true;

	/** Fallback range when no weapon range is requested or available. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Attack", meta = (ClampMin = "0.0"))
	float BaseRange = 0.f;

	/** Base area radius. AreaOfEffect modifies this as a percentage for Skill.AoE abilities. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Area", meta = (ClampMin = "0.0"))
	float BaseAreaRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Costs")
	FPHSkillCostData Costs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Projectile")
	FPHSkillProjectileData Projectile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Aura")
	FPHSkillAuraData Aura;

	/** Input for the existing authoritative combat pipeline. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Damage")
	FAnimationDamageInfo DamageInfo;
};

/** Immutable runtime snapshot consumed by skill execution and its spawned listeners. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHResolvedSkillData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Identity")
	FName SkillId;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Identity")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Identity")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Identity")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Tags")
	FGameplayTagContainer SkillTags;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Timing")
	float UseRate = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Timing")
	float UseIntervalSeconds = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Timing")
	float CooldownSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Attack")
	float Range = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Area")
	float AreaRadius = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Costs")
	FPHSkillCostData Costs;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Projectile")
	FPHSkillProjectileData Projectile;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Aura")
	FPHSkillAuraData Aura;

	/** Covenant-derived multiplier copied into summon execution data. */
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Summon")
	float MinionDamageMultiplier = 1.f;

	/** Covenant-derived maximum-health multiplier copied into summon execution data. */
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Summon")
	float MinionHealthMultiplier = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Damage")
	FAnimationDamageInfo DamageInfo;
};

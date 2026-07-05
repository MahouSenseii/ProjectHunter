// Shared combat enums used across ProjectHunter combat systems.
#pragma once

#include "CoreMinimal.h"
#include "CombatEnumLibrary.generated.h"

/**
 * Damage types resolved by the combat pipeline.
 * Fire/Ice/Lightning/Light form the "elemental" group for elemental-wide
 * modifiers (ElementalDamage, ElementalMoreDamage, ElementalDamageTakenMultiplier).
 * Physical mitigates through Armour; every other type mitigates through its
 * resistance. Corruption is neither physical nor elemental.
 */
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

/** Differentiates neutral, hostile, and friendly characters for targeting. */
UENUM(BlueprintType)
enum class ECombatAlignment : uint8
{
	None UMETA(DisplayName = "None"),
	Enemy UMETA(DisplayName = "Enemy"),
	Ally UMETA(DisplayName = "Ally")
};

/**
 * High level faction identity used to determine relationships between actors.
 * Alignment (Enemy / Ally / Neutral) should be calculated from faction relationships.
 */
UENUM(BlueprintType)
enum class EFaction : uint8
{
	Players UMETA(DisplayName = "Players"),

	// Player-controlled characters hostile to normal players (PvP criminals, rogues).
	PlayerKillers UMETA(DisplayName = "Player Killers"),

	// Non-player characters that protect towns or friendly areas.
	Guard UMETA(DisplayName = "Guard"),

	Enemy UMETA(DisplayName = "Enemy"),

	// Wildlife / creatures that may be neutral or reactive.
	Creature UMETA(DisplayName = "Creature"),

	// NPC civilians / merchants / quest givers.
	Civilian UMETA(DisplayName = "Civilian"),

	Neutral UMETA(DisplayName = "Neutral")
};

/** Attack delivery classification kept for targeting/AI and gameplay-tag mapping. */
UENUM(BlueprintType)
enum class EAttackType : uint8
{
	AT_None UMETA(DisplayName = "None"),
	AT_Melee UMETA(DisplayName = "Melee"),
	AT_Ranged UMETA(DisplayName = "Ranged"),
	AT_Spell UMETA(DisplayName = "Spell")
};

/** Combat status for tracking whether a character is engaged in combat. */
UENUM(BlueprintType)
enum class ECombatStatus : uint8
{
	OutOfCombat UMETA(DisplayName = "Out of Combat"),
	InCombat UMETA(DisplayName = "In Combat"),
	EnteringCombat UMETA(DisplayName = "Entering Combat"),
	LeavingCombat UMETA(DisplayName = "Leaving Combat")
};

/**
 * How an incoming hit was resolved by the target.
 *
 * Normal     — full damage + ailments (standard hit).
 * Parry      — damage zeroed; ailments still roll at full base chance. The defender
 *              cancelled the force of the hit but elemental/status contact was still
 *              made. Blueprint reads HitResponse == Parry for counter-anims/VFX.
 * Invincible — all damage AND ailments fully negated, every result field zeroed
 *              (i-frames, divine blessings). Blueprint reads a clean "nothing happened".
 * Blocked    — damage absorbed by a resource (ArcaneShield, blood magic). Ailments
 *              still roll — the hit made contact even though damage was absorbed.
 */
UENUM(BlueprintType)
enum class EHitResponse : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Parry UMETA(DisplayName = "Parry"),
	Invincible UMETA(DisplayName = "Invincible"),
	Blocked UMETA(DisplayName = "Blocked")
};

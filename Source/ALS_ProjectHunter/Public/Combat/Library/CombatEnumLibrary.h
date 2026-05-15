#pragma once

// Shared combat enums used across ProjectHunter combat systems.
#include "CoreMinimal.h"
#include "CombatEnumLibrary.generated.h"

/**
 * differentiate between neutral, hostile, and friendly characters. (for targeting)
 */
UENUM(BlueprintType)
enum class ECombatAlignment : uint8
{
	None UMETA(DisplayName = "None"),       // Neutral or player character
	Enemy UMETA(DisplayName = "Enemy"),    // Hostile character
	Ally UMETA(DisplayName = "Ally")       // Friendly character
};

/**
 * High level faction identity used to determine relationships between actors.
 * Alignment (Enemy / Ally / Neutral) should be calculated based on faction relationships.
 * Add more fractions here if needed 
 */
UENUM(BlueprintType)
enum class EFaction : uint8
{
	// Player controlled characters
	Players UMETA(DisplayName = "Players"),

	/*
	 * Player-controlled characters that are hostile to normal players.
	 * (Used for PvP criminals, rogues, etc.)
	 */
	PlayerKillers UMETA(DisplayName = "Player Killers"),

	// Non-player characters that protect towns or friendly areas
	Guard UMETA(DisplayName = "Guard"),

	// Generic hostile NPC enemies
	Enemy UMETA(DisplayName = "Enemy"),

	// Wildlife / creatures that may be neutral or reactive
	Creature UMETA(DisplayName = "Creature"),

	// NPC civilians / merchants / quest givers
	Civilian UMETA(DisplayName = "Civilian"),

	// No faction / default
	Neutral UMETA(DisplayName = "Neutral")
};


/**
 * Attack types: add more here for different types of attacks 
 */
UENUM(BlueprintType)
enum class EAttackType : uint8
{
	AT_None       UMETA(DisplayName = "None"),
	AT_Melee      UMETA(DisplayName = "Melee"),
	AT_Ranged     UMETA(DisplayName = "Ranged"),
	AT_Spell      UMETA(DisplayName = "Spell")
};

/**
 * Combat status for tracking whether a character is engaged in combat
 */
UENUM(BlueprintType)
enum class ECombatStatus : uint8
{
	OutOfCombat UMETA(DisplayName = "Out of Combat"),
	InCombat UMETA(DisplayName = "In Combat"),
	EnteringCombat UMETA(DisplayName = "Entering Combat"),  // Transition state
	LeavingCombat UMETA(DisplayName = "Leaving Combat")     // Transition state
};

/**
 * How an incoming hit was resolved by the target.
 *
 * Normal     — full damage + ailments (standard hit).
 * Parry      — damage zeroed; ailments roll at full base chance (mitigation ratio skipped).
 *              The defender cancelled the force of the hit but elemental/status contact
 *              was still made — attacker's venomous blade, igniting staff, etc. can still
 *              tag the parrier. This is the attacker's counterplay vs. a skilled blocker.
 *              Blueprint can read HitResponse == Parry to trigger counter-animations, VFX.
 * Invincible — all damage AND ailments fully negated, every result field zeroed.
 *              (i-frames, divine blessings, etc.) Blueprint reads a clean "nothing happened".
 * Blocked    — damage absorbed by a resource (ArcaneShield, blood magic, etc.). Ailments
 *              still roll — the hit made contact even though damage was absorbed.
 */
UENUM(BlueprintType)
enum class EHitResponse : uint8
{
	Normal      UMETA(DisplayName = "Normal"),
	Parry       UMETA(DisplayName = "Parry"),
	Invincible  UMETA(DisplayName = "Invincible"),
	Blocked     UMETA(DisplayName = "Blocked")
};

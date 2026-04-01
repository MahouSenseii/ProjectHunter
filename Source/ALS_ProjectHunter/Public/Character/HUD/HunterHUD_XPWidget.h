// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Character/HUD/HunterHUDBaseWidget.h"
#include "HunterHUD_XPWidget.generated.h"

class UCharacterProgressionManager;

/**
 * HUD widget that tracks the player's XP and level via UCharacterProgressionManager.
 *
 * Data flow:
 *   AHunterHUD calls InitializeForCharacter() → NativeInitializeForCharacter() binds
 *   to the character's ProgressionManager OnXPGained and OnLevelUp delegates.
 *   On pawn change (respawn / repossession) the binding is released and re-established
 *   automatically through the base class lifecycle.
 *
 * Blueprint implementation:
 *   Implement OnXPBarUpdated   — drive the XP progress bar (CurrentXP / XPToNextLevel).
 *   Implement OnLevelUpEffect  — trigger level-up animation / fanfare.
 *   Use GetCurrentLevel / GetCurrentXP / GetXPToNextLevel for data-binding.
 */
UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UHunterHUD_XPWidget : public UHunterHUDBaseWidget
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────────────────
	// Accessors — safe to poll from Blueprint at any time
	// ─────────────────────────────────────────────────────────────────────────

	/** Current character level (1-based). */
	UFUNCTION(BlueprintPure, Category = "HUD|XP")
	int32 GetCurrentLevel() const { return CachedLevel; }

	/** Current experience points this level. */
	UFUNCTION(BlueprintPure, Category = "HUD|XP")
	int64 GetCurrentXP() const { return CachedCurrentXP; }

	/** Experience required to reach the next level. */
	UFUNCTION(BlueprintPure, Category = "HUD|XP")
	int64 GetXPToNextLevel() const { return CachedXPToNextLevel; }

	/**
	 * XP fill fraction (0–1) suitable for driving a progress bar.
	 * Returns 1.0 if at max level.
	 */
	UFUNCTION(BlueprintPure, Category = "HUD|XP")
	float GetXPFillPercent() const;

protected:
	// ─────────────────────────────────────────────────────────────────────────
	// HunterHUDBaseWidget overrides
	// ─────────────────────────────────────────────────────────────────────────

	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;

	// ─────────────────────────────────────────────────────────────────────────
	// Blueprint events — implement visuals in BP
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Fired whenever XP changes (after gains and after a level-up snapshot).
	 *
	 * @param CurrentXP      Raw current XP value
	 * @param XPToNextLevel  XP threshold for the next level
	 * @param FillPercent    CurrentXP / XPToNextLevel  (0–1); use this for the bar
	 * @param Level          Current level at the time of the update
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|XP")
	void OnXPBarUpdated(int64 CurrentXP, int64 XPToNextLevel, float FillPercent, int32 Level);

	/**
	 * Fired when the player levels up — trigger your level-up VFX / sound here.
	 *
	 * @param NewLevel           The new level just reached
	 * @param StatPointsAwarded  Stat points granted this level
	 * @param SkillPointsAwarded Skill points granted this level
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|XP")
	void OnLevelUpEffect(int32 NewLevel, int32 StatPointsAwarded, int32 SkillPointsAwarded);

private:
	// ─────────────────────────────────────────────────────────────────────────
	// ProgressionManager delegate callbacks
	// ─────────────────────────────────────────────────────────────────────────

	UFUNCTION()
	void HandleXPGained(int64 FinalXP, int64 BaseXP, float TotalMultiplier);

	UFUNCTION()
	void HandleLevelUp(int32 NewLevel, int32 StatPointsAwarded, int32 SkillPointsAwarded);

	/** Snapshots the current state from the manager and broadcasts OnXPBarUpdated. */
	void BroadcastXPState();

	// ─────────────────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────────────────

	int32 CachedLevel         = 1;
	int64 CachedCurrentXP     = 0;
	int64 CachedXPToNextLevel = 100;  // sane default; overwritten on init

	/** Weak reference to the bound manager — cleared in NativeReleaseCharacter. */
	TWeakObjectPtr<UCharacterProgressionManager> BoundProgressionManager;
};

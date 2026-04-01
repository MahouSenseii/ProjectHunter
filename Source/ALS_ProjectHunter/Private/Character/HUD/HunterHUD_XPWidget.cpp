// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#include "Character/HUD/HunterHUD_XPWidget.h"
#include "Character/PHBaseCharacter.h"
#include "Character/Component/CharacterProgressionManager.h"

// ─────────────────────────────────────────────────────────────────────────────
// HunterHUDBaseWidget overrides
// ─────────────────────────────────────────────────────────────────────────────

void UHunterHUD_XPWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	UCharacterProgressionManager* PM = Character->GetProgressionManager();
	if (!PM)
	{
		return;
	}

	BoundProgressionManager = PM;

	// ── Bind live delegates ───────────────────────────────────────────────
	PM->OnXPGained.AddDynamic(this, &UHunterHUD_XPWidget::HandleXPGained);
	PM->OnLevelUp.AddDynamic(this,  &UHunterHUD_XPWidget::HandleLevelUp);

	// ── Snapshot initial state ────────────────────────────────────────────
	CachedLevel         = PM->Level;
	CachedCurrentXP     = PM->CurrentXP;
	CachedXPToNextLevel = FMath::Max(PM->XPToNextLevel, static_cast<int64>(1));

	BroadcastXPState();
}

void UHunterHUD_XPWidget::NativeReleaseCharacter()
{
	UCharacterProgressionManager* PM = BoundProgressionManager.Get();
	if (!PM)
	{
		BoundProgressionManager.Reset();
		return;
	}

	PM->OnXPGained.RemoveDynamic(this, &UHunterHUD_XPWidget::HandleXPGained);
	PM->OnLevelUp.RemoveDynamic(this,  &UHunterHUD_XPWidget::HandleLevelUp);

	BoundProgressionManager.Reset();
}

// ─────────────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────────────

float UHunterHUD_XPWidget::GetXPFillPercent() const
{
	if (CachedXPToNextLevel <= 0)
	{
		return 1.f;  // Max level — treat bar as full
	}
	return FMath::Clamp(static_cast<float>(CachedCurrentXP) / static_cast<float>(CachedXPToNextLevel), 0.f, 1.f);
}

// ─────────────────────────────────────────────────────────────────────────────
// ProgressionManager callbacks
// ─────────────────────────────────────────────────────────────────────────────

void UHunterHUD_XPWidget::HandleXPGained(int64 FinalXP, int64 BaseXP, float TotalMultiplier)
{
	UCharacterProgressionManager* PM = BoundProgressionManager.Get();
	if (!PM)
	{
		return;
	}

	// Re-snapshot from the manager so we always reflect the authoritative state
	// (the delegate fires AFTER the manager has already updated its properties).
	CachedLevel         = PM->Level;
	CachedCurrentXP     = PM->CurrentXP;
	CachedXPToNextLevel = FMath::Max(PM->XPToNextLevel, static_cast<int64>(1));

	BroadcastXPState();
}

void UHunterHUD_XPWidget::HandleLevelUp(int32 NewLevel, int32 StatPointsAwarded, int32 SkillPointsAwarded)
{
	UCharacterProgressionManager* PM = BoundProgressionManager.Get();
	if (!PM)
	{
		return;
	}

	CachedLevel         = NewLevel;
	CachedCurrentXP     = PM->CurrentXP;
	CachedXPToNextLevel = FMath::Max(PM->XPToNextLevel, static_cast<int64>(1));

	// Fire the level-up visual event first so BP can play the fanfare while
	// the bar reflects the XP state at the start of the new level.
	OnLevelUpEffect(NewLevel, StatPointsAwarded, SkillPointsAwarded);
	BroadcastXPState();
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

void UHunterHUD_XPWidget::BroadcastXPState()
{
	OnXPBarUpdated(CachedCurrentXP, CachedXPToNextLevel, GetXPFillPercent(), CachedLevel);
}

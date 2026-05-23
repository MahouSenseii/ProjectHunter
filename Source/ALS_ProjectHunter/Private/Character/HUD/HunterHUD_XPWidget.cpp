#include "Character/HUD/HunterHUD_XPWidget.h"
#include "Character/PHBaseCharacter.h"
#include "Progression/Components/CharacterProgressionManager.h"

void UHunterHUD_XPWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	UCharacterProgressionManager* PM = Character->GetProgressionManager();
	if (!PM)
	{
		return;
	}

	BoundProgressionManager = PM;

	PM->OnXPGained.AddDynamic(this, &UHunterHUD_XPWidget::HandleXPGained);
	PM->OnLevelUp.AddDynamic(this,  &UHunterHUD_XPWidget::HandleLevelUp);

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

float UHunterHUD_XPWidget::GetXPFillPercent() const
{
	if (CachedXPToNextLevel <= 0)
	{
		return 1.f;
	}
	return FMath::Clamp(static_cast<float>(CachedCurrentXP) / static_cast<float>(CachedXPToNextLevel), 0.f, 1.f);
}

void UHunterHUD_XPWidget::HandleXPGained(int64 FinalXP, int64 BaseXP, float TotalMultiplier)
{
	UCharacterProgressionManager* PM = BoundProgressionManager.Get();
	if (!PM)
	{
		return;
	}

	// The delegate fires AFTER the manager has already updated its properties.
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

	// Fire the level-up event first so BP plays the fanfare while the bar reflects XP at the start of the new level.
	OnLevelUpEffect(NewLevel, StatPointsAwarded, SkillPointsAwarded);
	BroadcastXPState();
}

void UHunterHUD_XPWidget::BroadcastXPState()
{
	OnXPBarUpdated(CachedCurrentXP, CachedXPToNextLevel, GetXPFillPercent(), CachedLevel);
}

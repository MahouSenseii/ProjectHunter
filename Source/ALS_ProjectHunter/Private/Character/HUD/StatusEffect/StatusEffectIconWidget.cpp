// Character/HUD/StatusEffect/StatusEffectIconWidget.cpp
#include "Character/HUD/StatusEffect/StatusEffectIconWidget.h"
#include "AbilitySystemComponent.h"

DEFINE_LOG_CATEGORY(LogStatusEffectIcon);

void UStatusEffectIconWidget::BindToEffect(UAbilitySystemComponent* InASC,
	FActiveGameplayEffectHandle InHandle,
	UTexture2D* InIcon,
	const FText& InEffectName,
	bool InIsBuff)
{
	BoundASC = InASC;
	EffectHandle = InHandle;
	bExpiredBroadcast = false;

	// Cache total duration for normalised progress
	TotalDuration = 0.0f;
	CachedRemainingTime = 0.0f;

	if (InASC && InHandle.IsValid())
	{
		const FActiveGameplayEffect* AGE = InASC->GetActiveGameplayEffect(InHandle);
		if (AGE)
		{
			const float StartTime  = AGE->StartWorldTime;
			const float Duration   = AGE->GetDuration();
			if (Duration > 0.0f)
			{
				TotalDuration = Duration;
				const float Elapsed = InASC->GetWorld()->GetTimeSeconds() - StartTime;
				CachedRemainingTime = FMath::Max(0.0f, Duration - Elapsed);
			}
			else
			{
				// Infinite / instant — display as permanent
				TotalDuration = -1.0f;
				CachedRemainingTime = -1.0f;
			}
		}
	}

	BP_OnIconDataSet(InIcon, CachedRemainingTime, InIsBuff, InEffectName);
}

void UStatusEffectIconWidget::UnbindEffect()
{
	EffectHandle = FActiveGameplayEffectHandle();
	BoundASC.Reset();
	bExpiredBroadcast = false;
}

float UStatusEffectIconWidget::GetNormalisedProgress() const
{
	if (TotalDuration <= 0.0f)
	{
		return 1.0f; // Permanent effect — always full
	}
	return FMath::Clamp(CachedRemainingTime / TotalDuration, 0.0f, 1.0f);
}

void UStatusEffectIconWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!EffectHandle.IsValid() || !BoundASC.IsValid())
	{
		if (!bExpiredBroadcast)
		{
			bExpiredBroadcast = true;
			BP_OnEffectExpired();
		}
		return;
	}

	UAbilitySystemComponent* ASC = BoundASC.Get();
	const FActiveGameplayEffect* AGE = ASC->GetActiveGameplayEffect(EffectHandle);
	if (!AGE)
	{
		// Effect was removed externally
		if (!bExpiredBroadcast)
		{
			bExpiredBroadcast = true;
			CachedRemainingTime = 0.0f;
			BP_OnEffectExpired();
		}
		return;
	}

	if (TotalDuration > 0.0f)
	{
		const float Elapsed = ASC->GetWorld()->GetTimeSeconds() - AGE->StartWorldTime;
		CachedRemainingTime = FMath::Max(0.0f, TotalDuration - Elapsed);

		BP_OnTimeUpdate(CachedRemainingTime, GetNormalisedProgress());

		if (CachedRemainingTime <= 0.0f && !bExpiredBroadcast)
		{
			bExpiredBroadcast = true;
			BP_OnEffectExpired();
		}
	}
	// Infinite effects: no time update needed
}

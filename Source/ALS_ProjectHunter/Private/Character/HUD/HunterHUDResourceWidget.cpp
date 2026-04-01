// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#include "Character/HUD/HunterHUDResourceWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "Character/PHBaseCharacter.h"

// ─────────────────────────────────────────────────────────────────────────────
// HunterHUDBaseWidget overrides
// ─────────────────────────────────────────────────────────────────────────────

void UHunterHUDResourceWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	const UHunterAttributeSet* AS  = Character->GetAttributeSet();
	if (!ASC || !AS)
	{
		return;
	}

	// ── Snapshot initial values ──────────────────────────────────────────────
	if (CurrentAttribute.IsValid())
	{
		bool bFound = false;
		CachedCurrent = ASC->GetGameplayAttributeValue(CurrentAttribute, bFound);
	}

	if (MaxAttribute.IsValid())
	{
		bool bFound = false;
		CachedMax = FMath::Max(ASC->GetGameplayAttributeValue(MaxAttribute, bFound), 1.f);
	}

	if (ReservedAttribute.IsValid())
	{
		bool bFound = false;
		CachedReserved = FMath::Max(ASC->GetGameplayAttributeValue(ReservedAttribute, bFound), 0.f);
	}

	// ── Bind live attribute change delegates ─────────────────────────────────
	if (CurrentAttribute.IsValid())
	{
		CurrentHandle = ASC->GetGameplayAttributeValueChangeDelegate(CurrentAttribute)
			.AddUObject(this, &UHunterHUDResourceWidget::HandleCurrentChanged);
	}

	if (MaxAttribute.IsValid())
	{
		MaxHandle = ASC->GetGameplayAttributeValueChangeDelegate(MaxAttribute)
			.AddUObject(this, &UHunterHUDResourceWidget::HandleMaxChanged);
	}

	if (ReservedAttribute.IsValid())
	{
		ReservedHandle = ASC->GetGameplayAttributeValueChangeDelegate(ReservedAttribute)
			.AddUObject(this, &UHunterHUDResourceWidget::HandleReservedChanged);
	}

	// Send the initial state to Blueprint immediately
	BroadcastResourceState();
}

void UHunterHUDResourceWidget::NativeReleaseCharacter()
{
	APHBaseCharacter* Character = BoundCharacter.Get();
	if (!Character)
	{
		return;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	if (CurrentHandle.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(CurrentAttribute).Remove(CurrentHandle);
		CurrentHandle.Reset();
	}

	if (MaxHandle.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(MaxAttribute).Remove(MaxHandle);
		MaxHandle.Reset();
	}

	if (ReservedHandle.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(ReservedAttribute).Remove(ReservedHandle);
		ReservedHandle.Reset();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────────────

float UHunterHUDResourceWidget::GetFillPercent() const
{
	return (CachedMax > 0.f) ? FMath::Clamp(CachedCurrent / CachedMax, 0.f, 1.f) : 0.f;
}

float UHunterHUDResourceWidget::GetReservedPercent() const
{
	return (CachedMax > 0.f) ? FMath::Clamp(CachedReserved / CachedMax, 0.f, 1.f) : 0.f;
}

// ─────────────────────────────────────────────────────────────────────────────
// GAS attribute callbacks
// ─────────────────────────────────────────────────────────────────────────────

void UHunterHUDResourceWidget::HandleCurrentChanged(const FOnAttributeChangeData& Data)
{
	const float OldValue = CachedCurrent;
	CachedCurrent        = FMath::Max(Data.NewValue, 0.f);
	OnCurrentValueChanged(CachedCurrent, CachedCurrent - OldValue);
	BroadcastResourceState();
}

void UHunterHUDResourceWidget::HandleMaxChanged(const FOnAttributeChangeData& Data)
{
	CachedMax = FMath::Max(Data.NewValue, 1.f);
	OnMaxValueChanged(CachedMax);
	BroadcastResourceState();
}

void UHunterHUDResourceWidget::HandleReservedChanged(const FOnAttributeChangeData& Data)
{
	CachedReserved = FMath::Max(Data.NewValue, 0.f);
	OnReservedValueChanged(CachedReserved);
	BroadcastResourceState();
}

void UHunterHUDResourceWidget::BroadcastResourceState()
{
	OnResourceUpdated(CachedCurrent, CachedMax, CachedReserved,
	                  GetFillPercent(), GetReservedPercent());
}

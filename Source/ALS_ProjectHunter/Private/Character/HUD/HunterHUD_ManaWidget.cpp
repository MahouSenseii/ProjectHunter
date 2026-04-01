// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#include "Character/HUD/HunterHUD_ManaWidget.h"
#include "AbilitySystem/HunterAttributeSet.h"

UHunterHUD_ManaWidget::UHunterHUD_ManaWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentAttribute  = UHunterAttributeSet::GetManaAttribute();
	MaxAttribute      = UHunterAttributeSet::GetMaxEffectiveManaAttribute();
	ReservedAttribute = UHunterAttributeSet::GetReservedManaAttribute();
}

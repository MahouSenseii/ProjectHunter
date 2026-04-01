// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#include "Character/HUD/HunterHUD_HealthWidget.h"
#include "AbilitySystem/HunterAttributeSet.h"

UHunterHUD_HealthWidget::UHunterHUD_HealthWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentAttribute  = UHunterAttributeSet::GetHealthAttribute();
	MaxAttribute      = UHunterAttributeSet::GetMaxEffectiveHealthAttribute();
	ReservedAttribute = UHunterAttributeSet::GetReservedHealthAttribute();
}

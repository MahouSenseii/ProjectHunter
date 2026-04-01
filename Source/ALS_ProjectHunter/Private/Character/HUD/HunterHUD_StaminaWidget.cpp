// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#include "Character/HUD/HunterHUD_StaminaWidget.h"
#include "AbilitySystem/HunterAttributeSet.h"

UHunterHUD_StaminaWidget::UHunterHUD_StaminaWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentAttribute  = UHunterAttributeSet::GetStaminaAttribute();
	MaxAttribute      = UHunterAttributeSet::GetMaxEffectiveStaminaAttribute();
	ReservedAttribute = UHunterAttributeSet::GetReservedStaminaAttribute();
}

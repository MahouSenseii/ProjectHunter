#include "AbilitySystem/ModMagCalc/MMC_Regen.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Library/PHCharacterEnumLibrary.h"

UMMC_Regen::UMMC_Regen()
{
    RegenType = EVitalRegenType::Health;

    // ===========================
    // Amount Captures
    // ===========================
    HealthRegenAmountDef.AttributeToCapture = UPHAttributeSet::GetHealthRegenAmountAttribute();
    HealthRegenAmountDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    HealthRegenAmountDef.bSnapshot = false;

    ManaRegenAmountDef.AttributeToCapture = UPHAttributeSet::GetManaRegenAmountAttribute();
    ManaRegenAmountDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    ManaRegenAmountDef.bSnapshot = false;

    StaminaRegenAmountDef.AttributeToCapture = UPHAttributeSet::GetStaminaRegenAmountAttribute();
    StaminaRegenAmountDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    StaminaRegenAmountDef.bSnapshot = false;

    ArcaneShieldRegenAmountDef.AttributeToCapture = UPHAttributeSet::GetArcaneShieldRegenAmountAttribute();
    ArcaneShieldRegenAmountDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    ArcaneShieldRegenAmountDef.bSnapshot = false;

    // ===========================
    // Rate Captures (NEW)
    // ===========================
    HealthRegenRateDef.AttributeToCapture = UPHAttributeSet::GetHealthRegenRateAttribute();
    HealthRegenRateDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    HealthRegenRateDef.bSnapshot = false;

    ManaRegenRateDef.AttributeToCapture = UPHAttributeSet::GetManaRegenRateAttribute();
    ManaRegenRateDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    ManaRegenRateDef.bSnapshot = false;

    StaminaRegenRateDef.AttributeToCapture = UPHAttributeSet::GetStaminaRegenRateAttribute();
    StaminaRegenRateDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    StaminaRegenRateDef.bSnapshot = false;

    ArcaneShieldRegenRateDef.AttributeToCapture = UPHAttributeSet::GetArcaneShieldRegenRateAttribute();
    ArcaneShieldRegenRateDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    ArcaneShieldRegenRateDef.bSnapshot = false;

    // Add all captures
    RelevantAttributesToCapture.Add(HealthRegenAmountDef);
    RelevantAttributesToCapture.Add(ManaRegenAmountDef);
    RelevantAttributesToCapture.Add(StaminaRegenAmountDef);
    RelevantAttributesToCapture.Add(ArcaneShieldRegenAmountDef);
    RelevantAttributesToCapture.Add(HealthRegenRateDef);
    RelevantAttributesToCapture.Add(ManaRegenRateDef);
    RelevantAttributesToCapture.Add(StaminaRegenRateDef);
    RelevantAttributesToCapture.Add(ArcaneShieldRegenRateDef);
}

float UMMC_Regen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluationParameters;
    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;

    float RegenAmount = 0.0f;
    float RegenRate = 1.0f;
    float BaseTickRate = 1.0f;
    // Get the appropriate regen amount AND rate based on type
    switch (RegenType)
    {
    case EVitalRegenType::Health:
        GetCapturedAttributeMagnitude(HealthRegenAmountDef, Spec, EvaluationParameters, RegenAmount);
        GetCapturedAttributeMagnitude(HealthRegenRateDef, Spec, EvaluationParameters, RegenRate);
        break;
    case EVitalRegenType::Mana:
        GetCapturedAttributeMagnitude(ManaRegenAmountDef, Spec, EvaluationParameters, RegenAmount);
        GetCapturedAttributeMagnitude(ManaRegenRateDef, Spec, EvaluationParameters, RegenRate);
        break;
    case EVitalRegenType::Stamina:
        GetCapturedAttributeMagnitude(StaminaRegenAmountDef, Spec, EvaluationParameters, RegenAmount);
        GetCapturedAttributeMagnitude(StaminaRegenRateDef, Spec, EvaluationParameters, RegenRate);
        break;
    case EVitalRegenType::ArcaneShield:
        GetCapturedAttributeMagnitude(ArcaneShieldRegenAmountDef, Spec, EvaluationParameters, RegenAmount);
        GetCapturedAttributeMagnitude(ArcaneShieldRegenRateDef, Spec, EvaluationParameters, RegenRate);
        break;
    }

    // Prevent division by zero
    if (RegenRate <= 0.0f)
    {
        RegenRate = 1.0f;
    }

    // Calculate amount per tick: Amount * (BaseTickRate / DesiredRate)
    // Example: 10 HP every 2 seconds with BaseTickRate=1.0
    // Result: 10 * (1.0 / 2.0) = 5 HP per tick
    float AmountPerTick = RegenAmount * (BaseTickRate / RegenRate);

    return AmountPerTick;
}
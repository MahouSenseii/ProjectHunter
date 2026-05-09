#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "AbilitySystem/Library/MMCEnumLibrary.h"
#include "HunterMMC_EffectiveResource.generated.h"

/**
 * Effective = MaxValue - ReservedAmount, clamped to >= 0.
 */
UCLASS(Abstract)
class ALS_PROJECTHUNTER_API UHunterMMC_EffectiveResource : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UHunterMMC_EffectiveResource();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

protected:
	virtual EHunterResourceType GetResourceType() const PURE_VIRTUAL(UHunterMMC_EffectiveResource::GetResourceType, return EHunterResourceType::Health;);

	float GetCapturedValue(
		const FGameplayEffectSpec& Spec,
		const FGameplayEffectAttributeCaptureDefinition& CaptureDefinition,
		float DefaultValue) const;

	static float CalculateReservedAmount(float MaxValue, float PercentValue, float FlatValue);
};

UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_EffectiveHealth : public UHunterMMC_EffectiveResource
{
	GENERATED_BODY()

protected:
	virtual EHunterResourceType GetResourceType() const override
	{
		return EHunterResourceType::Health;
	}
};

UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_EffectiveMana : public UHunterMMC_EffectiveResource
{
	GENERATED_BODY()

protected:
	virtual EHunterResourceType GetResourceType() const override
	{
		return EHunterResourceType::Mana;
	}
};

UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_EffectiveStamina : public UHunterMMC_EffectiveResource
{
	GENERATED_BODY()

protected:
	virtual EHunterResourceType GetResourceType() const override
	{
		return EHunterResourceType::Stamina;
	}
};

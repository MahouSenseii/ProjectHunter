#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "HunterMMC_ReservedResource.generated.h"

UENUM()
enum class EHunterReservedResourceType : uint8
{
	Health,
	Mana,
	Stamina
};

/**
 * Reserved = RoundHalfToEven((MaxValue * (Percent / 100)) + FlatValue)
 */
UCLASS(Abstract)
class ALS_PROJECTHUNTER_API UHunterMMC_ReservedResource : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UHunterMMC_ReservedResource();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

protected:
	virtual EHunterReservedResourceType GetResourceType() const PURE_VIRTUAL(UHunterMMC_ReservedResource::GetResourceType, return EHunterReservedResourceType::Health;);

	float GetCapturedValue(
		const FGameplayEffectSpec& Spec,
		const FGameplayEffectAttributeCaptureDefinition& CaptureDefinition,
		float DefaultValue) const;

	static float CalculateReservedAmount(float MaxValue, float PercentValue, float FlatValue);
};

UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_ReservedHealth : public UHunterMMC_ReservedResource
{
	GENERATED_BODY()

protected:
	virtual EHunterReservedResourceType GetResourceType() const override
	{
		return EHunterReservedResourceType::Health;
	}
};

UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_ReservedMana : public UHunterMMC_ReservedResource
{
	GENERATED_BODY()

protected:
	virtual EHunterReservedResourceType GetResourceType() const override
	{
		return EHunterReservedResourceType::Mana;
	}
};

UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_ReservedStamina : public UHunterMMC_ReservedResource
{
	GENERATED_BODY()

protected:
	virtual EHunterReservedResourceType GetResourceType() const override
	{
		return EHunterReservedResourceType::Stamina;
	}
};
